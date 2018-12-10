/**
** SuperTile.h
**/

#pragma once
#ifndef SUPERTILE_H
#define SUPERTILE_H
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

// Defaults
#define ST_SIZE_MIN 8
#define ST_SIZE_MAX 128
#define ST_SIZE_DFLT 16

// Conversions
#define D2FIX(x) (((long)x)<<16)

enum {
	ST_INPUT = 0,
	ST_TARGET_LAYER_ID,
	ST_SIZE_SRC_ID,
	ST_SIZE_DEST_ID,
	ST_NUM_PARAMS
};

typedef struct {
	double h, s, l;
} HSL;

double getHSLMag (HSL *a, HSL *b) {
	/* Vector Magnitude Between HSL */
	double dh = min(fabs(b->h - a->h), min(b->h + (1.0 - a->h), a->h + (1.0 - b->h)));
	double ds = b->s - a->s;
	double dl = b->l - a->l;
	return sqrt(dh * dh + ds * ds + dl * dl);
}

void getHSL(double r, double b, double g, HSL *dest) {
	/* Convert RGB [0, 1] to HSL [0, 1] */
	double maxRGB = max(r, max(g, b));
	double minRGB = min(r, min(g, b));
	double d = maxRGB - minRGB;
	dest->l = (maxRGB + minRGB) / 2;
	if (d == 0) {
		dest->h = 0;
		dest->s = 0;
	} else {
		dest->s = (dest->l <= 0.5) ? (d / (maxRGB + minRGB)) : (d / (2 - maxRGB - minRGB));
		dest->h = (r == maxRGB) ? (g - b) / d : ((g == maxRGB) ? 2.0 + (b - r) / d : 4.0 + (r - g) / d);
		dest->h /= 6.0;
	}
}

void getHSL8 (PF_Pixel8 *pixel, HSL *dest) {
	/* Convert 8Bit RGB to HSL */
	double r = pixel->red / (double)PF_MAX_CHAN8;
	double g = pixel->green / (double)PF_MAX_CHAN8;
	double b = pixel->blue / (double)PF_MAX_CHAN8;
	getHSL(r, g, b, dest);
}

void getHSL16(PF_Pixel16 *pixel, HSL *dest) {
	/* Convert 16Bit RGB to HSL */
	double r = pixel->red / (double)PF_MAX_CHAN16;
	double g = pixel->green / (double)PF_MAX_CHAN16;
	double b = pixel->blue / (double)PF_MAX_CHAN16;
	getHSL(r, g, b, dest);
}

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

#endif // SUPERTILE_H