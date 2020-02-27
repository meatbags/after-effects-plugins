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
#define BRIGHTNESS8(p) ((double)(p->red + p->green + p->blue) / (double)PF_MAX_CHAN8 / 3.0)
#define BRIGHTNESS16(p) ((double)(p->red + p->green + p->blue) / (double)PF_MAX_CHAN16 / 3.0)

enum {
	INPUT_LAYER = 0,
	PARAM_LOWER,
	PARAM_UPPER,
	PARAM_SEED,
	PARAM_RANDOMISE,
	PARAM_ALPHA,
	PARAM_COUNT
};

typedef struct {
	double lower;
	double upper;
	double seed;
	int randomise;
	int alpha;
	PF_EffectWorld *ref;
} ScatterShadeInfo;

// get noise [0, 1]
double noise(double x, double y) {
	double dot = x * 12.9898 + y * 78.233;
	double s = abs(sin(dot) * 43758.5453);
	return s - (long)s;
}

boolean isBright(
	int x,
	int y,
	double brightness,
	ScatterShadeInfo *info
) {
	if (brightness <= info->lower) {
		return false;
	} else if (brightness >= info->upper) {
		return true;
	} else {
		double range = info->upper - info->lower;
		double n = noise(x / 100. + info->seed, y / 100.) * range + info->lower;
		return brightness > n;
	}
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