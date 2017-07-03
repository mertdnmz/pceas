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
 * Secrets.cpp
 *
 *  Created on: Mar 28, 2017
 *      Author: m3r7
 */

#include "Secrets.h"
#include "PceasException.h"

namespace pceas {

Secrets::Secrets() {
}

Secrets::~Secrets() {
}

void Secrets::addSecret(std::string label, SecretValue val) {
	auto const& p = secretsMap.insert(std::pair<std::string, SecretValue>(label, val));
	if (!p.second) {
		throw PceasException("Tried to insert secret with existing label : " + label);
	}
}

SecretValue Secrets::getSecretWithLabel(std::string label) const {
	auto it = secretsMap.find(label);
	if (it == secretsMap.end()) {
		throw PceasException("Party does not know secret with label : " + label);
	}
	return it->second;
}

} /* namespace pceas */
