/*
 * CommitTable.cpp
 *
 *  Created on: Feb 19, 2017
 *      Author: m3r7
 */

#include "CommitmentTable.h"

namespace pceas {

CommitmentTable::CommitmentTable(PartyId p, fmpz_t const& m):pid(p) {
	fmpz_init(mod);
	fmpz_set(mod, m);
	counter = 0;
}

CommitmentTable::~CommitmentTable() {
	for (auto const& pair : records) {
		delete pair.second;
	}
	fmpz_clear(mod);
}

CommitmentId CommitmentTable::addRecord(PartyId owner) {
	CommitmentId cid = getNextCommitId();
	addRecord(owner, cid);
	return cid;
}

CommitmentId CommitmentTable::addRecord(PartyId owner, CommitmentId cid) {
	/*
	 * We have a way of generating commitments IDs, such that all honest
	 * can agree without interaction. We use this to prevent dishonest from
	 * disturbing the protocol by sending conflicting IDs (or IDs that will
	 * conflict with IDs which will be generated in the future) :
	 *  - Due to concensus broadcast of all commitment intentions, all honest
	 *    can agree that a conflict happened.
	 *  - Implementation guarantees that all honest will agree on the value of
	 *    'counter' at all times.
	 *  - We disallow the use of the prefix we used for share naming scheme.
	 *  For any other conflict, all honest will generate a new ID using 'counter',
	 *  (hence they will all generate the same ID).
	 */
	if (cid.empty() || exists(cid)) {
		do {
			cid = getNextCommitId();
		} while (exists(cid));
	}
	CommitmentRecord* rec = new CommitmentRecord(owner, mod, pid);
	rec->setCommitid(cid);
	records.insert(make_pair(cid, rec));
	return cid;
}

void CommitmentTable::addRecord(CommitmentRecord* cr) {
	if (cr->getCommitid().empty() || exists(cr->getCommitid())) {
		throw runtime_error("Bad commitment ID : "+cr->getCommitid());
	}
	records.insert(make_pair(cr->getCommitid(), cr));
}

void CommitmentTable::removeRecord(CommitmentRecord* cr) {
	auto it = records.find(cr->getCommitid());
	if (it == records.end()) {
		throw runtime_error("Record not found : "+cr->getCommitid());
	}
	records.erase(it);
}

bool CommitmentTable::exists(CommitmentId cid) const {
	return records.find(cid) != records.end();
}

CommitmentRecord* CommitmentTable::getRecord(CommitmentId cid) {
	auto it = records.find(cid);
	if (it == records.end()) {
		return nullptr;
	}
	return it->second;
}

CommitmentRecord* CommitmentTable::getRecordForOngoingCommitment(PartyId partyid) {
	for (auto const& pair : records) {
		if (pair.second->getOwner() == partyid && pair.second->inProgress()) {
			return pair.second;
		}
	}
	return nullptr;
}

vector<CommitmentId> CommitmentTable::getOngoingCommits() const {
	vector<CommitmentId> cids;
	for (auto const& pair : records) {
		if (pair.second->inProgress()) {
			cids.push_back(pair.first);
		}
	}
	return cids;
}

vector<CommitmentRecord*> CommitmentTable::getVSSharesReceivedBy(PartyId r) const {
	vector<CommitmentRecord*> crs;
	for (auto const& pair : records) {
		if (pair.second->isVss() && pair.second->getOwner() == r) {
			crs.push_back(pair.second);
		}
	}
	return crs;
}

ulong CommitmentTable::getInputShareCountReceivedBy(PartyId r) const {
	return getInputSharesReceivedBy(r).size();
}

vector<CommitmentRecord*> CommitmentTable::getInputSharesReceivedBy(PartyId r) const {
	vector<CommitmentRecord*> ret;
	for (auto const& pair : records) {
		if (pair.second->isInput() && pair.second->getOwner() == r) {
			ret.push_back(pair.second);
		}
	}
	return ret;
}

vector<CommitmentRecord*> CommitmentTable::getOutputShares() const {
	vector<CommitmentRecord*> ret;
	for (auto const& pair : records) {
		if (pair.second->isOutput()) {
			ret.push_back(pair.second);
		}
	}
	return ret;
}

void CommitmentTable::clearVssFlags() {
	for (auto const& pair : records) {
		pair.second->setVss(false);
	}
}

void CommitmentTable::cleanUp() {
	for (auto it = records.begin(); it != records.end(); ) {
		if (!it->second->isPermanent()) {
			delete it->second;
			it = records.erase(it);
		} else {
			++it;
		}
	}
}

void CommitmentTable::rename(CommitmentId oldName, CommitmentId newName) {
	if (records.find(newName) != records.end()) {
		throw runtime_error("Commitment ID already exists : "+newName);
	}
	auto it = records.find(oldName);
	if (it == records.end()) {
		throw runtime_error("Bad commitment ID : "+oldName);
	}
	CommitmentRecord* cr = it->second;
	records.erase(it);
	cr->setCommitid(newName);
	records.insert(pair<CommitmentId, CommitmentRecord*>(newName, cr));
}

CommitmentId CommitmentTable::getNextCommitId() {
	CommitmentId cid = "party";
	cid+=to_string(pid);
	cid+="_commitment_";
	cid+=to_string(++counter);
	return cid;
}

void CommitmentTable::print(stringstream& ss) {
	ss << endl << "//////////////////////////////////////////////////////////////" << endl;
	ss << "Records for Party " << to_string(pid) << endl;
	for (auto const& pair : records) {
		pair.second->print(ss);
	}
}

} /* namespace pceas */
