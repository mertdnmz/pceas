/*
 * DisputedValue.h
 *
 *  Created on: Feb 20, 2017
 *      Author: m3r7
 */

#ifndef DISPUTEDVALUE_H_
#define DISPUTEDVALUE_H_

#include "../core/Pceas.h"

namespace pceas {

struct DisputedValue {
public:
	DisputedValue();
	virtual ~DisputedValue();
	PartyId disputer;
	PartyId disputed;
	fmpz_t val;
	bool opened;
};

} /* namespace pceas */

#endif /* DISPUTEDVALUE_H_ */
