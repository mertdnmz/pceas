/*
 * VerifiableShare.h
 *
 *  Created on: Feb 21, 2017
 *      Author: m3r7
 */

#ifndef VERIFIABLESHARE_H_
#define VERIFIABLESHARE_H_

#include <memory>
#include "../core/Pceas.h"
#include <fmpz_mod_poly.h>

namespace pceas {

struct VerifiableShare {
public:
	VerifiableShare(fmpz_t const& mod);
	virtual ~VerifiableShare();
	fmpz_mod_poly_t fkx;
	PartyId k;
};

typedef std::shared_ptr<VerifiableShare> VerifiableSharePtr;

} /* namespace pceas */

#endif /* VERIFIABLESHARE_H_ */
