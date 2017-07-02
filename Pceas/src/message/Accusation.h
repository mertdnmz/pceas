/*
 * Accusation.h
 *
 *  Created on: Mar 10, 2017
 *      Author: m3r7
 */

#ifndef ACCUSATION_H_
#define ACCUSATION_H_

#include "../core/Pceas.h"

namespace pceas {

struct Accusation {
public:
	Accusation();
	virtual ~Accusation();
	PartyId accused;
	std::string reason;//debug info
};

} /* namespace pceas */

#endif /* ACCUSATION_H_ */
