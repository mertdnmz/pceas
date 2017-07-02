/*
 * CommitRecord.cpp
 *
 *  Created on: Feb 19, 2017
 *      Author: m3r7
 */

#include "../math/MathUtil.h"
#include "CommitmentRecord.h"

namespace pceas {

CommitmentRecord::CommitmentRecord(PartyId owner, fmpz_t const& m, PartyId recordHolder):
		owner(owner), recordHolder(recordHolder), verifiableShare(VerifiableShare(m)), broadcastVerifiableShare(VerifiableShare(m)), fx_0(VerifiableShare(m)) {
	fmpz_init(mod);
	fmpz_set(mod, m);
	fmpz_init(share);
	fmpz_init(openedValue);
	inprogress = true;
	success = false;
	opened = false;
	designatedOpened = false;
	input = false;
	inputLabel = NONE;
	output = false;
	vssFlag = false;
	newVerifiableShareBroadcast = false;
	inconsistentBroadcast = false;
	permanent = false;
	distributer = NOPARTY;
	mulTriple = false;
}

CommitmentRecord::~CommitmentRecord() {
	fmpz_clear(mod);
	fmpz_clear(share);
	fmpz_clear(openedValue);
}

void CommitmentRecord::addDispute(PartyId disputer, PartyId disputed) {
	for (auto const& dv : disputes) {
		if (dv.disputer == disputer && dv.disputed == disputed) {//ignore duplicates
			return;
		}
	}
	DisputedValue dv;
	dv.disputer = disputer;
	dv.disputed = disputed;
	disputes.push_back(dv);
}

void CommitmentRecord::setDisputeValue(PartyId disputer, PartyId disputed, fmpz_t const& val) {
	for (auto& dv : disputes) {
		if (dv.disputer == disputer && dv.disputed == disputed) {
			fmpz_set(dv.val, val);
			dv.opened = true;
			return;//adddispute guarantees single match
		}
	}
}

void CommitmentRecord::addAccuser(PartyId accuser) {
	accusers.insert(accuser);
}

PartyId CommitmentRecord::getAccuserCount() const {
	return accusers.size();
}

bool CommitmentRecord::isAccuser(PartyId party) const {
	auto it = accusers.find(party);
	return it != accusers.end();
}

bool CommitmentRecord::isValueOpenToUs() const {
	return isValueOpenTo(recordHolder);
}

bool CommitmentRecord::isValueOpenTo(PartyId p) const {
	return isOpened() || isDesignatedOpenedTo(p) || (owner == p);
}

void CommitmentRecord::setOpenedValue(fmpz_t const& ov) {
	//we reduce modulo p before we record, as we will often compare values
	fmpz_set(openedValue, ov);
	fmpz_mod(openedValue, openedValue, mod);
}

fmpz_t const& CommitmentRecord::getOpenedValue() const {
	if (!isValueOpenToUs()) {
		throw runtime_error("Tried to retrieve value for unopened commitment.");
	}
	return openedValue;
}

void CommitmentRecord::print(stringstream& ss) {
	ss << "---------------------Party" << to_string(recordHolder) << "---------------------" << endl;
	ss << "Commitment ID : \t" << commitid << endl;
	ss << "Owner : \t" << to_string(owner) << endl;
	ss << "opened : \t" << to_string(opened) << endl;
	if (!designatedOpenTargets.empty()) {
		ss << "designatedOpened to : \t";
		for (auto const& p : designatedOpenTargets) {
			ss << to_string(p) << "\t";
		}
		ss << endl;
	}
	ss << "Opened Value : \t" << MathUtil::fmpzToStr(openedValue) << endl;
	ss << "Share : \t" << MathUtil::fmpzToStr(share) << endl;
	ss << "success : \t" << to_string(success) << endl;
	ss << "inprogress : \t" << to_string(inprogress) << endl;
	if (inprogress) {
		ss << "inconsistentBroadcast : \t" << to_string(inconsistentBroadcast) << endl;
		ss << "newVerifiableShareBroadcast : \t" << to_string(newVerifiableShareBroadcast) << endl;
	}
	if (!disputes.empty()) {
		ss << "Disputes : \t" << endl;
		for (auto const& d : disputes) {
			ss << to_string(d.disputer) << " -> " << to_string(d.disputed) << " : "
					<< MathUtil::fmpzToStr(d.val) << " Opened : " << to_string(d.opened) << endl;
		}
	}
	if (!accusers.empty()) {
		ss << "Accusers : \t" << endl;
		for (auto const& ac : accusers) {
			ss << to_string(ac) << "\t";
		}
		ss << endl;
	}
	ss << "vssFlag : \t" << to_string(vssFlag) << endl;
	ss << "input : \t" << to_string(input) << endl;
	ss << "output : \t" << to_string(output) << endl;
	ss << "permanent : \t" << to_string(permanent) << endl;
}

} /* namespace pceas */
