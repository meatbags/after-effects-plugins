#pragma once
#ifndef NORMAL_H
#define NORMAL_H

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

#define PI 3.14159265359
#define PI_2 6.28318530718
#define PI_HALF 1.57079632679

enum {
	INPUT_LAYER = 0,
	PARAM_STRENGTH,
	PARAM_INVERT,
	PARAM_USE_SAMPLER,
	PARAM_SAMPLES,
	PARAM_SAMPLE_RADIUS,
	PARAM_BLEND,
	PARAM_COUNT
};

int clamp(int value, int vMin, int vMax) {
	return max(vMin, min(vMax, value));
}

double blend(double a, double b, double t) {
	return a + (b - a) * t;
}

double noise(double x, double y) {
	double dot = x * 12.9898 + y * 78.233;
	double s = abs(sin(dot) * 43758.5453);
	return s - (long)s;
}

double prand(double x, double y, double seed) {
	return noise((x + seed) / 2.0, cos((y - seed) / 2.0));
}

boolean inThreshold(double a, double b, double threshold) {
	return abs(b - a) <= threshold;
}

boolean inBounds(int x, int y, PF_EffectWorld* ref) {
	return x >= 0 && y >= 0 && x < ref->width && y < ref->height;
}

PF_Pixel8* getPixel8(PF_EffectWorld* inputP, int x, int y) {
	return ((PF_Pixel8 *)(
		(char*)inputP->data + (y * inputP->rowbytes) + x * sizeof(PF_Pixel8))
	);
}

PF_Pixel16* getPixel16(PF_EffectWorld *inputP, int x, int y) {
	return ((PF_Pixel16 *)(
		(char*)inputP->data + (y * inputP->rowbytes) + x * sizeof(PF_Pixel16))
	);
}

double intensity8(PF_EffectWorld* ref, int x, int y) {
	x = clamp(x, 0, ref->width - 1);
	y = clamp(y, 0, ref->height - 1);
	PF_Pixel8* p = getPixel8(ref, x, y);
	return (p->red + p->green + p->blue) / (3.0 * PF_MAX_CHAN8);
}

double intensity16(PF_EffectWorld *ref, int x, int y) {
	x = clamp(x, 0, ref->width - 1);
	y = clamp(y, 0, ref->height - 1);
	PF_Pixel16* p = getPixel16(ref, x, y);
	return (p->red + p->green + p->blue) / (3.0 * PF_MAX_CHAN16);
}

typedef struct {
	double x;
	double y;
	double z;
} Vector;

typedef struct {
	double seed;
	double strength;
	int invert;
	int useSampler;
	int samples;
	double sample_radius;
	double blend;
	double flat_threshold;
	PF_EffectWorld *ref;
	PF_InData in_data;
} NormalInfo;

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
#endif // NORMAL_H