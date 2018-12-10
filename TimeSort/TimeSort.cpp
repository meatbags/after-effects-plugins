/**
 ** TimeSort
 ** A pixel-sorting plugin which operates along the time dimension.
 **/

#include "TimeSort.h"
#include "Noise.h"

static PF_Err About(
	PF_InData *in_data,
	PF_OutData *out_data,
	PF_ParamDef *params[],
	PF_LayerDef *output
) {
	AEGP_SuiteHandler suites(in_data->pica_basicP);
	suites.ANSICallbacksSuite1()->sprintf(out_data->return_msg, "%s v%d.%d\r%s", "TimeSort", MAJOR_VERSION, MINOR_VERSION, "Pixel time sorting by @meatbags");
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
	int chunk_max = 24;
	int chunk_min = 4;
	int chunk_dflt = 8;
	int lower_dflt = 0;
	int upper_dflt = 100;
	int noise_dflt = 50;
	
	// Create UI params
	AEFX_CLR_STRUCT(def);
	PF_ADD_SLIDER("Chunk Size", chunk_min, chunk_max, chunk_min, chunk_max, chunk_dflt, TIMESORT_CHUNK_SIZE);
	AEFX_CLR_STRUCT(def);
	PF_ADD_FLOAT_SLIDERX("Lower Threshold", 0, 100, 0, 100, lower_dflt, PF_Precision_TENTHS, NULL, NULL, TIMESORT_THRESHOLD_LOWER);
	AEFX_CLR_STRUCT(def);
	PF_ADD_FLOAT_SLIDERX("Upper Threshold", 0, 100, 0, 100, upper_dflt, PF_Precision_TENTHS, NULL, NULL, TIMESORT_THRESHOLD_UPPER);
	AEFX_CLR_STRUCT(def);
	PF_ADD_POPUPX("Sort Order", 4, 1, "Ascending|Descending|Peak|Trough", NULL, TIMESORT_ORDER);
	AEFX_CLR_STRUCT(def);
	PF_ADD_FLOAT_SLIDERX("Noise", 0, 100, 0, 100, noise_dflt, PF_Precision_TENTHS, NULL, NULL, TIMESORT_NOISE);
	out_data->num_params = TIMESORT_NUM_PARAMS;
	
	return PF_Err_NONE;
}

static PF_Err Sort8(
	void *refcon,
	A_long xL,
	A_long yL,
	PF_Pixel8 *inP,
	PF_Pixel8 *outP
) {
	PF_Err err = PF_Err_NONE;
	TimeSortInfo *info = (TimeSortInfo*)refcon;

	// Allocate memory
	AEGP_MemHandle mem_handle;
	Pixel8Data *pixels;
	int mem_size = info->checked_layers * sizeof(Pixel8Data);
	info->mem_suite->AEGP_NewMemHandle(NULL, "Sort8 Error", mem_size, AEGP_MemFlag_NONE, &mem_handle);
	info->mem_suite->AEGP_LockMemHandle(mem_handle, (void**)&pixels);

	// Populate pixel buffer
	int offset = yL * info->row_bytes + xL * sizeof(PF_Pixel8);
	for (int i = 0; i < info->checked_layers; i++) {
		pixels[i] = getPixelData8((PF_Pixel8 *)((char *)info->data + i * info->layer_bytes + offset));
	}

	if (filter8(pixels[info->target_index].value, info->lower, info->upper)) {
		// Calculate sort range
		int index_first = info->target_index;
		int index_last = info->target_index;
		double noise = (getNoise(xL, yL) + 1) / 2 * info->noise_scale;
		int default_index = (info->default_index + (int)round(noise * info->chunk_size)) % info->target_index;

		// Find start of chunk
		for (int i = info->target_index - 1; i >= default_index; i--) {
			if (!filter8(pixels[i].value, info->lower, info->upper)) {
				index_first = i + 1;
				break;
			} else if (i == default_index) {
				index_first = i;
				//index_first = info->default_start_index;
			}
		}

		// Find chunk end, working forward from target index
		int limit = index_first + info->chunk_extent;
		for (int i = info->target_index + 1; i <= limit; i++) {
			if (!filter8(pixels[i].value, info->lower, info->upper)) {
				index_last = i - 1;
				break;
			} else if (i == limit) {
				index_last = i;
			}
		}

		// Sort chunk
		quickSort8(pixels, index_first, index_last);

		// Arrange chunk by modifying index
		// 1 - Ascending, 2 - Descending, 3 - Peak, 4 - Trough
		int index = info->target_index;
		if (info->order == 3 || info->order == 4) {
			int index_centre = index_first + (index_last - index_first) / 2;
			index = index_first + (2 * abs(index - index_centre) - (index > index_centre ? 1 : 0));
		} 
		if (info->order == 2 || info->order == 4) {
			index = index_first + (index_last - index);
		}

		// unfold animation?
		//if (info->unfold == 1) {
		//	index = index_first + (index_last - index);
		//}

		// Output
		outP->alpha = pixels[index].p.alpha;
		outP->red = pixels[index].p.red;
		outP->green = pixels[index].p.green;
		outP->blue = pixels[index].p.blue;
	} else {
		// Target pixel failed, set outP -> inP
		outP->alpha = inP->alpha;
		outP->red = inP->red;
		outP->green = inP->green;
		outP->blue = inP->blue;
	}

	// Free memory
	info->mem_suite->AEGP_UnlockMemHandle(mem_handle);
	info->mem_suite->AEGP_FreeMemHandle(mem_handle);

	return err;
}

// return ((PF_Pixel8 *)((char*)inputP->data + (y * inputP->rowbytes) + x * sizeof(PF_Pixel8)));

static PF_Err Render(
	PF_InData *in_data,
	PF_OutData *out_data,
	PF_ParamDef *params[],
	PF_LayerDef *output
) {
	PF_Err err = PF_Err_NONE;
	PF_Err err2 = PF_Err_NONE;
	AEGP_SuiteHandler suites(in_data->pica_basicP);
	PF_LayerDef *input_layer = &params[TIMESORT_INPUT]->u.ld;

	// Effect params
	TimeSortInfo info;
	AEFX_CLR_STRUCT(info);
	info.chunk_size = params[TIMESORT_CHUNK_SIZE]->u.sd.value;
	info.lower = params[TIMESORT_THRESHOLD_LOWER]->u.fs_d.value / 100.0;
	info.upper = params[TIMESORT_THRESHOLD_UPPER]->u.fs_d.value / 100.0;
	info.order = params[TIMESORT_ORDER]->u.pd.value;
	info.noise_scale = params[TIMESORT_NOISE]->u.fs_d.value / 100.0;

	// Calculate limits
	info.chunk_extent = info.chunk_size - 1;
	info.time = in_data->current_time;
	info.time_step = in_data->time_step;
	info.time_start = info.time - in_data->time_step * info.chunk_extent;
	info.time_stop = info.time + in_data->time_step * info.chunk_extent;
	info.checked_layers = ((info.time_stop - info.time_start) / in_data->time_step) + 1;
	info.row_bytes = input_layer->rowbytes;
	info.width = input_layer->width;
	info.height = input_layer->height;

	// Allocate memory
	AEGP_MemHandle mem_handle;
	info.layer_bytes = input_layer->rowbytes * input_layer->height;
	A_u_long mem_size = info.checked_layers * info.layer_bytes;
	info.mem_suite = suites.MemorySuite1();
	info.mem_suite->AEGP_NewMemHandle(NULL, "Memory Error", mem_size, AEGP_MemFlag_NONE, &mem_handle);
	info.mem_suite->AEGP_LockMemHandle(mem_handle, (void**)&info.data);
	
	// Get layer data across time range
	for (int i = 0; i < info.checked_layers; i++) {
		if (!err) {
			// Checkout layer at time
			PF_ParamDef checkout;
			AEFX_CLR_STRUCT(checkout);
			A_long t = info.time_start + i * in_data->time_step;
			ERR(PF_CHECKOUT_PARAM(in_data, TIMESORT_INPUT, t, in_data->time_step, in_data->time_scale, &checkout));

			// Copy layer to buffer
			int base_offset = info.layer_bytes * i;
			for (int y = 0; y < checkout.u.ld.height; y++) {
				int offset = y * checkout.u.ld.rowbytes;
				memcpy((char *)info.data + base_offset + offset, (char *)checkout.u.ld.data + offset, checkout.u.ld.rowbytes);
			}

			// Mark index at current time
			if (t == info.time) {
				info.target_index = i;
			}

			// Check in layer
			ERR2(PF_CHECKIN_PARAM(in_data, &checkout));
		}
	}
	
	// Set default chunk start index
	info.chunk_time = info.chunk_size * in_data->time_step;
	info.default_index = max(0, info.target_index - ((info.time % info.chunk_time) / info.time_step));
	info.unfold = ((info.time_start + info.default_index * info.time_step) / info.chunk_time) % 2;

	// Per-pixel sort
	if (!err) {
		bool world_is_deep = PF_WORLD_IS_DEEP(output);
		A_long lines = output->extent_hint.bottom - output->extent_hint.top;
		if (!world_is_deep) {
			ERR(suites.Iterate8Suite1()->iterate(in_data, 0, lines, input_layer, NULL, (void*)&info, Sort8, output));
		}
	}

	// Free memory
	info.mem_suite->AEGP_UnlockMemHandle(mem_handle);
	info.mem_suite->AEGP_FreeMemHandle(mem_handle);

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
		"TimeSort", // Name
		"ADBE_TimeSort_v1", // Match Name
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