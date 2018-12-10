/**
 ** PixelSort
 ** Pixel sorting plugin with pattern options.
 ** NOTE -- About, GlobalSetup, ParamsSetup are AESDK boilerplate
 **/

#include "PixelSort.h"
#include "QuickSort.h"

static PF_Err About(
	PF_InData *in_data,
	PF_OutData *out_data,
	PF_ParamDef *params[],
	PF_LayerDef *output
) {
	AEGP_SuiteHandler suites(in_data->pica_basicP);
	suites.ANSICallbacksSuite1()->sprintf(out_data->return_msg, "%s v%d.%d\r%s", "PixelSort", MAJOR_VERSION, MINOR_VERSION, "Pixel sorting functions by @meatbags");
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
	int chunk_min = 2;
	int chunk_max = 2000;
	int chunk_dflt = 50;
	int lower_dflt = 0;
	int upper_dflt = 100;
	int displace_dflt = 100;
	
	// Create UI params
	AEFX_CLR_STRUCT(def);
	PF_ADD_POPUPX("Type", 2, 1, "Directional|Radial", NULL, PIXELSORT_ORDER);
	AEFX_CLR_STRUCT(def);
	PF_ADD_POPUPX("Sort By", 1, 1, "Brightness", NULL, PIXELSORT_ORDER);
	AEFX_CLR_STRUCT(def);
	PF_ADD_SLIDER("Max Chunk Size", chunk_min, chunk_max, chunk_min, chunk_max, chunk_dflt, PIXELSORT_CHUNK_SIZE);
	AEFX_CLR_STRUCT(def);
	PF_ADD_ANGLE("Direction", 0, PIXELSORT_DIRECTION);
	AEFX_CLR_STRUCT(def);
	PF_ADD_FLOAT_SLIDERX("Lower Threshold Input", 0, 100, 0, 100, lower_dflt, PF_Precision_TENTHS, NULL, NULL, PIXELSORT_LOWER_IN);
	AEFX_CLR_STRUCT(def);
	PF_ADD_FLOAT_SLIDERX("Upper Threshold Input", 0, 100, 0, 100, upper_dflt, PF_Precision_TENTHS, NULL, NULL, PIXELSORT_UPPER_IN);
	AEFX_CLR_STRUCT(def);
	PF_ADD_FLOAT_SLIDERX("Lower Threshold Output", 0, 100, 0, 100, lower_dflt, PF_Precision_TENTHS, NULL, NULL, PIXELSORT_LOWER_OUT);
	AEFX_CLR_STRUCT(def);
	PF_ADD_FLOAT_SLIDERX("Upper Threshold Output", 0, 100, 0, 100, upper_dflt, PF_Precision_TENTHS, NULL, NULL, PIXELSORT_UPPER_OUT);
	AEFX_CLR_STRUCT(def);
	PF_ADD_POPUPX("Sort Order", 4, 1, "Ascending|Descending|Peak|Trough", NULL, PIXELSORT_ORDER);
	AEFX_CLR_STRUCT(def);
	PF_ADD_POINT("Centre", 50, 50, false, PIXELSORT_CENTRE);
	AEFX_CLR_STRUCT(def);
	PF_ADD_LAYER("Displacement Map", PF_LayerDefault_NONE, PIXELSORT_DISPLACEMENT);
	AEFX_CLR_STRUCT(def);
	PF_ADD_FLOAT_SLIDERX("Displacement Scale", -100, 100, -100, 100, displace_dflt, PF_Precision_TENTHS, NULL, NULL, PIXELSORT_DISPLACEMENT_AMOUNT);

	// Register
	out_data->num_params = PIXELSORT_NUM_PARAMS;
	
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
	PixelSortInfo *info = (PixelSortInfo*)refcon;

	// Snap x, y to rotated grid
	/*
	double x = (xL - info->centre_x) + 0.5;
	double y = (yL - info->centre_y) + 0.5;
	double grid_distance = x * info->vec_x + y * info->vec_y;
	double grid_offset = round(grid_distance) - grid_distance;
	double grid_distance_perp = x * -info->vec_y + y * info->vec_x;
	double grid_offset_perp = round(grid_distance_perp) - grid_distance_perp;
	x += info->vec_x * grid_offset + (-info->vec_y * grid_offset_perp);
	y += info->vec_y * grid_offset + info->vec_x * grid_offset_perp;
	x += info->centre_x;
	y += info->centre_y;
	*/
	double x = xL + 0.5;
	double y = yL + 0.5;

	// Set search vector
	//double theta = fmod(info->direction, PF_PI / 2) + PF_PI / 4;
	//double scale = 1 + (PF_SQRT2 - 1) * abs(sin(theta));
	double step_x, step_y;
	if (info->use_displacement) {
		PF_Pixel8 *p = getPixel(info->map, xL % info->map->width, yL % info->map->height);
		double weight = (((p->red + p->green + p->blue) / (double)PF_MAX_CHAN8) / 3.0) * 2 - 1;
		double angle = info->direction + (weight * PF_PI * 2 * info->displacement_scale);
		step_x = cos(angle);
		step_y = sin(angle);
	} else {
		step_x = info->vec_x;
		step_y = info->vec_y;
	}

	// Find start or chunk or screen edge
	int samp_x = (int)floor(x);
	int samp_y = (int)floor(y);
	int chunk_size = 0;
	bool valid_pixel = true;
	while (valid_pixel && isOnScreen(info->layer, samp_x, samp_y)) {
		Pixel8Data p = getPixel8Data(getPixel(info->layer, samp_x, samp_y));
		valid_pixel = filter(p.value, info->lower_in, info->upper_in);
		if (valid_pixel) {
			chunk_size++;
			x -= step_x;
			y -= step_y;
			samp_x = (int)floor(x);
			samp_y = (int)floor(y);
		}
	}

	if (chunk_size > 0) {
		// Allocate memory for chunk
		AEGP_MemHandle mem_handle;
		Pixel8Data *pixels;
		int mem_size = info->chunk_size * sizeof(Pixel8Data);
		info->mem_suite->AEGP_NewMemHandle(NULL, "Memory Error", mem_size, AEGP_MemFlag_NONE, &mem_handle);
		info->mem_suite->AEGP_LockMemHandle(mem_handle, (void**)&pixels);

		// Get index & move sample to chunk start
		int chunk_mod = chunk_size % info->chunk_size;
		int offset_to_start;
		if (chunk_mod == 0 && chunk_size >= info->chunk_size) {
			offset_to_start = chunk_size - info->chunk_size + 1;
		} else {
			offset_to_start = chunk_size - chunk_mod + 1;
		}
		int index = chunk_size - offset_to_start;
		x += step_x * offset_to_start;
		y += step_y * offset_to_start;
		samp_x = (int)floor(x);
		samp_y = (int)floor(y);
		
		// Populate pixel buffer, find last index
		int index_first = 0;
		int index_last = 0;
		chunk_size = 0;
		for (int i = 0; i < info->chunk_size; i++) {
			if (isOnScreen(info->layer, samp_x, samp_y)) {
				Pixel8Data p = getPixel8Data(getPixel(info->layer, samp_x, samp_y));
				if (filter(p.value, info->lower_out, info->upper_out)) {
					pixels[i] = p;
					index_last = i;
					chunk_size += 1;

					// New sample point
					x += step_x;
					y += step_y;
					samp_x = (int)floor(x);
					samp_y = (int)floor(y);
				} else {
					break;
				}
			} else {
				break;
			}
		}

		if (chunk_size > 1) {
			// Sort
			quickSort8(pixels, index_first, index_last);

			// Modify index to reorder chunk
			if (info->order_by == 3 || info->order_by == 4) {
				int centre = index_last / 2;
				index = 2 * abs(index - centre) - (index > centre ? 1 : 0);
			}
			if (info->order_by == 2 || info->order_by == 4) {
				index = index_last - index;
			}
		} else {
			index = 0;
		}

		// Write to out pixel
		outP->alpha = pixels[index].p.alpha;
		outP->red = pixels[index].p.red;
		outP->green = pixels[index].p.green;
		outP->blue = pixels[index].p.blue;

		// Free memory
		info->mem_suite->AEGP_UnlockMemHandle(mem_handle);
		info->mem_suite->AEGP_FreeMemHandle(mem_handle);
	} else {
		// Pixel outside threshold, don't alter
		outP->alpha = inP->alpha;
		outP->red = inP->red;
		outP->green = inP->green;
		outP->blue = inP->blue;
	}

	return err;
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
	PF_LayerDef *input_layer = &params[PIXELSORT_INPUT]->u.ld;
	double scale = input_layer->width / (double)in_data->width;

	// Effect params
	PixelSortInfo info;
	AEFX_CLR_STRUCT(info);
	info.type = params[PIXELSORT_TYPE]->u.pd.value;
	info.sort_by = params[PIXELSORT_KEY]->u.pd.value;
	info.chunk_size = (int)round(params[PIXELSORT_CHUNK_SIZE]->u.sd.value * scale);
	info.direction = (FIX2D(params[PIXELSORT_DIRECTION]->u.ad.value) - 90) * PF_RAD_PER_DEGREE;
	info.lower_in = params[PIXELSORT_LOWER_IN]->u.fs_d.value / 100.0;
	info.upper_in = params[PIXELSORT_UPPER_IN]->u.fs_d.value / 100.0;
	info.lower_out = params[PIXELSORT_LOWER_OUT]->u.fs_d.value / 100.0;
	info.upper_out = params[PIXELSORT_UPPER_OUT]->u.fs_d.value / 100.0;
	info.order_by = params[PIXELSORT_ORDER]->u.pd.value;
	info.centre_x = FIX2D(params[PIXELSORT_CENTRE]->u.td.x_value);
	info.centre_y = FIX2D(params[PIXELSORT_CENTRE]->u.td.y_value);
	info.displacement_scale = params[PIXELSORT_DISPLACEMENT_AMOUNT]->u.fs_d.value / 100.0;
	info.vec_x = cos(info.direction);
	info.vec_y = sin(info.direction);

	// Memory suite reference
	info.mem_suite = suites.MemorySuite1();

	// Layer data
	info.layer = input_layer;
	info.map = &params[PIXELSORT_DISPLACEMENT]->u.ld;
	info.rowbytes = input_layer->rowbytes;

	// Checkout displacement Layer
	PF_ParamDef checkout;
	AEFX_CLR_STRUCT(checkout);
	ERR(PF_CHECKOUT_PARAM(in_data, PIXELSORT_DISPLACEMENT, in_data->current_time, in_data->time_step, in_data->time_scale, &checkout));
	if (checkout.u.ld.width > 0) {
		info.map = &checkout.u.ld;
		info.use_displacement = true;
	} else {
		info.use_displacement = false;
	}

	// Per-pixel sort
	if (!err) {
		bool world_is_deep = PF_WORLD_IS_DEEP(output);
		A_long lines = output->extent_hint.bottom - output->extent_hint.top;
		if (!world_is_deep) {
			ERR(suites.Iterate8Suite1()->iterate(in_data, 0, lines, input_layer, NULL, (void*)&info, Sort8, output));
		}
	}

	// Check in displacement layer
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
		"PixelSort", // Name
		"ADBE_PixelSort_v1", // Match Name
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