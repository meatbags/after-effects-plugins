#pragma once
#ifndef HALFTONE_SAMPLER_H
#define HALFTONE_SAMPLER_H


#define SHAPE_ROUND 1
#define SHAPE_SQUARE 2
#define SHAPE_EUCLID 3
#define SHAPE_ELLIPSE 4
#define SHAPE_LINE 5
#define SHAPE_DIAMOND 6
#define SHAPE_FLOWER 7

PF_Pixel16 *getPixel16(
	PF_EffectWorld *inputP,
	int x,
	int y
) {
	return ((PF_Pixel16 *)((char*)inputP->data + (y * inputP->rowbytes) + x * sizeof(PF_Pixel16)));
}

PF_Pixel8 *getPixel8(
	PF_EffectWorld *inputP,
	int x,
	int y
) {
	return ((PF_Pixel8 *)((char*)inputP->data + (y * inputP->rowbytes) + x * sizeof(PF_Pixel8)));
}

struct Samples8 {
	PF_Pixel8 *sample8_1;
	PF_Pixel8 *sample8_2;
	PF_Pixel8 *sample8_3;
	PF_Pixel8 *sample8_4;
};

struct Samples16 {
	PF_Pixel16 *sample16_1;
	PF_Pixel16 *sample16_2;
	PF_Pixel16 *sample16_3;
	PF_Pixel16 *sample16_4;
};

struct Sampler {
	double angle = 0;
	Vector normal = Vector(0, 0);
	Vector p1 = Vector(0, 0);
	Vector p2 = Vector(0, 0);
	Vector p3 = Vector(0, 0);
	Vector p4 = Vector(0, 0);

	Sampler(double x, double y, Vector n) {
		p1.x = x;
		p1.y = y;
		normal.x = n.x;
		normal.y = n.y;
		angle = atan2(n.y, n.x);
	}

	double distanceToShapeRadius(
		double radius,
		double radius_max,
		Vector point,
		Vector dot,
		A_u_char shape
	) {
		// shape the distance and radius
		double dist = hypot(point.x - dot.x, point.y - dot.y);
		double rad = radius;

		if (shape == SHAPE_ROUND) {
			rad = pow(rad, 1.125) * radius_max;
		}
		else if (shape == SHAPE_SQUARE) {
			double theta = atan2(dot.y - point.y, dot.x - point.x) - angle;
			double st = abs(sin(theta));
			double ct = abs(cos(theta));
			rad = (rad + ((st > ct) ? rad - st * rad : rad - ct * rad)) * radius_max;
		}
		else if (shape == SHAPE_EUCLID) {
			double theta = atan2(dot.y - point.y, dot.x - point.x);
			double st = abs(sin(theta));
			double ct = abs(cos(theta));
			double square = rad + ((st > ct) ? rad - st * rad : rad - ct * rad);
			
			if (rad <= 0.5) {
				rad = BLEND(rad, square, rad * 2) * radius_max;
			} else {
				double r_max = rad + abs(SQRT2_MINUS_ONE * sin(2 * theta) * rad);
				rad = BLEND(square, r_max, pow((rad - 0.5) * 2, 0.4)) * radius_max;
			}
		}
		else if (shape == SHAPE_ELLIPSE) {
			rad = pow(rad, 1.125) * radius_max;

			if (dist != 0.0) {
				double dot_p = abs((point.x - dot.x) / dist * normal.x + (point.y - dot.y) / dist * normal.y);
				dist = (dist * (1.0 - SQRT2_HALF_MINUS_ONE)) + dot_p * dist * SQRT2_MINUS_ONE;
			}
		}
		else if (shape == SHAPE_LINE) {
			rad = pow(rad, 1.5) * radius_max;
			double dp = (point.x - dot.x) * normal.x + (point.y - dot.y) * normal.y;
			dist = hypot(normal.x * dp, normal.y * dp);
		}
		else if (shape == SHAPE_DIAMOND) {
			double theta = atan2(dot.y - point.y, dot.x - point.x);
			rad = (rad - abs(SQRT2_HALF_MINUS_ONE * sin(2 * theta) * rad)) * radius_max;
		}
		else { // SHAPE_FLOWER
			double theta = atan2(dot.y - point.y, dot.x - point.x);
			rad = rad * SQRT2 * abs(sin(2 * theta)) * radius_max;
		}
		
		return rad - dist;
	}

	PF_Err write8(
		int channel,
		A_u_char *ch,
		Vector point,
		A_u_char mode,
		double radius,
		double aa,
		Samples8 *samp
	) {
		PF_Err err = PF_Err_NONE;

		// write to target channel
		if (channel == 1) {
			writeChannel8(ch, samp->sample8_1->red, point, p1, mode, radius, aa);
			writeChannel8(ch, samp->sample8_2->red, point, p2, mode, radius, aa);
			writeChannel8(ch, samp->sample8_3->red, point, p3, mode, radius, aa);
			writeChannel8(ch, samp->sample8_4->red, point, p4, mode, radius, aa);
		} else if (channel == 2) {
			writeChannel8(ch, samp->sample8_1->green, point, p1, mode, radius, aa);
			writeChannel8(ch, samp->sample8_2->green, point, p2, mode, radius, aa);
			writeChannel8(ch, samp->sample8_3->green, point, p3, mode, radius, aa);
			writeChannel8(ch, samp->sample8_4->green, point, p4, mode, radius, aa);
		} else {
			writeChannel8(ch, samp->sample8_1->blue, point, p1, mode, radius, aa);
			writeChannel8(ch, samp->sample8_2->blue, point, p2, mode, radius, aa);
			writeChannel8(ch, samp->sample8_3->blue, point, p3, mode, radius, aa);
			writeChannel8(ch, samp->sample8_4->blue, point, p4, mode, radius, aa);
		}

		return err;
	}

	void writeChannel8(
		A_u_char *out,
		A_u_char sample,
		Vector point,
		Vector dot,
		A_u_char mode,
		double radius_max,
		double aa
	) {
		double res = distanceToShapeRadius(sample / 255.0, radius_max, point, dot, mode);

		if (res > 0) {
			*out = (A_u_char)min(255.0, (*out + (CLAMP(0.0, 1.0, res / aa) * 255)));
		}
	}

	PF_Err sample8(
		HalftoneInfo *info,
		Samples8 *samp
	) {
		PF_Err err = PF_Err_NONE;
		double width = info->samp_pb.src->width - 1;
		double height = info->samp_pb.src->height - 1;
		
		// get sample points
		Vector s1 = getGridPoint(p1.x, p1.y, info->origin, normal, info->grid_step, info->grid_half_step);
		Vector s2 = getGridPoint(p2.x, p2.y, info->origin, normal, info->grid_step, info->grid_half_step);
		Vector s3 = getGridPoint(p3.x, p3.y, info->origin, normal, info->grid_step, info->grid_half_step);
		Vector s4 = getGridPoint(p4.x, p4.y, info->origin, normal, info->grid_step, info->grid_half_step);

		// restrict bounds
		s1.clamp(width, height);
		s2.clamp(width, height);
		s3.clamp(width, height);
		s4.clamp(width, height);

		// get sample pixels
		samp->sample8_1 = getPixel8(info->ref, (int)s1.x, (int)s1.y);
		samp->sample8_2 = getPixel8(info->ref, (int)s2.x, (int)s2.y);
		samp->sample8_3 = getPixel8(info->ref, (int)s3.x, (int)s3.y);
		samp->sample8_4 = getPixel8(info->ref, (int)s4.x, (int)s4.y);

		return err;
	}

	PF_Err write16(
		int channel,
		A_u_short *ch,
		Vector point,
		A_u_char shape,
		double radius_max,
		double aa,
		Samples16 *samp
	) {
		PF_Err err = PF_Err_NONE;

		// write target channel
		if (channel == 1) {
			writeChannel16(ch, samp->sample16_1->red, point, p1, shape, radius_max, aa);
			writeChannel16(ch, samp->sample16_2->red, point, p2, shape, radius_max, aa);
			writeChannel16(ch, samp->sample16_3->red, point, p3, shape, radius_max, aa);
			writeChannel16(ch, samp->sample16_4->red, point, p4, shape, radius_max, aa);
		}
		else if (channel == 2) {
			writeChannel16(ch, samp->sample16_1->green, point, p1, shape, radius_max, aa);
			writeChannel16(ch, samp->sample16_2->green, point, p2, shape, radius_max, aa);
			writeChannel16(ch, samp->sample16_3->green, point, p3, shape, radius_max, aa);
			writeChannel16(ch, samp->sample16_4->green, point, p4, shape, radius_max, aa);
		}
		else {
			writeChannel16(ch, samp->sample16_1->blue, point, p1, shape, radius_max, aa);
			writeChannel16(ch, samp->sample16_2->blue, point, p2, shape, radius_max, aa);
			writeChannel16(ch, samp->sample16_3->blue, point, p3, shape, radius_max, aa);
			writeChannel16(ch, samp->sample16_4->blue, point, p4, shape, radius_max, aa);
		}

		return err;
	}

	void writeChannel16(
		A_u_short *out,
		A_u_short sample,
		Vector point,
		Vector dot,
		A_u_char shape,
		double radius_max,
		double aa
	) {
		double res = distanceToShapeRadius(sample / (double)PF_MAX_CHAN16, radius_max, point, dot, shape);

		if (res > 0) {
			*out = (A_u_short)min((double)PF_MAX_CHAN16, (*out + (CLAMP(0.0, 1.0, res / aa) * PF_MAX_CHAN16)));
		}
	}

	PF_Err sample16(
		HalftoneInfo *info,
		Samples16 *samp
	) {
		PF_Err err = PF_Err_NONE;
		double width = info->samp_pb.src->width - 1;
		double height = info->samp_pb.src->height - 1;

		Vector s1 = getGridPoint(p1.x, p1.y, info->origin, normal, info->grid_step, info->grid_half_step);
		Vector s2 = getGridPoint(p2.x, p2.y, info->origin, normal, info->grid_step, info->grid_half_step);
		Vector s3 = getGridPoint(p3.x, p3.y, info->origin, normal, info->grid_step, info->grid_half_step);
		Vector s4 = getGridPoint(p4.x, p4.y, info->origin, normal, info->grid_step, info->grid_half_step);

		s1.clamp(width, height);
		s2.clamp(width, height);
		s3.clamp(width, height);
		s4.clamp(width, height);

		samp->sample16_1 = getPixel16(info->ref, (int)s1.x, (int)s1.y);
		samp->sample16_2 = getPixel16(info->ref, (int)s2.x, (int)s2.y);
		samp->sample16_3 = getPixel16(info->ref, (int)s3.x, (int)s3.y);
		samp->sample16_4 = getPixel16(info->ref, (int)s4.x, (int)s4.y);

		return err;
	}
};

Sampler getSampler(double x, double y, Vector origin, Vector normal, double step, double half_step) {
	// project point onto rotated grid, get containing cell
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

	// set first point to nearest gridpoint
	Sampler res = Sampler(
		x - normal.x * normal_scale + normal.y * line_scale,
		y - normal.y * normal_scale - normal.x * line_scale,
		normal
	);

	// calculate cell corners
	double normal_step = normal_dir * ((offset_normal < half_step) ? step : -step);
	double line_step = line_dir * ((offset_line < half_step) ? step : -step);
	res.p2.x = res.p1.x - normal.x * normal_step;
	res.p2.y = res.p1.y - normal.y * normal_step;
	res.p3.x = res.p1.x + normal.y * line_step;
	res.p3.y = res.p1.y - normal.x * line_step;
	res.p4.x = res.p1.x - normal.x * normal_step + normal.y * line_step;
	res.p4.y = res.p1.y - normal.y * normal_step - normal.x * line_step;

	return res;
}

#endif