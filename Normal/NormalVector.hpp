#pragma once
#ifndef HALFTONE_MATHS_H
#define HALFTONE_MATHS_H

// PF_Fixed = long, PF_FpLong = double
#define D2FIX(x) (((long)x)<<16)
#define FIX2D(x) (x / ((double)(1L << 16)))
#define PI2 6.28318531
#define PI 3.14159265
#define HALF_PI 1.57079632
#define SQRT2 1.41421356
#define SQRT2_HALF 0.70710678
#define SQRT2_MINUS_ONE 0.41421356
#define SQRT2_HALF_MINUS_ONE 0.20710678
#define CLAMP(mn, mx, x) (max(mn, min(mx, x)))
#define BLEND(a, b, x) (a * (1 - x) + b * x)

struct Vector {
	double x;
	double y;

	Vector(double x_start, double y_start) : x(x_start), y(y_start) {}
	
	void set(double x_set, double y_set) {
		x = x_set;
		y = y_set;
	}

	double getMagnitude() {
		return hypot(x, y);
	}

	double getDistanceTo(Vector *p) {
		return hypot(p->x - x, p->y - y);
	}

	void clamp(double x_max, double y_max) {
		x = CLAMP(0.0, x_max, x);
		y = CLAMP(0.0, y_max, y);
	}
};

Vector getGridPoint(double x, double y, Vector origin, Vector normal, double step, double half_step) {
	// project point onto rotated grid, get nearest gridpoint
	double dot_normal = normal.x * (x - origin.x) + normal.y * (y - origin.y);
	double dot_line = -normal.y * (x - origin.x) + normal.x * (y - origin.y);
	double offset_x = normal.x * dot_normal;
	double offset_y = normal.y * dot_normal;
	double offset_normal = fmod(hypot(offset_x, offset_y), step);
	double normal_dir = (dot_normal < 0) ? 1 : -1;
	double normal_scale = ((offset_normal < half_step) ? -offset_normal : step - offset_normal) * normal_dir;
	double offset_line = fmod(hypot((x - offset_x) - origin.x, (y - offset_y) - origin.y), step);
	double line_dir = (dot_line < 0) ? 1 : -1;
	double line_scale = ((offset_line < half_step) ? -offset_line : step - offset_line) * line_dir;

	// set nearest gridpoint in new vector
	return Vector(
		x - normal.x * normal_scale + normal.y * line_scale,
		y - normal.y * normal_scale - normal.x * line_scale
	);
}

#endif // HALFTONE_MATHS_H