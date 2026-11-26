/*
 * OURS. The agent pipe's shared half - agent_core.h says what it is.
 *
 * TWO KINDS OF HOOK. A patch hook overwrites whole instructions at any
 * address with a JMP to a per-slot stub, which saves the registers,
 * the flags and the x87/SSE state, logs, restores them, runs the
 * displaced instructions and jumps back. They are decoded first, and a
 * site is refused unless the stub can run them elsewhere: no branch,
 * call or return. Nothing may jump INTO the displaced bytes; those past
 * the JMP are INT3 so such a jump faults instead of running garbage.
 *
 * With AGENT_PENTER defined (the port's Debug build, compiled with /Gh
 * /GH) a hook on a function that starts with CALL _penter is an entry
 * hook, served from _penter, and HOOK add ... exit is an exit hook,
 * served from _pexit and matched by the address range end= gives.
 * THIS FILE MUST NEVER BE COMPILED WITH /Gh: _penter would call
 * itself until the stack is gone.
 */
#include <windows.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <GL/gl.h>
#pragma comment(lib, "opengl32.lib")
#include <math.h>

#include "agent_core.h"

#define GL_TEXTURE0_            0x84c0
#define GL_ACTIVE_TEXTURE_      0x84e0
#define GL_TEXTURE_BINDING_2D_  0x8069
#define TEXDUMP_MAX             4096

#define AGENT_LINE_MAX    512
#define AGENT_REPLY_MAX   (80 * 1024)
#define AGENT_READ_MAX    4096
#define AGENT_WRITE_MAX   128
#define AGENT_WAIT_MS     2000
#define HOOK_MAX          16
#define HOOK_LOG_MAX      12
#define HOOK_TAG_MAX      24
#define HOOK_STEAL_MAX    20
#define HOOK_STUB_SIZE    96
#define TRACE_MAX         256
#define TRACE_LINE_MAX    256

typedef void (APIENTRY *ActiveTexture_Fn)(GLenum unit);

/* The stack a stub or _penter hands on: pushfd above pushad. */
typedef struct HookRegs {
	DWORD efl, edi, esi, ebp, esp, ebx, edx, ecx, eax;
} HookRegs;

enum { BASE_REG, BASE_ARG, BASE_RA, BASE_ABS, BASE_TEX, BASE_MAT, BASE_ST0, BASE_NUM, BASE_VP };
enum { REG_EAX, REG_ECX, REG_EDX, REG_EBX, REG_ESP, REG_EBP, REG_ESI, REG_EDI, REG_EFL };
enum { KIND_PATCH, KIND_ENTRY, KIND_EXIT };

/* One log= or when= operand: a value, or the memory at value+off. */
typedef struct HookTok {
	unsigned char base;      /* BASE_*                               */
	unsigned char index;     /* REG_*, argument, unit or element     */
	unsigned char matrix;    /* BASE_MAT: 0 texture, 1 mv, 2 proj    */
	char          width;     /* 0, or b w l f d s                    */
	unsigned char deref;     /* 1: read width at value+off           */
	LONG          off;       /* BASE_NUM: the number itself          */
	char          text[24];  /* as the client wrote it               */
} HookTok;

typedef struct Hook {
	int            armed;
	int            kind;
	DWORD          addr;
	DWORD          end;      /* KIND_EXIT: the function's bound      */
	int            len;
	unsigned char  saved[HOOK_STEAL_MAX];
	char           tag[HOOK_TAG_MAX];
	int            nlog;
	HookTok        log[HOOK_LOG_MAX];
	int            hasWhen;
	HookTok        when;
	char           whenOp;   /* = ! < > or 0 for non-zero            */
	double         whenNum;
	DWORD          whenU;
	unsigned       cap;
	unsigned       hits;
	int            hasTex;
	HookTok        tex;      /* texdump=: the GL texture name        */
	char           texPath[MAX_PATH];
} Hook;

typedef struct Value {
	int    ok;
	int    isFloat;
	int    isStr;
	DWORD  u;
	double d;
	char   s[32];
} Value;

static const Agent_Host *g_host;
static Hook              g_hooks[HOOK_MAX];
static unsigned char    *g_stubs;
static DWORD             g_hostThread;
static unsigned          g_frame;
static char              g_trace[TRACE_MAX][TRACE_LINE_MAX];
static int               g_traceHead;
static int               g_traceCount;
static unsigned          g_traceDropped;

static CRITICAL_SECTION  g_mbLock;
static HANDLE            g_mbDone;
static volatile LONG     g_mbPending;
static volatile LONG     g_mbDeferred;
static volatile LONG     g_cancel;
static CRITICAL_SECTION  g_callLock;
static char              g_mbLine[AGENT_LINE_MAX];
static char             *g_mbReply;

/* OURS. Copy that returns 0 instead of faulting. */
static int mem_copy(void *dst, const void *src, size_t n)
{
	__try {
		memcpy(dst, src, n);
	} __except (EXCEPTION_EXECUTE_HANDLER) {
		return 0;
	}
	return 1;
}

/* OURS. The memory write WRITE and the patch hooks use: code pages too. */
static int mem_patch(DWORD addr, const void *src, size_t n)
{
	DWORD old;
	int   ok;

	if (!VirtualProtect((void *)addr, n, PAGE_EXECUTE_READWRITE, &old)) {
		return 0;
	}
	ok = mem_copy((void *)addr, src, n);
	VirtualProtect((void *)addr, n, old, &old);
	FlushInstructionCache(GetCurrentProcess(), (void *)addr, n);
	return ok;
}

/* OURS */
static void agent_log(const char *fmt, ...)
{
	char    text[256];
	va_list ap;

	if (g_host == NULL || g_host->log == NULL) {
		return;
	}
	va_start(ap, fmt);
	_vsnprintf(text, sizeof text - 1, fmt, ap);
	va_end(ap);
	text[sizeof text - 1] = 0;
	g_host->log(text);
}

/* OURS. Appends to a reply, never past its end. */
void Agent_Say(char *reply, const char *fmt, ...)
{
	size_t  used = strlen(reply);
	va_list ap;

	if (used + 1 >= AGENT_REPLY_MAX) {
		return;
	}
	va_start(ap, fmt);
	_vsnprintf(reply + used, AGENT_REPLY_MAX - used - 1, fmt, ap);
	va_end(ap);
	reply[AGENT_REPLY_MAX - 1] = 0;
}

/* OURS */
void Agent_SayHex(char *reply, const unsigned char *p, int n)
{
	int i;

	for (i = 0; i < n; i++) {
		Agent_Say(reply, "%02x", p[i]);
	}
}

/* OURS. The value of key= in a command line, as one word. */
int Agent_Arg(const char *line, const char *key, char *out, int size)
{
	size_t      klen = strlen(key);
	const char *p = line;
	int         n = 0;

	while ((p = strstr(p, key)) != NULL) {
		if ((p == line || p[-1] == ' ') && p[klen] == '=') {
			p += klen + 1;
			while (*p != 0 && *p != ' ' && *p != '\r' && *p != '\n' && n < size - 1) {
				out[n++] = *p++;
			}
			out[n] = 0;
			return 1;
		}
		p += klen;
	}
	return 0;
}

/* OURS. True if word stands alone in the command line. */
int Agent_ArgFlag(const char *line, const char *word)
{
	size_t      wlen = strlen(word);
	const char *p = line;

	while ((p = strstr(p, word)) != NULL) {
		if ((p == line || p[-1] == ' ')
		 && (p[wlen] == 0 || p[wlen] == ' ' || p[wlen] == '\r' || p[wlen] == '\n')) {
			return 1;
		}
		p += wlen;
	}
	return 0;
}

/* OURS. addr= and end= are hex; anything else is C notation. */
int Agent_ArgU32(const char *line, const char *key, unsigned long *out)
{
	char  text[32];
	char *end;
	int   hex = strcmp(key, "addr") == 0 || strcmp(key, "end") == 0;

	if (!Agent_Arg(line, key, text, sizeof text) || text[0] == 0) {
		return 0;
	}
	*out = strtoul(text, &end, hex ? 16 : 0);
	return *end == 0;
}

/* OURS */
void Agent_FrameDone(void)
{
	g_frame++;
}

/* OURS */
unsigned Agent_Frames(void)
{
	return g_frame;
}

/* OURS */
static int hex_digit(char c)
{
	if (c >= '0' && c <= '9') {
		return c - '0';
	}
	if (c >= 'a' && c <= 'f') {
		return c - 'a' + 10;
	}
	if (c >= 'A' && c <= 'F') {
		return c - 'A' + 10;
	}
	return -1;
}

/*
 * OURS. The length of the instruction at p, or 0 if it is not one a
 * stub can run from another address. Covers the integer, x87 and SSE
 * forms MSVC emits in prologues and arithmetic; anything else is
 * refused, never guessed.
 */
static int insn_len(const unsigned char *p)
{
	const unsigned char *q = p;
	int                  opsize = 4;
	int                  imm = 0;
	int                  modrm = 0;
	unsigned char        op;

	while (*q == 0x66 || *q == 0xf2 || *q == 0xf3 || *q == 0x64) {
		if (*q == 0x66) {
			opsize = 2;
		}
		q++;
	}
	op = *q++;
	if (op == 0x0f) {
		op = *q++;
		if ((op >= 0x70 && op <= 0x73) || op == 0xc2 || op == 0xc6) {
			modrm = 1;
			imm = 1;
		} else if ((op >= 0x10 && op <= 0x17) || (op >= 0x28 && op <= 0x2f)
		        || (op >= 0x51 && op <= 0x7f && op != 0x77) || (op >= 0x40 && op <= 0x4f)
		        || op == 0xaf || op == 0xb6 || op == 0xb7 || op == 0xbe || op == 0xbf
		        || (op >= 0x90 && op <= 0x9f) || op == 0xd6 || op == 0xe6 || op == 0xef) {
			modrm = 1;
		} else {
			return 0;
		}
	} else if (op >= 0x40 && op <= 0x61) {
		/* inc, dec, push, pop, pushad, popad */
	} else if (op == 0x90 || op == 0x98 || op == 0x99 || op == 0x9b || op == 0xfc) {
	} else if (op < 0x40 && (op & 7) < 4 && op != 0x0f) {
		modrm = 1;                               /* add or adc and sub xor cmp r/m */
	} else if (op < 0x40 && (op & 7) == 4) {
		imm = 1;                                 /* op al, imm8 */
	} else if (op < 0x40 && (op & 7) == 5) {
		imm = opsize;                            /* op eax, imm */
	} else if (op == 0x68) {
		imm = 4;
	} else if (op == 0x6a) {
		imm = 1;
	} else if (op == 0x69) {
		modrm = 1;
		imm = opsize;
	} else if (op == 0x6b || op == 0x80 || op == 0x83 || op == 0xc0 || op == 0xc1
	        || op == 0xc6) {
		modrm = 1;
		imm = 1;
	} else if (op == 0x81 || op == 0xc7) {
		modrm = 1;
		imm = opsize;
	} else if ((op >= 0x84 && op <= 0x8b) || op == 0x8d || op == 0xd1 || op == 0xd3
	        || (op >= 0xd8 && op <= 0xdf)) {
		modrm = 1;
	} else if (op >= 0xa0 && op <= 0xa3) {
		imm = 4;                                 /* mov eax, [moffs32] */
	} else if (op == 0xa8) {
		imm = 1;
	} else if (op == 0xa9) {
		imm = opsize;
	} else if (op >= 0xb0 && op <= 0xb7) {
		imm = 1;
	} else if (op >= 0xb8 && op <= 0xbf) {
		imm = opsize;
	} else if (op == 0xf6 || op == 0xf7) {
		modrm = 1;
		if (((*q >> 3) & 7) == 0) {
			imm = op == 0xf6 ? 1 : opsize;       /* test r/m, imm */
		}
	} else if (op == 0xfe || op == 0xff) {
		int reg = (*q >> 3) & 7;

		if (reg != 0 && reg != 1 && !(op == 0xff && reg == 6)) {
			return 0;                            /* call, jmp: not relocatable */
		}
		modrm = 1;
	} else {
		return 0;
	}
	if (modrm) {
		unsigned char m = *q++;
		int           mod = m >> 6;
		int           rm = m & 7;

		if (mod != 3 && rm == 4) {
			unsigned char sib = *q++;

			if (mod == 0 && (sib & 7) == 5) {
				q += 4;
			}
		}
		if (mod == 0 && rm == 5) {
			q += 4;
		} else if (mod == 1) {
			q += 1;
		} else if (mod == 2) {
			q += 4;
		}
	}
	return (int)(q - p) + imm;
}

static void __cdecl agent_hook_hit(int slot, HookRegs *r);

/* OURS. The bytes of slot's stub, ending in a jump back to addr+len. */
static void stub_build(int slot, const Hook *h)
{
	unsigned char *s = g_stubs + slot * HOOK_STUB_SIZE;
	unsigned char *p = s;
	DWORD          fn = (DWORD)agent_hook_hit;
	LONG           rel;
	static const unsigned char head[] = {
		0x60,                               /* pushad                */
		0x9c,                               /* pushfd                */
		0xfc,                               /* cld                   */
		0x8b, 0xc4,                         /* mov eax, esp          */
		0x83, 0xe4, 0xf0,                   /* and esp, -16          */
		0x81, 0xec, 0x00, 0x02, 0x00, 0x00, /* sub esp, 0x200        */
		0x0f, 0xae, 0x04, 0x24,             /* fxsave [esp]          */
		0x50,                               /* push eax  (saved esp) */
		0x50                                /* push eax  (HookRegs*) */
	};
	static const unsigned char tail[] = {
		0xff, 0xd0,                         /* call eax              */
		0x83, 0xc4, 0x08,                   /* add esp, 8            */
		0x58,                               /* pop eax               */
		0x0f, 0xae, 0x0c, 0x24,             /* fxrstor [esp]         */
		0x8b, 0xe0,                         /* mov esp, eax          */
		0x9d,                               /* popfd                 */
		0x61                                /* popad                 */
	};

	memcpy(p, head, sizeof head);
	p += sizeof head;
	*p++ = 0x68;                            /* push slot             */
	memcpy(p, &slot, 4);
	p += 4;
	*p++ = 0xb8;                            /* mov eax, agent_hook_hit */
	memcpy(p, &fn, 4);
	p += 4;
	memcpy(p, tail, sizeof tail);
	p += sizeof tail;
	memcpy(p, h->saved, h->len);
	p += h->len;
	rel = (LONG)(h->addr + h->len) - (LONG)((DWORD)p + 5);
	*p++ = 0xe9;
	memcpy(p, &rel, 4);
	FlushInstructionCache(GetCurrentProcess(), s, HOOK_STUB_SIZE);
}

/* OURS. glActiveTexture from the current context, by either name. */
static ActiveTexture_Fn active_texture(void)
{
	static ActiveTexture_Fn fn;

	if (fn == NULL) {
		fn = (ActiveTexture_Fn)wglGetProcAddress("glActiveTexture");
	}
	if (fn == NULL) {
		fn = (ActiveTexture_Fn)wglGetProcAddress("glActiveTextureARB");
	}
	return fn;
}

/* OURS. The 2D texture bound on unit, the active unit left as it was. */
static DWORD tex_bound(int unit)
{
	ActiveTexture_Fn fn = active_texture();
	GLint            was = GL_TEXTURE0_;
	GLint            name = 0;

	if (fn == NULL) {
		return 0xffffffff;
	}
	glGetIntegerv(GL_ACTIVE_TEXTURE_, &was);
	fn(GL_TEXTURE0_ + unit);
	glGetIntegerv(GL_TEXTURE_BINDING_2D_, &name);
	fn((GLenum)was);
	return (DWORD)name;
}

/* OURS. A matrix, from the host's own stack if it keeps one, else GL's. */
static void matrix_get(int which, float m[16])
{
	static const GLenum queries[] = { GL_TEXTURE_MATRIX, GL_MODELVIEW_MATRIX, GL_PROJECTION_MATRIX };

	if (g_host != NULL && g_host->matrix != NULL && g_host->matrix(which, m)) {
		return;
	}
	glGetFloatv(queries[which], m);
}

/* OURS. mtexN, mmvN or mprojN: element N, 0..15 column major; 0 if not one. */
static int mat_parse(const char *p, HookTok *t)
{
	static const char *names[] = { "mtex", "mmv", "mproj" };
	char              *end;
	unsigned long      n;
	int                i;

	for (i = 0; i < 3; i++) {
		size_t len = strlen(names[i]);

		if (strncmp(p, names[i], len) == 0 && p[len] >= '0' && p[len] <= '9') {
			n = strtoul(p + len, &end, 10);
			if (*end != 0 || n > 15) {
				return 0;
			}
			t->base = BASE_MAT;
			t->matrix = (unsigned char)i;
			t->index = (unsigned char)n;
			return 1;
		}
	}
	return 0;
}

/*
 * OURS. Parses one operand: eax..edi efl, aN, ra, texN, a matrix
 * element, st0, or abs, then [bwlfds][+-off].
 */
static int tok_parse(const char *text, HookTok *t)
{
	static const char *regs[] = { "eax", "ecx", "edx", "ebx", "esp", "ebp", "esi", "edi", "efl" };
	const char        *p = text;
	char              *end;
	int                i;

	memset(t, 0, sizeof *t);
	strncpy(t->text, text, sizeof t->text - 1);
	if (mat_parse(p, t)) {
		return 1;
	}
	if (strcmp(p, "st0") == 0) {
		t->base = BASE_ST0;
		return 1;
	}
	if (strncmp(p, "vp", 2) == 0 && p[2] >= '0' && p[2] <= '3' && p[3] == 0) {
		t->base = BASE_VP;
		t->index = (unsigned char)(p[2] - '0');
		return 1;
	}
	if (p[0] >= '0' && p[0] <= '9') {
		t->base = BASE_NUM;
		t->off = (LONG)strtoul(p, &end, 0);
		return *end == 0;
	}
	for (i = 0; i < 9; i++) {
		if (strncmp(p, regs[i], 3) == 0) {
			t->base = BASE_REG;
			t->index = (unsigned char)i;
			p += 3;
			break;
		}
	}
	if (i == 9) {
		if (strncmp(p, "abs", 3) == 0) {
			t->base = BASE_ABS;
			p += 3;
		} else if (strncmp(p, "ra", 2) == 0) {
			t->base = BASE_RA;
			p += 2;
		} else if (strncmp(p, "tex", 3) == 0 && p[3] >= '0' && p[3] <= '7' && p[4] == 0) {
			t->base = BASE_TEX;
			t->index = (unsigned char)(p[3] - '0');
			return 1;
		} else if (p[0] == 'a' && p[1] >= '0' && p[1] <= '9') {
			t->base = BASE_ARG;
			t->index = (unsigned char)(p[1] - '0');
			p += 2;
		} else {
			return 0;
		}
	}
	if (*p != 0 && strchr("bwlfds", *p) != NULL) {
		t->width = *p++;
		t->deref = t->width == 's';
	}
	if (*p == '+' || *p == '-') {
		t->off = strtol(p, &end, 16);
		if (*end != 0 || t->width == 0) {
			return 0;
		}
		t->deref = 1;
		p = end;
	}
	if (*p != 0) {
		return 0;
	}
	if (t->base == BASE_ABS && !t->deref) {
		return 0;
	}
	return t->width == 0 || t->width == 'l' || t->width == 'f' || t->deref;
}

/* OURS. An x87 80-bit value as a double, from its bytes. */
static double ext80(const unsigned char *b)
{
	unsigned long lo, hi;
	int           ex = ((b[9] & 0x7f) << 8) | b[8];
	double        m;

	memcpy(&lo, b, 4);
	memcpy(&hi, b + 4, 4);
	m = ((double)hi * 4294967296.0 + (double)lo) / 9223372036854775808.0;
	if (ex == 0 && m == 0.0) {
		return 0.0;
	}
	m = ldexp(m, ex - 16383);
	return (b[9] & 0x80) ? -m : m;
}

/* OURS. An operand's value at this hit; fx is the FXSAVE area, or NULL. */
static Value tok_eval(const HookTok *t, const HookRegs *r, const unsigned char *fx)
{
	Value         v;
	DWORD         base = 0;
	const DWORD  *regs = &r->edi;
	static const int order[] = { 7, 6, 5, 4, 3, 2, 1, 0 };   /* pushad order */
	unsigned char b;
	WORD          w;
	float         f;
	float         m[16];
	GLint         vp[4];

	memset(&v, 0, sizeof v);
	v.ok = 1;
	switch (t->base) {
	case BASE_REG:
		base = t->index == REG_EFL ? r->efl : regs[order[t->index]];
		break;
	case BASE_ARG:
		v.ok = mem_copy(&base, (const DWORD *)r->esp + 1 + t->index, 4);
		break;
	case BASE_RA:
		v.ok = mem_copy(&base, (const DWORD *)r->esp, 4);
		break;
	case BASE_TEX:
		v.u = tex_bound(t->index);
		return v;
	case BASE_MAT:
		matrix_get(t->matrix, m);
		v.isFloat = 1;
		v.d = m[t->index];
		return v;
	case BASE_NUM:
		v.u = (DWORD)t->off;
		return v;
	case BASE_VP:
		glGetIntegerv(GL_VIEWPORT, vp);
		v.u = (DWORD)vp[t->index];
		return v;
	case BASE_ST0:
		v.ok = fx != NULL;
		v.isFloat = 1;
		v.d = fx != NULL ? ext80(fx + 32) : 0.0;
		return v;
	default:
		break;
	}
	if (!v.ok) {
		return v;
	}
	if (!t->deref) {
		v.u = base;
		if (t->width == 'f') {
			memcpy(&f, &base, 4);
			v.isFloat = 1;
			v.d = f;
		}
		return v;
	}
	base += (DWORD)t->off;
	switch (t->width) {
	case 's':
		v.isStr = 1;
		for (w = 0; w < sizeof v.s - 1; w++) {
			if (!mem_copy(&b, (void *)(base + w), 1) || b == 0) {
				break;
			}
			v.s[w] = (char)(b >= 0x20 && b < 0x7f ? b : '.');
		}
		break;
	case 'b':
		v.ok = mem_copy(&b, (void *)base, 1);
		v.u = b;
		break;
	case 'w':
		v.ok = mem_copy(&w, (void *)base, 2);
		v.u = w;
		break;
	case 'l':
		v.ok = mem_copy(&v.u, (void *)base, 4);
		break;
	case 'f':
		v.ok = mem_copy(&f, (void *)base, 4);
		v.isFloat = 1;
		v.d = f;
		break;
	default:
		v.ok = mem_copy(&v.d, (void *)base, 8);
		v.isFloat = 1;
		break;
	}
	return v;
}

/* OURS */
static int when_ok(const Hook *h, const HookRegs *r, const unsigned char *fx)
{
	Value v;

	if (!h->hasWhen) {
		return 1;
	}
	v = tok_eval(&h->when, r, fx);
	if (!v.ok) {
		return 0;
	}
	if (!v.isFloat) {
		v.d = (double)v.u;
	}
	switch (h->whenOp) {
	case '=': return v.isFloat ? v.d == h->whenNum : v.u == h->whenU;
	case '!': return v.isFloat ? v.d != h->whenNum : v.u != h->whenU;
	case '<': return v.d < h->whenNum;
	case '>': return v.d > h->whenNum;
	default:  return v.isFloat ? v.d != 0.0 : v.u != 0;
	}
}

static int tex_dump(DWORD tex, const char *path, char *msg, size_t size);

/* OURS. texdump= on a hit: the texture to texPath, a # there the hit number. */
static void hook_texdump(const Hook *h, const HookRegs *r, const unsigned char *fx, char *line)
{
	char   path[MAX_PATH];
	char   rest[MAX_PATH];
	char   msg[96];
	char  *hash;
	size_t used = strlen(line);
	Value  v = tok_eval(&h->tex, r, fx);

	strcpy(path, h->texPath);
	hash = strchr(path, '#');
	if (hash != NULL) {
		strcpy(rest, hash + 1);
		_snprintf(hash, MAX_PATH - (hash - path), "%u%s", h->hits, rest);
		path[MAX_PATH - 1] = 0;
	}
	if (!v.ok) {
		strcpy(msg, "ERR its operand is unreadable");
	} else {
		tex_dump(v.u, path, msg, sizeof msg);
	}
	_snprintf(line + used, TRACE_LINE_MAX - used, " texdump=%s", msg);
	line[TRACE_LINE_MAX - 1] = 0;
}

/* OURS. One trace line for a hit that passes cap and when=. */
static void hook_record(Hook *h, const HookRegs *r, const unsigned char *fx)
{
	char *line;
	int   used;
	int   i;

	if ((h->cap != 0 && h->hits >= h->cap) || !when_ok(h, r, fx)) {
		return;
	}
	h->hits++;
	if (g_traceCount == TRACE_MAX) {
		g_traceDropped++;
	} else {
		g_traceCount++;
	}
	line = g_trace[g_traceHead];
	g_traceHead = (g_traceHead + 1) % TRACE_MAX;
	used = _snprintf(line, TRACE_LINE_MAX, "[%s] hit=%u frame=%u", h->tag, h->hits, g_frame);
	for (i = 0; i < h->nlog && used > 0 && used < TRACE_LINE_MAX - 40; i++) {
		Value v = tok_eval(&h->log[i], r, fx);
		int   n;

		if (!v.ok) {
			n = _snprintf(line + used, TRACE_LINE_MAX - used, " %s=?", h->log[i].text);
		} else if (v.isStr) {
			n = _snprintf(line + used, TRACE_LINE_MAX - used, " %s=\"%s\"", h->log[i].text, v.s);
		} else if (v.isFloat) {
			n = _snprintf(line + used, TRACE_LINE_MAX - used, " %s=%.9g", h->log[i].text, v.d);
		} else {
			n = _snprintf(line + used, TRACE_LINE_MAX - used, " %s=%08lx", h->log[i].text, v.u);
		}
		used = n < 0 ? -1 : used + n;
	}
	line[TRACE_LINE_MAX - 1] = 0;
	if (h->hasTex) {
		hook_texdump(h, r, fx, line);
	}
}

/*
 * OURS. What a patch stub calls. Runs on whatever thread reached the
 * hooked address; only the host's own thread records, so the trace
 * ring has one writer.
 */
static void __cdecl agent_hook_hit(int slot, HookRegs *r)
{
	Hook *h = &g_hooks[slot];

	if (!h->armed || h->kind != KIND_PATCH || GetCurrentThreadId() != g_hostThread) {
		return;
	}
	hook_record(h, r, NULL);
}

#ifdef AGENT_PENTER

static volatile LONG g_entryArmed;
static volatile LONG g_exitArmed;
static int           g_inHook;

/*
 * OURS. What _penter and _pexit call. Both are called with the
 * function's own return address just above theirs - _penter before
 * the prologue, _pexit after the epilogue's pop ebp, right before the
 * RET - so in both the operands are shifted to read as at the
 * function's entry: ra, then a0... [esp] is the return into the
 * function, which names it: the CALL's end at entry, the RET at exit.
 * At exit eax and st0 hold the return value.
 */
static void __cdecl penter_hit(HookRegs *r, int isExit, const unsigned char *fx)
{
	HookRegs v = *r;
	DWORD    ret = 0;
	int      i;

	if (g_inHook || GetCurrentThreadId() != g_hostThread) {
		return;
	}
	g_inHook = 1;
	mem_copy(&ret, (void *)r->esp, 4);
	v.esp = r->esp + 4;
	for (i = 0; i < HOOK_MAX; i++) {
		Hook *h = &g_hooks[i];

		if (!h->armed) {
			continue;
		}
		if ((!isExit && h->kind == KIND_ENTRY && h->addr == ret - 5)
		 || (isExit && h->kind == KIND_EXIT && h->addr <= ret && ret < h->end)) {
			hook_record(h, &v, fx);
		}
	}
	g_inHook = 0;
}

/* OURS. The /Gh hook: nothing armed costs one compare. */
__declspec(naked) void __cdecl _penter(void)
{
	__asm {
		cmp     g_entryArmed, 0
		jne     slow
		ret
	slow:
		pushad
		pushfd
		mov     eax, esp
		and     esp, -16
		sub     esp, 0x200
		fxsave  [esp]
		mov     ecx, esp
		fninit
		push    eax
		push    ecx
		push    0
		push    eax
		call    penter_hit
		add     esp, 12
		pop     eax
		fxrstor [esp]
		mov     esp, eax
		popfd
		popad
		ret
	}
}

/* OURS. The /GH hook, the same with isExit set. */
__declspec(naked) void __cdecl _pexit(void)
{
	__asm {
		cmp     g_exitArmed, 0
		jne     slow
		ret
	slow:
		pushad
		pushfd
		mov     eax, esp
		and     esp, -16
		sub     esp, 0x200
		fxsave  [esp]
		mov     ecx, esp
		fninit
		push    eax
		push    ecx
		push    1
		push    eax
		call    penter_hit
		add     esp, 12
		pop     eax
		fxrstor [esp]
		mov     esp, eax
		popfd
		popad
		ret
	}
}

/* OURS. True if the code at addr is CALL _penter, through up to two JMP thunks. */
static int is_instrumented(DWORD addr)
{
	unsigned char op = 0;
	LONG          rel = 0;
	DWORD         to;
	int           i;

	if (!mem_copy(&op, (void *)addr, 1) || op != 0xe8 || !mem_copy(&rel, (void *)(addr + 1), 4)) {
		return 0;
	}
	to = addr + 5 + (DWORD)rel;
	for (i = 0; i < 3; i++) {
		if (to == (DWORD)_penter) {
			return 1;
		}
		if (!mem_copy(&op, (void *)to, 1) || op != 0xe9 || !mem_copy(&rel, (void *)(to + 1), 4)) {
			return 0;
		}
		to = to + 5 + (DWORD)rel;
	}
	return 0;
}

#endif

/* OURS. The when= and log= of HOOK add; 0 with reply filled on a bad one. */
static int hook_parse(const char *line, Hook *h, char *reply)
{
	char  text[200];
	char *tok;
	char *op;

	if (Agent_Arg(line, "when", text, sizeof text)) {
		op = strpbrk(text, "=!<>");
		h->hasWhen = 1;
		if (op != NULL) {
			h->whenOp = *op;
			*op = 0;
			op += h->whenOp == '!' ? 2 : 1;
			h->whenNum = atof(op);
			h->whenU = strtoul(op, NULL, 0);
		}
		if (!tok_parse(text, &h->when)) {
			Agent_Say(reply, "ERR when=%s is not an operand\n", text);
			return 0;
		}
	}
	if (Agent_Arg(line, "texdump", text, sizeof text)) {
		if (!tok_parse(text, &h->tex)) {
			Agent_Say(reply, "ERR texdump=%s is not an operand\n", text);
			return 0;
		}
		if (!Agent_Arg(line, "path", h->texPath, sizeof h->texPath)) {
			Agent_Say(reply, "ERR texdump= needs path=<file>, a # in it the hit number\n");
			return 0;
		}
		h->hasTex = 1;
	}
	if (!Agent_Arg(line, "log", text, sizeof text)) {
		strcpy(text, "ra");
	}
	for (tok = strtok(text, ","); tok != NULL; tok = strtok(NULL, ",")) {
		if (h->nlog == HOOK_LOG_MAX || !tok_parse(tok, &h->log[h->nlog])) {
			Agent_Say(reply, "ERR log token '%s' is not an operand, or more than %d\n",
			          tok, HOOK_LOG_MAX);
			return 0;
		}
		h->nlog++;
	}
	return 1;
}

/* OURS. Arms h as a patch hook at its addr: decode, stub, jump. */
static int hook_patch(Hook *h, int slot, unsigned long want, char *reply)
{
	unsigned char code[HOOK_STEAL_MAX + 16];
	unsigned char jmp[HOOK_STEAL_MAX];
	LONG          rel;
	int           len = 0;
	int           n;
	int           i;

	if (!mem_copy(code, (void *)h->addr, sizeof code)) {
		Agent_Say(reply, "ERR 0x%08lx is not readable\n", h->addr);
		return 0;
	}
	while (len < 5) {
		n = insn_len(code + len);
		if (n == 0) {
			Agent_Say(reply, "ERR cannot relocate the instruction at 0x%08lx (bytes ",
			          h->addr + len);
			Agent_SayHex(reply, code + len, 4);
			Agent_Say(reply, ") - a branch, call, return or an opcode the decoder lacks\n");
			return 0;
		}
		len += n;
	}
	if (len > HOOK_STEAL_MAX || (want != 0 && (int)want != len)) {
		Agent_Say(reply, "ERR the instructions at 0x%08lx span %d bytes, len=%lu asked\n",
		          h->addr, len, want);
		return 0;
	}
	for (i = 0; i < HOOK_MAX; i++) {
		if (g_hooks[i].armed && g_hooks[i].kind == KIND_PATCH
		 && h->addr < g_hooks[i].addr + g_hooks[i].len && g_hooks[i].addr < h->addr + len) {
			Agent_Say(reply, "ERR overlaps hook %d at 0x%08lx\n", i, g_hooks[i].addr);
			return 0;
		}
	}
	h->len = len;
	memcpy(h->saved, code, len);
	stub_build(slot, h);
	memset(jmp, 0xcc, sizeof jmp);
	rel = (LONG)(g_stubs + slot * HOOK_STUB_SIZE) - (LONG)(h->addr + 5);
	jmp[0] = 0xe9;
	memcpy(jmp + 1, &rel, 4);
	if (!mem_patch(h->addr, jmp, len)) {
		Agent_Say(reply, "ERR cannot write 0x%08lx\n", h->addr);
		return 0;
	}
	return 1;
}

/* OURS. HOOK add addr=X [exit end=Y] [len=N] [tag=T] [cap=N] [when=..] [log=..] */
static void hook_add(const char *line, char *reply)
{
	static const char *kinds[] = { "patch", "entry", "exit" };
	Hook               h;
	unsigned long      addr;
	unsigned long      want = 0;
	unsigned long      cap = 200;
	unsigned long      end = 0;
	int                slot;

	if (!Agent_ArgU32(line, "addr", &addr)) {
		Agent_Say(reply, "ERR HOOK add needs addr=<hex>\n");
		return;
	}
	Agent_ArgU32(line, "len", &want);
	Agent_ArgU32(line, "cap", &cap);
	for (slot = 0; slot < HOOK_MAX && g_hooks[slot].armed; slot++) {
	}
	if (slot == HOOK_MAX) {
		Agent_Say(reply, "ERR all %d hook slots are in use\n", HOOK_MAX);
		return;
	}
	memset(&h, 0, sizeof h);
	h.addr = addr;
	h.cap = cap;
	if (!Agent_Arg(line, "tag", h.tag, sizeof h.tag)) {
		_snprintf(h.tag, sizeof h.tag, "%08lx", addr);
	}
	if (!hook_parse(line, &h, reply)) {
		return;
	}
#ifdef AGENT_PENTER
	if (is_instrumented(addr)) {
		h.kind = Agent_ArgFlag(line, "exit") ? KIND_EXIT : KIND_ENTRY;
		if (h.kind == KIND_EXIT && (!Agent_ArgU32(line, "end", &end) || end <= addr)) {
			Agent_Say(reply, "ERR an exit hook needs end=<hex>, the function's bound\n");
			return;
		}
		h.end = end;
	}
#endif
	(void)end;
	if (h.kind == KIND_PATCH && Agent_ArgFlag(line, "exit")) {
		Agent_Say(reply, "ERR 0x%08lx does not start with CALL _penter - exit hooks need"
		          " a function of the /Gh /GH build\n", addr);
		return;
	}
	if (h.kind == KIND_PATCH && !hook_patch(&h, slot, want, reply)) {
		return;
	}
	h.armed = 1;
	g_hooks[slot] = h;
#ifdef AGENT_PENTER
	if (h.kind == KIND_ENTRY) {
		InterlockedIncrement(&g_entryArmed);
	} else if (h.kind == KIND_EXIT) {
		InterlockedIncrement(&g_exitArmed);
	}
#endif
	agent_log("hook %d %s at 0x%08lx, tag %s", slot, kinds[h.kind], addr, h.tag);
	Agent_Say(reply, "OK HOOK idx=%d addr=0x%08lx kind=%s tag=%s", slot, addr, kinds[h.kind], h.tag);
	if (h.kind == KIND_PATCH) {
		Agent_Say(reply, " len=%d bytes=", h.len);
		Agent_SayHex(reply, h.saved, h.len);
	} else if (h.kind == KIND_EXIT) {
		Agent_Say(reply, " end=0x%08lx", h.end);
	}
	Agent_Say(reply, "\n");
}

/* OURS. Disarms slot; a patch hook's bytes go back if they are still our jump. */
static int hook_remove(int slot)
{
	Hook         *h = &g_hooks[slot];
	unsigned char now = 0;

	if (!h->armed) {
		return 0;
	}
	if (h->kind == KIND_PATCH) {
		if (!mem_copy(&now, (void *)h->addr, 1) || now != 0xe9
		 || !mem_patch(h->addr, h->saved, h->len)) {
			return 0;
		}
	}
#ifdef AGENT_PENTER
	if (h->kind == KIND_ENTRY) {
		InterlockedDecrement(&g_entryArmed);
	} else if (h->kind == KIND_EXIT) {
		InterlockedDecrement(&g_exitArmed);
	}
#endif
	h->armed = 0;
	agent_log("hook %d at 0x%08lx removed", slot, h->addr);
	return 1;
}

/* OURS */
static void cmd_hook(const char *line, char *reply)
{
	static const char *kinds[] = { "patch", "entry", "exit" };
	unsigned long      idx;
	int                i;
	int                n = 0;

	if (Agent_ArgFlag(line, "add")) {
		hook_add(line, reply);
	} else if (Agent_ArgFlag(line, "del")) {
		if (!Agent_ArgU32(line, "idx", &idx) || idx >= HOOK_MAX || !hook_remove((int)idx)) {
			Agent_Say(reply, "ERR HOOK del needs idx= of an armed hook\n");
		} else {
			Agent_Say(reply, "OK HOOK del idx=%lu\n", idx);
		}
	} else if (Agent_ArgFlag(line, "clear")) {
		for (i = 0; i < HOOK_MAX; i++) {
			n += hook_remove(i);
		}
		Agent_Say(reply, "OK HOOK clear removed=%d\n", n);
	} else if (Agent_ArgFlag(line, "list")) {
		for (i = 0; i < HOOK_MAX; i++) {
			n += g_hooks[i].armed;
		}
		Agent_Say(reply, "OK HOOK list lines=%d\n", n);
		for (i = 0; i < HOOK_MAX; i++) {
			if (g_hooks[i].armed) {
				Agent_Say(reply, "%d 0x%08lx %s len=%d tag=%s hits=%u cap=%u\n", i,
				          g_hooks[i].addr, kinds[g_hooks[i].kind], g_hooks[i].len, g_hooks[i].tag,
				          g_hooks[i].hits, g_hooks[i].cap);
			}
		}
	} else {
		Agent_Say(reply, "ERR HOOK takes add, del, clear or list\n");
	}
}

/* OURS. TRACE [keep]: the hits since the last drain, oldest first. */
static void cmd_trace(const char *line, char *reply)
{
	int first = (g_traceHead - g_traceCount + TRACE_MAX) % TRACE_MAX;
	int i;

	Agent_Say(reply, "OK TRACE lines=%d dropped=%u\n", g_traceCount, g_traceDropped);
	for (i = 0; i < g_traceCount; i++) {
		Agent_Say(reply, "%s\n", g_trace[(first + i) % TRACE_MAX]);
	}
	if (!Agent_ArgFlag(line, "keep")) {
		g_traceCount = 0;
		g_traceDropped = 0;
	}
}

/* OURS. WRITE addr=X value=HEX - code or data; was= is the undo. */
static void cmd_write(const char *line, char *reply)
{
	char          text[AGENT_WRITE_MAX * 2 + 2];
	unsigned char bytes[AGENT_WRITE_MAX];
	unsigned char was[AGENT_WRITE_MAX];
	unsigned long addr;
	int           n;
	int           i;

	if (!Agent_ArgU32(line, "addr", &addr) || !Agent_Arg(line, "value", text, sizeof text)) {
		Agent_Say(reply, "ERR WRITE needs addr=<hex> value=<hex bytes>\n");
		return;
	}
	n = (int)strlen(text) / 2;
	for (i = 0; i < n; i++) {
		int hi = hex_digit(text[i * 2]);
		int lo = hex_digit(text[i * 2 + 1]);

		if (hi < 0 || lo < 0) {
			n = 0;
			break;
		}
		bytes[i] = (unsigned char)(hi * 16 + lo);
	}
	if (n == 0 || strlen(text) % 2 != 0) {
		Agent_Say(reply, "ERR value= must be 1 to %d hex bytes\n", AGENT_WRITE_MAX);
		return;
	}
	if (!mem_copy(was, (void *)addr, n) || !mem_patch(addr, bytes, n)) {
		Agent_Say(reply, "ERR cannot write %d bytes at 0x%08lx\n", n, addr);
		return;
	}
	Agent_Say(reply, "OK WRITE addr=0x%08lx len=%d was=", addr, n);
	Agent_SayHex(reply, was, n);
	Agent_Say(reply, "\n");
}

/*
 * OURS. A GL texture's name, size and internal format into msg, and
 * with a path its level 0 as RGBA8 - "RTEX", width, height, then the
 * rows bottom up. The active unit's binding is put back. 0 with msg
 * starting "ERR" if there is nothing to dump.
 */
static int tex_dump(DWORD tex, const char *path, char *msg, size_t size)
{
	unsigned char *pixels;
	GLint          was = 0;
	GLint          w = 0;
	GLint          h = 0;
	GLint          format = 0;
	size_t         used;

	glGetIntegerv(GL_TEXTURE_BINDING_2D_, &was);
	glBindTexture(GL_TEXTURE_2D, tex);
	glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &w);
	glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &h);
	glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_INTERNAL_FORMAT, &format);
	if (w <= 0 || h <= 0 || w > TEXDUMP_MAX || h > TEXDUMP_MAX) {
		glBindTexture(GL_TEXTURE_2D, (GLuint)was);
		_snprintf(msg, size, "ERR texture %lu has no 2D level 0 (%dx%d)", tex, w, h);
		msg[size - 1] = 0;
		return 0;
	}
	_snprintf(msg, size, "%lu width=%d height=%d format=0x%x", tex, w, h, format);
	msg[size - 1] = 0;
	if (path != NULL) {
		pixels = (unsigned char *)malloc((size_t)w * h * 4);
		if (pixels != NULL) {
			glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
		}
		if (pixels == NULL || !Agent_WriteRtex(path, w, h, pixels)) {
			used = strlen(msg);
			_snprintf(msg + used, size - used, " - cannot write the file");
			msg[size - 1] = 0;
		}
		free(pixels);
	}
	glBindTexture(GL_TEXTURE_2D, (GLuint)was);
	return 1;
}

/* OURS. TEXDUMP tex=N [path=FILE]: tex_dump, between frames. */
static void cmd_texdump(const char *line, char *reply)
{
	char          path[MAX_PATH];
	char          msg[160];
	unsigned long tex;
	int           hasPath;

	if (!Agent_ArgU32(line, "tex", &tex)) {
		Agent_Say(reply, "ERR TEXDUMP needs tex=<GL texture name>\n");
		return;
	}
	hasPath = Agent_Arg(line, "path", path, sizeof path);
	if (!tex_dump(tex, hasPath ? path : NULL, msg, sizeof msg)) {
		Agent_Say(reply, "%s\n", msg);
		return;
	}
	Agent_Say(reply, "OK TEXDUMP tex=%s", msg);
	if (hasPath) {
		Agent_Say(reply, " path=%s", path);
	}
	Agent_Say(reply, "\n");
}

/* OURS */
int Agent_WriteRtex(const char *path, int w, int h, const unsigned char *pixels)
{
	FILE *f = fopen(path, "wb");

	if (f == NULL) {
		return 0;
	}
	fwrite("RTEX", 1, 4, f);
	fwrite(&w, 4, 1, f);
	fwrite(&h, 4, 1, f);
	fwrite(pixels, 4, (size_t)w * h, f);
	fclose(f);
	return 1;
}

/* OURS. The commands that run on the host's thread; 2 if the host will reply later. */
static int agent_execute(const char *line, char *reply)
{
	int done = 1;

	if (strncmp(line, "TEXDUMP", 7) == 0) {
		cmd_texdump(line, reply);
	} else if (strncmp(line, "WRITE", 5) == 0) {
		cmd_write(line, reply);
	} else if (strncmp(line, "HOOK", 4) == 0) {
		cmd_hook(line, reply);
	} else if (strncmp(line, "TRACE", 5) == 0) {
		cmd_trace(line, reply);
	} else if (g_host->execute == NULL || (done = g_host->execute(line, reply)) == 0) {
		Agent_Say(reply, "ERR unknown command - STATUS READ WRITE HOOK TRACE TEXDUMP %s\n",
		          g_host->verbs != NULL ? g_host->verbs : "");
		done = 1;
	}
	return done;
}

/* OURS */
void Agent_Service(void)
{
	if (!g_mbPending) {
		return;
	}
	EnterCriticalSection(&g_mbLock);
	if (g_mbPending) {
		if (g_mbDeferred) {
			g_mbDeferred = g_host->poll == NULL || !g_host->poll(g_mbReply) ? 1 : 0;
		} else {
			g_mbDeferred = agent_execute(g_mbLine, g_mbReply) == 2;
		}
		if (!g_mbDeferred) {
			g_mbPending = 0;
			SetEvent(g_mbDone);
		}
	}
	LeaveCriticalSection(&g_mbLock);
}

/*
 * OURS. Parks line for Agent_Service and waits for its reply. A command
 * the host has taken holds the lock until it finishes, so a long one -
 * a STEP of many frames - is waited for, not abandoned; so is one the
 * host has deferred. One never taken times out. Clients take turns:
 * a second command waits for the first, while STATUS, READ and CANCEL
 * never wait.
 */
static void mailbox_call(const char *line, char *reply)
{
	EnterCriticalSection(&g_callLock);
	g_cancel = 0;
	EnterCriticalSection(&g_mbLock);
	strcpy(g_mbLine, line);
	g_mbReply = reply;
	ResetEvent(g_mbDone);
	g_mbDeferred = 0;
	g_mbPending = 1;
	LeaveCriticalSection(&g_mbLock);
	while (WaitForSingleObject(g_mbDone, AGENT_WAIT_MS) != WAIT_OBJECT_0) {
		EnterCriticalSection(&g_mbLock);
		if (g_mbPending && !g_mbDeferred) {
			g_mbPending = 0;
			Agent_Say(reply, "ERR the host is not running its loop (window inactive?) - nothing ran\n");
			LeaveCriticalSection(&g_mbLock);
			break;
		}
		LeaveCriticalSection(&g_mbLock);
	}
	LeaveCriticalSection(&g_callLock);
}

/* OURS. True once CANCEL has been sent during the running command. */
int Agent_Cancelled(void)
{
	return g_cancel != 0;
}

/* OURS. READ addr=X len=N, on the pipe thread. */
static void cmd_read(const char *line, char *reply)
{
	unsigned char buf[AGENT_READ_MAX];
	unsigned long addr;
	unsigned long len = 0;

	if (!Agent_ArgU32(line, "addr", &addr) || !Agent_ArgU32(line, "len", &len)
	 || len == 0 || len > AGENT_READ_MAX) {
		Agent_Say(reply, "ERR READ needs addr=<hex> len=1..%d\n", AGENT_READ_MAX);
		return;
	}
	if (!mem_copy(buf, (void *)addr, len)) {
		Agent_Say(reply, "ERR 0x%08lx..+%lu is not readable\n", addr, len);
		return;
	}
	Agent_Say(reply, "OK READ addr=0x%08lx len=%lu lines=1\n", addr, len);
	Agent_SayHex(reply, buf, (int)len);
	Agent_Say(reply, "\n");
}

/*
 * OURS. STATUS, READ and CANCEL answer on the pipe thread, at once, even
 * while another client's command runs; STATUS then names it as busy=.
 */
static void agent_line(char *line, char *reply)
{
	size_t n = strlen(line);
	char   verb[16];
	int    i;
	int    armed = 0;

	while (n > 0 && (line[n - 1] == '\r' || line[n - 1] == ' ')) {
		line[--n] = 0;
	}
	reply[0] = 0;
	if (strncmp(line, "STATUS", 6) == 0) {
		for (i = 0; i < HOOK_MAX; i++) {
			armed += g_hooks[i].armed;
		}
		Agent_Say(reply, "OK STATUS frame=%u", g_frame);
		if (g_host->status != NULL) {
			g_host->status(reply);
		}
		Agent_Say(reply, " hooks=%d trace=%d", armed, g_traceCount);
		if (g_mbPending) {
			for (i = 0; i < (int)sizeof verb - 1 && g_mbLine[i] > ' '; i++) {
				verb[i] = g_mbLine[i];
			}
			verb[i] = 0;
			Agent_Say(reply, " busy=%s", verb);
		}
		Agent_Say(reply, "\n");
	} else if (strncmp(line, "READ", 4) == 0) {
		cmd_read(line, reply);
	} else if (strncmp(line, "CANCEL", 6) == 0) {
		g_cancel = 1;
		Agent_Say(reply, "OK CANCEL running=%d\n", g_mbPending ? 1 : 0);
	} else {
		mailbox_call(line, reply);
	}
}

/* OURS. Writes all of text, or gives up. */
static int pipe_write(HANDLE pipe, const char *text)
{
	DWORD left = (DWORD)strlen(text);
	DWORD done;

	while (left > 0) {
		if (!WriteFile(pipe, text, left, &done, NULL) || done == 0) {
			return 0;
		}
		text += done;
		left -= done;
	}
	return 1;
}

/* OURS. One client on its own thread: lines in until it hangs up. */
static DWORD WINAPI pipe_session(void *param)
{
	HANDLE pipe = (HANDLE)param;
	char  *reply = (char *)malloc(AGENT_REPLY_MAX);
	char   line[AGENT_LINE_MAX];
	int    used = 0;
	char   c;
	DWORD  got;

	while (reply != NULL && ReadFile(pipe, &c, 1, &got, NULL) && got == 1) {
		if (c != '\n') {
			if (used < AGENT_LINE_MAX - 1) {
				line[used++] = c;
			}
			continue;
		}
		line[used] = 0;
		used = 0;
		agent_line(line, reply);
		if (!pipe_write(pipe, reply)) {
			break;
		}
	}
	free(reply);
	FlushFileBuffers(pipe);
	DisconnectNamedPipe(pipe);
	CloseHandle(pipe);
	return 0;
}

/* OURS. Accepts clients, any number at once, each on a thread of its own. */
static DWORD WINAPI agent_thread(void *param)
{
	HANDLE pipe;
	HANDLE session;

	(void)param;
	for (;;) {
		pipe = CreateNamedPipeA(g_host->pipe, PIPE_ACCESS_DUPLEX,
		                        PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT
		                        | PIPE_REJECT_REMOTE_CLIENTS,
		                        PIPE_UNLIMITED_INSTANCES, AGENT_REPLY_MAX, AGENT_LINE_MAX, 0, NULL);
		if (pipe == INVALID_HANDLE_VALUE) {
			agent_log("agent: CreateNamedPipe failed, %lu", GetLastError());
			return 1;
		}
		if (!ConnectNamedPipe(pipe, NULL) && GetLastError() != ERROR_PIPE_CONNECTED) {
			CloseHandle(pipe);
			continue;
		}
		session = CreateThread(NULL, 0, pipe_session, pipe, 0, NULL);
		if (session == NULL) {
			DisconnectNamedPipe(pipe);
			CloseHandle(pipe);
			continue;
		}
		CloseHandle(session);
	}
}

/* OURS */
int Agent_Start(const Agent_Host *host)
{
	HANDLE thread;

	g_host = host;
	g_hostThread = GetCurrentThreadId();
	g_stubs = (unsigned char *)VirtualAlloc(NULL, HOOK_MAX * HOOK_STUB_SIZE,
	                                        MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
	InitializeCriticalSection(&g_mbLock);
	InitializeCriticalSection(&g_callLock);
	g_mbDone = CreateEventA(NULL, TRUE, FALSE, NULL);
	if (g_stubs == NULL || g_mbDone == NULL) {
		agent_log("agent: cannot allocate - not started");
		return 0;
	}
	thread = CreateThread(NULL, 0, agent_thread, NULL, 0, NULL);
	if (thread == NULL) {
		agent_log("agent: cannot start its thread");
		return 0;
	}
	CloseHandle(thread);
	agent_log("agent listening on %s, %d hook slots", host->pipe, HOOK_MAX);
	return 1;
}
