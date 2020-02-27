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
#define FIX2LONG(x) (x / (1L << 16));
#define FIX2D(x) (x / ((double)(1L << 16)))
#define BRIGHT8(p) ((double)(p->red + p->green + p->blue) / (double)PF_MAX_CHAN8 / 3.0)
#define BRIGHT16(p) ((double)(p->red + p->green + p->blue) / (double)PF_MAX_CHAN16 / 3.0)
#define MANHATTAN(x0, y0, x1, y1) (abs(y1 - y0) + abs(x1 - x0))

enum {
	INPUT_LAYER = 0,
	PARAM_POINT,
	PARAM_THRESHOLD,
	PARAM_PASSES,
	PARAM_SHOW_SOURCE,
	PARAM_RADIUS,
	PARAM_COUNT
};

typedef struct {
	A_long x;
	A_long y;
	double threshold;
	int pass;
	int pass_max;
	int show_source;
	int radius;
	PF_EffectWorld *layer;
	boolean *filled_map;
} FillAreaInfo;

PF_Pixel8 *GetPixel8(PF_EffectWorld *inputP, int x,	int y) {
	return ((PF_Pixel8 *)((char*)inputP->data + (y * inputP->rowbytes) + x * sizeof(PF_Pixel8)));
}

void Copy8(PF_Pixel8 *out, PF_Pixel8 *in) {
	out->alpha = in->alpha;
	out->red = in->red;
	out->green = in->green;
	out->blue = in->blue;
}

void Copy8(PF_Pixel8 *p, A_u_char alpha, A_u_char red, A_u_char green, A_u_char blue) {
	p->alpha = alpha;
	p->red = red;
	p->green = green;
	p->blue = blue;
}

boolean PixelOK8(FillAreaInfo *info, int x, int y) {
	return (GetPixel8(info->layer, x, y)->alpha / 255.0) <= info->threshold;
}

boolean IsFilled(FillAreaInfo *info, int x, int y) {
	return info->filled_map[y * info->layer->width + x];
}

void Fill(FillAreaInfo *info, int x, int y) {
	info->filled_map[y * info->layer->width + x] = true;
}

boolean Walk8(
	FillAreaInfo *info,
	A_long x,
	A_long y
) {
	// check on screen
	int w = info->layer->width;
	int h = info->layer->height;
	if (info->x < 0 || info->y < 0 || info->x >= w || info->y >= h) {
		return false;
	}

	// initial pass
	if (info->pass == 0) {
		if (info->x == x) {
			int dy = info->y - y;
			int stepy = dy >= 0 ? 1 : -1;
			for (int sy = y; sy != info->y; sy += stepy) {
				if (!PixelOK8(info, x, sy)) {
					return false;
				}
			}
		} else if (info->y == y) {
			int dx = info->x - x;
			int stepx = dx >= 0 ? 1 : -1;
			for (int sx = x; sx != info->x; sx += stepx) {
				if (!PixelOK8(info, sx, y)) {
					return false;
				}
			}
		} else if (abs(info->x - x) == abs(info->y - y)) {
			int dx = info->x - x;
			int dy = info->y - y;
			int stepx = dx >= 0 ? 1 : -1;
			int stepy = dy >= 0 ? 1 : -1;
			int sy = y;
			for (int sx = x; sx != info->x; sx += stepx) {
				if (!PixelOK8(info, sx, sy)) {
					return false;
				}
				sy += stepy;
			}
		} else {
			return false;
		}
		Fill(info, x, y);
		return true;
	}

	// scan up
	for (int sy = y; sy>-1; sy--) {
		if (IsFilled(info, x, sy)) {
			Fill(info, x, y);
			return true;
		}
		if (!PixelOK8(info, x, sy))
			break;
	}

	// scan down
	for (int sy = y; sy<h; sy++) {
		if (IsFilled(info, x, sy)) {
			Fill(info, x, y);
			return true;
		}
		if (!PixelOK8(info, x, sy))
			break;
	}

	// scan left
	for (int sx=x; sx>-1; sx--) {
		if (IsFilled(info, sx, y)) {
			Fill(info, x, y);
			return true;
		}
		if (!PixelOK8(info, sx, y))
			break;
	}

	// scan right
	for (int sx=x; sx<w; sx++) {
		if (IsFilled(info, sx, y)) {
			Fill(info, x, y);
			return true;
		}
		if (!PixelOK8(info, sx, y))
			break;
	}

	return false;
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