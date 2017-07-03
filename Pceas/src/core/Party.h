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
 * Party.h
 *
 *  Created on: Feb 7, 2017
 *      Author: m3r7
 */

#ifndef PARTY_H_
#define PARTY_H_

#include <vector>
#include <unordered_set>
#include <unordered_map>
#include <mutex>
#include <condition_variable>
#include "Pceas.h"
#include "CommitmentTable.h"
#include "MultiplicationTriple.h"
#include "Secrets.h"
#include "../circuit/Circuit.h"
#include "../communication/SecureChannel.h"
#include "../communication/ConsensusBroadcast.h"
#include "../math/MathUtil.h"

using namespace std;

namespace pceas {

class Party {
public:
	Party(PartyId pid, ulong partyCount, ulong threshold, ulong fieldPrime);
	virtual ~Party();
	void runProtocol();
	void runProtocolSequential(string prevRunResultLabel, Circuit* nextCircuit);
private:
	/** BEGIN Protocols implemented by the party **/
	void runPceps();
	void runPceas(bool circuitRandomization, bool finalRun = true);
	/** Subprotocols implemented by the party **/
	//Secret Sharing
	void distributeShares(fmpz_t const& val, string label = NONE);
	//Verifiable Secret Sharing (VSS)
	void distributeVerifiableShares(fmpz_t const& val, string uniqueSuffix, string label = NONE, bool preprocessingPhase = false, bool inputSharingPhase = false);
	void distributeVerifiableShares(CommitmentId cid, string uniqueSuffix, string label = NONE, bool preprocessingPhase = false, bool inputSharingPhase = false); // VSS from existing commitment
	//Preprocessing stage for 'CEAS with Circuit Randomization'
	void runPreprocessing();
	/** The 3 protocols below implement Fcom ideal functionality **/
	//Protocol 'Protocol Perfect-Com-Simple'
	CommitmentId commit(fmpz_t const& val, CommitmentId predeterminedCommitId = NONE);
	void publicCommit(CommitmentRecord* cr, fmpz_t const& val);
	void publicCommitToZero(CommitmentRecord* cr);
	void open(CommitmentId cid = NONE);
	void designatedOpen(CommitmentId commitid, PartyId k, bool isOutputOpening = false);
	CommitmentId addCommitments(CommitmentId cid1, CommitmentId cid2);
	CommitmentId constMultCommitment(fmpz_t const& c, CommitmentId cid);
	CommitmentId constAddCommitment(fmpz_t const& c, CommitmentId cid);
	CommitmentId substractCommitments(CommitmentId cid1, CommitmentId cid2);
	//Protocol 'Perfect Transfer' (of commitment)
	void transferCommitment(CommitmentId commitid, PartyId k);
	//Protocol 'Perfect Commitment Multiplication'
	CommitmentId multiplyCommitments(CommitmentId cid1 = NONE, CommitmentId cid2 = NONE);
	/** END Protocols **/

	void calculateDelta(fmpz_mod_poly_t& delta, PartyId i);
	void setRecombinationVector();
	CommitmentId runDegreeReduction(vector<CommitmentRecord*> const& shares, GateNumber gn);
	CommitmentId sumShares(vector<CommitmentRecord*> const& shares, GateNumber gn, const char* multiplicandId);
	void calculatePartyShare(ulong i, fmpz_mod_poly_t const& f);
	void calculatePartyShares(fmpz_mod_poly_t const& f);
	fmpz_t const& calculateZeroShare(fmpz_mod_poly_t const& poly);
	CommitmentRecord* zeroShareFor(PartyId p, CommitmentId cid);
	bool checkConsistency(const PartyId x, const PartyId y, fmpz_t const& val, const PartyId k, fmpz_mod_poly_t const& f_k);

	bool isCorrupt(PartyId p) const;
	void addCorrupt(PartyId p);
	PartyId getTargetForIteration(ulong i) const;
	PartyId getSourceFromTarget(PartyId target, PartyId sampleSource, PartyId sampleTarget) const;
	PartyId getTargetFromSource(PartyId source, PartyId sampleSource, PartyId sampleTarget) const;
	void sanityChecks();

	PartyId pid;//party ID
	/**
	 * An arithmetic circuit for the function to be securely evaluated
	 */
	Circuit* circuit;
	/**
	 * Holds information about commitments
	 */
	CommitmentTable* commitments;
	/**
	 * Holds IDs of parties who were detected to be dishonest.
	 * All honest players will agree on the set of dishonest players.
	 * Corrupted parties will be excluded from recombination.
	 */
	unordered_set<PartyId> corrupted;

	fmpz* recombinationVector;
	fmpz* shares; // temporary space for holding incoming and outgoing shares
	fmpz* multipointVec;//holds party ID's 1,2,...,N. Used for multipoint evaluation of shares

	/**
	 * Holds multiplication triples generated in preprocessing phase
	 * (if circuit randomization is used)
	 */
	unordered_map<GateNumber, MultiplicationTriple> triples;

	/**
	 * Number of computing parties.
	 */
	ulong N;
	/**
	 * Polynomial degree for Shamir's secret sharing scheme (= threshold - 1)
	 */
	ulong D;
	/**
	 * The prime number defining our Field : Z/pZ
	 */
	fmpz_t FIELD_PRIME;

	Protocol running;
	ulong maxDishonest; //Max number of dishonest parties the running protocol can handle

	fmpz_t value;//a member which can hold an integer value. ( 'a' in [a,f] and [[a,f]] )
	fmpz_mod_poly_t poly;//a member which can holds a polynomial over Z/pZ. ( 'f' in [a,f] and [[a,f]] )
	MathUtil* mu;

	/** BEGIN Commitment ID generation(=naming) schemes **/
	static constexpr const char* POLY_F = "f";
	static constexpr const char* POLY_G = "g";
	static constexpr const char* POLY_H = "h";

	static constexpr const char SEPERATOR = '@';
	static const string SHARE_PREFIX;
	static const string TRIPLE_PREFIX;

	CommitmentId getCoeffCommitIdForTransfer(CommitmentId cid, PartyId source, PartyId target, ulong coeff) const;
	CommitmentId getCoeffCommitIdForMult(string polyName, CommitmentId cid1, CommitmentId cid2, ulong coeff) const;
	CommitmentId getCoeffCommitIdForSharing(CommitmentId cid, ulong coeff) const;
	CommitmentId getCombinedCoeffCommitIdForTransfer(CommitmentId cid, ulong k, PartyId transferSource, PartyId transferTarget) const;
	CommitmentId getCombinedCoeffCommitIdForMult(string polyName, CommitmentId cid, CommitmentId cid1, CommitmentId cid2, ulong k, ulong degree) const;
	CommitmentId getCombinedCoeffCommitIdForSharing(CommitmentId cid, ulong k) const;
	CommitmentId combineCoeffCommitsForTransfer(CommitmentId cid, ulong k, PartyId transferSource, PartyId transferTarget);
	CommitmentId combineCoeffCommitsForMult(string polyName, CommitmentId cid, CommitmentId cid1, CommitmentId cid2, ulong k, ulong degree);
	CommitmentId combineCoeffCommitsForSharing(CommitmentId cid, ulong k);
	CommitmentId getMultipliedCommitId(CommitmentId cid1, CommitmentId cid2) const;
	CommitmentId getAddedCommitId(CommitmentId cid1, CommitmentId cid2) const;
	CommitmentId getConstMultCommitId(fmpz_t const& c, CommitmentId cid) const;
	pair<CommitmentId, CommitmentId> getSortedPair(CommitmentId cid1, CommitmentId cid2) const;
	CommitmentId getTransferedCommitId(CommitmentTransfer const& ct) const;
	CommitmentId getTransferedCommitId(CommitmentId id, PartyId source, PartyId target) const;
	CommitmentId makeShareName(PartyId distributer, PartyId receiver, string uniqueSuffix, bool input = false, bool mulTriple = false, bool assigned = false) const;
	CommitmentId makeShareName(string prefix, PartyId distributer, PartyId receiver, string uniqueSuffix) const;
	CommitmentId getShareNameFor(PartyId k, CommitmentId cid) const;
	vector<string> splitShareName(CommitmentId cid) const;
	CommitmentId makeTripleName(PartyId owner, string type, GateNumber gn) const;
	/** END **/

public:
	/** BEGIN Below are fields and methods related to the simulation of 'Secure Synchronous Communication' and 'Consensus Broadcast' **/
	/**
	 * Party signals to the synchronizing thread, that interaction is required to be able to continue.
	 */
	bool interactive;
	/**
	 * Synchronizing thread uses this flag to 'clock' the party. In other words,
	 * synchronizing thread signals to the thread running the protocol that,
	 * interaction is complete and protocol can resume.
	 */
	bool messagesReady;
	bool done;
	mutex m;
	condition_variable cv;
	SecureChannel** channels;
	ConsensusBroadcast* broadcast;

	void setBroadcast(ConsensusBroadcast* cb);
	void setChannelTo(ulong i, SecureChannel* sc);
	void runDummyInteractiveProtocol(uint i);//for debugging the synchronizer
	/** END **/
	/** Other simulation related fields and methods : **/
	void setCircuit(Circuit* c) {
		this->circuit = c;
	}
	void setProtocol(Protocol p) {
		this->running = p;
	}
	void addSecret(string label, ulong val);
	PartyId getDataUser() const {
		return dataUser;
	}
	void setDataUser(PartyId dataUser) {
		this->dataUser = dataUser;
	}
	void setDishonest() {
		this->dishonest = true;
	}
	void print(stringstream& ss);
private:
	bool dishonest; // For simulating dishonest behaviour. Not required by the protocols.
	PartyId dataUser; // evaluation result will be opened to this party.
	Secrets* secrets; // (Secret) inputs to the computation. Will be secret shared in the input sharing phase.
	MessagePtr newMsg() const;
	void interact();
	void end();
};

} /* namespace pceas */

#endif /* PARTY_H_ */
