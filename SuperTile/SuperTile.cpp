/**
** SuperTile.cpp
**/

#include "SuperTile.h"

static PF_Err About(
	PF_InData *in_data,
	PF_OutData *out_data,
	PF_ParamDef *params[],
	PF_LayerDef *output
) {
	AEGP_SuiteHandler suites(in_data->pica_basicP);
	suites.ANSICallbacksSuite1()->sprintf(out_data->return_msg, "%s v%d.%d\r%s", "SuperTile", MAJOR_VERSION, MINOR_VERSION, "Sprite tiling by @meatbags");
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
	
	// Create UI params
	AEFX_CLR_STRUCT(def);
	PF_ADD_LAYER("Target Layer", PF_LayerDefault_NONE, ST_TARGET_LAYER_ID);
	AEFX_CLR_STRUCT(def);
	PF_ADD_FLOAT_SLIDERX("Tile Size", ST_SIZE_MIN, ST_SIZE_MAX, ST_SIZE_MIN, ST_SIZE_MAX, ST_SIZE_DFLT, PF_Precision_INTEGER, NULL, PF_PUI_NONE, ST_SIZE_SRC_ID);
	AEFX_CLR_STRUCT(def);
	PF_ADD_FLOAT_SLIDERX("Output Size", ST_SIZE_MIN, ST_SIZE_MAX, ST_SIZE_MIN, ST_SIZE_MAX, ST_SIZE_DFLT, PF_Precision_INTEGER, NULL, PF_PUI_NONE, ST_SIZE_DEST_ID);
	out_data->num_params = ST_NUM_PARAMS;
	
	return PF_Err_NONE;
}

static PF_Err Sample(
	PF_InData *in_data,
	PF_LayerDef *layer,
	HSL *hsl_array,
	double tile_size,
	double tile_offset,
	int columns,
	int index_max,
	bool world_is_deep
) {
	PF_Err err = PF_Err_NONE;

	// Setup
	AEGP_SuiteHandler suites(in_data->pica_basicP);
	PF_SampPB src_sampPB;
	src_sampPB.src = layer;
	src_sampPB.x_radius = D2FIX(tile_offset);
	src_sampPB.y_radius = D2FIX(tile_offset);

	// Sample
	if (world_is_deep) {
		for (int i = 0; i < index_max; ++i) {
			int x = i % columns;
			int y = (i - x) / columns;
			PF_Fixed fX = D2FIX(round(x * tile_size + tile_offset));
			PF_Fixed fY = D2FIX(round(y * tile_size + tile_offset));
			PF_Pixel16 pixel;
			ERR(suites.Sampling16Suite1()->area_sample16(in_data->effect_ref, fX, fY, &src_sampPB, &pixel));
			getHSL16(&pixel, &hsl_array[i]);
		}
	} else {
		for (int i = 0; i < index_max; ++i) {
			int x = i % columns;
			int y = (i - x) / columns;
			PF_Fixed fX = D2FIX(round(x * tile_size + tile_offset));
			PF_Fixed fY = D2FIX(round(y * tile_size + tile_offset));
			PF_Pixel8 pixel;
			ERR(suites.Sampling8Suite1()->area_sample(in_data->effect_ref, fX, fY, &src_sampPB, &pixel));
			getHSL8(&pixel, &hsl_array[i]);
		}
	}

	return err;
}

static PF_Err Render(
	PF_InData		*in_data,
	PF_OutData		*out_data,
	PF_ParamDef		*params[],
	PF_LayerDef		*output
) {
	PF_Err err = PF_Err_NONE;
	PF_Err err2 = PF_Err_NONE;

	// Setup
	AEGP_SuiteHandler suites(in_data->pica_basicP);
	PF_EffectWorld *input_layer = &params[ST_INPUT]->u.ld;
	double qscale = input_layer->width / (double)in_data->width;
	bool world_is_deep = PF_WORLD_IS_DEEP(output);

	// Effect params
	double src_tile_size = params[ST_SIZE_SRC_ID]->u.fs_d.value * qscale;
	double src_tile_offset = src_tile_size / 2;
	double input_tile_size = params[ST_SIZE_DEST_ID]->u.fs_d.value * qscale;
	double input_tile_offset = input_tile_size / 2;
		
	// Checkout tile layer
	PF_ParamDef checkout;
	AEFX_CLR_STRUCT(checkout);
	ERR(PF_CHECKOUT_PARAM(in_data, ST_TARGET_LAYER_ID, in_data->current_time, in_data->time_step, in_data->time_scale, &checkout));

	if (!err && checkout.u.ld.width > 0) {
		// Source and input sizes
		int src_columns = (int)ceil(checkout.u.ld.width / src_tile_size);
		int src_rows = (int)ceil(checkout.u.ld.height / src_tile_size);
		int src_cell_count = src_columns * src_rows;
		int input_columns = (int)ceil(input_layer->width / input_tile_size);
		int input_rows = (int)ceil(input_layer->height / input_tile_size);
		int input_cell_count = input_columns * input_rows;
		
		// Allocate memory
		AEGP_MemHandle mem_handle_src;
		HSL *src_hsl;
		int src_mem_size = src_cell_count * sizeof(HSL);
		ERR(suites.MemorySuite1()->AEGP_NewMemHandle(NULL, "Memory Allocation Error (1)", src_mem_size, AEGP_MemFlag_CLEAR, &mem_handle_src));
		ERR(suites.MemorySuite1()->AEGP_LockMemHandle(mem_handle_src, (void**)&src_hsl));
			
		AEGP_MemHandle mem_handle_input;
		HSL *input_hsl;
		int input_mem_size = input_cell_count * sizeof(HSL);
		ERR(suites.MemorySuite1()->AEGP_NewMemHandle(NULL, "Memory Allocation Error (2)", input_mem_size, AEGP_MemFlag_CLEAR, &mem_handle_input));
		ERR(suites.MemorySuite1()->AEGP_LockMemHandle(mem_handle_input, (void**)&input_hsl));

		// Sample source layer and get HSL
		Sample(in_data, &checkout.u.ld, src_hsl, src_tile_size, src_tile_offset, src_columns, src_cell_count, world_is_deep);
		
		// Sample input layer and get HSL
		Sample(in_data, input_layer, input_hsl, input_tile_size, input_tile_offset, input_columns, input_cell_count, world_is_deep);

		// Clear output buffer
		PF_Rect clear_rect;
		clear_rect.left = in_data->output_origin_x;
		clear_rect.right = clear_rect.left + input_layer->width;
		clear_rect.top = in_data->output_origin_y;
		clear_rect.bottom = clear_rect.top + input_layer->height;			
		ERR(suites.FillMatteSuite2()->fill(in_data->effect_ref, NULL, &clear_rect, output));

		// Map input HSL to source HSL and copy region
		for (int i = 0; i < input_cell_count; ++i) {
			// Compate HSL vectors, find best match
			double dMin = 1000.0;
			int res = i;

			for (int j = 0; j < src_cell_count; ++j) {
				double d = getHSLMag(&input_hsl[i], &src_hsl[j]);
				if (d < dMin) {
					dMin = d;
					res = j;
				}
			}

			// Get source region
			PF_Rect src_rect;
			src_rect.left = (int)round((res % src_columns) * src_tile_size);
			src_rect.top = (int)round(((res - res % src_columns) / src_columns) * src_tile_size);
			src_rect.right = (int)round(src_rect.left + src_tile_size);
			src_rect.bottom = (int)round(src_rect.top + src_tile_size);
				
			// Prepare destination region
			double scale = 0.75 + (1.0 - input_hsl[i].l) * 0.25;
			double x = (i % input_columns) * input_tile_size + input_tile_offset;
			double y = ((i - i % input_columns) / input_columns) * input_tile_size + input_tile_offset;
			PF_Rect dest_rect;
			dest_rect.left = (int)floor(x - input_tile_offset * scale);
			dest_rect.top = (int)floor(y - input_tile_offset * scale);
			dest_rect.right = (int)ceil(x + input_tile_offset * scale);
			dest_rect.bottom = (int)ceil(y + input_tile_offset * scale);

			// copy
			suites.WorldTransformSuite1()->copy(in_data->effect_ref, &checkout.u.ld, output, &src_rect, &dest_rect);
		}

		// Free up memory
		suites.MemorySuite1()->AEGP_UnlockMemHandle(mem_handle_input);
		suites.MemorySuite1()->AEGP_UnlockMemHandle(mem_handle_src);
		suites.MemorySuite1()->AEGP_FreeMemHandle(mem_handle_input);
		suites.MemorySuite1()->AEGP_FreeMemHandle(mem_handle_src);
	} else {
		// No valid layer, copy input->output
		PF_Rect rect;
		rect.left = in_data->output_origin_x;
		rect.right = rect.left + input_layer->width;
		rect.top = in_data->output_origin_y;
		rect.bottom = rect.top + input_layer->height;
		ERR(PF_COPY(input_layer, output, NULL, &rect));
	}

	ERR2(PF_CHECKIN_PARAM(in_data, &checkout));
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
		"SuperTile", // Name
		"ADBE_Super_Tile", // Match Name
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