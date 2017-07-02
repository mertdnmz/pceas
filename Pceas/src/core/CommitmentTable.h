/*
 * CommitTable.h
 *
 *  Created on: Feb 19, 2017
 *      Author: m3r7
 */

#ifndef COMMITTABLE_H_
#define COMMITTABLE_H_

#include <string>
#include <unordered_map>
#include <vector>

#include "CommitmentRecord.h"

using namespace std;

namespace pceas {

class CommitmentTable {
public:
	CommitmentTable(PartyId p, fmpz_t const& m);
	virtual ~CommitmentTable();
	CommitmentId addRecord(PartyId owner);
	CommitmentId addRecord(PartyId owner, CommitmentId cid);
	void addRecord(CommitmentRecord* cr);
	void removeRecord(CommitmentRecord* cr);
	bool exists(CommitmentId cid) const;
	CommitmentRecord* getRecord(CommitmentId cid);
	CommitmentRecord* getRecordForOngoingCommitment(PartyId partyid);
	vector<CommitmentId> getOngoingCommits() const;
	vector<CommitmentRecord*> getVSSharesReceivedBy(PartyId r) const;
	vector<CommitmentRecord*> getInputSharesReceivedBy(PartyId r) const;
	vector<CommitmentRecord*> getOutputShares() const;
	ulong getInputShareCountReceivedBy(PartyId r) const;
	void clearVssFlags();
	void cleanUp();
	void rename(CommitmentId oldName, CommitmentId newName);

	void print(stringstream& ss);
private:
	ulong counter;//counts the number of commitments. Used for making unique commit identifiers.
	PartyId pid;
	fmpz_t mod;
	unordered_map<CommitmentId,CommitmentRecord*> records; // Holds 'commit ID' - 'record' pairs.
	CommitmentId getNextCommitId();
};

} /* namespace pceas */

#endif /* COMMITTABLE_H_ */
