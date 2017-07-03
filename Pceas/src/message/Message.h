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
 * Message.h
 *
 *  Created on: Feb 8, 2017
 *      Author: m3r7
 */

#ifndef MESSAGE_H_
#define MESSAGE_H_

#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <memory>
#include <string>
#include "DisputedValue.h"
#include "VerifiableShare.h"
#include "CommitmentTransfer.h"
#include "CommitmentMult.h"
#include "Verifier.h"
#include "Accusation.h"

using namespace std;

namespace pceas {

class Message {
public:
	Message(PartyId sender, fmpz_t const& mod);
	virtual ~Message();

	void setVerifiableShare(fmpz_mod_poly_t const& fkx);
	void addOpenedVerifiableShare(PartyId k, fmpz_mod_poly_t const& fkx);
	void addAccused(PartyId id, string reason = "");
	void addVerifier(CommitmentId cid, fmpz_t const& val);
	bool getVerifier(CommitmentId cid, fmpz_t& val);
	void addDispute(CommitmentId cid, PartyId p);
	void getDisputes(CommitmentId cid, unordered_set<PartyId>& set) const;
	void addDisputedValue(PartyId disputer, PartyId disputed, fmpz_t const& val);
	bool getDisputedValue(PartyId disputer, PartyId disputed, fmpz_t& val);
	void addTransfer(CommitmentId c, PartyId s, PartyId t);
	void addMultiplication(CommitmentId c1, CommitmentId c2, CommitmentId c3, PartyId o);
	void addMultiplication();
	PartyId getSender() const {
		return sender;
	}
	void setShare(const fmpz_t val) {
		fmpz_set(share, val);
	}
	fmpz_t const& getShare() {
		return share;
	}
	fmpz_mod_poly_t const& getVerifiableShare() const {
		return verifiableShare.fkx;
	}
	const vector<VerifiableSharePtr>& getOpenedVerifiableShares() const {
		return openedVerifiableShares;
	}
	const CommitmentId& getCommitId() const {
		return commitId;
	}
	void setCommitId(const CommitmentId& commitId) {
		this->commitId = commitId;
	}
	const vector<Accusation>& getAccusations() const {
		return accusations;
	}
	bool isDesignatedOpenRejected() const {
		return designatedOpenRejected;
	}
	void setDesignatedOpenRejected() {
		this->designatedOpenRejected = true;
	}
	const vector<CommitmentTransfer>& getTransfers() const {
		return transfers;
	}
	const vector<CommitmentMult>& getMultiplications() const {
		return multiplications;
	}
	const vector<Message>& getBatchMessages() const {
		return batchMessages;
	}
	void addBatchMessage(Message const& m) {
		this->batchMessages.push_back(m);
	}
	bool isSuccess() const {
		return success;
	}
	void setSuccess(bool success) {
		this->success = success;
	}
	const string& getDebugInfo() const {
		return debugInfo;
	}
	void setDebugInfo(const string& debugInfo) {
		this->debugInfo = debugInfo;
	}
	PartyId getTarget() const {
		return target;
	}
	void setTarget(PartyId target) {
		this->target = target;
	}
	bool isInput() const {
		return input;
	}
	void setInput(string label) {
		this->input = true;
		this->inputLabel = label;
	}
	const string& getInputLabel() const {
		return inputLabel;
	}
	void printMsg();
	void printMsg(ulong channel);
private:
	PartyId sender; //party ID of sender.
	fmpz_t mod;

	fmpz_t share;
	VerifiableShare verifiableShare;
	bool input;
	string inputLabel;
	CommitmentId commitId;
	PartyId target;//Indicates the target of the ongoing action (If the action has a target) For example, it may hold the ID of the receiving party during a designated open

	/**
	 * A map of a commitment identifier and a point on a polynomial
	 * to be used for consistency check.
	 * Because multiple commitments take place simultaneously,
	 * parties are to send multiple verifiers (for distinct commitments)
	 * to each player in a single round.
	 */
	unordered_map<CommitmentId, Verifier> commitVerifiers;

	unordered_map< CommitmentId, unordered_set<PartyId> > disputes;

	vector<DisputedValue> disputedValues;

	vector<Accusation> accusations;

	vector<VerifiableSharePtr> openedVerifiableShares;

	bool designatedOpenRejected;

	vector<CommitmentTransfer> transfers;

	vector<CommitmentMult> multiplications;

	bool success;

	string debugInfo;

	vector<Message> batchMessages;
};

typedef shared_ptr<Message> MessagePtr;

} /* namespace pceas */

#endif /* MESSAGE_H_ */
