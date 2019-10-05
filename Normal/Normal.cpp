#include "Normal.hpp"

PF_Pixel16 *getPixel16(
	PF_EffectWorld *inputP,
	int x,
	int y
) {
	return ((PF_Pixel16 *)((char*)inputP->data + (y * inputP->rowbytes) + x * sizeof(PF_Pixel16)));
}

PF_Pixel8 *getPixel8(
	PF_EffectWorld *inputP,
	int x,
	int y
) {
	return ((PF_Pixel8 *)((char*)inputP->data + (y * inputP->rowbytes) + x * sizeof(PF_Pixel8)));
}

static PF_Err Normal16(
	void *refcon,
	A_long xL,
	A_long yL,
	PF_Pixel16 *inP,
	PF_Pixel16 *outP
) {
	PF_Err err = PF_Err_NONE;
	register NormalInfo *info = (NormalInfo*)refcon;

	// get pixel intensity
	int width = info->ref->width - 1;
	int height = info->ref->height - 1;
	double TL = getIntensity16(getPixel16(info->ref, clamp(xL - 1, width), clamp(yL - 1, height)));
	double T  = getIntensity16(getPixel16(info->ref, xL, clamp(yL - 1, height)));
	double TR = getIntensity16(getPixel16(info->ref, clamp(xL + 1, width), clamp(yL + 1, height)));
	double R  = getIntensity16(getPixel16(info->ref, clamp(xL + 1, width), yL));
	double L  = getIntensity16(getPixel16(info->ref, clamp(xL - 1, width), yL));
	double BL = getIntensity16(getPixel16(info->ref, clamp(xL - 1, width), clamp(yL + 1, height)));
	double B  = getIntensity16(getPixel16(info->ref, xL, clamp(yL + 1, height)));
	double BR = getIntensity16(getPixel16(info->ref, clamp(xL + 1, width), clamp(yL + 1, height)));

	// Sobel filter
	double dX = (TR + 2.0 * R + BR) - (TL + 2.0 * L + BL);
	double dY = (BL + 2.0 * B + BR) - (TL + 2.0 * T + TR);
	double dZ = 1.0 / info->strength;

	// normalise
	double mag = sqrt(dX * dX + dY * dY + dZ * dZ);
	if (mag != 0) {
		dX /= mag;
		dY /= mag;
		dZ /= mag;
	}

	// invert depth
	if (info->invert) {
		dX *= -1;
		dY *= -1;
	}

	// set pixel
	outP->alpha = inP->alpha;
	outP->red = (A_u_short)(((dX + 1) / 2) * PF_MAX_CHAN16);
	outP->green = (A_u_short)(((dY + 1) / 2) * PF_MAX_CHAN16);
	outP->blue = (A_u_short)(((dZ + 1) / 2) * PF_MAX_CHAN16);

	return err;
}

static PF_Err Normal8(
	void *refcon,
	A_long xL,
	A_long yL,
	PF_Pixel8 *inP,
	PF_Pixel8 *outP
) {
	PF_Err err = PF_Err_NONE;
	register NormalInfo *info = (NormalInfo*)refcon;

	// get pixel intensity
	int width = info->ref->width - 1;
	int height = info->ref->height - 1;
	double TL = getIntensity8(getPixel8(info->ref, clamp(xL - 1, width), clamp(yL - 1, height)));
	double T  = getIntensity8(getPixel8(info->ref, xL, clamp(yL - 1, height)));
	double TR = getIntensity8(getPixel8(info->ref, clamp(xL + 1, width), clamp(yL + 1, height)));
	double R  = getIntensity8(getPixel8(info->ref, clamp(xL + 1, width), yL));
	double L  = getIntensity8(getPixel8(info->ref, clamp(xL - 1, width), yL));
	double BL = getIntensity8(getPixel8(info->ref, clamp(xL - 1, width), clamp(yL + 1, height)));
	double B  = getIntensity8(getPixel8(info->ref, xL, clamp(yL + 1, height)));
	double BR = getIntensity8(getPixel8(info->ref, clamp(xL + 1, width), clamp(yL + 1, height)));

	// Sobel filter
	double dX = (TR + 2.0 * R + BR) - (TL + 2.0 * L + BL);
	double dY = (BL + 2.0 * B + BR) - (TL + 2.0 * T + TR);
	double dZ = 1.0 / info->strength;

	// normalise
	double mag = sqrt(dX * dX + dY * dY + dZ * dZ);
	if (mag != 0) {
		dX /= mag;
		dY /= mag;
		dZ /= mag;
	}

	// invert depth
	if (info->invert) {
		dX *= -1;
		dY *= -1;
	}

	// set pixel
	outP->alpha = inP->alpha;
	outP->red = (A_u_char)(((dX + 1) / 2) * PF_MAX_CHAN8);
	outP->green = (A_u_char)(((dY + 1) / 2) * PF_MAX_CHAN8);
	outP->blue = (A_u_char)(((dZ + 1) / 2) * PF_MAX_CHAN8);

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
	NormalInfo info;
	PF_EffectWorld *inputP = &params[INPUT_LAYER]->u.ld;

	// get ui
	AEFX_CLR_STRUCT(info);
	info.strength = ((double)params[PARAM_STRENGTH]->u.fs_d.value);
	info.invert = (int)((params[PARAM_INVERT]->u.bd.value));
	info.ref = inputP;
	info.in_data = *in_data;

	if (PF_WORLD_IS_DEEP(output)) {
		ERR(suites.Iterate16Suite1()->iterate(in_data, 0, linesL, inputP, NULL, (void*)&info, Normal16, output));
	} else {
		ERR(suites.Iterate8Suite1()->iterate(in_data, 0, linesL, inputP, NULL, (void*)&info, Normal8, output));
	}

	return err;
}

static PF_Err ParamsSetup(
	PF_InData *in_data,
	PF_OutData *out_data,
	PF_ParamDef	*params[],
	PF_LayerDef	*output
) {
	PF_ParamDef	def;
	
	// define ui
	AEFX_CLR_STRUCT(def);
	PF_ADD_FLOAT_SLIDER("Strength", 1, 100, 0, 10, 0, 1, 0, 0, 0, PARAM_STRENGTH);
	AEFX_CLR_STRUCT(def);
	PF_ADD_CHECKBOXX("Invert", 0, 0, PARAM_INVERT);
	AEFX_CLR_STRUCT(def);
	out_data->num_params = PARAM_COUNT;

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

static PF_Err About(
	PF_InData *in_data,
	PF_OutData *out_data,
	PF_ParamDef *params[],
	PF_LayerDef *output
) {
	AEGP_SuiteHandler suites(in_data->pica_basicP);
	suites.ANSICallbacksSuite1()->sprintf(out_data->return_msg, "%s v%d.%d\r%s", "Colour Halftone", MAJOR_VERSION, MINOR_VERSION, "Normalmapping by @meatbags");
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
