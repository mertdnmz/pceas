/*
 * CommitmentTransfer.h
 *
 *  Created on: Feb 25, 2017
 *      Author: m3r7
 */

#ifndef COMMITMENTTRANSFER_H_
#define COMMITMENTTRANSFER_H_

#include <unordered_map>
#include <unordered_set>
#include <sstream>
#include "../core/Pceas.h"

namespace pceas {

using namespace std;

struct CommitmentTransfer {
public:
	CommitmentTransfer();
	CommitmentTransfer(bool err);
	CommitmentTransfer(CommitmentId commitId, PartyId transferSource, PartyId transferTarget);
	virtual ~CommitmentTransfer();
	CommitmentId commitId;
	PartyId transferSource;
	PartyId transferTarget;
	bool error;//something went wrong with this transfer. We ignore it until Step 5.
	unordered_set<PartyId> rejecters;//steps 3-4
	CommitmentId transferedCommitId;
	unordered_map<PartyId, CommitmentId> fkx;//coefficients combined for source
	unordered_map<PartyId, CommitmentId> gkx;//coefficients combined for target

	void addFkx(PartyId k, CommitmentId c);
	CommitmentId getFkx(PartyId k) const;
	void addGkx(PartyId k, CommitmentId c);
	CommitmentId getGkx(PartyId k) const;
	void addRejecter(PartyId p);
	bool isRejected() const;

	void print(stringstream& ss) const;
};

} /* namespace pceas */

#endif /* COMMITMENTTRANSFER_H_ */
