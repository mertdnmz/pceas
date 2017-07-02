/*
 * CommitmentMult.h
 *
 *  Created on: Feb 26, 2017
 *      Author: m3r7
 */

#ifndef COMMITMENTMULT_H_
#define COMMITMENTMULT_H_

#include <unordered_map>
#include <unordered_set>
#include <sstream>
#include "../core/Pceas.h"

namespace pceas {

using namespace std;

struct CommitmentMult {
public:
	CommitmentMult();
	CommitmentMult(CommitmentId c1, CommitmentId c2, CommitmentId c3, PartyId owner);
	virtual ~CommitmentMult();
	CommitmentId cid1;
	CommitmentId cid2;
	CommitmentId cid3;
	PartyId owner;
	bool error;
	unordered_set<PartyId> rejecters;
	unordered_map<PartyId, CommitmentId> fkx;
	unordered_map<PartyId, CommitmentId> gkx;
	unordered_map<PartyId, CommitmentId> hkx;

	void addFkx(PartyId k, CommitmentId c);
	CommitmentId getFkx(PartyId k) const;
	void addGkx(PartyId k, CommitmentId c);
	CommitmentId getGkx(PartyId k) const;
	void addHkx(PartyId k, CommitmentId c);
	CommitmentId getHkx(PartyId k) const;
	void addRejecter(PartyId p);
	bool isRejected() const;

	void print(stringstream& ss) const;
};

} /* namespace pceas */

#endif /* COMMITMENTMULT_H_ */
