// Timescan.h

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
#define FIX2D(x) (x / ((double)(1L << 16)))
#define FIX2LONG(x) (x / (1L << 16));

enum {
	TIMESCAN_INPUT = 0,
	TIMESCAN_MODE,
	TIMESCAN_POSITION,
	TIMESCAN_SCALE,
	TIMESCAN_AUTOPAN,
	TIMESCAN_AUTOPAN_SPEED,
	TIMESCAN_MIRROR,
	TIMESCAN_NUM_PARAMS
};

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