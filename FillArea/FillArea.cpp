#include "FillArea.hpp"

static PF_Err FillArea8(
	void *refcon,
	A_long x,
	A_long y,
	PF_Pixel8 *in,
	PF_Pixel8 *out
) {
	PF_Err err = PF_Err_NONE;
	register FillAreaInfo *info = (FillAreaInfo*)refcon;
	bool last = info->pass == info->pass_max;
	int index = y * info->layer->width + x;
	bool filled = IsFilled(info, x, y) || Walk8(info, x, y);
	
	if (!last) {
		Copy8(out, in);
	} else {
		if (filled) {
			A_u_char a = 255 - in->alpha;
			Copy8(out, a, 255, 0, 0);
		} else if (info->show_source) {
			Copy8(out, in->alpha, 255, 0, 0);
		} else {
			Copy8(out, 0, 0, 0, 0);
		}
	}

	return err;
}

static PF_Err FillArea16(
	void *refcon,
	A_long x,
	A_long y,
	PF_Pixel16 *in,
	PF_Pixel16 *out
) {
	PF_Err err = PF_Err_NONE;
	register FillAreaInfo *info = (FillAreaInfo*)refcon;
	out->alpha = PF_MAX_CHAN16;
	out->red = PF_MAX_CHAN16;
	out->green = PF_MAX_CHAN16;
	out->blue = PF_MAX_CHAN16;
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
	A_long rows = output->extent_hint.bottom - output->extent_hint.top;
	PF_EffectWorld *layer = &params[INPUT_LAYER]->u.ld;

	// UI
	FillAreaInfo info;
	AEFX_CLR_STRUCT(info);
	info.x = FIX2LONG(params[PARAM_POINT]->u.td.x_value);
	info.y = FIX2LONG(params[PARAM_POINT]->u.td.y_value);
	info.threshold = (double)params[PARAM_THRESHOLD]->u.sd.value / 100.0;
	info.pass = 0;
	info.pass_max = params[PARAM_PASSES]->u.sd.value;
	info.show_source = PF_Boolean((params[PARAM_SHOW_SOURCE]->u.bd.value));
	info.radius = (int)params[PARAM_RADIUS]->u.sd.value;
	info.layer = layer;

	// GET MEMORY
	AEGP_MemHandle mem_handle;
	int lim = layer->width * layer->height;
	int mem_size = lim * sizeof(boolean);
	AEGP_MemorySuite1 *mem_suite = suites.MemorySuite1();
	mem_suite->AEGP_NewMemHandle(NULL, "Sort8 Error", mem_size, AEGP_MemFlag_NONE, &mem_handle);
	mem_suite->AEGP_LockMemHandle(mem_handle, (void**)&info.filled_map);
	for (int i = 0; i < lim; i++) {
		info.filled_map[i] = 0;
	}

	if (PF_WORLD_IS_DEEP(output)) {
		ERR(suites.Iterate16Suite1()->iterate(in_data, 0, rows, layer, NULL, (void*)&info, FillArea16, output));
	} else {
		while (info.pass <= info.pass_max) {
			ERR(suites.Iterate8Suite1()->iterate(in_data, 0, rows, layer, NULL, (void*)&info, FillArea8, output));
			info.pass += 1;
		}
	}

	// FREE MEMORY
	mem_suite->AEGP_UnlockMemHandle(mem_handle);
	mem_suite->AEGP_FreeMemHandle(mem_handle);
	
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
		"FillArea",
		MAJOR_VERSION,
		MINOR_VERSION,
		"FillArea plugin by @meatbags"
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
	int threshold = 0;
	int passes = 2;
	int radius = 0;

	AEFX_CLR_STRUCT(def);
	PF_ADD_POINT("Point", 50, 50, false, PARAM_POINT);
	AEFX_CLR_STRUCT(def);
	PF_ADD_SLIDER("Threshold", 0, 100, 0, 100, threshold, PARAM_THRESHOLD);
	AEFX_CLR_STRUCT(def);
	PF_ADD_SLIDER("Passes", 0, 100, 0, 10, passes, PARAM_PASSES);
	AEFX_CLR_STRUCT(def);
	PF_ADD_CHECKBOXX("Show Source", 0, 0, PARAM_SHOW_SOURCE);
	AEFX_CLR_STRUCT(def);
	PF_ADD_SLIDER("Radius", 0, 5000, 0, 500, radius, PARAM_RADIUS);
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
