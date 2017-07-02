/*
 * VerifiableShare.cpp
 *
 *  Created on: Feb 21, 2017
 *      Author: m3r7
 */

#include "VerifiableShare.h"

namespace pceas {

VerifiableShare::VerifiableShare(fmpz_t const& mod) {
	fmpz_mod_poly_init(fkx, mod);
	fmpz_mod_poly_zero(fkx);
	k = 0;
}

VerifiableShare::~VerifiableShare() {
	fmpz_mod_poly_clear(fkx);
}

} /* namespace pceas */
