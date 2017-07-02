/*
 * CommitmentTransfer.cpp
 *
 *  Created on: Feb 25, 2017
 *      Author: m3r7
 */

#include "CommitmentTransfer.h"
#include "../core/PceasException.h"

namespace pceas {

CommitmentTransfer::CommitmentTransfer() {
	transferSource = NOPARTY;
	transferTarget = NOPARTY;
	error = true;
}

CommitmentTransfer::CommitmentTransfer(CommitmentId commitId, PartyId transferSource, PartyId transferTarget):commitId(commitId), transferSource(transferSource), transferTarget(transferTarget) {
	error = false;
}

CommitmentTransfer::~CommitmentTransfer() {
}

void CommitmentTransfer::addFkx(PartyId k, CommitmentId c) {
	auto const& p = fkx.insert(pair<PartyId, CommitmentId>(k, c));
	if (!p.second) {
		throw PceasException("Could not insert combined coeff. commitment ID");
	}
}

CommitmentId CommitmentTransfer::getFkx(PartyId k) const {
	auto it = fkx.find(k);
	if (it == fkx.end()) {
		throw PceasException("Could not find combined coeff. commitment ID");
	}
	return it->second;
}

void CommitmentTransfer::addGkx(PartyId k, CommitmentId c) {
	auto const& p = gkx.insert(pair<PartyId, CommitmentId>(k, c));
	if (!p.second) {
		throw PceasException("Could not insert combined coeff. commitment ID");
	}
}

CommitmentId CommitmentTransfer::getGkx(PartyId k) const {
	auto it = gkx.find(k);
	if (it == gkx.end()) {
		throw PceasException("Could not find combined coeff. commitment ID");
	}
	return it->second;
}

void CommitmentTransfer::addRejecter(PartyId p) {
	rejecters.insert(p);
}

bool CommitmentTransfer::isRejected() const {
	return !rejecters.empty();
}

void CommitmentTransfer::print(stringstream& ss) const {
	ss << "Commitment ID : \t" << commitId << endl;
	ss << "transferSource : \t" << to_string(transferSource) << endl;
	ss << "transferTarget : \t" << to_string(transferTarget) << endl;
	ss << "exception : \t" << to_string(error) << endl;
	if (!rejecters.empty()) {
		ss << "rejecters : ";
		for (auto const r : rejecters) {
			ss << to_string(r) << "\t";
		}
		ss << endl;
	}
}

} /* namespace pceas */
