#pragma once
#ifndef QUICKSORT_H
#define QUICKSORT_H
#include "PixelSort.h"

void swap8(Pixel8Data *a, Pixel8Data *b) {
	// swap pixel data values
	A_u_char alpha = a->p.alpha;
	A_u_char red = a->p.red;
	A_u_char green = a->p.green;
	A_u_char blue = a->p.blue;
	double value = a->value;
	a->p.alpha = b->p.alpha;
	a->p.red = b->p.red;
	a->p.green = b->p.green;
	a->p.blue = b->p.blue;
	a->value = b->value;
	b->p.alpha = alpha;
	b->p.red = red;
	b->p.green = green;
	b->p.blue = blue;
	b->value = value;
}

int partition8(Pixel8Data *p, int low, int high) {
	double pivot = p[high].value;
	int i = low - 1;
	for (int j = low; j <= high - 1; j++) {
		if (p[j].value <= pivot) {
			i++;
			swap8(&p[i], &p[j]);
		}
	}
	swap8(&p[i + 1], &p[high]);
	return i + 1;
}

void quickSort8(Pixel8Data *p, int low, int high) {
	if (low < high) {
		int part = partition8(p, low, high);
		quickSort8(p, low, part - 1);
		quickSort8(p, part + 1, high);
	}
}


#endif // QUICKSORT_H