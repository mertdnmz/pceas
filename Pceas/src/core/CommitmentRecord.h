/*
 * CommitRecord.h
 *
 *  Created on: Feb 19, 2017
 *      Author: m3r7
 */

#ifndef COMMITRECORD_H_
#define COMMITRECORD_H_

#include <unordered_set>
#include <vector>
#include <sstream>
#include "../message/DisputedValue.h"
#include "../message/VerifiableShare.h"
#include "Pceas.h"

using namespace std;

namespace pceas {

class CommitmentRecord {
public:
	CommitmentRecord(PartyId owner, fmpz_t const& m, PartyId recordHolder);
	virtual ~CommitmentRecord();
	PartyId getOwner() const {
		return owner;
	}
	void setOwner(PartyId owner) {
		this->owner = owner;
	}
	const CommitmentId& getCommitid() const {
		return commitid;
	}
	void setCommitid(const CommitmentId& commitid) {
		this->commitid = commitid;
	}
	void setfx_0(fmpz_mod_poly_t const& poly) {
		fmpz_mod_poly_set(fx_0.fkx, poly);
	}
	fmpz_mod_poly_t const& getfx_0() {
		return fx_0.fkx;
	}
	void setVerifiableShare(fmpz_mod_poly_t const& poly) {
		fmpz_mod_poly_set(verifiableShare.fkx, poly);
	}
	void setBroadcastVerifiableShare(fmpz_mod_poly_t const& poly) {
		fmpz_mod_poly_set(broadcastVerifiableShare.fkx, poly);
		newVerifiableShareBroadcast = true;
	}
	bool isNewVerifiableShareBroadcast() const {
		return newVerifiableShareBroadcast;
	}
	fmpz_mod_poly_t const& getVerifiableShare() {
		return verifiableShare.fkx;
	}
	fmpz_mod_poly_t const& getBroadcastVerifiableShare() {
		return broadcastVerifiableShare.fkx;
	}
	void setShare(fmpz_t const& s) {
		fmpz_set(share, s);
	}
	fmpz_t const& getShare() {
		return share;
	}
	void setOpened() {
		this->opened = true;
	}
	bool isOpened() const {
		return opened;
	}
	void addDesignatedOpen(PartyId target) {
		this->designatedOpenTargets.insert(target);
	}
	bool isDesignatedOpenedTo(PartyId p) const {
		return (designatedOpenTargets.find(p) != designatedOpenTargets.end());
	}
	void setOpenedValue(fmpz_t const& ov);
	fmpz_t const& getOpenedValue() const;
	bool isValueOpenTo(PartyId p) const;
	bool isValueOpenToUs() const;
	bool inProgress() const {
		return inprogress;
	}
	void setDone(bool result) {
		this->inprogress = false;
		this->success = result;
	}
	const vector<DisputedValue>& getDisputes() const {
		return disputes;
	}
	const unordered_set<PartyId>& getAccusers() const {
		return accusers;
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
	bool isOutput() const {
		return output;
	}
	void markAsOutput() {
		this->output = true;
	}
	void clearOutputFlag() {
		this->output = false;
	}
	bool isVss() const {
		return vssFlag;
	}
	void setVss(bool vss) {
		this->vssFlag = vss;
	}
	bool isPermanent() const {
		return permanent;
	}
	void setPermanent() {
		this->permanent = true;
	}
	bool isInconsistentBroadcast() const {
		return inconsistentBroadcast;
	}
	void setInconsistentBroadcast() {
		this->inconsistentBroadcast = true;
	}
	bool isSuccess() const {
		return success;
	}
	PartyId getDistributer() const {
		return distributer;
	}
	void setDistributer(PartyId distributer) {
		this->distributer = distributer;
	}
	const string& getShareNameSuffix() const {
		return shareNameSuffix;
	}
	void setShareNameSuffix(const string& shareNameSuffix) {
		this->shareNameSuffix = shareNameSuffix;
	}
	bool isMulTriple() const {
		return mulTriple;
	}
	void setMulTriple(bool triple) {
		this->mulTriple = triple;
	}
	void addDispute(PartyId disputer, PartyId disputed);
	void setDisputeValue(PartyId disputer, PartyId disputed, fmpz_t const& val);
	void addAccuser(PartyId accuser);
	PartyId getAccuserCount() const;
	bool isAccuser(PartyId party) const;

	void print(stringstream& ss);
private:
	PartyId owner; // ID of the party who made the commitment
	PartyId recordHolder;
	fmpz_t mod;
	CommitmentId commitid;
	vector<DisputedValue> disputes;
	unordered_set<PartyId> accusers; //parties who accused the owner
	VerifiableShare verifiableShare; // Received from owner at Commit Step 1. Our verifiable share for this commitment.
	bool newVerifiableShareBroadcast;
	VerifiableShare broadcastVerifiableShare; // If a verifiable share is broadcast in Step 6 of commitment, we record it here
	fmpz_t share; // our share for this commitment
	VerifiableShare fx_0; // f(x, 0). Owner stores this if commitment is successful. Later used for opening the commitment.
	bool inconsistentBroadcast;//missing or inconsistent broadcast during ongoing commitment. All honest parties will agree on the value of this flag (without interaction).
	bool inprogress;//true if commitment is in progress
	bool success;//false if commitment failed and ended up in a forced publicCommit

	bool opened;//is set to true when a commitment is opened with 'open'
	bool designatedOpened;//is set to true when the commitment is opened to some party with 'designatedOpen'
	unordered_set<PartyId> designatedOpenTargets;//set of parties who had this commitment 'designatedOpen'ed to them
	fmpz_t openedValue;

	bool input;//true if this a commitment to a share of an input
	string inputLabel;
	bool output;//true if this a commitment to a share of the output
	bool vssFlag;//true if the commitment is being distributed (ongoing VSS)
	bool permanent;//if true, record will persist after a cleanup
	PartyId distributer;//Party who distributed the share. (Receiver of a share becomes the owner as result of commitment transfer)
	string shareNameSuffix;//'input sharing round number' for inputs, 'gate number' for others.
	bool mulTriple;//true if this record belongs to a multiplication triple (created during preprocessing of PCEAS with circuit randomization)
};

} /* namespace pceas */

#endif /* COMMITRECORD_H_ */
