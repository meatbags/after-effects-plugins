#include "Normal.hpp"

static PF_Err Normal8(
	void *refcon,
	A_long xL,
	A_long yL,
	PF_Pixel8 *inP,
	PF_Pixel8 *outP
) {
	PF_Err err = PF_Err_NONE;
	register NormalInfo *info = (NormalInfo*)refcon;

	// FIRST PASS -- SOBEL FILTER
	double TL = intensity8(info->ref, xL - 1, yL - 1);
	double T  = intensity8(info->ref, xL, yL - 1);
	double TR = intensity8(info->ref, xL + 1, yL + 1);
	double R  = intensity8(info->ref, xL + 1, yL);
	double L  = intensity8(info->ref, xL - 1, yL);
	double BL = intensity8(info->ref, xL - 1, yL + 1);
	double B  = intensity8(info->ref, xL, yL + 1);
	double BR = intensity8(info->ref, xL + 1, yL + 1);
	double nx = (TR + 2.0 * R + BR) - (TL + 2.0 * L + BL);
	double ny = (BL + 2.0 * B + BR) - (TL + 2.0 * T + TR);
	double nz = 1.0 / info->strength;
	double mag = sqrt(nx*nx + ny*ny + nz*nz);
	if (mag != 0) {
		nx /= mag;
		ny /= mag;
		nz /= mag;
	}

	// SECOND PASS -- SAMPLER
	if (info->useSampler) {
		// source pixel intensity
		double source_sample = intensity8(info->ref, xL, yL);
		double sx = 0;
		double sy = 0;
		double sz = 1.0 / info->strength;
		for (int i = 0; i < info->samples; ++i) {
			// generate pseudo-random offset
			double noise_s = source_sample;
			double noise_x = xL * ((xL + yL) % 2 == 0 ? 1 : -1);
			double noise_y = yL * ((xL + yL) % 2 == 0 ? 1 : -1);
			double noise_i = i / 100.0 * (source_sample > 0.5 ? 1 : -1) * ((xL + yL) % 2 == 0 ? 1 : -1);
			double rand1 = prand(noise_x + noise_s, noise_y + noise_s, info->seed + noise_i);
			double rand2 = prand(noise_y - noise_s, noise_x - noise_s, info->seed - noise_i);
			double angle = rand1 * PI_2;
			double dist = rand2 * info->sample_radius;
			double sin_a = sin(angle);
			double cos_a = cos(angle);
			int samp_x = (int)round(xL + cos_a * dist);
			int samp_y = (int)round(yL + sin_a * dist);

			// sample intensity at offset
			double sample = intensity8(info->ref, samp_x, samp_y);
			if (sample != source_sample) {
				double f = dist / info->sample_radius;
				double t = 1.0 - (f + (f - f*f));
				sx += cos_a * t * sample;
				sy += sin_a * t * sample;
			}
		}
		double mag = sqrt(sx*sx + sy*sy + sz*sz);
		if (mag != 0) {
			sx /= mag;
			sy /= mag;
			sz /= mag;
		}
		nx = blend(nx, sx, info->blend);
		ny = blend(ny, sy, info->blend);
		nz = blend(nz, sz, info->blend);
	}


	// INVERT
	if (info->invert) {
		nx *= -1;
		ny *= -1;
	}

	// SET PIXEL
	outP->alpha = inP->alpha;
	outP->red = (A_u_char)(((nx + 1) / 2) * PF_MAX_CHAN8);
	outP->green = (A_u_char)(((ny + 1) / 2) * PF_MAX_CHAN8);
	outP->blue = (A_u_char)(((nz + 1) / 2) * PF_MAX_CHAN8);
	return err;
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

	// FIRST PASS -- SOBEL FILTER
	double TL = intensity16(info->ref, xL - 1, yL - 1);
	double T  = intensity16(info->ref, xL, yL - 1);
	double TR = intensity16(info->ref, xL + 1, yL + 1);
	double R  = intensity16(info->ref, xL + 1, yL);
	double L  = intensity16(info->ref, xL - 1, yL);
	double BL = intensity16(info->ref, xL - 1, yL + 1);
	double B  = intensity16(info->ref, xL, yL + 1);
	double BR = intensity16(info->ref, xL + 1, yL + 1);
	double nx = (TR + 2.0 * R + BR) - (TL + 2.0 * L + BL);
	double ny = (BL + 2.0 * B + BR) - (TL + 2.0 * T + TR);
	double nz = 1.0 / info->strength;
	double mag = sqrt(nx*nx + ny*ny + nz*nz);
	if (mag != 0) {
		nx /= mag;
		ny /= mag;
		nz /= mag;
	}

	// SECOND PASS -- SAMPLER
	if (info->useSampler) {
		double source_sample = intensity16(info->ref, xL, yL);
		double sx = 0;
		double sy = 0;
		double sz = 1.0 / info->strength;
		for (int i = 0; i < info->samples; ++i) {
			double noise_s = source_sample;
			double noise_x = xL * ((xL + yL) % 2 == 0 ? 1 : -1);
			double noise_y = yL * ((xL + yL) % 2 == 0 ? 1 : -1);
			double noise_i = i / 100.0 * (source_sample > 0.5 ? 1 : -1);
			double rand1 = prand(noise_x + noise_s, noise_y + noise_s, info->seed + noise_i);
			double rand2 = prand(noise_y - noise_s, noise_x - noise_s, info->seed - noise_i);
			double angle = rand1 * PI_2;
			double dist = rand2 * info->sample_radius;
			double sin_a = sin(angle);
			double cos_a = cos(angle);
			int samp_x = (int)round(xL + cos_a * dist);
			int samp_y = (int)round(yL + sin_a * dist);
			double sample = intensity16(info->ref, samp_x, samp_y);
			if (sample != source_sample) {
				double f = dist / info->sample_radius;
				double t = 1.0 - (f + (f - f*f));
				sx += cos_a * t * sample;
				sy += sin_a * t * sample;
			}
		}
		double mag = sqrt(sx*sx + sy*sy + sz*sz);
		if (mag != 0) {
			sx /= mag;
			sy /= mag;
			sz /= mag;
		}
		nx = blend(nx, sx, info->blend);
		ny = blend(ny, sy, info->blend);
		nz = blend(nz, sz, info->blend);
	}


	// INVERT
	if (info->invert) {
		nx *= -1;
		ny *= -1;
	}

	// SET PIXEL
	outP->alpha = inP->alpha;
	outP->red = (A_u_short)(((nx + 1) / 2) * PF_MAX_CHAN16);
	outP->green = (A_u_short)(((ny + 1) / 2) * PF_MAX_CHAN16);
	outP->blue = (A_u_short)(((nz + 1) / 2) * PF_MAX_CHAN16);
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

	// UI
	AEFX_CLR_STRUCT(info);
	info.strength = ((double)params[PARAM_STRENGTH]->u.fs_d.value);
	info.invert = (int)((params[PARAM_INVERT]->u.bd.value));
	info.useSampler = (int)((params[PARAM_USE_SAMPLER]->u.bd.value));
	info.samples = ((int)params[PARAM_SAMPLES]->u.sd.value);
	info.sample_radius = ((double)params[PARAM_SAMPLE_RADIUS]->u.fs_d.value);
	info.blend = ((double)params[PARAM_BLEND]->u.fs_d.value) / 100.0;
	info.ref = inputP;
	info.in_data = *in_data;
	info.seed = (1 + in_data->current_time) / 100.0;
	info.flat_threshold = 1 / 100.0;

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
	
	// DEFAULTS
	int strength = 1;
	int invert = 0;
	int use_sampler = 0;
	int samples = 50;
	int sample_radius = 20;
	int blend = 40;

	// UI
	AEFX_CLR_STRUCT(def);
	PF_ADD_FLOAT_SLIDER("Strength", 1, 100, 0, 10, 0, strength, 0, 0, 0, PARAM_STRENGTH);
	AEFX_CLR_STRUCT(def);
	PF_ADD_CHECKBOXX("Invert", invert, 0, PARAM_INVERT);
	AEFX_CLR_STRUCT(def);
	PF_ADD_CHECKBOXX("Use Sampler", use_sampler, 0, PARAM_USE_SAMPLER);
	AEFX_CLR_STRUCT(def);
	PF_ADD_SLIDER("Samples", 0, 1000, 0, 100, samples, PARAM_SAMPLES);
	AEFX_CLR_STRUCT(def);
	PF_ADD_FLOAT_SLIDER("Sample Radius", 1, 20000, 1, 100, 0, sample_radius, 0, 0, 0, PARAM_SAMPLE_RADIUS);
	AEFX_CLR_STRUCT(def);
	PF_ADD_FLOAT_SLIDER("Blend", 0, 100, 0, 100, 0, blend, 0, 0, 0, PARAM_BLEND);
	
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
	suites.ANSICallbacksSuite1()->sprintf(out_data->return_msg, "%s v%d.%d\r%s", "Normal Mapping", MAJOR_VERSION, MINOR_VERSION, "Normalmapping by @meatbags");
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
