/*
 * DisputedValue.cpp
 *
 *  Created on: Feb 20, 2017
 *      Author: m3r7
 */

#include "DisputedValue.h"

namespace pceas {

DisputedValue::DisputedValue() {
	fmpz_init(val);
	disputer = 0;
	disputed = 0;
	opened = false;
}

DisputedValue::~DisputedValue() {
	fmpz_clear(val);
}

} /* namespace pceas */
