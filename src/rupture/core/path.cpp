#include "path.h"

#include <string.h>

#include "file.h"

/* ORIGIN 0x00441ed0 Path_Construct. */
Path *Path_Construct(Path *p)
{
	Track_Construct(&p->pointTrack);
	Track_Construct(&p->aTrack);
	Track_Construct(&p->bTrack);
	return p;
}

/*
 * OURS. fread over the bytes File_Read gave: a short read sets eof and
 * leaves dst as it was, as the CRT's does for a whole-record fread.
 */
static void read_field(const std::vector<u8> &buf, size_t *pos, bool *eof, void *dst, size_t size)
{
	if (*pos + size > buf.size()) {
		*pos = buf.size();
		*eof = true;
		return;
	}
	memcpy(dst, &buf[*pos], size);
	*pos += size;
}

/*
 * OURS. Path_Load's scalar track: (i, v[i], 0) per entry, looping off,
 * resampled down to n / div points and back up to n.
 */
static void scalar_track(Track *t, const std::vector<double> &v, size_t n, double div)
{
	Vec4   q;
	size_t i;

	Track_Construct(t);
	q.kind = 1;
	q.z = 0.0f;
	for (i = 0; i < n; i++) {
		q.x = (float)i;
		q.y = (float)v[i];
		t->points.push_back(q);
		t->count++;
	}
	t->loop = 0;
	Track_Resample(t, (int)((double)n / div), 3);
	Track_Resample(t, (int)n, 3);
}

/*
 * ORIGIN 0x0045caf0 Path_Load. A missing file leaves the path as it
 * was. The read loop tests feof before each record, so the last record
 * is pushed twice. Then each column is smoothed into its track - resampled
 * by arc length down to a fifth (points), a twentieth (b) or a tenth
 * (a) of its entries and back up - and the two double columns are
 * overwritten with the smoothed y, b first.
 */
void Path_Load(Path *p, const char *file)
{
	std::vector<u8> buf;
	size_t          pos = 0, n, i;
	bool            eof = false;
	float           x = 0.0f, y = 0.0f, z = 0.0f;
	double          a = 0.0, b = 0.0;
	Vec4            v;
	Track           t;

	if (!File_Read(file, buf)) {
		return;
	}
	p->points.clear();
	p->b.clear();
	p->a.clear();
	while (!eof) {
		read_field(buf, &pos, &eof, &x, 4);
		read_field(buf, &pos, &eof, &y, 4);
		read_field(buf, &pos, &eof, &z, 4);
		read_field(buf, &pos, &eof, &a, 8);
		read_field(buf, &pos, &eof, &b, 8);
		v.kind = 1;
		v.x = x;
		v.y = y;
		v.z = z;
		p->points.push_back(v);
		p->b.push_back(b);
		p->a.push_back(a);
	}
	n = p->points.size();
	Track_Construct(&t);
	for (i = 0; i < n; i++) {
		t.points.push_back(p->points[i]);
		t.count++;
	}
	t.loop = 0;
	Track_Resample(&t, (int)((double)n / 5.0), 3);       /* 0x004f40c8 */
	Track_Resample(&t, (int)n, 3);
	Track_Copy(&p->pointTrack, &t);
	scalar_track(&t, p->b, n, 20.0);                     /* 0x004f3798 */
	Track_Copy(&p->bTrack, &t);
	for (i = 0; i < n; i++) {
		p->b[i] = (double)t.points[i].y;
	}
	scalar_track(&t, p->a, n, 10.0);                     /* 0x004f43b8 */
	Track_Copy(&p->aTrack, &t);
	for (i = 0; i < n; i++) {
		p->a[i] = (double)t.points[i].y;
	}
}
