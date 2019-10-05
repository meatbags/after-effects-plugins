#include "Crosshatch.hpp"

static PF_Err Crosshatch8(
	void *refcon,
	A_long xL,
	A_long yL,
	PF_Pixel8 *inP,
	PF_Pixel8 *outP
) {
	PF_Err err = PF_Err_NONE;
	register CrosshatchInfo *info = (CrosshatchInfo*)refcon;

	// get crosshatch value [0, 1]
	double weight = getWeight(xL, yL, BRIGHT8(inP), info);

	// write out (BW or alpha)
	A_u_char res = (A_u_char)round(weight * PF_MAX_CHAN8);
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

static PF_Err Crosshatch16(
	void *refcon,
	A_long xL,
	A_long yL,
	PF_Pixel16 *inP,
	PF_Pixel16 *outP
) {
	PF_Err err = PF_Err_NONE;
	register CrosshatchInfo *info = (CrosshatchInfo*)refcon;

	// get crosshatch value [0, 1]
	double weight = getWeight(xL, yL, BRIGHT16(inP), info);

	// write out
	A_u_short res = (A_u_short)round(weight * PF_MAX_CHAN16);
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
	CrosshatchInfo info;
	double qscale = inputP->width / (double)in_data->width;

	// get user input
	AEFX_CLR_STRUCT(info);
	info.size = (double)params[PARAM_SIZE]->u.fs_d.value * qscale;
	info.thickness = (double)params[PARAM_THICKNESS]->u.fs_d.value * 0.5 * qscale;
	info.steps = (int)params[PARAM_STEPS]->u.sd.value;
	info.angle_max = FIX2D(params[PARAM_ANGLE_MAX]->u.ad.value) * PF_RAD_PER_DEGREE;
	info.angle_offset = FIX2D(params[PARAM_ANGLE_OFFSET]->u.ad.value) * PF_RAD_PER_DEGREE;
	info.cutoff = (double)params[PARAM_CUTOFF]->u.fs_d.value / 100.0;
	info.lower = (double)params[PARAM_LOWER]->u.fs_d.value / 100.0;
	info.upper = (double)params[PARAM_UPPER]->u.fs_d.value / 100.0;
	info.centre_x = FIX2D(params[PARAM_CENTRE]->u.td.x_value);
	info.centre_y = FIX2D(params[PARAM_CENTRE]->u.td.y_value);
	info.aa = PF_Boolean((params[PARAM_AA]->u.bd.value));
	info.alpha = PF_Boolean((params[PARAM_ALPHA]->u.bd.value));
	info.ref = inputP;

	if (PF_WORLD_IS_DEEP(output)) {
		ERR(suites.Iterate16Suite1()->iterate(in_data, 0, linesL, inputP, NULL, (void*)&info, Crosshatch16, output));
	} else {
		ERR(suites.Iterate8Suite1()->iterate(in_data, 0, linesL, inputP, NULL, (void*)&info, Crosshatch8, output));
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
		"Crosshatch",
		MAJOR_VERSION,
		MINOR_VERSION,
		"Crosshatch shader by @meatbags"
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
	int size = 8;
	int thickness = 2;
	int steps = 4;
	int angle_max = 90;
	int angle_off = 0;
	int black = 0;
	int lower = 15;
	int upper = 75;

	AEFX_CLR_STRUCT(def);
	PF_ADD_FLOAT_SLIDER("Size", 4, 2048, 4, 64, 0, size, 0, 0, 0, PARAM_SIZE);
	AEFX_CLR_STRUCT(def);
	PF_ADD_FLOAT_SLIDER("Thickness", 1, 100, 1, 10, 0, thickness, 0, 0, 0, PARAM_THICKNESS);
	AEFX_CLR_STRUCT(def);
	PF_ADD_SLIDER("Steps", 1, 64, 1, 16, steps, PARAM_STEPS);
	AEFX_CLR_STRUCT(def);
	PF_ADD_ANGLE("Max angle", angle_max, PARAM_ANGLE_MAX);
	AEFX_CLR_STRUCT(def);
	PF_ADD_ANGLE("Offset", angle_off, PARAM_ANGLE_OFFSET);
	AEFX_CLR_STRUCT(def);
	PF_ADD_FLOAT_SLIDER("Black Cut-off", 0, 100, 0, 100, 0, black, 0, 0, 0, PARAM_CUTOFF);
	AEFX_CLR_STRUCT(def);
	PF_ADD_FLOAT_SLIDER("Lower Threshold", 0, 100, 0, 100, 0, lower, 0, 0, 0, PARAM_LOWER);
	AEFX_CLR_STRUCT(def);
	PF_ADD_FLOAT_SLIDER("Upper Threshold", 0, 100, 0, 100, 0, upper, 0, 0, 0, PARAM_UPPER);
	AEFX_CLR_STRUCT(def);
	PF_ADD_POINT("Grid Centre", 50, 50, false, PARAM_CENTRE);
	AEFX_CLR_STRUCT(def);
	PF_ADD_CHECKBOXX("Anti-aliasing", 1, 0, PARAM_AA);
	AEFX_CLR_STRUCT(def);
	PF_ADD_CHECKBOXX("Alpha channel", 1, 0, PARAM_ALPHA);
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
