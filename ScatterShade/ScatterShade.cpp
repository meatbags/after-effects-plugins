#include "ScatterShade.hpp"

static PF_Err ScatterShade8(
	void *refcon,
	A_long xL,
	A_long yL,
	PF_Pixel8 *inP,
	PF_Pixel8 *outP
) {
	PF_Err err = PF_Err_NONE;
	register ScatterShadeInfo *info = (ScatterShadeInfo*)refcon;
	A_u_char res = isBright(xL, yL, BRIGHTNESS8(inP), info) ? PF_MAX_CHAN8 : 0;
	
	if (!info->alpha) {
		outP->alpha = PF_MAX_CHAN8;
		outP->red = res;
		outP->blue = res;
		outP->green = res;
	} else {
		outP->alpha = PF_MAX_CHAN8 - res;
		outP->red = 0;
		outP->blue = 0;
		outP->green = 0;
	}

	return err;
}

static PF_Err ScatterShade16(
	void *refcon,
	A_long xL,
	A_long yL,
	PF_Pixel16 *inP,
	PF_Pixel16 *outP
) {
	PF_Err err = PF_Err_NONE;
	register ScatterShadeInfo *info = (ScatterShadeInfo*)refcon;
	A_u_short res = isBright(xL, yL, BRIGHTNESS16(inP), info) ? PF_MAX_CHAN16 : 0;

	if (!info->alpha) {
		outP->alpha = PF_MAX_CHAN16;
		outP->red = res;
		outP->blue = res;
		outP->green = res;
	} else {
		outP->alpha = PF_MAX_CHAN16 - res;
		outP->red = 0;
		outP->blue = 0;
		outP->green = 0;
	}

	return err;
}

static PF_Err
Render(
	PF_InData		*in_data,
	PF_OutData		*out_data,
	PF_ParamDef		*params[],
	PF_LayerDef		*output
) {
	PF_Err err = PF_Err_NONE;
	AEGP_SuiteHandler suites(in_data->pica_basicP);
	A_long linesL = output->extent_hint.bottom - output->extent_hint.top;
	PF_EffectWorld *inputP = &params[INPUT_LAYER]->u.ld;
	ScatterShadeInfo info;

	// UI
	AEFX_CLR_STRUCT(info);
	info.lower = (double)params[PARAM_LOWER]->u.fs_d.value / 100.0;
	info.upper = (double)params[PARAM_UPPER]->u.fs_d.value / 100.0;
	info.seed = (double)params[PARAM_SEED]->u.fs_d.value;
	int randomise = PF_Boolean((params[PARAM_RANDOMISE]->u.bd.value));
	if (randomise) {
		srand((unsigned)in_data->current_time);
		info.seed += rand();
	}
	info.alpha = PF_Boolean((params[PARAM_ALPHA]->u.bd.value));
	info.ref = inputP;

	if (PF_WORLD_IS_DEEP(output)) {
		ERR(suites.Iterate16Suite1()->iterate(in_data, 0, linesL, inputP, NULL, (void*)&info, ScatterShade16, output));
	} else {
		ERR(suites.Iterate8Suite1()->iterate(in_data, 0, linesL, inputP, NULL, (void*)&info, ScatterShade8, output));
	}

	return err;
}

static PF_Err About(
	PF_InData *in_data,
	PF_OutData *out_data,
	PF_ParamDef *params[],
	PF_LayerDef *output
) {
	AEGP_SuiteHandler suites(in_data->pica_basicP);
	suites.ANSICallbacksSuite1()->sprintf(
		out_data->return_msg, "%s v%d.%d\r%s",
		"ScatterShade",
		MAJOR_VERSION,
		MINOR_VERSION,
		"ScatterShade shader by @meatbags"
	);

	return PF_Err_NONE;
}

static PF_Err GlobalSetup(
	PF_InData *in_data,
	PF_OutData *out_data,
	PF_ParamDef	*params[],
	PF_LayerDef	*output
) {
	out_data->out_flags = PF_OutFlag_DEEP_COLOR_AWARE;
	out_data->my_version = PF_VERSION(MAJOR_VERSION, MINOR_VERSION, BUG_VERSION, STAGE_VERSION, BUILD_VERSION);

	return PF_Err_NONE;
}

static PF_Err ParamsSetup(
	PF_InData *in_data,
	PF_OutData *out_data,
	PF_ParamDef	*params[],
	PF_LayerDef	*output
) {
	PF_ParamDef	def;

	// defaults
	int lower = 25;
	int upper = 75;
	int seed = 0;
	int randomise = 0;
	int alpha = 0;

	AEFX_CLR_STRUCT(def);
	PF_ADD_FLOAT_SLIDER("Lower Threshold", 0, 100, 0, 100, 0, lower, 0, 0, 0, PARAM_LOWER);
	AEFX_CLR_STRUCT(def);
	PF_ADD_FLOAT_SLIDER("Upper Threshold", 0, 100, 0, 100, 0, upper, 0, 0, 0, PARAM_UPPER);
	AEFX_CLR_STRUCT(def);
	PF_ADD_FLOAT_SLIDER("Random Seed", 0, 100000, 0, 100, 0, seed, 0, 0, 0, PARAM_SEED);
	AEFX_CLR_STRUCT(def);
	PF_ADD_CHECKBOXX("Randomise", randomise, 0, PARAM_RANDOMISE);
	AEFX_CLR_STRUCT(def);
	PF_ADD_CHECKBOXX("Alpha channel", alpha, 0, PARAM_ALPHA);
	out_data->num_params = PARAM_COUNT;

	return PF_Err_NONE;
}

DllExport
PF_Err
EntryPointFunc(
	PF_Cmd cmd,
	PF_InData *in_data,
	PF_OutData *out_data,
	PF_ParamDef *params[],
	PF_LayerDef *output,
	void *extra
) {
	PF_Err err = PF_Err_NONE;
	try {
		switch (cmd) {
		case PF_Cmd_ABOUT:
			err = About(in_data, out_data, params, output);
			break;
		case PF_Cmd_GLOBAL_SETUP:
			err = GlobalSetup(in_data, out_data, params, output);
			break;
		case PF_Cmd_PARAMS_SETUP:
			err = ParamsSetup(in_data, out_data, params, output);
			break;
		case PF_Cmd_RENDER:
			err = Render(in_data, out_data, params, output);
			break;
		}
	}
	catch (PF_Err &thrown_err) {
		err = thrown_err;
	}

	return err;
}
