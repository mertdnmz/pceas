/**************************************************************************************
**
** Copyright (C) 2017 Mert Dönmez
**
** This file is part of PCEAS
**
** PCEAS is free software: you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation, either version 3 of the License, or
** (at your option) any later version.
**
** PCEAS is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with PCEAS.  If not, see <http://www.gnu.org/licenses/>.
**************************************************************************************/
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
