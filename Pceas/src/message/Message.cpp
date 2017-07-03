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
 * Message.cpp
 *
 *  Created on: Feb 8, 2017
 *      Author: m3r7
 */

#include "Message.h"
#include <iostream>
#include "../math/MathUtil.h"
#include "../core/PceasException.h"

namespace pceas {

Message::Message(PartyId sender, fmpz_t const& m) : sender(sender), verifiableShare(VerifiableShare(m)) {
	fmpz_init(mod);
	fmpz_set(mod, m);
	fmpz_init(share);
	designatedOpenRejected = false;
	success = false;
	input = false;
	inputLabel = NONE;
	target = 0;
}

Message::~Message() {
	fmpz_clear(share);
	fmpz_clear(mod);
}

void Message::addVerifier(CommitmentId cid, fmpz_t const& val) {
	Verifier v;
	fmpz_set(v.vf, val);
	auto const& p = commitVerifiers.insert(pair<CommitmentId, Verifier>(cid, v));
	if (!p.second) {
		throw PceasException("Could not insert verifier");
	}
}

bool Message::getVerifier(CommitmentId cid, fmpz_t& val) {
	auto it = commitVerifiers.find(cid);
	if (it == commitVerifiers.end()) {
		return false;
	}
	fmpz_set(val, it->second.vf);
	return true;
}

void Message::addDispute(CommitmentId cid, PartyId p) {
	auto it = disputes.find(cid);
	if (it == disputes.end()) {
		unordered_set<PartyId> s;
		s.insert(p);
		disputes.insert(pair< CommitmentId, unordered_set<PartyId> >(cid, s));
	} else {
		disputes[cid].insert(p);
	}
}

void Message::getDisputes(CommitmentId cid, unordered_set<PartyId>& set) const {
	set.clear();
	auto it = disputes.find(cid);
	if (it != disputes.end()) {
		set = it->second;
	}
}

void Message::addAccused(PartyId id, string reason) {
	Accusation ac;
	ac.accused = id;
	ac.reason = reason;
	accusations.push_back(ac);
}

void Message::addTransfer(CommitmentId c, PartyId s, PartyId t) {
	CommitmentTransfer ct(c, s, t);
	transfers.push_back(ct);
}

void Message::addMultiplication(CommitmentId c1, CommitmentId c2, CommitmentId c3, PartyId o) {
	CommitmentMult cm(c1,c2,c3, o);
	multiplications.push_back(cm);
}

void Message::addOpenedVerifiableShare(PartyId k, fmpz_mod_poly_t const& fkx) {
	VerifiableSharePtr vs(new VerifiableShare(mod));
	fmpz_mod_poly_set(vs->fkx, fkx);
	vs->k = k;
	openedVerifiableShares.push_back(vs);
}

void Message::setVerifiableShare(fmpz_mod_poly_t const& fkx) {
	fmpz_mod_poly_set(verifiableShare.fkx, fkx);
}

void Message::addDisputedValue(PartyId disputer, PartyId disputed, fmpz_t const& val) {
	DisputedValue dv;
	dv.disputer = disputer;
	dv.disputed = disputed;
	fmpz_set(dv.val, val);
	disputedValues.push_back(dv);
}

bool Message::getDisputedValue(PartyId disputer, PartyId disputed, fmpz_t& val) {
	for (auto const& dv : disputedValues) {
		if (dv.disputer == disputer && dv.disputed == disputed) {
			fmpz_set(val,dv.val);
			return true;
		}
	}
	return false;
}

/**
 * For inspecting message contents..
 */
void Message::printMsg() {//Print Broadcast Message
	if (!debugInfo.empty()) {
		cout << debugInfo << endl;
	}
	cout << "Sent by Party " + to_string(sender) << endl;
	if (!commitId.empty()) {
		cout << "Commitment ID : " + commitId << endl;
	}
	if (target != 0) {
		cout << "Target Party " + to_string(target) << endl;
	}
	if (input != 0) {
		cout << "Input label " + inputLabel << endl;
	}
	cout << "Share : " << endl;
	cout << MathUtil::fmpzToStr(share); flint_printf("\n");
	cout << "Verifiable Share : " << endl;
	fmpz_mod_poly_print_pretty(verifiableShare.fkx, "x"); flint_printf("\n");
	if (!commitVerifiers.empty()) {
		cout << "Commitment Verifiers : " << endl;
		for (auto const& pair : commitVerifiers) {
			cout << MathUtil::fmpzToStr(pair.second.vf) << "\t";
		}
		cout << endl;
	}
	if (!disputes.empty()) {
		cout << "Disputes : " << endl;
		for (auto const& pair : disputes) {
			cout << "I dispute following parties for commitment " + pair.first << " : " << endl;
			for (auto const& p : pair.second) {
				cout << to_string(p) << "\t";
			}
			cout << endl;
		}
		cout << endl;
	}
	if (!disputedValues.empty()) {
		cout << "As commitment owner of " << commitId << " I open values for following disputes : " << endl;
		for (auto const& d : disputedValues) {
			cout << to_string(d.disputer) << " --disputes--> " << to_string(d.disputed) << " : " << MathUtil::fmpzToStr(d.val) << endl;
		}
	}
	if (!accusations.empty()) {
		cout << "Accused parties : " << endl;
		for (auto const& ac : accusations) {
			cout << to_string(ac.accused) << "\t" << ac.reason << endl;
		}
	}
	if (target != 0) {
		cout << "designatedOpenRejected : " << to_string(designatedOpenRejected) << endl;
	}
	if (!transfers.empty()) {
		cout << "Transfers : " << endl;
		for (auto const& t : transfers) {
			stringstream ss;
			t.print(ss);
			cout << ss.str();
			cout << "----" << endl;
		}
	}
	if (!multiplications.empty()) {
		cout << "Multiplications : " << endl;
		for (auto const& m : multiplications) {
			stringstream ss;
			m.print(ss);
			cout << ss.str();
			cout << "----" << endl;
		}
	}
	cout << endl << "------------------------------------" << endl;
}
void Message::printMsg(ulong channel) {//Print Private Message
	cout << "Private Message to Party " + to_string(channel+1) << endl;
	printMsg();
}

} /* namespace pceas */
