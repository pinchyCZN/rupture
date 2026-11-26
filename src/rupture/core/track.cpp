#include "track.h"

/* ORIGIN 0x0040d800 Track_Construct. A new track loops. */
Track *Track_Construct(Track *t)
{
	Track_Clear(t);
	t->loop = 1;
	return t;
}

/*
 * ORIGIN 0x00417360 Track_ConstructMode. The loop byte is not set: the
 * demo leaves it as the stack had it. Here it is 0; Track_Resample,
 * the one caller, reads its temporary only at whole indices, where
 * looping or not gives the same point.
 */
Track *Track_ConstructMode(Track *t, int mode)
{
	Track_Clear(t);
	t->mode = mode;
	t->loop = 0;
	return t;
}

/* ORIGIN 0x0040d0d0 Track_Clear. */
void Track_Clear(Track *t)
{
	t->unk58 = 0.0;
	t->count = 0;
	t->mode = 3;
	t->points.clear();
}

/* OURS. Track_Eval's index clamp: below n - 1, else n - 1; then not below 0. */
static unsigned short clamp_index(int i, int n)
{
	int c = i < n - 1 ? i : n - 1;

	return (unsigned short)(c > 0 ? c : 0);
}

/*
 * ORIGIN 0x004b3ed0 Track_Eval. A Catmull-Rom through points[i - 1 ..
 * i + 2] at i = (int)s, f = s - i, written per component as the code
 * has it. Indices are 16-bit. Not looping, the ends clamp; past the
 * count the result is the last point and below 0 the first, kind and
 * all. Looping, an s outside 2 .. n - 2 is first pushed up by n * 10000
 * and rounded to float, and the indices wrap. The kind of out is left
 * as the caller set it.
 */
void Track_Eval(const Track *t, float s, Vec4 *out)
{
	const Vec4    *p = &t->points[0];
	int            n = t->count, i;
	unsigned short i0, im1, ip1, ip2;
	short          i0s;
	float          f, f2, p0, p1, p2, p3, m1, m2;
	int            k;

	if (t->loop == 0) {
		if (2.0f < s && s < (float)(n - 2)) {
			i = (int)s;
			f = s - (float)i;
			i0 = (unsigned short)i;
		} else {
			i = (int)s;
			f = s - (float)i;
			if ((float)n < s) {
				f = 1.0f;
			}
			if (s < 0.0f) {
				f = 0.0f;
			}
			i0 = clamp_index(i, n);
		}
		i0s = (short)i0;
		im1 = clamp_index(i0s - 1, n);
		i0 = clamp_index(i0s, n);
		ip1 = clamp_index(i0s + 1, n);
		ip2 = clamp_index(i0s + 2, n);
	} else {
		if (2.0f < s && s < (float)(n - 2)) {
			i = (int)s;
			f = s - (float)i;
			i0 = (unsigned short)i;
		} else {
			s = (float)(s + n * 10000);
			i = (int)s;
			f = s - (float)i;
			i0 = (unsigned short)(i % n);
		}
		i0s = (short)i0;
		if (i0s < n - 2 && i0s > 0) {
			im1 = (unsigned short)(i0 - 1);
			ip1 = (unsigned short)(i0 + 1);
			ip2 = (unsigned short)(i0 + 2);
		} else if (i0s == 0) {
			i0 = 0;
			ip1 = 1;
			ip2 = 2;
			im1 = (unsigned short)((i - 1) % n);
		} else {
			im1 = (unsigned short)(i0 - 1);
			ip1 = (unsigned short)((i0s + 1) % n);
			ip2 = (unsigned short)((i0s + 2) % n);
		}
	}
	f2 = f * f;
	for (k = 0; k < 3; k++) {
		p0 = (&p[im1].x)[k];
		p1 = (&p[i0].x)[k];
		p2 = (&p[ip1].x)[k];
		p3 = (&p[ip2].x)[k];
		m1 = (p2 - p0) * 0.5f;
		m2 = (p3 - p1) * 0.5f;
		(&out->x)[k] = p1 + m1 * f
		             + f2 * (f * (m2 + m1 + (p1 * 2.0f - 2.0f * p2))
		                     + (((p2 * 3.0f - 3.0f * p1) - (m1 + m1)) - m2));
	}
	if (t->loop == 0) {
		if ((float)n < s) {
			*out = p[n - 1];
		}
		if (s < 0.0f) {
			*out = p[0];
		}
	}
}

/*
 * ORIGIN 0x004b51b0 Track_Resample: the track re-evaluated at n
 * parameters equally spaced by arc length, measured over count * sub
 * steps. A segment shorter than 0.001 (0x004f3858, 0x004f3860) is
 * taken as 0.001. The demo pushes each point into all three of its
 * vectors; only the first is carried. Read from the decompile; the
 * x87 forms are not checked [open].
 */
void Track_Resample(Track *t, int n, int sub)
{
	std::vector<float> len;
	Track              tmp;
	Vec4               a, b, d, c;
	float              total = 0.0f, target, seg, u;
	int                i, j, k, steps;

	if (t->points.size() <= 3) {
		return;
	}
	Track_ConstructMode(&tmp, 1);
	if (sub == 0) {
		return;
	}
	steps = t->count * sub;
	len.resize(steps + 1);
	len[0] = 0.0f;
	for (i = 0; i < steps; i++) {
		a.kind = 1;
		a.x = a.y = a.z = 0.0f;
		b = a;
		Track_Eval(t, (float)i / (float)sub, &a);
		Track_Eval(t, (float)(i + 1) / (float)sub, &b);
		Vec4_Sub(&a, &b, &d);
		total = Vec4_Length(&d) + total;
		len[i + 1] = total;
	}
	c.kind = 1;
	c.x = c.y = c.z = 0.0f;
	for (k = 0; k < n; k++) {
		target = ((float)k / (float)n) * total;
		for (j = 0; j < steps; j++) {
			if (len[j] <= target && target <= len[j + 1]) {
				break;
			}
		}
		if (j == steps) {
			j = steps - 1;                /* the demo reads past its table here */
		}
		seg = len[j + 1] - len[j];
		u = ((target - len[j]) / (seg <= 0.001f ? 0.001f : seg) + (float)j) / (float)sub;
		Track_Eval(t, u, &c);
		tmp.points.push_back(c);
		tmp.count++;
	}
	Track_Clear(t);
	for (k = 0; k < n; k++) {
		Track_Eval(&tmp, (float)k, &c);
		t->points.push_back(c);
		t->count++;
	}
}

/* ORIGIN 0x004b50c0 Track_Copy. */
Track *Track_Copy(Track *t, const Track *src)
{
	size_t i;

	Track_Clear(t);
	t->count = src->count;
	t->mode = src->mode;
	t->loop = src->loop;
	for (i = 0; i < src->points.size(); i++) {
		t->points.push_back(src->points[i]);
	}
	return t;
}
