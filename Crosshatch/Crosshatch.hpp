#pragma once
#ifndef HALFTONE_H
#define HALFTONE_H

#define PF_TABLE_BITS 12
#define PF_TABLE_SZ_16 4096
#define PF_DEEP_COLOR_AWARE 1
#include "AEConfig.h"

#ifdef AE_OS_WIN
typedef unsigned short PixelType;
#include <Windows.h>
#endif

#include <math.h>
#include <random>
#include "entry.h"
#include "AE_Effect.h"
#include "AE_EffectCB.h"
#include "AE_Macros.h"
#include "Param_Utils.h"
#include "AE_EffectCBSuites.h"
#include "String_Utils.h"
#include "AE_GeneralPlug.h"
#include "AEFX_ChannelDepthTpl.h"
#include "AEGP_SuiteHandler.h"

#define	MAJOR_VERSION 1
#define	MINOR_VERSION 0
#define	BUG_VERSION	0
#define	STAGE_VERSION PF_Stage_DEVELOP
#define	BUILD_VERSION 1

// helpers
#define FIX2D(x) (x / ((double)(1L << 16)))
#define BRIGHT8(p) ((double)(p->red + p->green + p->blue) / (double)PF_MAX_CHAN8 / 3.0)
#define BRIGHT16(p) ((double)(p->red + p->green + p->blue) / (double)PF_MAX_CHAN16 / 3.0)

enum {
	INPUT_LAYER = 0,
	PARAM_SIZE,
	PARAM_THICKNESS,
	PARAM_STEPS,
	PARAM_ANGLE_MAX,
	PARAM_ANGLE_OFFSET,
	PARAM_CUTOFF,
	PARAM_LOWER,
	PARAM_UPPER,
	PARAM_CENTRE,
	PARAM_AA,
	PARAM_ALPHA,
	PARAM_COUNT
};

typedef struct {
	double size;
	double thickness;
	int steps;
	double angle_max;
	double angle_offset;
	double cutoff;
	double lower;
	double upper;
	double centre_x;
	double centre_y;
	int aa;
	int alpha;
	PF_EffectWorld *ref;
} CrosshatchInfo;

double gridCollision(
	int x,
	int y,
	double theta,
	int seed,
	CrosshatchInfo *info
) {
	// define grid line
	//srand(seed);
	//double px = rand() * 100;
	//double py = rand() * 100;
	double px = info->centre_x + seed;
	double py = info->centre_y + seed;
	double qx = px + cos(theta);
	double qy = py + sin(theta);
	double A = py - qy;
	double B = qx - px;
	double C = px * qy + qx * py;

	// get distance to line, grid
	double dist = abs(A * x + B * y + C) / sqrt(A * A + B * B);
	double d = fmod(dist, info->size);
	if (d < info->thickness) {
		return info->aa ? 1.0 - (d / info->thickness) : 1.0;
	} else if (info->size - d < info->thickness) {
		return info->aa ? 1.0 - ((info->size - d) / info->thickness) : 1.0;
	} else {
		return 0.0;
	}
}

double getWeight(
	int x,
	int y,
	double brightness,
	CrosshatchInfo *info
) {
	double weight = 0.0;

	if (brightness <= info->cutoff) {
		weight = 1.0;
	} else if (brightness >= info->upper || brightness <= info->lower) {
		weight = 0.0;
	} else {
		int lim = info->steps;
		double bright_step = (info->upper - info->lower) / (info->steps + 1);

		for (int i=0; i<lim; i++) {
			double threshold = info->lower + bright_step * (i + 1);

			if (brightness <= threshold) {
				int j = i % 2 == 0 ? i / 2 : lim - (i / 2 + 1);
				double t = lim == 1 ? 0.0 : j / (lim - 1.0);
				double theta = info->angle_offset + t * info->angle_max;
				weight += gridCollision(x, y, theta, i, info);
			}
		}
	}

	return max(0.0, 1.0 - weight);
}

#ifdef __cplusplus
extern "C" {
#endif

	DllExport PF_Err
	EntryPointFunc(
		PF_Cmd cmd,
		PF_InData *in_data,
		PF_OutData *out_data,
		PF_ParamDef *params[],
		PF_LayerDef	*output,
		void *extra
	);

#ifdef __cplusplus
}
#endif
#endif // HALFTONE_H