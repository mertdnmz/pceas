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
 * Secrets.h
 *
 *  Created on: Mar 28, 2017
 *      Author: m3r7
 */

#ifndef SECRETS_H_
#define SECRETS_H_

#include <unordered_map>
#include "Pceas.h"

namespace pceas {

class Secrets {
public:
	Secrets();
	virtual ~Secrets();
	void addSecret(std::string label, SecretValue val);
	SecretValue getSecretWithLabel(std::string label) const;
	const std::unordered_map<std::string, SecretValue>& getSecrets() const {
		return secretsMap;
	}
private:
	std::unordered_map<std::string, SecretValue> secretsMap;//label - value pairs
};

} /* namespace pceas */

#endif /* SECRETS_H_ */
