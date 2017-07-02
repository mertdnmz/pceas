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
