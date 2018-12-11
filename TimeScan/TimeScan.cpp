// TimeScan.cpp

#include "TimeScan.h"

static PF_Err About(
	PF_InData *in_data,
	PF_OutData *out_data,
	PF_ParamDef *params[],
	PF_LayerDef *output
) {
	AEGP_SuiteHandler suites(in_data->pica_basicP);
	suites.ANSICallbacksSuite1()->sprintf(out_data->return_msg, "%s v%d.%d\r%s", "TimeScan", MAJOR_VERSION, MINOR_VERSION, "Pixel time sorting by @meatbags");
	return PF_Err_NONE;
}

static PF_Err GlobalSetup(
	PF_InData *in_data,
	PF_OutData *out_data,
	PF_ParamDef	*params[],
	PF_LayerDef	*output
) {
	out_data->out_flags = PF_OutFlag_DEEP_COLOR_AWARE;
	out_data->my_version = PF_VERSION(MAJOR_VERSION, MINOR_VERSION, BUG_VERSION, STAGE_VERSION,	BUILD_VERSION);
	return PF_Err_NONE;
}

static PF_Err ParamsSetup(
	PF_InData *in_data,
	PF_OutData *out_data,
	PF_ParamDef	*params[],
	PF_LayerDef	*output
) {
	PF_ParamDef	def;
	
	// Defaults
	int scale = 3;
	int speed = 24;

	// Set up user interface
	AEFX_CLR_STRUCT(def);
	PF_ADD_POPUP("Mode", 2, 1, "Vertical|Horizontal", TIMESCAN_MODE);
	AEFX_CLR_STRUCT(def);
	PF_ADD_POINT("Scan", 50, 50, NULL, TIMESCAN_POSITION);
	AEFX_CLR_STRUCT(def);
	PF_ADD_SLIDER("Scale", 1, 50, 1, 50, scale, TIMESCAN_SCALE);
	AEFX_CLR_STRUCT(def);
	PF_ADD_CHECKBOXX("Auto Pan", 1, NULL, TIMESCAN_AUTOPAN);
	AEFX_CLR_STRUCT(def);
	PF_ADD_SLIDER("Auto Pan Speed", 1, 1000, 1, 100, speed, TIMESCAN_AUTOPAN_SPEED);
	AEFX_CLR_STRUCT(def);
	PF_ADD_CHECKBOXX("Mirror", 0, NULL, TIMESCAN_MIRROR);
	out_data->num_params = TIMESCAN_NUM_PARAMS;
	
	return PF_Err_NONE;
}

static PF_Err Render(
	PF_InData *in_data,
	PF_OutData *out_data,
	PF_ParamDef *params[],
	PF_LayerDef *output
) {
	PF_Err err = PF_Err_NONE;
	PF_Err err2 = PF_Err_NONE;
	AEGP_SuiteHandler suites(in_data->pica_basicP);
	PF_LayerDef *input_layer = &params[TIMESCAN_INPUT]->u.ld;
	float qscale = input_layer->width / (float)in_data->width;

	// Create output buffer
	PF_LayerDef buffer;
	suites.WorldSuite1()->new_world(in_data->effect_ref, input_layer->width, input_layer->height, PF_NewWorldFlag_CLEAR_PIXELS, &buffer);
	int mode = params[TIMESCAN_MODE]->u.pd.value;
	A_long x = FIX2LONG(params[TIMESCAN_POSITION]->u.td.x_value);
	A_long y = FIX2LONG(params[TIMESCAN_POSITION]->u.td.y_value);
	int scale = max(1, (int)round(params[TIMESCAN_SCALE]->u.sd.value * qscale));
	int mirror_enabled = params[TIMESCAN_MIRROR]->u.button_d.value;

	// Autopan x, y values
	if (params[TIMESCAN_AUTOPAN]->u.button_d.value) {
		int speed = max(1, (int)round(params[TIMESCAN_AUTOPAN_SPEED]->u.sd.value * qscale));
		double t = in_data->current_time / (double)in_data->time_scale;
		x = x + (int)round(t * speed);
		y = y + (int)round(t * speed);
		x = (x / input_layer->width) % 2 == 0 ? input_layer->width - (x % input_layer->width) - 1 : x % input_layer->width;
		y = (y / input_layer->height) % 2 == 0 ? input_layer->height - (y % input_layer->height) - 1 : y % input_layer->height;
	}

	// Settings
	A_long time = in_data->current_time;
	PF_Rect src, dst;
	src.left = mode == 1 ? x : in_data->output_origin_x;
	src.right = src.left + (mode == 1 ? 1 : input_layer->width);
	src.top = mode == 1 ? in_data->output_origin_y : y;
	src.bottom = src.top + (mode == 1 ? input_layer->height : 1);
	dst.left = in_data->output_origin_x;
	dst.right = dst.left + input_layer->width;
	dst.top = in_data->output_origin_y;
	dst.bottom = dst.top + input_layer->height;
	A_long centre = (mirror_enabled)
		? (mode == 1 ? input_layer->width / 2 : input_layer->height / 2) - scale / 2
		: (mode == 1 ? input_layer->width : input_layer->height);
	int index = 0;

	while (!err && time >= 0 && dst.right >= in_data->output_origin_x && dst.bottom >= in_data->output_origin_y) {
		// Get sample layer
		PF_ParamDef checkout;
		AEFX_CLR_STRUCT(checkout);
		ERR(PF_CHECKOUT_PARAM(in_data, TIMESCAN_INPUT, time, in_data->time_step, in_data->time_scale, &checkout));

		// Copy sample to buffer
		if (mode == 1) {
			if (mirror_enabled) {
				dst.left = centre + index * scale;
				dst.right = dst.left + scale;
				ERR(suites.WorldTransformSuite1()->copy(in_data->effect_ref, &checkout.u.ld, &buffer, &src, &dst));
			}
			dst.left = centre - index * scale;
			dst.right = dst.left + scale;
			ERR(suites.WorldTransformSuite1()->copy(in_data->effect_ref, &checkout.u.ld, &buffer, &src, &dst));
		} else {
			if (mirror_enabled) {
				dst.top = centre + index * scale;
				dst.bottom = dst.top + scale;
				ERR(suites.WorldTransformSuite1()->copy(in_data->effect_ref, &checkout.u.ld, &buffer, &src, &dst));
			}
			dst.top = centre - index * scale;
			dst.bottom = dst.top + scale;
			ERR(suites.WorldTransformSuite1()->copy(in_data->effect_ref, &checkout.u.ld, &buffer, &src, &dst));
		}

		// Fill time gap
		if (time == 0) {
			if (mirror_enabled) {
				dst.left = mode == 1 ? centre + (index + 1) * scale : dst.left;
				dst.right = mode == 1 ? in_data->output_origin_x + input_layer->width : dst.right;
				dst.top = mode == 1 ? dst.top : centre + (index + 1) * scale;
				dst.bottom = mode == 1 ? dst.bottom : in_data->output_origin_y + input_layer->height;
				ERR(suites.WorldTransformSuite1()->copy(in_data->effect_ref, &checkout.u.ld, &buffer, &src, &dst));
			}
			dst.left = mode == 1 ? in_data->output_origin_x : dst.left;
			dst.right = mode == 1 ? centre - index * scale : dst.right;
			dst.top = mode == 1 ? dst.top : in_data->output_origin_y;
			dst.bottom = mode == 1 ? dst.bottom : centre - index * scale;
			ERR(suites.WorldTransformSuite1()->copy(in_data->effect_ref, &checkout.u.ld, &buffer, &src, &dst));
		}

		// Check in layer
		ERR2(PF_CHECKIN_PARAM(in_data, &checkout));

		// Update settings
		index += 1;
		time -= in_data->time_step;
	}

	// Copy to output
	ERR(suites.WorldTransformSuite1()->copy(in_data->effect_ref, &buffer, output, NULL, NULL));

	return err;
}

extern "C" DllExport
PF_Err PluginDataEntryFunction(
	PF_PluginDataPtr inPtr,
	PF_PluginDataCB inPluginDataCallBackPtr,
	SPBasicSuite* inSPBasicSuitePtr,
	const char* inHostName,
	const char* inHostVersion
) {
	PF_Err result = PF_Err_INVALID_CALLBACK;
	result = PF_REGISTER_EFFECT(
		inPtr,
		inPluginDataCallBackPtr,
		"TimeScan", // Name
		"ADBE_TimeScan_v1", // Match Name
		"Meatbags", // Category
		AE_RESERVED_INFO
	);
	return result;
}

PF_Err
EffectMain(
	PF_Cmd			cmd,
	PF_InData		*in_data,
	PF_OutData		*out_data,
	PF_ParamDef		*params[],
	PF_LayerDef		*output,
	void			*extra
) {
	AEGP_SuiteHandler suites(in_data->pica_basicP);
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
		default:
			break;
		}
	}
	catch (const PF_Err &thrown_err) {
		err = thrown_err;
	}
	return err;
}