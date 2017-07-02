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
