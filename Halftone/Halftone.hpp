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
#include "HalftoneVector.hpp"

#define	MAJOR_VERSION 1
#define	MINOR_VERSION 0
#define	BUG_VERSION	0
#define	STAGE_VERSION PF_Stage_DEVELOP
#define	BUILD_VERSION 1

enum {
	INPUT_LAYER = 0,
	PARAM_MODE,
	PARAM_RADIUS,
	PARAM_AA,
	PARAM_ANGLE_1,
	PARAM_ANGLE_2,
	PARAM_ANGLE_3,
	PARAM_USE_GREYSCALE,
	PARAM_COUNT
};

typedef struct {
	A_u_char mode;
	double grid_step;
	double grid_half_step;
	double aa;
	//double angle_0;
	double angle_1;
	double angle_2;
	double angle_3;
	PF_EffectWorld *ref;
	PF_InData in_data;
	PF_SampPB samp_pb;
	PF_Boolean greyscale;
	//PF_ProgPtr ref;
	Vector origin = Vector(0, 0);
	Vector normal_1 = Vector(0, 0);
	Vector normal_2 = Vector(0, 0);
	Vector normal_3 = Vector(0, 0);
} HalftoneInfo;

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