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
