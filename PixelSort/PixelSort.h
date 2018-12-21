/**
 ** PixelSort.h
 **/

#pragma once
#ifndef PIXELSORT_H
#define PIXELSORT_H

//#define PF_DEEP_COLOR_AWARE 1 // 16bpc flag for AE_Effect.h
#include "AEConfig.h"
#include "entry.h"
#include "AE_Effect.h"
#include "AE_EffectCB.h"
#include "AE_Macros.h"
#include "Param_Utils.h"
#include "AE_EffectCBSuites.h"
#include "String_Utils.h"
#include "AE_GeneralPlug.h"
#include "AEGP_SuiteHandler.h"
#include <cmath>

#ifdef AE_OS_WIN
#include <Windows.h>
#endif	

// Version info
#define	MAJOR_VERSION 1
#define	MINOR_VERSION 0
#define	BUG_VERSION	0
#define	STAGE_VERSION PF_Stage_DEVELOP
#define	BUILD_VERSION 1

// Conversions
#define D2FIX(x) (((long)x)<<16)
#define FIX2D(x) (x / ((double)(1L << 16)))

enum {
	PIXELSORT_INPUT = 0,
	PIXELSORT_TYPE,
	PIXELSORT_KEY,
	PIXELSORT_CHUNK_SIZE,
	PIXELSORT_DIRECTION,
	PIXELSORT_LOWER,
	PIXELSORT_UPPER,
	PIXELSORT_LOWER_PADDING,
	PIXELSORT_UPPER_PADDING,
	PIXELSORT_ORDER,
	PIXELSORT_CENTRE,
	PIXELSORT_DISPLACEMENT,
	PIXELSORT_DISPLACEMENT_AMOUNT,
	PIXELSORT_NUM_PARAMS
};


bool isOnScreen(PF_LayerDef *layer, int x, int y) {
	return (x > -1 && y > -1 && x < layer->width && y < layer->height);
}

bool filter(double value, double lower, double upper) {
	return (value >= lower && value <= upper);
}

typedef struct {
	PF_Pixel16 p;
	double value;
} Pixel16Data;

typedef struct {
	PF_Pixel8 p;
	double value;
} Pixel8Data;

PF_Pixel8 *getPixel8(PF_LayerDef *layer, int x, int y) {
	return (PF_Pixel8*)((char *)layer->data + (y * layer->rowbytes) + (x * sizeof(PF_Pixel8)));
}

PF_Pixel16 *getPixel16(PF_LayerDef *layer, int x, int y) {
	return (PF_Pixel16*)((char *)layer->data + (y * layer->rowbytes) + (x * sizeof(PF_Pixel16)));
}

Pixel8Data getPixel8Data(PF_Pixel8 *p) {
	Pixel8Data res;
	res.p.alpha = p->alpha;
	res.p.red = p->red;
	res.p.green = p->green;
	res.p.blue = p->blue;
	res.value = ((p->red + p->green + p->blue) / (double)PF_MAX_CHAN8) / 3.0;
	return res;
}

Pixel16Data getPixel16Data(PF_Pixel8 *p) {
	Pixel16Data res;
	res.p.alpha = p->alpha;
	res.p.red = p->red;
	res.p.green = p->green;
	res.p.blue = p->blue;
	res.value = ((p->red + p->green + p->blue) / (double)PF_MAX_CHAN16) / 3.0;
	return res;
}

typedef struct {
	AEGP_MemorySuite1 *mem_suite;
	
	// User settings
	int type;
	int chunk_size;
	double lower_in;
	double upper_in;
	double lower_out;
	double upper_out;
	int sort_by;
	int order_by;
	double direction;
	double centre_x;
	double centre_y;
	double displacement_scale;

	// Derived
	double vec_x;
	double vec_y;
	bool use_displacement;

	// Layer data
	PF_LayerDef *layer;
	PF_LayerDef *map;
	A_long rowbytes;
} PixelSortInfo;

extern "C" {
	DllExport
		PF_Err
		EffectMain(
			PF_Cmd cmd,
			PF_InData *in_data,
			PF_OutData *out_data,
			PF_ParamDef *params[],
			PF_LayerDef	*output,
			void *extra
		);
}

#endif // PIXELSORT_H