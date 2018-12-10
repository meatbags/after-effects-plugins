/**
 ** TimeSort.h
 **/

#pragma once
#ifndef TIMESORT_H
#define TIMESORT_H
#define PF_DEEP_COLOR_AWARE 1 // 16bpc flag for AE_Effect.h
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

enum {
	TIMESORT_INPUT = 0,
	TIMESORT_CHUNK_SIZE,
	TIMESORT_THRESHOLD_LOWER,
	TIMESORT_THRESHOLD_UPPER,
	TIMESORT_ORDER,
	TIMESORT_NOISE,
	TIMESORT_NUM_PARAMS
};


typedef struct {
	PF_Pixel8 p;
	double value;
} Pixel8Data;

bool filter8(double value, double lower, double upper) {
	return (value >= lower && value <= upper);
}

void swap8(Pixel8Data *a, Pixel8Data *b) {
	// cache a
	A_u_char alpha = a->p.alpha;
	A_u_char red = a->p.red;
	A_u_char green = a->p.green;
	A_u_char blue = a->p.blue;
	double value = a->value;
	
	// b -> a
	a->p.alpha = b->p.alpha;
	a->p.red = b->p.red;
	a->p.green = b->p.green;
	a->p.blue = b->p.blue;
	a->value = b->value;
	
	// cache -> b
	b->p.alpha = alpha;
	b->p.red = red;
	b->p.green = green;
	b->p.blue = blue;
	b->value = value;
}

int partition8(Pixel8Data *p, int low, int high) {
	double pivot = p[high].value;
	int i = low - 1;
	for (int j = low; j <= high - 1; j++) {
		if (p[j].value >= pivot) {
			i++;
			swap8(&p[i], &p[j]);
		}
	}
	swap8(&p[i + 1], &p[high]);
	return i + 1;
}

void quickSort8(Pixel8Data *p, int low, int high) {
	if (low < high) {
		int part = partition8(p, low, high);
		quickSort8(p, low, part - 1);
		quickSort8(p, part + 1, high);
	}
}

Pixel8Data getPixelData8(PF_Pixel8 *p) {
	Pixel8Data res;
	res.p.alpha = p->alpha;
	res.p.red = p->red;
	res.p.green = p->green;
	res.p.blue = p->blue;
	A_u_char maxRGB = max(p->red, max(p->green, p->blue));
	A_u_char minRGB = min(p->red, min(p->green, p->blue));
	res.value = ((maxRGB + minRGB) / (double)PF_MAX_CHAN8) / 2.0;
	return res;
}

typedef struct {
	AEGP_MemorySuite1 *mem_suite;
	int chunk_size;
	int chunk_extent;
	double upper;
	double lower;
	int order;

	// time & indices
	A_long time;
	A_long chunk_time;
	A_long time_start;
	A_long time_stop;
	A_long time_step;
	int target_index;
	int default_index;
	int unfold;
	double noise_scale;

	// layer
	PF_Pixel8 *data;
	int width;
	int height;
	int checked_layers;
	A_long layer_bytes;
	A_long row_bytes;
} TimeSortInfo;

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

#endif // TIMESORT_H