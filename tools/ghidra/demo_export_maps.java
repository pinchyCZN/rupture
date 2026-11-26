// demo_export_maps.java
//
// Export the Ghidra database's types, data globals and named functions as
// three flat text files under reverse/exports/. The database stays the
// source of truth; these are the grep-able view of it.
//
// They exist because reading a decompiled function means answering
// "what is at +0x98 of this thing" and "what is DAT_001eb760" dozens of
// times, and a 40 KB text file answers both with one grep. Round-tripping
// each question through the MCP costs a tool call apiece.
//
//   struct_map.txt     every struct, union, enum and typedef we defined
//   symbol_map.txt     named data globals, with address, type and size
//   function_map.txt   every function that is not still FUN_
//
// Run it after any session that applied a type or a name. From the Script
// Manager, or through the MCP by passing this file's source to
// run_script_inline.
//
//   demo_export_maps.java [OUTDIR]
//
// OUTDIR defaults to reverse/exports inside the repository this script
// file is sitting in, worked out from its own path rather than written
// down. tools/export_maps.py always passes OUTDIR anyway, so that
// default only matters when the script is run from Ghidra's own script
// manager.
//
// WHAT IT CANNOT KNOW is where a type came from and how sure we are. Both
// live in the type's Ghidra description, in a fixed first line:
//
//     [verified] origin=1000:7f5e  the sine table
//
// which exports as src=verified, ORIGIN=1000:7f5e, note=the rest. A type
// with no tag exports as src=untagged, and structmap.py check reports it.
// That keeps one home for the evidence (CLAUDE.md rule 4) instead of a
// tag in the DB and a second, drifting copy in the text.
//
// Commas are replaced with semicolons in every emitted field, because the
// format is comma-separated and a function-pointer type name contains
// commas. Nothing else is escaped.
//
// @category PCDEMOS

import generic.jar.ResourceFile;
import java.io.File;
import java.io.FileOutputStream;
import java.io.OutputStreamWriter;
import java.io.Writer;
import java.util.ArrayList;
import java.util.Collections;
import java.util.Iterator;
import java.util.List;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.data.Array;
import ghidra.program.model.data.Composite;
import ghidra.program.model.data.DataType;
import ghidra.program.model.data.DataTypeComponent;
import ghidra.program.model.data.DataTypeManager;
import ghidra.program.model.data.Enum;
import ghidra.program.model.data.Pointer;
import ghidra.program.model.data.SourceArchive;
import ghidra.program.model.data.Structure;
import ghidra.program.model.data.TypeDef;
import ghidra.program.model.data.Union;
import ghidra.program.model.listing.Data;
import ghidra.program.model.listing.Function;
import ghidra.program.model.listing.FunctionIterator;
import ghidra.program.model.mem.MemoryBlock;
import ghidra.program.model.symbol.Namespace;
import ghidra.program.model.symbol.SourceType;
import ghidra.program.model.symbol.Symbol;
import ghidra.program.model.symbol.SymbolIterator;
import ghidra.program.model.symbol.SymbolType;

public class demo_export_maps extends GhidraScript {

    // OURS - tools/ghidra/this.java, so three parents up is the
    // repository root. A written-down path would name one machine.
    private String defaultOut() {
        ResourceFile f = getSourceFile();
        int up;

        for (up = 0; f != null && up < 3; up++) f = f.getParentFile();
        if (f != null)
            return f.getAbsolutePath() + File.separator + "reverse"
                 + File.separator + "exports";
        return "reverse" + File.separator + "exports";
    }

    // Categories Ghidra fills in by itself. Nothing in them is a finding.
    //
    // /DOS is the MZ loader's OLD_IMAGE_DOS_HEADER, created by the
    // importer. It is not ours, it can never carry an evidence tag, and
    // leaving it in made structmap.py check report an untagged type on a
    // project with no types of its own yet - a false alarm that trains
    // the eye to ignore the one report that must stay clean (rule 5).
    // Not every name here is one this loader can produce; a category
    // that never appears costs nothing to skip.
    private static final String[] SKIP_CATS = {
        "/demangler", "/DWARF", "/PDB", "/ELF", "/XBE", "/DOS",
    };

    // Names Ghidra invents. A label we did not choose is not knowledge.
    //
    // The source type cannot do this job on its own: ANALYSIS-sourced
    // names are a mix of anything an analyser chose - switch labels
    // like caseD_2 among them - and only the spelling separates a real
    // name from noise. Import thunks and Function ID
    // matches arrive as IMPORTED or ANALYSIS names; the spelling test
    // below is what keeps Ghidra's own inventions out.
    private static final String[] GENERATED = {
        "FUN_", "thunk_FUN_", "caseD_", "switchD_", "LAB_", "SUB_",
        "j_FUN_", "DAT_", "PTR_", "unaff_", "UNK_",
    };

    // Ghidra's auto-label shape: a stem plus the address it sits at,
    // s_SomeString_001c4000. Nothing we name ends in eight hex digits.
    private static final Pattern AUTOLABEL =
        Pattern.compile(".*_[0-9A-Fa-f]{8}$");

    private static boolean generated(String name) {
        for (int i = 0; i < GENERATED.length; i++) {
            if (name.startsWith(GENERATED[i])) return true;
        }
        return AUTOLABEL.matcher(name).matches();
    }

    private static final Pattern TAG = Pattern.compile(
        "^\\s*\\[(verified|inferred|observed|guess)\\]\\s*");
    // origin=1000:7f5e or origin=0001bedd. THE SEGMENTED FORM IS THE
    // NORMAL ONE HERE, and a flat-address pattern truncates it silently:
    // this matched "1000" and left ":7f5e" stranded in the prose, so the
    // ORIGIN column named the wrong byte while looking perfectly valid.
    private static final Pattern ORIGIN = Pattern.compile(
        "origin\\s*=\\s*(?:0x)?([0-9A-Fa-f]{1,4}:[0-9A-Fa-f]{1,4}"
        + "|(?:0x)?[0-9A-Fa-f]{1,8})");

    // ---------------------------------------------------------------- util

    private static String clean(String s) {
        if (s == null) return "";
        return s.replace(',', ';').replace('\t', ' ')
                .replace('\r', ' ').replace('\n', ' ').trim();
    }

    /** Longhand for println, because PrintWriter would emit CRLF. */
    private static void w(Writer out, String s) throws Exception {
        out.write(s);
        out.write("\n");
    }

    private static String hex(long v) {
        return "0x" + Long.toHexString(v).toUpperCase();
    }

    /** Who chose this name. The only column that says "off-mission". */
    private static String provenance(SourceType st) {
        if (st == SourceType.USER_DEFINED) return "user";
        if (st == SourceType.IMPORTED) return "imported";
        if (st == SourceType.ANALYSIS) return "sdk";
        return "other";
    }

    // -------------------------------------------------------- description

    /** The three things a description carries: tag, origin, prose. */
    private static class Desc {
        String src = "untagged";
        String origin = "";
        String note = "";
    }

    private static Desc describe(DataType dt) {
        Desc d = new Desc();
        String s = dt.getDescription();
        if (s == null) s = "";
        s = s.split("\n")[0];
        Matcher m = TAG.matcher(s);
        if (m.find()) {
            d.src = m.group(1);
            s = s.substring(m.end());
        }
        m = ORIGIN.matcher(s);
        if (m.find()) {
            d.origin = m.group(1).toLowerCase();
            // A segmented origin stays exactly as written: it is how the
            // database spells that address, and padding it would make it
            // ungreppable. Only a flat one is zero-filled to 8.
            if (d.origin.indexOf(':') < 0) {
                while (d.origin.length() < 8) d.origin = "0" + d.origin;
            }
            s = m.replaceFirst("");
        }
        d.note = clean(s);
        return d;
    }

    // --------------------------------------------------------------- types

    private boolean ours(DataType dt) {
        DataTypeManager dtm = currentProgram.getDataTypeManager();
        SourceArchive sa = dt.getSourceArchive();
        if (sa != null && !sa.equals(dtm.getLocalSourceArchive())) {
            return false;       // came out of an archive; not a finding
        }
        String cat = dt.getCategoryPath().getPath();
        for (String skip : SKIP_CATS) {
            if (cat.equals(skip) || cat.startsWith(skip + "/")) return false;
        }
        return true;
    }

    /**
     * Array dimensions as Ghidra nests them, outermost first, with the
     * innermost element type. int[4][3] is an Array(4) of Array(3) of int.
     */
    private static String dims(DataType dt, StringBuilder elemType) {
        List<Integer> ds = new ArrayList<Integer>();
        DataType cur = dt;
        while (cur instanceof Array) {
            Array a = (Array) cur;
            ds.add(Integer.valueOf(a.getNumElements()));
            cur = a.getDataType();
        }
        elemType.append(cur.getName());
        if (ds.isEmpty()) return "";
        StringBuilder sb = new StringBuilder();
        for (int i = 0; i < ds.size(); i++) {
            if (i > 0) sb.append('x');
            sb.append(ds.get(i));
        }
        return sb.toString();
    }

    private void emitComposite(Writer out, Composite c) throws Exception {
        Desc d = describe(c);
        String kind = (c instanceof Union) ? "union" : "struct";
        w(out, "");
        w(out, "type," + clean(c.getName()) + "," + kind + ","
                + hex(c.getLength()) + "," + d.src + "," + d.origin
                + (d.note.isEmpty() ? "" : "," + d.note));

        DataTypeComponent[] comps = c.getComponents();
        int padFrom = -1, padTo = -1;
        for (int i = 0; i < comps.length; i++) {
            DataTypeComponent m = comps[i];
            DataType mt = m.getDataType();
            boolean undefined = (m.getFieldName() == null)
                    && (mt == DataType.DEFAULT || mt.getLength() <= 0
                        || mt.getName().startsWith("undefined"));
            if (undefined) {
                // Collapse a run of undefined bytes into one pad row, so
                // members plus pads equal SIZE exactly and check can prove it.
                if (padFrom < 0) padFrom = m.getOffset();
                padTo = m.getOffset() + m.getLength();
                continue;
            }
            if (padFrom >= 0) {
                w(out, "\t(pad),undefined," + pad8(padFrom) + ",,"
                        + hex(padTo - padFrom));
                padFrom = -1;
            }
            StringBuilder elem = new StringBuilder();
            String count = dims(mt, elem);
            String note = clean(m.getComment());
            w(out, "\t" + clean(m.getFieldName()) + "," + clean(elem.toString())
                    + "," + pad8(m.getOffset()) + "," + count + ","
                    + hex(m.getLength())
                    + (note.isEmpty() ? "" : "," + note));
        }
        if (padFrom >= 0) {
            w(out, "\t(pad),undefined," + pad8(padFrom) + ",,"
                    + hex(padTo - padFrom));
        }
    }

    private static String pad8(int off) {
        String s = Integer.toHexString(off).toUpperCase();
        while (s.length() < 4) s = "0" + s;
        return "0x" + s;
    }

    private void emitEnum(Writer out, Enum e) throws Exception {
        Desc d = describe(e);
        w(out, "");
        w(out, "type," + clean(e.getName()) + ",enum," + hex(e.getLength())
                + "," + d.src + "," + d.origin
                + (d.note.isEmpty() ? "" : "," + d.note));
        String[] names = e.getNames();
        for (int i = 0; i < names.length; i++) {
            w(out, "\t" + clean(names[i]) + ","
                    + hex(e.getValue(names[i])));
        }
    }

    private void emitTypedef(Writer out, TypeDef t) throws Exception {
        Desc d = describe(t);
        String to = clean(t.getDataType().getName());
        w(out, "");
        w(out, "type," + clean(t.getName()) + ",typedef," + hex(t.getLength())
                + "," + d.src + "," + d.origin + ",-> " + to
                + (d.note.isEmpty() ? "" : "; " + d.note));
    }

    private int writeStructMap(File dir) throws Exception {
        DataTypeManager dtm = currentProgram.getDataTypeManager();
        List<Composite> comps = new ArrayList<Composite>();
        List<Enum> enums = new ArrayList<Enum>();
        List<TypeDef> tds = new ArrayList<TypeDef>();

        Iterator<DataType> it = dtm.getAllDataTypes();
        while (it.hasNext()) {
            DataType dt = it.next();
            if (!ours(dt)) continue;
            if (dt instanceof Composite) comps.add((Composite) dt);
            else if (dt instanceof Enum) enums.add((Enum) dt);
            else if (dt instanceof TypeDef) tds.add((TypeDef) dt);
        }
        Collections.sort(comps, new ByName());
        Collections.sort(enums, new ByName());
        Collections.sort(tds, new ByName());

        File f = new File(dir, "struct_map.txt");
        Writer out = new OutputStreamWriter(
                new FileOutputStream(f), "US-ASCII");
        try {
            w(out, "# demo struct map   format=demo-struct-map/1");
            w(out, "#");
            w(out, "# GENERATED from the Ghidra database by");
            w(out, "# tools/ghidra/demo_export_maps.java. Do not hand-edit:");
            w(out, "# the database is the source of truth and the next export");
            w(out, "# discards anything written here.");
            w(out, "#");
            w(out, "#   type,NAME,KIND,SIZE,SRC,ORIGIN[,note]");
            w(out, "#");
            w(out, "# struct and union members:");
            w(out, "#   <TAB>name,type,offset,count,size[,note]");
            w(out, "# enum members:");
            w(out, "#   <TAB>name,value");
            w(out, "# a typedef has no members; its target is the note.");
            w(out, "#");
            w(out, "# count is blank for a scalar, decimal for an array, NxM");
            w(out, "# for [N][M] with N outermost. size is the member total.");
            w(out, "# Hex carries 0x; offsets are at least four digits.");
            w(out, "#");
            w(out, "# SRC and ORIGIN come from the type's Ghidra description,");
            w(out, "# whose first line reads");
            w(out, "#     [verified] origin=0007e360  what the thing is");
            w(out, "# SRC is verified / inferred / observed / guess, and");
            w(out, "# untagged where the description says nothing - which");
            w(out, "# structmap.py check reports.");
            w(out, "#");
            w(out, "# PADS ARE EXPLICIT, as (pad) rows, so that members plus");
            w(out, "# pads equal SIZE exactly. check enforces it.");
            w(out, "#");
            w(out, "# Commas inside a name, type or note were replaced with");
            w(out, "# semicolons on export. Nothing else is escaped.");

            for (int i = 0; i < comps.size(); i++) {
                emitComposite(out, comps.get(i));
            }
            for (int i = 0; i < enums.size(); i++) emitEnum(out, enums.get(i));
            for (int i = 0; i < tds.size(); i++) emitTypedef(out, tds.get(i));
        } finally {
            out.close();
        }
        println("struct_map.txt    " + comps.size() + " composite(s), "
                + enums.size() + " enum(s), " + tds.size() + " typedef(s)");
        return comps.size() + enums.size() + tds.size();
    }

    private static class ByName implements java.util.Comparator<DataType> {
        public int compare(DataType a, DataType b) {
            return a.getName().compareTo(b.getName());
        }
    }

    // ------------------------------------------------------------- globals

    private int writeSymbolMap(File dir) throws Exception {
        List<String> rows = new ArrayList<String>();
        SymbolIterator it = currentProgram.getSymbolTable().getAllSymbols(true);
        while (it.hasNext()) {
            Symbol s = it.next();
            if (s.getSymbolType() != SymbolType.LABEL) continue;
            if (s.isDynamic()) continue;
            if (s.getSource() == SourceType.DEFAULT) continue;
            if (generated(s.getName())) continue;

            // Ghidra's jump-table labels are named plainly - "switchD"
            // and "default" - inside a namespace that carries the
            // address, so no name-prefix test catches them: 183 of a
            // first run's 344 rows were these.
            //
            // THE TEST IS THE JUMP TABLE, NOT THE NAMESPACE. Requiring
            // the global namespace instead looks equivalent and throws
            // away every library global - the D3D_g_*, D3DRS_* and
            // g_Xapi* symbols all sit in a library namespace, and that
            // filter cut 120 real rows down from 161.
            Namespace ns = s.getParentNamespace();
            if (!ns.isGlobal()
                    && (ns.getName().startsWith("switchD_")
                        || s.getName().equals("switchD")
                        || s.getName().equals("default"))) {
                continue;
            }

            Address a = s.getAddress();
            MemoryBlock b = currentProgram.getMemory().getBlock(a);
            if (b == null) continue;

            // DO NOT FILTER ON b.isExecute(). A real-mode image is one
            // flat rwx blob, and on a self-modifying packed one the
            // distinction is meaningless anyway - an executability test
            // returns either everything or nothing.
            //
            // What actually distinguishes a global from a code label is
            // whether it sits inside a function body.
            if (currentProgram.getFunctionManager()
                    .getFunctionContaining(a) != null) {
                continue;
            }

            Data data = getDataAt(a);
            String type = "undefined";
            String count = "";
            long size = 0;
            if (data != null) {
                StringBuilder elem = new StringBuilder();
                count = dims(data.getDataType(), elem);
                type = clean(elem.toString());
                size = data.getLength();
            }
            String src = provenance(s.getSource());
            rows.add(a.toString() + "," + clean(s.getName()) + "," + type
                     + "," + count + "," + hex(size) + "," + src
                     + "," + clean(b.getName()));
        }
        Collections.sort(rows);

        File f = new File(dir, "symbol_map.txt");
        Writer out = new OutputStreamWriter(
                new FileOutputStream(f), "US-ASCII");
        try {
            w(out, "# demo symbol map   format=demo-symbol-map/1");
            w(out, "#");
            w(out, "# GENERATED by tools/ghidra/demo_export_maps.java.");
            w(out, "# Named data globals - everything outside .text that has");
            w(out, "# a name somebody chose. DATA, not a document.");
            w(out, "#");
            w(out, "#   ADDRESS,NAME,TYPE,COUNT,SIZE,SRC,BLOCK");
            w(out, "#");
            w(out, "# ADDRESS is the virtual address, 8 hex digits, no 0x.");
            w(out, "# TYPE is undefined where no type has been applied, and");
            w(out, "# SIZE is then 0 - an untyped global is a byte with a");
            w(out, "# label on it, not a known object.");
            w(out, "#");
            w(out, "# SRC: user is a name we chose. A DOS MZ ships no");
            w(out, "# library metadata, so there should be no sdk rows -");
            w(out, "# one appearing means an analyser guessed a name.");
            w(out, "#");
            w(out, "# Ghidra's own DAT_/PTR_/s_SomeString_001c4000 labels are");
            w(out, "# excluded. A label we have not chosen is not knowledge.");
            w(out, "#");
            w(out, "# Enum constants are NOT here - they live with their enum");
            w(out, "# in struct_map.txt, which is their one home.");
            w(out, "");
            for (int i = 0; i < rows.size(); i++) w(out, rows.get(i));
        } finally {
            out.close();
        }
        println("symbol_map.txt    " + rows.size() + " named global(s)");
        return rows.size();
    }

    // ----------------------------------------------------------- functions

    private int writeFunctionMap(File dir) throws Exception {
        List<String> rows = new ArrayList<String>();
        int unnamed = 0;
        FunctionIterator it =
                currentProgram.getFunctionManager().getFunctions(true);
        while (it.hasNext()) {
            Function fn = it.next();
            String name = fn.getName();
            Symbol sym = fn.getSymbol();
            if (generated(name) || sym == null) {
                unnamed++;
                continue;
            }
            Address a = fn.getEntryPoint();
            MemoryBlock b = currentProgram.getMemory().getBlock(a);
            long size = fn.getBody().getNumAddresses();
            String src = provenance(sym.getSource());
            if (fn.isThunk()) src = src + ";thunk";
            rows.add(a.toString() + "," + hex(size) + "," + clean(name)
                     + "," + src + ","
                     + (b == null ? "?" : clean(b.getName())));
        }
        Collections.sort(rows);

        File f = new File(dir, "function_map.txt");
        Writer out = new OutputStreamWriter(
                new FileOutputStream(f), "US-ASCII");
        try {
            w(out, "# demo function map   format=demo-function-map/1");
            w(out, "#");
            w(out, "# GENERATED by tools/ghidra/demo_export_maps.java.");
            w(out, "# Every function that is no longer FUN_. DATA, not a");
            w(out, "# document.");
            w(out, "#");
            w(out, "#   ADDRESS,SIZE,NAME,SRC,BLOCK");
            w(out, "#");
            w(out, "# ADDRESS is the entry point, 8 hex digits, no 0x.");
            w(out, "# SIZE is the body in bytes, which for a function");
            w(out, "# Ghidra split or mis-bounded is the wrong number - a");
            w(out, "# cross-check on the database, not an independent fact.");
            w(out, "#");
            w(out, "# SRC: user is a name we chose. A DOS MZ ships no");
            w(out, "# library metadata, so there should be no sdk rows -");
            w(out, "# one appearing means an analyser guessed a name.");
            w(out, "#");
            w(out, "# " + unnamed + " function(s) carry an invented name");
            w(out, "# - FUN_, thunk_FUN_, caseD_, LAB_ - and are not listed.");
            w(out, "");
            for (int i = 0; i < rows.size(); i++) w(out, rows.get(i));
        } finally {
            out.close();
        }
        println("function_map.txt  " + rows.size() + " named, "
                + unnamed + " still machine-named");
        return rows.size();
    }

    // ---------------------------------------------------------------- main

    public void run() throws Exception {
        String[] args = getScriptArgs();
        File dir = new File(args.length > 0 && !args[0].isEmpty()
                ? args[0] : defaultOut());
        if (!dir.isDirectory() && !dir.mkdirs()) {
            println("cannot create " + dir);
            return;
        }
        println("program: " + currentProgram.getName());
        println("out:     " + dir);
        writeStructMap(dir);
        writeSymbolMap(dir);
        writeFunctionMap(dir);
        println("done. run structmap.py check next.");
    }
}
