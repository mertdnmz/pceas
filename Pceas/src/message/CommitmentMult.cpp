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
 * CommitmentMult.cpp
 *
 *  Created on: Feb 26, 2017
 *      Author: m3r7
 */

#include "CommitmentMult.h"
#include "../core/PceasException.h"

namespace pceas {

CommitmentMult::CommitmentMult() {
	owner = NOPARTY;
	error = true;
}

CommitmentMult::CommitmentMult(CommitmentId c1, CommitmentId c2, CommitmentId c3, PartyId owner):cid1(c1),cid2(c2),cid3(c3),owner(owner){
	error = false;
}

CommitmentMult::~CommitmentMult() {
}

void CommitmentMult::addFkx(PartyId k, CommitmentId c) {
	auto const& p = fkx.insert(pair<PartyId, CommitmentId>(k, c));
	if (!p.second) {
		throw PceasException("Could not insert combined coeff. commitment ID");
	}
}

CommitmentId CommitmentMult::getFkx(PartyId k) const {
	auto it = fkx.find(k);
	if (it == fkx.end()) {
		throw PceasException("Could not find combined coeff. commitment ID");
	}
	return it->second;
}

void CommitmentMult::addGkx(PartyId k, CommitmentId c) {
	auto const& p = gkx.insert(pair<PartyId, CommitmentId>(k, c));
	if (!p.second) {
		throw PceasException("Could not insert combined coeff. commitment ID");
	}
}

CommitmentId CommitmentMult::getGkx(PartyId k) const {
	auto it = gkx.find(k);
	if (it == gkx.end()) {
		throw PceasException("Could not find combined coeff. commitment ID");
	}
	return it->second;
}

void CommitmentMult::addHkx(PartyId k, CommitmentId c) {
	auto const& p = hkx.insert(pair<PartyId, CommitmentId>(k, c));
	if (!p.second) {
		throw PceasException("Could not insert combined coeff. commitment ID");
	}
}

CommitmentId CommitmentMult::getHkx(PartyId k) const {
	auto it = hkx.find(k);
	if (it == hkx.end()) {
		throw PceasException("Could not find combined coeff. commitment ID");
	}
	return it->second;
}

void CommitmentMult::addRejecter(PartyId p) {
	rejecters.insert(p);
}

bool CommitmentMult::isRejected() const {
	return !rejecters.empty();
}

void CommitmentMult::print(stringstream& ss) const {
	ss << "Commitment Mult : \t" << cid1 << " * " << cid2 << " = " << cid3 << endl;
	if (!rejecters.empty()) {
		ss << "rejecters : ";
		for (auto const r : rejecters) {
			ss << to_string(r) << "\t";
		}
		ss << endl;
	}
}

} /* namespace pceas */
