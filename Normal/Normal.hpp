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

enum {
	INPUT_LAYER = 0,
	PARAM_STRENGTH,
	PARAM_INVERT,
	PARAM_COUNT
};

int clamp(int v, int vMax) {
	return max(0, min(vMax, v));
}

double getIntensity16(
	PF_Pixel16* p
) {
	return (p->red + p->green + p->blue) / (3.0 * PF_MAX_CHAN16);
}

double getIntensity8(
	PF_Pixel8* p
) {
	return (p->red + p->green + p->blue) / (3.0 * PF_MAX_CHAN8);
}

typedef struct {
	double x;
	double y;
	double z;
} Vector;

typedef struct {
	double strength;
	int invert;
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