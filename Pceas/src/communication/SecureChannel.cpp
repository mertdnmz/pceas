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
 * SecureChannel.cpp
 *
 *  Created on: Feb 13, 2017
 *      Author: m3r7
 */

#include "SecureChannel.h"
#include "../core/PceasException.h"

namespace pceas {

SecureChannel::SecureChannel() {
}

SecureChannel::~SecureChannel() {
}

void SecureChannel::send(MessagePtr m) {
	future = m;
}

MessagePtr SecureChannel::recv() {
	if (message == nullptr) {
		throw PceasException("No message.");
	}
	return message;
}

bool SecureChannel::hasMsg() const {
	return message != nullptr;
}

/**
 * Called by synchronizer
 */
void SecureChannel::swapToFuture() {
	message = nullptr;//discard past
	swap(message, future);
}

} /* namespace pceas */
