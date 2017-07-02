/*
 * Wire.cpp
 *
 *  Created on: Feb 14, 2017
 *      Author: m3r7
 */

#include "Wire.h"

namespace pceas {

Wire::Wire() {
	fmpz_init(value);
	assigned = false;
	prev = nullptr;
	next = nullptr;
	inputLabel = NONE;
}

Wire::~Wire() {
	fmpz_clear(value);
}

} /* namespace pceas */
