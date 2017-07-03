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
 * Party.cpp
 *
 *  Created on: Feb 7, 2017
 *      Author: m3r7
 */

#include "Party.h"
#include <thread>
#include <algorithm>
#include <iostream>
#include <unordered_set>
#include "../circuit/AdditionGate.h"
#include "../circuit/ConstantMultGate.h"
#include "../circuit/MultiplicationGate.h"
#include "../math/SymmBivariatePoly.h"
#include "PceasException.h"

namespace pceas {

Party::Party(PartyId pid, ulong partyCount, ulong threshold, ulong fieldPrime):pid(pid), N(partyCount) {
	D = threshold-1;
	fmpz_init_set_ui(FIELD_PRIME, fieldPrime);
	fmpz_init_set_ui(value, 0);
	fmpz_mod_poly_init(poly, FIELD_PRIME);
	recombinationVector = _fmpz_vec_init(N);
	shares = _fmpz_vec_init(N);
	multipointVec = _fmpz_vec_init(N);
	for (ulong i = 0; i < N; ++i) {
		fmpz_t temp;
		fmpz_init_set_ui(temp, i+1);
		fmpz_mod(temp, temp, FIELD_PRIME);//multipoint evaluation of FLINT library requires that elements of this vector are reduced modulo p (though this is not really necessary in our case, since we already have the constraint that field size must be greater than number of computing parties)
		fmpz_set(multipointVec+i, temp);
		fmpz_clear(temp);
	}
	channels = new SecureChannel*[N];
	broadcast = nullptr;
	interactive = false;
	messagesReady = false;
	circuit = nullptr;
	commitments = new CommitmentTable(pid, FIELD_PRIME);
	secrets = new Secrets();
	dataUser = 0;
	done = false;
	dishonest = false;
	mu = new MathUtil(pid);
	running = PROT_NONE;
	maxDishonest = 0;
}

Party::~Party() {
	delete circuit;
	fmpz_clear(FIELD_PRIME);
	fmpz_clear(value);
	fmpz_mod_poly_clear(poly);
	_fmpz_vec_clear(recombinationVector, N);
	_fmpz_vec_clear(shares, N);
	_fmpz_vec_clear(multipointVec, N);
	for (ulong i = 0; i < N; ++i) {
		delete channels[i];
	}
	delete[] channels;
	delete commitments;
	delete secrets;
	delete mu;
}

void Party::runProtocol() {
	try {
		switch (running) {
			case PCEPS:
				runPceps();
				break;
			case PCEAS:
				runPceas(false);
				break;
			case PCEAS_WITH_CIRCUIT_RANDOMIZATION:
				runPceas(true);
				break;
			default:
				throw runtime_error("Unknown protocol.");
				break;
		}
	} catch (PceasException& e) {
		if (isCorrupt(pid)) {
			end();
		} else {
			throw e;//something unexpected happened
		}
	}
}

void Party::runProtocolSequential(string prevRunResultLabel, Circuit* nextCircuit) {
	try {
		switch (running) {
			case PCEAS:
			{
				runPceas(false, false);
				//prepare for next run (note : we will keep the set of corrupt parties from previous run)
				const string inputSharingUniqueSuffix = to_string(nextCircuit->getInputCount());//input count chosen as unique suffix so that we don't have name conflict with record names associated with inputs
				const CommitmentId committedShareToResult = circuit->retrieveOutputCid();
				CommitmentTable* tableForNextRun = new CommitmentTable(pid, FIELD_PRIME);
				for (PartyId k = 1; k <= N; ++k) {
					//locate records for the shares of result of previous run, before resetting commitment records
					CommitmentId oldName_k = getShareNameFor(k, committedShareToResult);
					CommitmentRecord* cr_k = commitments->getRecord(oldName_k);
					//add records for the shares of result of previous run, to the table we will use for next run
					CommitmentId newName_k = makeShareName(NOPARTY, k, inputSharingUniqueSuffix, true, false);
					commitments->rename(oldName_k, newName_k);
					cr_k->clearOutputFlag();//output of prev circuit
					cr_k->setInput(prevRunResultLabel);//becomes input of next circuit
					commitments->removeRecord(cr_k);
					tableForNextRun->addRecord(cr_k);
				}
				swap(commitments, tableForNextRun);
				delete tableForNextRun;//deleting table of records for first run
				delete circuit;
				circuit = nextCircuit;
				runPceas(false, true);
				break;
			}
			default:
				throw runtime_error("Unsupproted protocol for sequential run.");
				break;
		}
	} catch (PceasException& e) {
		if (isCorrupt(pid)) {
			end();
		} else {
			throw e;//something unexpected happened
		}
	}
}

/**
 * Protocol 'CEPS' (Circuit Evaluation with Passive Security)
 */
void Party::runPceps() {

	sanityChecks();
	setRecombinationVector();//calculate recombination vector

	// Step 1 of 3 :input sharing
	const unsigned long CIRCUIT_INPUT_NUM = circuit->getInputCount();
	auto const& secretsMap = secrets->getSecrets();
	auto it = secretsMap.begin();
	vector<MessagePtr> messages;
	while (messages.size() < CIRCUIT_INPUT_NUM) {
		if (it != secretsMap.end()) {//if still has secrets to share
			fmpz_set_ui(value, it->second);
			distributeShares(value, it->first);
			it++;
		}

		interact();

		for (ulong i = 0; i < N; ++i) {//receive shares sent
			//"outward clocking"
			if (channels[i]->hasMsg()) {//only data providers are expected to send messages
				messages.push_back(channels[i]->recv());
			}
		}
	}
	for (auto const& m : messages) {
		circuit->assignInput(m->getShare(), m->getInputLabel());
	}

	// Step 2 of 3 : computation
	Gate* g;
	while ((g = circuit->getNext()) != nullptr) {
		switch (g->getType()) {
		case ADD:
		case CONST_MULT:
			g->localCompute();
			fmpz_mod(g->getLocalResult(), g->getLocalResult(), FIELD_PRIME);//reduce
			g->assignResult(g->getLocalResult());
			break;
		case MULT:
			g->localCompute();
			fmpz_mod(g->getLocalResult(), g->getLocalResult(), FIELD_PRIME);//reduce
			distributeShares(g->getLocalResult());

			interact();

			_fmpz_vec_zero(shares, N);
			for (ulong i = 0; i < N; ++i) {//receive shares sent by other parties
				//"outward clocking"
				if (!channels[i]->hasMsg()) {//Even if the protocol could handle some missing shares, we stop here because our assumption(no active cheaters) is violated.
					throw PceasException("A party fails to participate.");
				}
				fmpz_set(shares+i, channels[i]->recv()->getShare());
			}
			// We produce a degree D Shamir share, via degree reduction, by recombining local shares for a degree 2D polynomial
			_fmpz_vec_dot(g->getLocalResult(), recombinationVector, shares, N);
			fmpz_mod(g->getLocalResult(), g->getLocalResult(), FIELD_PRIME);//reduce
			g->assignResult(g->getLocalResult());
			break;
		}
	}

	// Step 3 of 3 : output reconstruction

	// find output gate's output and send it privately to the data user
	MessagePtr m = newMsg();
	m->setShare(circuit->retrieveOutput());
	channels[dataUser-1]->send(m);

	interact();

	if (pid == dataUser) {// data user performs interpolation to find f(0) and prints it
		ulong receivedShareCount = 0;
		_fmpz_vec_zero(shares, N);
		for (ulong i = 0; i < N; ++i) {//receive shares sent by other parties
			if (channels[i]->hasMsg()) {//T+1 shares will be enough, others will remain as zero (effectively excluding them from the upcoming dot product).
				fmpz_set(shares+i, channels[i]->recv()->getShare());
				receivedShareCount++;
			}
		}
		if (receivedShareCount > D) {//need at least T = D+1 shares for interpolation
			_fmpz_vec_dot(value, recombinationVector, shares, N);
			fmpz_mod(value, value, FIELD_PRIME);
			cout << "Evaluation result : " << MathUtil::fmpzToStr(value) << endl;
		} else {
			cout << "Data user did not receive enough shares to recover evaluation result. "
				 << "(Protocol cannot tolerate active cheaters.)" << endl;
		}
	}

	end();
}

/**
 * Protocol 'CEAS' (Circuit Evaluation with Active Security)
 */
void Party::runPceas(bool circuitRandomization, bool finalRun) {

	sanityChecks();
//	circuit->sortGates(); //necessary if order based input-wire matching is used
	setRecombinationVector();//note : we will recalculate recombination vector each time we mark a party as corrupt

	if (circuitRandomization) {
		// Preprocessing phase for 'CEAS with Circuit Randomization' - generates multiplication triples
		runPreprocessing();
		commitments->cleanUp();//to keep commitment table size managable, we remove records which are no longer needed
	}

	{// Step 1 of 3 :input sharing
		const unsigned long CIRCUIT_INPUT_NUM = circuit->getInputCount();
		auto const& secretsMap = secrets->getSecrets();
		auto it = secretsMap.begin();
		ulong inputSharingLoopCounter = 0;//any single party will loop at most CIRCUIT_INPUT_NUM times. we use this fact to break loop and end protocol if any dishonest refuse to distribute a share.
		while (commitments->getInputShareCountReceivedBy(pid) < CIRCUIT_INPUT_NUM && inputSharingLoopCounter < CIRCUIT_INPUT_NUM) {
			const string inputSharingUniqueSuffix = to_string(inputSharingLoopCounter);
			if (it != secretsMap.end()) {//distribute shares of each secret
				fmpz_set_ui(value, it->second);
				distributeVerifiableShares(value, inputSharingUniqueSuffix, it->first, false, true);
				it++;
			} else {
				/*
				 * Note : Ideally, if a party does not have any input to distribute, it should just
				 * passively participate in distribution process of others (VSS is highly interactive).
				 * For simplifying the implementation, we make these parties distribute some arbitrary value,
				 * which will be ignored. This does not increase the number of rounds required.
				 */
				fmpz_zero(value);
				distributeVerifiableShares(value, inputSharingUniqueSuffix, NONE, false, true);
			}
			inputSharingLoopCounter++;
		}
		commitments->cleanUp();//to keep commitment table size managable, we remove records which are no longer needed
		vector<CommitmentRecord*> inputShares = commitments->getInputSharesReceivedBy(pid);
		if (inputShares.size() < CIRCUIT_INPUT_NUM) {
			throw PceasException("Missing inputs.");
		}
		if (inputShares.size() > CIRCUIT_INPUT_NUM) {
			throw PceasException("Received more inputs than expected.");
		}
//		/*
//		 * Order-based input-wire matching : (REPLACED WITH LABEL-BASED MATCHING)
//		 *  - We start assigning (shares of) secrets to wires from the lowest gate number.
//		 *    (Gates should be sorted)
//		 *  - For input gates with multiple input wires, We don't need to care about
//		 *    the order of wires, since both addition and multiplication are commutative.
//		 *  - For each secret, shares are assigned in increasing PartyId order.
//		 *  Note : Ordering of secrets distributed by same party matters. We use 'input sharing
//		 *  round number'(read from sharenamesuffix) to distinguish between them.
//		 */
//		sort(inputShares.begin(), inputShares.end(), [this](CommitRecord* is1, CommitRecord* is2){
//			return (is1->getDistributer() <= is2->getDistributer() && stoul(is1->getShareNameSuffix()) < stoul(is2->getShareNameSuffix()));
//		});
		for (auto const& is : inputShares) {
			circuit->assignInputCid(is->getCommitid(), is->getInputLabel());
#ifdef VERBOSE
			cout << "Party " << to_string(pid) << " assigns wire " + is->getInputLabel() + " : \nCID = " << is->getCommitid()
				 << "\nOpenedVal = " << MathUtil::fmpzToStr(is->getOpenedValue()) << endl;
#endif
		}
	}
	interact();
#ifdef VERBOSE
	cout << "Gate computation phase starts." << endl;
#endif
	// Step 2 of 3 : computation
	Gate* g;
	while ((g = circuit->getNext()) != nullptr) {
		switch (g->getType()) {
		case ADD:
		{
			AdditionGate* ag = static_cast<AdditionGate*>(g);
			for (PartyId k = 1; k <= N; ++k) {
				/*
				 * To keep the commitment records synchronized, we do all other parties local computations, in addition to our own.
				 */
				const CommitmentId share_k_1 = getShareNameFor(k, ag->getInputCid1());
				const CommitmentId share_k_2 = getShareNameFor(k, ag->getInputCid2());
				CommitmentId add_k = addCommitments(share_k_1, share_k_2);
				CommitmentId result_k = makeShareName(NOPARTY, k, to_string(g->getGateNumber()), false, false, true);
				commitments->rename(add_k, result_k);
				CommitmentRecord* cr_k = commitments->getRecord(result_k);
				if (cr_k == nullptr || cr_k->getOwner() != k) {//should not happen
					throw PceasException("Wire is assigned invalid commitment.");
				}
				cr_k->setPermanent();
				if (k == pid) {
					g->assignResult(cr_k->getCommitid());
#ifdef VERBOSE
					cout << "Party " << to_string(pid) << " assigns output to gate# " << g->getGateNumber() << " (addition gate) : \nCID = "
						 << cr_k->getCommitid() << "\nOpenedValue = " << MathUtil::fmpzToStr(cr_k->getOpenedValue()) << endl;
#endif
				}
			}
		}
		break;
		case CONST_MULT:
		{
			ConstantMultGate* cmg = static_cast<ConstantMultGate*>(g);
			for (PartyId k = 1; k <= N; ++k) {
				/*
				 * To keep the commitment records synchronized, we do all other parties local computations, in addition to our own.
				 */
				const CommitmentId share_k = getShareNameFor(k, cmg->getInputCid());
				CommitmentId mult_k = constMultCommitment(cmg->getConstant(), share_k);
				CommitmentId result_k = makeShareName(NOPARTY, k, to_string(g->getGateNumber()), false, false, true);
				commitments->rename(mult_k, result_k);
				CommitmentRecord* cr_k = commitments->getRecord(result_k);
				if (cr_k == nullptr || cr_k->getOwner() != k) {//should not happen
					throw PceasException("Wire is assigned invalid commitment.");
				}
				cr_k->setPermanent();
				if (k == pid) {
					g->assignResult(cr_k->getCommitid());
#ifdef VERBOSE
					cout << "Party " << to_string(pid) << " assigns output to gate# " << g->getGateNumber() << " (const. mult. gate) : \nCID = "
						 << cr_k->getCommitid() << "\nOpenedValue = " << MathUtil::fmpzToStr(cr_k->getOpenedValue()) << endl;
#endif
				}
			}
		}
		break;
		case MULT:
		{
			MultiplicationGate* mg = static_cast<MultiplicationGate*>(g);
			if (circuitRandomization) {
				//construct a common representation (common to all honest parties) for a * b, using existing multiplication triples (generated in preprocessing phase)
				auto it = triples.find(g->getGateNumber()); // x, y, x*y
				if (it == triples.end()) {
					throw PceasException("Missing triple.");
				}
				CommitmentId e_pid, d_pid;
				for (PartyId k = 1; k <= N; ++k) {//To keep the commitment records synchronized, we do all other parties local computations, in addition to our own.
					const CommitmentId input1_k = getShareNameFor(k, mg->getInputCid1());
					const CommitmentId input2_k = getShareNameFor(k, mg->getInputCid2());
					CommitmentId e = substractCommitments(input1_k, makeTripleName(k, MultiplicationTriple::M1, g->getGateNumber())); // a - x
					CommitmentId d = substractCommitments(input2_k, makeTripleName(k, MultiplicationTriple::M2, g->getGateNumber())); // b - y
					CommitmentId eNew = makeTripleName(k, MultiplicationTriple::E, g->getGateNumber());
					CommitmentId dNew = makeTripleName(k, MultiplicationTriple::D, g->getGateNumber());
					commitments->rename(e, eNew);
					commitments->rename(d, dNew);
					if (k == pid) {
						e_pid = eNew;
						d_pid = dNew;
					}
				}
				/*
				 * We open e and d, and other parties will open theirs(e' = a - x', d' = b - y')
				 * Note that, these 'open's are the only interactions we need in order to process the multiplication gate.
				 * Via circuit randomization, much of the cost due to interactions for multiplications are pushed
				 * to the preprocessing phase, in which triples are (ideally - see 'runPreprocessing') generated
				 * in parallel, rather than one at a a time.
				 */
				open(e_pid);//INTERACTIVE
				open(d_pid);//INTERACTIVE
				MultiplicationTriple& triple = it->second;
				auto& receivedShares = triple.receivedShares;
				CommitmentId result_pid;
				//eliminate shares for which the sender of share is known to be dishonest (we marked parties as corrupt in previous steps)
				receivedShares.erase(remove_if(receivedShares.begin(), receivedShares.end(), [this](CommitmentRecord* cr){return isCorrupt(cr->getDistributer());}), receivedShares.end());
				if (receivedShares.size() > 2*D) {
					sort(receivedShares.begin(), receivedShares.end(), [](CommitmentRecord* is1, CommitmentRecord* is2){return is1->getDistributer() < is2->getDistributer();});
					result_pid = runDegreeReduction(receivedShares, g->getGateNumber());
#ifdef VERBOSE
					this_thread::sleep_for(chrono::milliseconds(pid*700));
					cout << "Party " << to_string(pid) << " recombined x.y : " << MathUtil::fmpzToStr(commitments->getRecord(result_pid)->getOpenedValue()) << endl;
#endif
				} else {
					/*
					 * Since deg(h) = 2D, we needed more than 2D shares for recombination.
					 * This protocol tolerates <= N / 3 dishonest.
					 * Not having enough shares means, our assumption failed. We stop execution..
					 */
					throw PceasException("More dishonest than the protocol can handle.");
				}
				for (PartyId k = 1; k <= N; ++k) {//To keep the commitment records synchronized, we do all other parties local computations, in addition to our own.
					CommitmentRecord* ek = commitments->getRecord(makeTripleName(k, MultiplicationTriple::E, g->getGateNumber()));
					CommitmentRecord* dk = commitments->getRecord(makeTripleName(k, MultiplicationTriple::D, g->getGateNumber()));
					if (ek == nullptr || !ek->isOpened() || dk == nullptr || !dk->isOpened()) {
						addCorrupt(k);//all honest will agree
						if (k == pid) {//keep corrupt parties alive for running test cases
							g->assignResult(result_pid);
						}
						continue;
					}
					const CommitmentId input1_k = getShareNameFor(k, mg->getInputCid1());
					const CommitmentId input2_k = getShareNameFor(k, mg->getInputCid2());
					const CommitmentId result_k = makeShareName(NOPARTY, k, to_string(g->getGateNumber()), false, false, true);
					CommitmentId temp_k = makeTripleName(k, MultiplicationTriple::PROD, g->getGateNumber());
					//[[a * b]] = [[x * y]] + e[[b]] + d[[a]] - e.d
					commitments->rename(result_k, temp_k);//initialize temp_k with [[x * y]]
					temp_k = addCommitments(temp_k, constMultCommitment(ek->getOpenedValue(), input2_k)); // result_k += e[[b]]
					temp_k = addCommitments(temp_k, constMultCommitment(dk->getOpenedValue(), input1_k)); // result_k += d[[a]]
					fmpz_mul(value, ek->getOpenedValue(), dk->getOpenedValue());
#ifdef VERBOSE
					if (k == pid) {
						cout << "Party " << to_string(pid) << "a,b : " << MathUtil::fmpzToStr(commitments->getRecord(input1_k)->getOpenedValue()) << "\t" << MathUtil::fmpzToStr(commitments->getRecord(input2_k)->getOpenedValue()) << endl;
						cout << " a.b + e.d : " << MathUtil::fmpzToStr(commitments->getRecord(temp_k)->getOpenedValue()) << "  e.d : " << MathUtil::fmpzToStr(value) << endl;
					}
#endif
					fmpz_neg(value, value);
					temp_k = constAddCommitment(value, temp_k); // result_k -= e.d
					commitments->rename(temp_k, result_k);
					CommitmentRecord* cr_k = commitments->getRecord(result_k);
					if (cr_k == nullptr || cr_k->getOwner() != k) {//should not happen
						throw PceasException("Wire is assigned invalid commitment.");
					}
					cr_k->setPermanent();
					if (k == pid) {
						g->assignResult(cr_k->getCommitid());
#ifdef VERBOSE
						cout << "Party " << to_string(pid) << " assigns output to gate# " << g->getGateNumber() << " (mult. gate) : \nCID = "
							 << cr_k->getCommitid() << "\nOpenedValue = " << MathUtil::fmpzToStr(cr_k->getOpenedValue()) << endl;

						cout << "Triple used were (M1, M2, E, D) : " <<  MathUtil::fmpzToStr(triple.firstMult->getOpenedValue()) << "\t" << MathUtil::fmpzToStr(triple.secontMult->getOpenedValue()) << "\t" << MathUtil::fmpzToStr(commitments->getRecord(e_pid)->getOpenedValue()) << "\t" << MathUtil::fmpzToStr(commitments->getRecord(d_pid)->getOpenedValue()) << endl;
						cout << "Received shares were : " << endl;
						for (auto const& s : receivedShares) {
							cout << MathUtil::fmpzToStr(s->getOpenedValue()) << "\t";
						}
						cout << endl;
#endif
					}
				}
			} else {
				/*
				 * [[ab;f.g]]_2t = [[a;f]]_t * [[b;g]]_t
				 */
				CommitmentId localMult = multiplyCommitments(mg->getInputCid1(), mg->getInputCid2());
				distributeVerifiableShares(localMult, to_string(g->getGateNumber()));
				vector<CommitmentRecord*> receivedShares = commitments->getVSSharesReceivedBy(pid);
				//eliminate shares for which the sender of share is known to be dishonest (we marked parties as corrupt in previous steps)
				receivedShares.erase(remove_if(receivedShares.begin(), receivedShares.end(), [this](CommitmentRecord* cr){return isCorrupt(cr->getDistributer());}), receivedShares.end());
				if (receivedShares.size() > 2*D) {
					sort(receivedShares.begin(), receivedShares.end(), [](CommitmentRecord* is1, CommitmentRecord* is2){return is1->getDistributer() < is2->getDistributer();});
					CommitmentId result = runDegreeReduction(receivedShares, g->getGateNumber());
					g->assignResult(result);
#ifdef VERBOSE
					cout << "Party " << to_string(pid) << " assigns output to gate# " << g->getGateNumber() << " (mult. gate) : \nCID = "
						 << result << "\nOpenedValue = " << MathUtil::fmpzToStr(commitments->getRecord(result)->getOpenedValue()) << endl;
#endif
				} else {
					/*
					 * Since deg(h) = 2D, we needed more than 2D shares for recombination.
					 * This protocol tolerates <= N / 3 dishonest.
					 * Not having enough shares means, our assumption failed. We stop execution..
					 */
					throw PceasException("More dishonest than the protocol can handle.");
				}
			}
		}
		break;
		}
		commitments->cleanUp();//to keep commitment table size managable, we remove records which are no longer needed
	}
	interact();
#ifdef VERBOSE
	cout << "Gate computation phase ends." << endl;
#endif
	{
		// Step 3 of 3 : output reconstruction
		// find output gate's output and send it privately to the data user
		CommitmentId result = circuit->retrieveOutputCid();
		for (ulong i = 0; i < N; ++i) {//since parties can not designatedOpen to the same party in parallel, they will take turns
			PartyId k = i + 1;
			if (k != dataUser) {
				if (k == pid) {//our turn to 'designatedOpen' share to dataUser
					designatedOpen(result, dataUser, true);//INTERACTIVE
				} else {//We will not 'designatedOpen' anything, but will participate in other's 'designatedOpen's.
					//note that single share per party is automatically enforced due to target selection scheme used in 'designatedOpen'
					const PartyId target = getTargetFromSource(pid, k, dataUser);//(when party k is opening to dataUser, we can only open to...)
					designatedOpen(NONE, target, true);//INTERACTIVE
				}
			}
		}
		if (pid == dataUser) {
			//We mark the share we have as output. (we did not 'designatedOpen' to self.
			//Shares from other parties have been marked during the 'designatedOpen's above.)
			commitments->getRecord(result)->markAsOutput();
			vector<CommitmentRecord*> outputShares = commitments->getOutputShares();
			if (outputShares.size() > N) {
				throw PceasException("Too many output shares");
			}
			//eliminate shares for which we have no access to the value, and for which it is known that sender of share (owner of 'designatedOpen'ed commitment) is known to be dishonest
			outputShares.erase(remove_if(outputShares.begin(), outputShares.end(), [this](CommitmentRecord* cr){return !cr->isValueOpenToUs() || isCorrupt(cr->getOwner());}), outputShares.end());
			if (outputShares.size() > D) {
				//use Lagrange interpolation to find output value and print it
				_fmpz_vec_zero(shares, N);
				for (auto const& os : outputShares) {//T = D+1 shares will be enough, others will remain as zero (effectively excluding them from the upcoming dot product).
					ulong arrIndex = os->getOwner() - 1;
					fmpz_set(shares+arrIndex, os->getOpenedValue());
				}
				_fmpz_vec_dot(value, recombinationVector, shares, N);
				fmpz_mod(value, value, FIELD_PRIME);
				cout << "Evaluation result : " << MathUtil::fmpzToStr(value) << endl;
			} else {
				/*
				 * Since deg(f) = D, we needed more than D shares for recombination.
				 * This protocol tolerates <= N / 3 dishonest.
				 * Not having enough shares means, our assumption failed. We stop execution..
				 */
				cout << "Data user did not receive enough shares to recover evaluation result. "
					 << "(More dishonest than the protocol can handle)" << endl;
			}
		}
	}
	if (finalRun) {
		end();
	} else {
		interact();
	}
}

/**
 * Secret sharing.
 * Distributes shares [a;f_a]_t
 */
void Party::distributeShares(fmpz_t const& val, string label) {
	fmpz_mod_poly_t f;
	fmpz_mod_poly_init(f, FIELD_PRIME);
	mu->sampleUnivariate(f, val, D);
	if (!MathUtil::degreeCheckEQ(f, D)) {//sanity check
		throw PceasException("Bad polynomial degree.");
	}
	calculatePartyShares(f);
	for (ulong i = 0; i < N; ++i) {//send shares created by us
//		calculatePartyShare(i, f);
		//"inward clocking"
		MessagePtr m = newMsg();
		m->setShare(shares+i);
		if (label != NONE) {
			m->setInput(label);
		}
		channels[i]->send(m);
	}
	fmpz_mod_poly_clear(f);
}

/**
 * Verifiable Secret Sharing - VSS
 * Distributes committed shares [[a;f_a]]_t
 * Honest players will either :
 *  - receive consistent shares OR
 *  - agree that the dealer is dishonest
 */
void Party::distributeVerifiableShares(fmpz_t const& val, string uniqueSuffix, string label, bool preprocessingPhase, bool inputSharingPhase) {
	if ((inputSharingPhase && preprocessingPhase) || (!inputSharingPhase && label != NONE)) {
		throw PceasException("Conflicting argument values.");
	}
	CommitmentId cid = commit(val);
	distributeVerifiableShares(cid, uniqueSuffix, label, preprocessingPhase, inputSharingPhase);
}

/**
 * Verifiable Secret Sharing - VSS
 * Distributes committed shares [[a;f_a]]_t from a commitment <a> owned by this party.
 * Honest parties will either :
 *  - receive consistent shares OR
 *  - agree that the dealer is dishonest
 */
void Party::distributeVerifiableShares(CommitmentId cid, string uniqueSuffix, string label, bool preprocessingPhase, bool inputSharingPhase) {
	{//Step 1
		commitments->clearVssFlags();
		CommitmentRecord* cr = commitments->getRecord(cid);
		if (cr == nullptr || cr->getOwner() != pid) {
			throw PceasException("Bad commitid : " + cid);
		}
		fmpz_mod_poly_t f;
		fmpz_mod_poly_init(f, FIELD_PRIME);
		mu->sampleUnivariate(f, cr->getOpenedValue(), D);
		if (!MathUtil::degreeCheckEQ(f, D)) {
			throw PceasException("Bad polynomial degree.");
		}
		//we commit to all coefficients (except the x^0 coefficient)
		for (ulong i = 1; i <= D; ++i) {
			fmpz_mod_poly_get_coeff_fmpz(value, f, i);
			commit(value, getCoeffCommitIdForSharing(cid, i));
		}
		fmpz_mod_poly_clear(f);
		//Let other parties know about the VSS we intend to perform
		MessagePtr bm = newMsg();
		bm->setCommitId(cid);
		if (label != NONE) {
			bm->setInput(label);
		}
		broadcast->broadcast(bm);
	}

	interact();

	CommitmentId shares[N][N];
	string labels[N];
	{//Step 2
		vector<MessagePtr> vssMessages;
		for (ulong i = 0; i < N; ++i) {
		PartyId p = i + 1;
			if (broadcast->hasMsg(p)) {
				MessagePtr m = broadcast->recv(p);
				CommitmentRecord* cri = commitments->getRecord(m->getCommitId());
				if (cri == nullptr || cri->getOwner() != m->getSender()) {
					addCorrupt(p);
				} else {
					vssMessages.push_back(m);
				}
			} else {
				addCorrupt(p);//every honest party must provide its share.
			}
		}
		/*
		 * At this stage every coefficient is committed to, and every party can locally form commitments
		 * to <f(k)> = cid + Ʃ ( k^i . <cid_coeff_i> ) using 'add' and 'scalarMult'.
		 */
		for (auto const& vss : vssMessages) {
			for (ulong i = 0; i < N; ++i) {
				PartyId k = i+1;
				CommitmentId share = combineCoeffCommitsForSharing(vss->getCommitId(), k);
				shares[vss->getSender()-1][i] = share;
			}
			if (inputSharingPhase && vss->isInput()) {
				labels[vss->getSender()-1] = vss->getInputLabel();
			} else {
				labels[vss->getSender()-1] = NONE;
			}
		}
	}
	{//Step 3
		/*
		 * Now we distribute our shares to the corresponding parties via 'transferCommit's.
		 * Note that each party is running a 'distributeCommittedShares'.
		 * In order to be able to run the 'transferCommit's in parallel,
		 * each party must be targeted at most once per iteration. (See 'getTargetForIteration')
		 */
		for (ulong i = 1; i < N; ++i) {//we start from i = 1, so that we don't transfer to self
			PartyId k = getTargetForIteration(i);
			transferCommitment(shares[pid-1][k-1], k);
		}

		/*
		 * Both for corrupt and honest, we use vss flag to locate shares.
		 * We also use a flag (input) to distinguish shares to be used as inputs.
		 * Each party maintains a list of corrupt parties, in such a way that every
		 * honest party agrees on this list at all times.
		 */
		for (ulong p = 1; p <= N; ++p) {//p:distributer of shares
			for (ulong k = 1; k <= N; ++k) {//k:receiver of shares
				if (isCorrupt(p)) {
					/*
					 * We use shares of 0, instead of shares distrubuted by parties
					 * who turned out to be corrupt. With shares of 0,
					 * they are effectively excluded from recombination.
					 */
					CommitmentRecord* zeroRecord = zeroShareFor(k, makeShareName(p, k, uniqueSuffix, inputSharingPhase, preprocessingPhase));
					zeroRecord->setVss(true);
					zeroRecord->setPermanent();
					zeroRecord->setDistributer(p);
					zeroRecord->setShareNameSuffix(uniqueSuffix);
					zeroRecord->setMulTriple(preprocessingPhase);
					/*
					 * Note : We are making a choice here. If an input provider is identified as corrupt,
					 * we can either :
					 *  - Set zero as input and continue
					 *  OR
					 *  - Not mark the record as input, in which case execution will stop because we won't have enough
					 *  inputs.
					 * For some functions, it might make sense to assume value 0 for missing inputs and continue.
					 * Here we choose the second option.
					 */
//					zeroRecord->setInput();
				} else {
					CommitmentId shareId;
					if (k == p) {
						shareId = shares[k-1][k-1]; // not transfered to self
					} else {
						shareId = getTransferedCommitId(shares[p-1][k-1], p, k);
					}
					string inputLabel = labels[p-1];
					CommitmentRecord* crShare = commitments->getRecord(shareId);
					commitments->rename(shareId, makeShareName(p, k, uniqueSuffix, inputSharingPhase, preprocessingPhase));
					crShare->setVss(true);
					crShare->setPermanent();
					crShare->setDistributer(p);
					crShare->setShareNameSuffix(uniqueSuffix);
					if (inputLabel != NONE) {
						crShare->setInput(inputLabel);
					}
					crShare->setMulTriple(preprocessingPhase);
				}
			}
		}
	}
	interact();
}

/**
 * We produce a degree D Shamir share, via degree reduction, by recombining local shares for a degree 2D polynomial
 * Note : During recombination of commitments, both values and polynomials fx_0 get dotted.
 * Note : To keep the commitment records synchronized, we do all other parties local computations, in addition to our own.
 */
CommitmentId Party::runDegreeReduction(vector<CommitmentRecord*> const& shares, GateNumber gn) {
	CommitmentId result; // our reduced share
	for (PartyId k = 1; k <= N; ++k) {
		CommitmentId combined_k = NONE;
		for (auto const& record : shares) {
			ulong arrIndex = record->getDistributer() - 1;
			fmpz_set(value, recombinationVector+arrIndex);
			const CommitmentId share_k = getShareNameFor(k, record->getCommitid());
			CommitmentId term_k = constMultCommitment(value, share_k);
			if (combined_k == NONE) {
				combined_k = term_k;
			} else {
				combined_k = addCommitments(combined_k, term_k);
			}
		}
		CommitmentId result_k = makeShareName(NOPARTY, k, to_string(gn), false, false, true);
		commitments->rename(combined_k, result_k);
		CommitmentRecord* cr_k = commitments->getRecord(result_k);
		if (cr_k == nullptr || cr_k->getOwner() != k) {//should not happen
			throw PceasException("Invalid commitment.");
		}
		cr_k->setPermanent();
		if (k == pid) {
			result = cr_k->getCommitid();
		}
	}
	return result;
}

/**
 * Carry out summation over shares
 */
CommitmentId Party::sumShares(vector<CommitmentRecord*> const& shares, GateNumber gn, const char* multiplicandId) {
	CommitmentId result; // our reduced share
	for (PartyId k = 1; k <= N; ++k) {
		CommitmentId combined_k = NONE;
		for (auto const& record : shares) {
			const CommitmentId share_k = getShareNameFor(k, record->getCommitid());
			if (combined_k == NONE) {
				combined_k = share_k;
			} else {
				combined_k = addCommitments(combined_k, share_k);
			}
		}
		CommitmentId result_k = makeTripleName(k, multiplicandId, gn);
		commitments->rename(combined_k, result_k);
		CommitmentRecord* cr_k = commitments->getRecord(result_k);
		if (cr_k == nullptr || cr_k->getOwner() != k) {//should not happen
			throw PceasException("Invalid commitment.");
		}
		cr_k->setPermanent();
		if (k == pid) {
			result = cr_k->getCommitid();
		}
	}
	return result;
}

/**
 * Generates “sufficiently many” multiplication triples.
 * Ideally, we would want to generate our triples in a parallel fashion. Different approaches exist for triple generation.
 * Below, we process triples one at a time, which sort of nullifies any overall gain we expect to get from
 * using circuit randomization. For us, it still makes sense to do it this way, because we aim to observe the
 * reduction in number of rounds for step 2, circuit evaluation.
 *
 * For convenience of implementation, instead of making a guess for “sufficiently many”, we count multiplicaton gates
 * in the circuit. However we note that, normally preprocessing phase is independent of the circuit to be evaluated.
 */
void Party::runPreprocessing() {
	for (auto const& g : circuit->getGates()) {
		if (g->getType() == MULT) {
			MultiplicationTriple triple;
			triples.insert(pair<GateNumber, MultiplicationTriple>(g->getGateNumber(), triple));
		}
	}
	const auto TRIPLE_COUNT = triples.size();
	//we need 2 * TRIPLE_COUNT randoms as x and y values
	fmpz_zero(value);
	fmpz_mod_poly_t randoms;
	fmpz_mod_poly_init(randoms, FIELD_PRIME);
	fmpz_mod_poly_zero(randoms);
	mu->sampleUnivariate(randoms, value, 2*TRIPLE_COUNT);
	ulong coeffCounter = 1;
	for (auto& t : triples) {
		fmpz_t x,y;
		fmpz_init_set_ui(x, 0);
		fmpz_init_set_ui(y, 0);

		/*
		 * Note :
		 *  - ('maxDishonest' + 1) individual randoms would be enough for the linear combination(in this case, sum)
		 *  to be random. Not every party would have to share a value.
		 */
		fmpz_mod_poly_get_coeff_fmpz(x, randoms, coeffCounter++);
		fmpz_mod_poly_get_coeff_fmpz(y, randoms, coeffCounter++);
		//Each party D-shares its random (for each multiplicand)
		distributeVerifiableShares(x, to_string(t.first)+MultiplicationTriple::M1, NONE, true); // INTERACTIVE.
		vector<CommitmentRecord*> receivedSharesM1 = commitments->getVSSharesReceivedBy(pid);
		distributeVerifiableShares(y, to_string(t.first)+MultiplicationTriple::M2, NONE, true); // INTERACTIVE.
		vector<CommitmentRecord*> receivedSharesM2 = commitments->getVSSharesReceivedBy(pid);
		/*
		 * Sum over shares of individual randoms to obtain a share for a single random.
		 * Notes:
		 *  - As long as one party is honest, sum will be random. No party will know this random value.
		 *  - We did not have to do a summation, any linear combination would do.
		 */
		CommitmentId idMult1 = sumShares(receivedSharesM1, t.first, MultiplicationTriple::M1);
		CommitmentId idMult2 = sumShares(receivedSharesM2, t.first, MultiplicationTriple::M2);

		CommitmentId product = multiplyCommitments(idMult1, idMult2); // INTERACTIVE.
		distributeVerifiableShares(product, to_string(t.first), NONE, true); // INTERACTIVE.
		t.second.receivedShares = commitments->getVSSharesReceivedBy(pid);
		for (PartyId k = 1; k <= N; ++k) {//set triples from all parties as permanent
			CommitmentId idMult1_k = makeTripleName(k, MultiplicationTriple::M1, t.first);
			CommitmentId idMult2_k = makeTripleName(k, MultiplicationTriple::M2, t.first);
			CommitmentId product_k = getMultipliedCommitId(idMult1_k, idMult2_k);
			CommitmentRecord* crFirst_k = commitments->getRecord(idMult1_k);
			CommitmentRecord* crSecond_k = commitments->getRecord(idMult2_k);
			CommitmentRecord* crProd_k = commitments->getRecord(product_k);
			if (!crFirst_k || !crSecond_k || !crProd_k || crFirst_k->getOwner() != k || crSecond_k->getOwner() != k || crProd_k->getOwner() != k) {
				addCorrupt(k);
				continue;
			}
			crFirst_k->setMulTriple(true);
			crSecond_k->setMulTriple(true);
			crProd_k->setMulTriple(true);
			crFirst_k->setPermanent();
			crSecond_k->setPermanent();
			crProd_k->setPermanent();
			if (k == pid) {
				t.second.firstMult = crFirst_k;
				t.second.secontMult = crSecond_k;
				t.second.product = crProd_k;
#ifdef VERBOSE
				this_thread::sleep_for(chrono::milliseconds(pid*700));
				cout << "Preprocessing for gate " << to_string(t.first) << " Party : " << to_string(pid) << ", Triple (M1, M2, PROD) : " <<  MathUtil::fmpzToStr(crFirst_k->getOpenedValue()) << "\t" << MathUtil::fmpzToStr(crSecond_k->getOpenedValue()) << "\t" << MathUtil::fmpzToStr(crProd_k->getOpenedValue()) << endl;
#endif
			}
		}
		fmpz_clear(x);
		fmpz_clear(y);
	}
	fmpz_mod_poly_clear(randoms);
}

/**
 * Returns a commitment record corresponding to a verifiable share of 0 by Party p, represented as [[0,o]], where o is the 0 polynomial.
 * Creates the record if it does not already exist.
 */
CommitmentRecord* Party::zeroShareFor(PartyId id, CommitmentId cid) {
	commitments->addRecord(id, cid);
	CommitmentRecord* zeroRecord = commitments->getRecord(cid);
	publicCommitToZero(zeroRecord);
	return zeroRecord;
}

/**
 * Party k commits to value 'val'
 */
CommitmentId Party::commit(fmpz_t const& val, CommitmentId predeterminedCommitId) {
	CommitmentId commitid;
	SymmBivariatePoly f(FIELD_PRIME, D, pid);
	{//Step 1
		//Prepare and privately send verifiable shares to other parties (for our own commitment)
		if (predeterminedCommitId == NONE) {
			commitid = commitments->addRecord(pid);
		} else {//create with supplied commitid. This might happen, for example, during transferCommit, when creating commitments for coefficients of the sampled polynomial
			commitid = commitments->addRecord(pid, predeterminedCommitId);
		}
		CommitmentRecord* commitRecord = commitments->getRecord(commitid);
		f.sampleBivariate(val, mu->getRandState());
		for (ulong i = 0; i < N; ++i) {
			fmpz_mod_poly_t fk_x;
			fmpz_mod_poly_init(fk_x, FIELD_PRIME);
			MessagePtr m = newMsg();
			m->setDebugInfo("Commit step 1");
			f.evaluate(fk_x, i+1);
			m->setCommitId(commitid);
			m->setVerifiableShare(fk_x);//send f(x,j) to Party j
			channels[i]->send(m);
			fmpz_mod_poly_clear(fk_x);
		}
		f.evaluateAtZero(poly);
		commitRecord->setfx_0(poly);//Will be used to open a commit (assuming the commit succeeds)
		commitRecord->setOpenedValue(calculateZeroShare(commitRecord->getfx_0()));//we also save f(0,0) to avoid calculating it from fx_0 every time
	}
	interact();
	{//Step 2 - calculate points on received verifiable shares/polynomials and privately exchange (for all commitments)
		MessagePtr messages[N];
		for (ulong i = 0; i < N; ++i) {
			messages[i] = newMsg();
			messages[i]->setDebugInfo("Commit step 2");
		}
		for (ulong i = 0; i < N; ++i) {
			if (channels[i]->hasMsg()) {
				MessagePtr m = channels[i]->recv();
				CommitmentId cid = m->getCommitId(); //each message declares a single commitment of a different party
				if (m->getSender() != pid) { //create commit record for commitments of other parties
					const bool disallowedID = (cid.compare(0, SHARE_PREFIX.size(), SHARE_PREFIX) == 0);
					if (disallowedID) {
						cid = commitments->addRecord(m->getSender());
					} else {
						cid = commitments->addRecord(m->getSender(), cid);
					}
				}
				commitments->getRecord(cid)->setVerifiableShare(m->getVerifiableShare());
				calculatePartyShares(m->getVerifiableShare());
				for (ulong j = 0; j < N; ++j) {//distribute shares for single commit to N messages for N parties
					fmpz_set(value, shares+j);
#ifdef COMMITMENT_SEND_INVALID_SHARE
					if (dishonest) {
						//send defective share to Party 1
						if (j == 0) {
							fmpz_add_ui(value, value, 1);// send value+1 instead of value
						}
					}//TEST CASE OK (Party 1 disputes all commitments. At Step 4 owners broadcasts disputed values. Broadcast values are accepted. No party gets accused. (Assuming only dishonest behaviour is that described by the test case.))
#endif
					messages[j]->addVerifier(cid, value);
				}
			}
		}
		for (ulong i = 0; i < N; ++i) {
			channels[i]->send(messages[i]);
		}
	}
	vector<CommitmentId> cids = commitments->getOngoingCommits();//If some message lacks information about some ongoing commitment, we want to know and take action. We also would like to
																 //ignore information in the message about commitments which have not been declared above..

	interact();
	{//Step 3 - Using the symmetry of f, we will detect inconsistencies (for all commitments)
		MessagePtr bm = newMsg(); //with this broadcast message we will tell every other party the inconsistencies detected
		bm->setDebugInfo("Commit step 3");
		for (ulong i = 0; i < N; ++i) {
			if (channels[i]->hasMsg()) {
				MessagePtr m = channels[i]->recv();
				if (m->getSender() != pid) { //ignore message from myself
					for (auto const& cid : cids) {
						CommitmentRecord* tempCr = commitments->getRecord(cid);
						//check degree for the polynomial sent by the owner of commitment in Step 1
						if (MathUtil::degreeCheckLTE(tempCr->getVerifiableShare(), D)) {//(Example for degree < T : f(x,y)=xy+x+y+2 (mod 7) f_6(x)=7x+8=1 (mod7)
							bool found = m->getVerifier(cid, value);//temporarily store sent value in 'value'
							if (found) {
								calculatePartyShare(i, tempCr->getVerifiableShare());// i = m->getSender()-1 (-1 due to conversion from party index to array index)
								/*
								 * Compare value sent by the party : f(m,n)
								 * with the value we calculated from our verifiable share : f(n,m)
								 */
								if (fmpz_equal(value, shares+i) == 0) {//NOT EQUAL
									bm->addDispute(cid, m->getSender());
								}
							} else {//party failed to send a commit verifier we are expecting. We can't check. We dispute to have the owner open.
								bm->addDispute(cid, m->getSender());
							}//if message contains an identifier we don't expect, we ignore it
						} else {
							//We cannot use this polynomial for any consistency checks, so we will dispute every party. Owner will be forced to broadcast this verfiable share, and we will use that one, if it is OK.
							//(here we only dispute one, others will be disputed in future iterations)
							bm->addDispute(cid, m->getSender());
						}
					}
				}
			}
		}
		broadcast->broadcast(bm);
	}
	interact();
	{//Step 4
		//First, we update our commitment records with all the broadcast disputes (for all commitments)
		MessagePtr bm = newMsg();
		bm->setDebugInfo("Commit step 4");
		for (ulong i = 0; i < N; ++i) {//(we could update records with own disputes in Step 3. Instead we don't ignore the self-message and do it here.)
			if (broadcast->hasMsg(i+1)) {
				MessagePtr m = broadcast->recv(i+1);
				for (auto const& cid : cids) {
					unordered_set<PartyId> s;
					m->getDisputes(cid, s);
					for (auto const& disputed : s) {//update commitment records with disputes
						commitments->getRecord(cid)->addDispute(m->getSender(), disputed);
					}
				}
			}
		}
		//Then, for our own commitment, we broadcast all disputed values
		CommitmentRecord* cr = commitments->getRecord(commitid);
		auto const& disputes = cr->getDisputes();
		for (auto const&  d : disputes) {
			/*
			 * Note: If party m disputes party n, and n disputes m,
			 * there is no need to evaluate and send both f(m,n) and f(n,m).
			 * We choose to send both, and later we will expect to receive
			 * values for both.
			 */
			f.evaluate(value, d.disputer, d.disputed);
			bm->setCommitId(commitid);
#ifdef COMMITMENT_DO_NOT_OPEN_DISPUTED
			if (!dishonest)//TEST CASE OK (If disputed owner refuses to open, her commitment fails and she is marked as corrupt)
#endif
			bm->addDisputedValue(d.disputer, d.disputed, value);
		}
		broadcast->broadcast(bm);
	}
	interact();
	{//Step 5
		//We check that all disputed values are broadcast and all broadcast values are consistent with
		//those received privately in Step 1 (for all commitments, except our own)
		//(Note : We don't need to consider the values exchanged with the committer in Step 2 separately,
		//because we already disputed it if it was inconsistent with what we received in Step 1.)
		MessagePtr bm = newMsg();
		bm->setDebugInfo("Commit step 5");
		for (auto const& cid : cids) {
			CommitmentRecord* cr = commitments->getRecord(cid);
			auto const& disputes = cr->getDisputes();
			if (cr->getOwner() != pid) {
				for (auto const& d : disputes) {
					if (!broadcast->hasMsg(cr->getOwner())) {//there was dispute over commitment, but the owner did not open
						bm->addAccused(cr->getOwner(), "Did not open (No message)");
						break;//we don't need to check any more disputes for this cid. we continue with next cid.
					}
					MessagePtr m = broadcast->recv(cr->getOwner());
					bool disputeOpened = m->getDisputedValue(d.disputer, d.disputed, value);//temporarily store in 'value'
					if (!disputeOpened) {
						bm->addAccused(cr->getOwner(), "Did not open. "+to_string(d.disputer)+" - "+to_string(d.disputed));
						break;//we don't need to check any more disputes for this cid. we continue with next cid.
					}
					bool ok = checkConsistency(d.disputer, d.disputed, value, pid, cr->getVerifiableShare());
					if (!ok) {
						bm->addAccused(cr->getOwner(), "Opened inconsistent value.");
						break;//we don't need to check any more disputes for this cid. we continue with next cid.
					}
#ifdef COMMITMENT_ACCUSE_HONEST_AFTER_DISPUTES_OPENED
					if (dishonest) {//accuse Party 1
						if (cr->getOwner() == 1) {
							bm->addAccused(cr->getOwner(), "Because I am a dirty cheater.");
						}
					}// TEST CASE OK  (Accused broadcasts verifiable share. Eventually, commitment of (wrongfully) accused Party 1 succeeds. Furthermore, accusing party updates its record with the newly broadcast verifiabe share.)
#endif
				}
			}
			for (auto const& d : disputes) {//Record broadcast disputed values. They will be used for further consistency checks in in Step 7 (if any broadcasts occur in Step 6)
				if (broadcast->hasMsg(cr->getOwner())) {
					MessagePtr m = broadcast->recv(cr->getOwner());
					if (m->getDisputedValue(d.disputer, d.disputed, value)) {//temporarily store in 'value')
						cr->setDisputeValue(d.disputer, d.disputed, value);
					}
				}
			}
		}
#ifdef COMMITMENT_DISHONEST_ACCUSED
		if (dishonest) {//Accuse self to create precondition for a test case
			bm->addAccused(pid, "");
		}
#endif
		broadcast->broadcast(bm);
	}
	interact();
	{//Step 6
		//We update our commitment records with accusations (for all commitments)
		MessagePtr bm = newMsg();
		bm->setDebugInfo("Commit step 6");
		for (ulong i = 0; i < N; ++i) {
			if (broadcast->hasMsg(i+1)) {
				MessagePtr m = broadcast->recv(i+1);
				auto const& accusedParties = m->getAccusations();
				for (auto const& ac : accusedParties) {
					CommitmentRecord* cr = commitments->getRecordForOngoingCommitment(ac.accused);
					cr->addAccuser(m->getSender());
				}
			}
		}
		//We broadcast the verifiable shares (previously privately sent to parties in Step 1) of every party that accused us. (for our own commitment)
		CommitmentRecord* cr = commitments->getRecord(commitid);
		auto const& accusers = cr->getAccusers();
		for (auto const& k : accusers) {
			fmpz_mod_poly_t fk_x;
			fmpz_mod_poly_init(fk_x, FIELD_PRIME);
			f.evaluate(fk_x, k);
#ifdef COMMITMENT_ACCUSED_DO_NOT_OPEN_VERIFIABLE_SHARE
			if(!dishonest)// TEST CASE OK  (Commitment fails and the owner is identified as corrupt)
#endif
			bm->addOpenedVerifiableShare(k, fk_x);
			fmpz_mod_poly_clear(fk_x);
		}
		broadcast->broadcast(bm);
	}
	interact();
	{//Step 7
		//We check that, in Step 6, verifiable shares of all accusing parties were broadcast
		//and broadcast values are consistent with values broadcast previously in Step 4,
		//and also with the verifiable share we received in Step 1. (for all commitments, except our own)
		MessagePtr bm = newMsg();
		bm->setDebugInfo("Commit step 7");
		for (ulong i = 0; i < N; ++i) {
			PartyId k = i+1;
			if (k != pid) {
				CommitmentRecord* cr = commitments->getRecordForOngoingCommitment(k);
				if (cr == nullptr) {
					throw PceasException("Missing record for ongoing commitment.");
				}
				if (cr->getAccusers().empty()) {
					continue; // no accusers, continue with next commitment
				}
				if (!broadcast->hasMsg(k)) {// there were accusers for commitment, but owner did not opened verifiable share(s)
					bm->addAccused(k);
					continue;//already accused. continue with next commitment.
				}
				unordered_set<PartyId> temp;
				MessagePtr m = broadcast->recv(k);
				auto const& openedShares = m->getOpenedVerifiableShares();
				//first we run over the shares to do check for completeness and updating records
				for (auto const& ovs : openedShares) {
					temp.insert(ovs->k);
					if (ovs->k == pid) {
						cr->setBroadcastVerifiableShare(ovs->fkx);
					}
				}
				if (temp != cr->getAccusers()) {//compare message contents with the set of accusers from our commitment record
					bm->addAccused(k); //sender failed to open shares for every accuser and/or opened some that were not required
					cr->setInconsistentBroadcast();//missing broadcast, all honest parties will agree
					continue;//already accused. continue with next commitment.
				}
				//now we check for consistencies
				for (auto const& ovs : openedShares) {
					if (!MathUtil::degreeCheckLTE(ovs->fkx, D)) {
						bm->addAccused(k);
						break;//already accused. continue with next commitment.
					}
					if (ovs->k == pid) { // broadcast share is our verifiable share for this commitment, which we received privately in Step 1. check for consistency.
						if (fmpz_mod_poly_equal(ovs->fkx, cr->getVerifiableShare()) == 0) {//NOT EQUAL
							bm->addAccused(k);
							break;//already accused. continue with next commitment.
						}
					}
					//check consistency of points broadcast in Step 4 with polynomials broadcast in Step 6
					auto const& disputes = cr->getDisputes();
					for (auto const& d : disputes) {
						if (d.opened) {//for each broadcast point
							bool ok = checkConsistency(d.disputer, d.disputed, d.val, ovs->k, ovs->fkx);
							if (!ok) {
								bm->addAccused(k);
								cr->setInconsistentBroadcast();//inconsistent broadcast, all honest parties will agree
								break;//already accused. continue with next commitment.
							}
						}
					}
				}
			}
		}
		broadcast->broadcast(bm);
	}
	interact();
	{//Step 8
		//We update our commitment records with accusations from Step 7 (for all commitments)
		for (ulong i = 0; i < N; ++i) {
			if (broadcast->hasMsg(i+1)) {
				MessagePtr m = broadcast->recv(i+1);
				for (auto const& ac : m->getAccusations()) {
					CommitmentRecord* cr = commitments->getRecordForOngoingCommitment(ac.accused);
					cr->addAccuser(m->getSender());
				}
			}
		}
		//If commitment did not fail, we update commitment records with shares
		for (auto const& cid : cids) {//for each ongoing commitment
			CommitmentRecord* cr = commitments->getRecord(cid);
			/*
			 * If owner broadcast inconsistent information OR
			 * If more than D parties have accused the commitment owner (meaning, at least one honest party accused),
			 * we know that commitment owner is corrupt and intended commitment has failed. (All honest will agree on these)
			 *
			 * If commitment was for an input (corrupt party providing input in input sharing phase), we will assume [[0,o]] as default input.
			 * If a commitment failed during the execution of mult gate, we again continue with assumption of [[0,o]]. (In the end, corrupt parties will be excluded from recombination)
			 * In any case, we force owner to a public commitment to 0, and continue.
			 */
			if (cr->isInconsistentBroadcast() || cr->getAccuserCount() > D) {
				cr->setDone(false);
				addCorrupt(cr->getOwner());
				publicCommitToZero(cr);//force a commitment to 0
			} else {
				cr->setDone(true);
				//If commitment did not fail, we update commitment records with shares
				if (cr->isAccuser(pid) && cr->isNewVerifiableShareBroadcast()) {//we accused and had new polynomial broadcast
					fmpz_mod_poly_set(poly, cr->getBroadcastVerifiableShare());
				} else {
					fmpz_mod_poly_set(poly, cr->getVerifiableShare());
				}
				cr->setShare(calculateZeroShare(poly));
			}
		}
	}
	interact();
	return commitid;
}

/**
 * A forced commitment. Cannot fail(Performed by honest parties on behalf of corrupt party)
 */
void Party::publicCommit(CommitmentRecord* cr, fmpz_t const& val) {
	cr->setShare(val);
	cr->setOpened();
	cr->setOpenedValue(val);
	MathUtil::zeroUnivariate(poly, val);
	cr->setVerifiableShare(poly);//f(k,x)
	cr->setfx_0(poly);//f(0,x)
	cr->setDone(false);//done, but not successfully
}

/**
 * A forced commitment. Cannot fail(Performed by honest parties on behalf of corrupt party)
 */
void Party::publicCommitToZero(CommitmentRecord* cr) {
	fmpz_t zero;
	fmpz_init_set_ui(zero, 0);
	publicCommit(cr, zero);
	fmpz_clear(zero);
}

/**
 * Commitment with ID 'commitid' is opened.'
 * Note : Another party may be simultaneously trying to open own commitment.
 * Note : If we come here from a parallel run of 'designatedOpen's,
 * only the parties whose 'designatedOpen's got rejected, will try to open,
 * but all will take part in 'open's of other parties.
 */
void Party::open(CommitmentId commitid) {
	vector<CommitmentId> opens;//holds commit IDs of commitments being opened
	{//Step 1
		MessagePtr bm = newMsg();
		bm->setDebugInfo("Open step 1 : " + commitid);
		bm->setCommitId(commitid);
		if (commitid != NONE) {
			CommitmentRecord* cr = commitments->getRecord(commitid);
			if (cr != nullptr && cr->getOwner() == pid) {
#ifdef OPEN_WITH_INVALID_FX0
				if (dishonest) {
					//A party tries to open its commitment differently.
					//TEST CASE OK : 'Open' does not succeed ('Open's of other parties succeed). Party is marked as corrupt.
					fmpz_mod_poly_neg(poly, cr->getfx_0());
					bm->setVerifiableShare(poly); // open with the negative instead
				} else
#endif
				bm->setVerifiableShare(cr->getfx_0());
			}
		}
		broadcast->broadcast(bm);
	}
	interact();
	{//Step 2
		MessagePtr bm = newMsg();
		bm->setDebugInfo("Open step 2 : " + commitid);
		for (ulong i = 0; i < N; ++i) {
			if (broadcast->hasMsg(i+1)) {
				MessagePtr m = broadcast->recv(i+1);
				CommitmentRecord* cr = commitments->getRecord(m->getCommitId());
				if (cr != nullptr && cr->getOwner() == m->getSender()) {
					opens.push_back(m->getCommitId());
					cr->setfx_0(m->getVerifiableShare());//update commitment records with received information
#ifdef OPEN_SEND_INVALID_VERIFIERS
					if (dishonest) { //A party tries to sabotage 'open's of other parties by sending invalid verifiers
						//TEST CASE OK  'open's of honest parties succeed
						fmpz_add_ui(value, cr->getShare(), 1); // send verifier + 1 instead of verifier
						bm->addVerifier(m->getCommitId(), value);
					} else
#endif
					bm->addVerifier(m->getCommitId(), cr->getShare());
				}
			}
		}
		broadcast->broadcast(bm);
	}
	interact();
	{//Step 3
		for (CommitmentId cid : opens) {
			CommitmentRecord* cr = commitments->getRecord(cid);
			if (MathUtil::degreeCheckLTE(cr->getfx_0(), D)) {//first check the polynomial received in Step 2
				//then check that we received sufficiently many valid shares
				ulong counter = 0;//number of valid shares
				for (ulong i = 0; i < N; ++i) {
					bool found = broadcast->hasMsg(i+1) && broadcast->recv(i+1)->getVerifier(cid, value);//temporarily store sent value in 'value'
					if (found) {
						calculatePartyShare(i, cr->getfx_0());
						if (fmpz_equal(value, shares+i) != 0) {//EQUAL
							counter++;
						}
					}
				}
				if (counter > 2*D) {//as the value of counter is solely determined by information from consenses broadcast, all honest parties will agree on it.
					cr->setOpened();
					cr->setOpenedValue(calculateZeroShare(cr->getfx_0()));//f(0,0)
				} else {
					addCorrupt(cr->getOwner());
				}
			} else {
				addCorrupt(cr->getOwner());
			}
		}
	}
	interact();
}

/**
 * Commitment with ID 'commitid' is opened to party 'k'
 * Note : Another party can simultaneously do designatedOpen(commitid', k'),
 * as long as k != k', and k and k' are properly chosen (see getSourceFromTarget).
 */
void Party::designatedOpen(CommitmentId commitid, PartyId k, bool isOutputOpening) {
	{//Step 1 - Similar to Step 1 of 'open'. We don't broadcast but send privately to 'k'
		if (commitid != NONE) {
			MessagePtr mk = newMsg();
			mk->setDebugInfo("DesignatedOpen step 1 - Private Msg for : " + commitid);
			CommitmentRecord* cr = commitments->getRecord(commitid);
			if (cr != nullptr && cr->getOwner() == pid) {
				mk->setCommitId(commitid);
#ifdef DESIGNATEDOPEN_WITH_INVALID_FX0
				if (dishonest) {
					//A party tries to open its commitment differently.
					//TEST CASE OK : 'designatedOpen' of dishonest party is rejected. dishonest is forced to do an 'open'.
					fmpz_mod_poly_neg(poly, cr->getfx_0());
					mk->setVerifiableShare(poly); // open with the negative instead
				} else
#endif
				mk->setVerifiableShare(cr->getfx_0());
			}
			channels[k-1]->send(mk);
		}
		//Let others know about the opens, so they can participate
		MessagePtr bm = newMsg();
		bm->setCommitId(commitid);
		bm->setTarget(k);
		bm->setDebugInfo("DesignatedOpen step 1 : " + commitid + (isOutputOpening ? " (Opening Output)" : (" Opening to Party " + to_string(k))));
		broadcast->broadcast(bm);
	}
	interact();
	vector< pair<CommitmentId, PartyId> > designatedOpens;
	const PartyId expectedOpenerToUs = getSourceFromTarget(pid, pid, k);
	{//Step 2 - Similar to Step 2 of 'open'. Verifiers are not broadcast, but sent privately to a single party (for each open)
		// Learn about ongoing opens
		for (ulong i = 0; i < N; ++i) {
			if (broadcast->hasMsg(i+1)) {
				MessagePtr m = broadcast->recv(i+1);
				CommitmentRecord* cr = commitments->getRecord(m->getCommitId());
				if (cr != nullptr && cr->getOwner() == m->getSender()) {
					designatedOpens.push_back(make_pair(m->getCommitId(), m->getTarget()));
					if (isOutputOpening) {//All paralel 'designatedOpen's are output openinig, or none
						cr->markAsOutput();
					}
				}
			}
		}
		// Process incoming private message (for the commitment being designatedOpened to us)
		if (channels[expectedOpenerToUs-1]->hasMsg()) {
			MessagePtr m = channels[expectedOpenerToUs-1]->recv();
			CommitmentRecord* cr = commitments->getRecord(m->getCommitId());
			if (cr != nullptr && cr->getOwner() == m->getSender()) {
				cr->setfx_0(m->getVerifiableShare());//update commitment records with received information
			}
		} // (if no message was sent, fx remains as zero polynomial, which is OK.)
		// Privately send verifiers for each open
		for (auto const& pa : designatedOpens) {
			MessagePtr mk = newMsg();
			mk->setDebugInfo("designatedOpen step 2");
			CommitmentRecord* cr = commitments->getRecord(pa.first);
#ifdef DESIGNATEDOPEN_SEND_INVALID_VERIFIERS
			if (dishonest) { //corrupt party tries to sabotage 'designatedOpen's of other parties by sending invalid verifiers
				//TEST CASE OK  'designatedOpen's of other parties succeed
				fmpz_add_ui(value, cr->getShare(), 1); // send verifier + 1 instead of verifier
				mk->addVerifier(pa.first, value);
			} else
#endif
			mk->addVerifier(pa.first, cr->getShare());
			channels[pa.second-1]->send(mk);
		}
	}
	interact();
	{//Step 3 - For the commitment being opened to us, we validate received fx_0 with verifiers. If invalid we broadcast reject to force a normal open.
		//Find the commitment being opened to us
		for (auto const& pa : designatedOpens) {
			if (pa.second == pid) {
				CommitmentRecord* cr = commitments->getRecord(pa.first);
				MessagePtr bm = newMsg();
				bm->setDebugInfo("designatedOpen step 3");
				bm->setCommitId(cr->getCommitid());
				bm->setTarget(pid);
				if (MathUtil::degreeCheckLTE(cr->getfx_0(), D)) {//first check the polynomial received in Step 2
					//then check that we received sufficiently many shares
					ulong counter = 0;//number of valid shares
					for (ulong i = 0; i < N; ++i) {
						bool found = channels[i]->hasMsg() && channels[i]->recv()->getVerifier(cr->getCommitid(), value);//temporarily store sent value in 'value'
						if (found) {
							calculatePartyShare(i, cr->getfx_0());
							if (fmpz_equal(value, shares+i) != 0) {//EQUAL
								counter++;
							}
						}
					}
					if (counter > 2*D) {
						cr->addDesignatedOpen(pid);
						cr->setOpenedValue(calculateZeroShare(cr->getfx_0()));//f(0,0)
					} else {
						bm->setDesignatedOpenRejected();
					}
				} else {
					bm->setDesignatedOpenRejected();
				}
				broadcast->broadcast(bm);
			}
		}
	}
	interact();
	{//STEP 4 - Open our commitment if designated open was rejected, or participate in other opens
		bool oursRejected = false;
		unordered_set<CommitmentId> rejected;
		for (ulong i = 0; i < N; ++i) {
			if (broadcast->hasMsg(i+1)) {
				MessagePtr m = broadcast->recv(i+1);
				CommitmentRecord* cr = commitments->getRecord(m->getCommitId());
				if (cr != nullptr && cr->getOwner() == getSourceFromTarget(m->getSender(), pid, k)) {//only the target can reject
					if (m->isDesignatedOpenRejected()) {
						if (cr->getOwner() == pid) {// if our designated open got rejected, we should 'open'
							oursRejected = true;
						}
						rejected.insert(cr->getCommitid());
					} else {
						cr->addDesignatedOpen(m->getSender());
					}
				} else {
					addCorrupt(m->getSender());//all honest will agree
				}
			}
		}
		if (!rejected.empty()) {
			if (oursRejected) {
#ifdef DESIGNATEDOPEN_DO_NOT_OPEN_REJECTED
				if (dishonest) { //A party whose 'designatedOpen' got rejected, refuses to do an 'open'
					open();//TEST CASE OK : Commitment remains unopened. As a result, party is marked as corrupt.
				} else
#endif
				open(commitid);
			} else {
				open();//we won't open anything, but participate in 'open's of other partie(s)
			}
		}
		for (auto const& cid : rejected) {
			CommitmentRecord* cr = commitments->getRecord(cid);
			if (!cr->isOpened()) {//'designatedOpen' rejected, but not 'open'ed.
				addCorrupt(cr->getOwner());
			}
		}
	}
	interact();
}

/**
 * Protocol 'Perfect Transfer'
 *
 * Commitment with ID 'commitid' is transfered to party 'k'
 * Note : Another party may be simultaneously trying transferCommit(commitid', k')
 */
void Party::transferCommitment(CommitmentId commitid, PartyId k) {
	{//Step 0 Let every party know which commitments are being transfered
		MessagePtr bm = newMsg();
		bm->setDebugInfo("transfer commitment step 0 : " + commitid + " to Party " + to_string(k));
		bm->addTransfer(commitid, pid, k);
		broadcast->broadcast(bm);
	}
	interact();
	vector<CommitmentTransfer> vecTrans;
	{//Step 1
		//Update records with broadcast information
		for (ulong i = 0; i < N; ++i) {
			PartyId sender = i + 1;
			if (broadcast->hasMsg(sender)) {
				auto const& receivedTransfers = broadcast->recv(sender)->getTransfers();
				if (receivedTransfers.size() == 1) {//we expect exactly 1 transfer per party
					CommitmentTransfer ct = receivedTransfers.back();
					const bool sourceTargetMatch = (ct.transferSource == getSourceFromTarget(ct.transferTarget, pid, k));
					CommitmentRecord* cr = commitments->getRecord(ct.commitId);
					const bool recordOk = ((cr != nullptr) && (cr->isSuccess()) && (cr->getOwner() == ct.transferSource) && (cr->getOwner() == sender));
					if (sourceTargetMatch && recordOk) {//all honest will agree
						CommitmentTransfer newCt(ct.commitId, ct.transferSource, ct.transferTarget);//ignore anything else the sent message contains
						vecTrans.push_back(newCt);
					} else{
						addCorrupt(sender);
					}
				} else {
					addCorrupt(sender);
				}
			}
		}
		//designatedOpen to transfer target
		designatedOpen(commitid, k);//INTERACTIVE
	}
	{//Step 2 - Mark transfers with failed opens to be handled in Step 5. Commit to value opened to us.
		CommitmentTransfer transToUs;
		for (auto& t : vecTrans) {
			if (!t.error) {
				CommitmentRecord* cr = commitments->getRecord(t.commitId);
				t.error = !cr->isValueOpenTo(t.transferTarget);
				if (!t.error && (pid == t.transferTarget)) {
					transToUs = t;
					fmpz_set(value, cr->getOpenedValue());//store until commit
				}
			}
		}
		if (!transToUs.error) {
#ifdef TRANSFER_TARGET_COMMITS_TO_DIFFERENT_VALUE
			if (dishonest && transToUs.transferSource == 3) { //dishonest transfer target will commit to a value different than what was transfered to her (by Party 3)
				//TEST CASE OK : Transfer to dishonest party is rejected. Transfered share is set with a public commitment
				//to value opened by honest transfer source (hence consistency of shares is guaranteed for the ongoing VSS).
				fmpz_add_ui(value, value, 1); // commit to value + 1 instead of value
			}
#endif
			commit(value, getTransferedCommitId(transToUs));//INTERACTIVE
		} else {//Participate in other's commitments (We make a dummy commitment to 0)
			fmpz_zero(value);
			commit(value);//INTERACTIVE
		}
	}
	{//Step 3 - We will enable other parties to check that the value we committed to is the same value original owner had committed to. Other 'transferTarget's will do the same.
		//Mark transfers with failed commitments to be handled in Step 5.
		CommitmentTransfer transFromUs;
		for (auto& t : vecTrans) {
			if (!t.error) {
				CommitmentRecord* cr = commitments->getRecord(getTransferedCommitId(t));
				if (cr != nullptr && (cr->getOwner() == t.transferTarget) && cr->isSuccess()) {
					t.transferedCommitId = cr->getCommitid();
				} else {
					t.error = true;
				}
			}
			if (pid == t.transferSource) {
				transFromUs = t;
			}
		}
		fmpz_mod_poly_t f;
		fmpz_mod_poly_init(f, FIELD_PRIME);
		if (!transFromUs.error) {
			//sample a polynomial with x^0 coefficient set to value (of the commitment which we transfer to Party k)
			mu->sampleUnivariate(f, commitments->getRecord(commitid)->getOpenedValue(), D);
			for (ulong i = 1; i <= D; ++i) {//commit to each coefficient
				fmpz_mod_poly_get_coeff_fmpz(value, f, i);
				commit(value, getCoeffCommitIdForTransfer(commitid, pid, k, i));//INTERACTIVE
			}
		} else {
			for (ulong i = 1; i <= D; ++i) {//Participate in other's commitments (We make dummy commitments to 0)
				fmpz_zero(value);
				commit(value);//INTERACTIVE
			}
		}
		if (!transFromUs.error) {
			//we privately send the coefficients of the sampled polynomial to target of transfer
			fmpz_zero(value);
			fmpz_mod_poly_set_coeff_fmpz(f, 0, value);//overwrite the zero coefficient with 0
#ifdef TRANSFER_SOURCE_SENDS_BAD_COEFFICIENT
			if (dishonest && transFromUs.transferTarget == 3) { //dishonest 'transfer source' will privately send a wrong value for the 1st coefficient (to Party 3)
				//TEST CASE OK : Transfer from dishonest party is rejected. Dishonest transfer source opens her commitment,
				//and a public commitment is made to that value (hence consistency of shares is guaranteed for the ongoing VSS)
				fmpz_mod_poly_get_coeff_fmpz(value, f, 1);
				fmpz_add_ui(value, value, 1);
				fmpz_mod_poly_set_coeff_fmpz(f, 1, value);// 1st coefficient set to original value + 1
			}
#endif
			MessagePtr m = newMsg();
			m->setDebugInfo("transfer commitment step 3 : " + commitid);
			m->setVerifiableShare(f);
			channels[k-1]->send(m);
		}
		fmpz_mod_poly_clear(f);
	}
	interact();
	{
		fmpz_mod_poly_t g;//holds the polynomial sampled by the transfer source (except 0 coefficient) for the transfer in which we are transfer target.
		fmpz_mod_poly_init(g, FIELD_PRIME);
		CommitmentTransfer transToUs;
		for (auto& t : vecTrans) {
			if (t.transferTarget == pid) {//we receive the coefficients for the transfer in which we are the target
				if (!t.error) {
					if (channels[t.transferSource-1]->hasMsg()) {
						MessagePtr mCoeff = channels[t.transferSource-1]->recv();
						if (MathUtil::degreeCheckEQ(mCoeff->getVerifiableShare(), D)) {
							fmpz_mod_poly_set(g, mCoeff->getVerifiableShare());
						} else {//we know at this point that transfer source is corrupt, but we don't mark it yet because other honest do not know.
							fmpz_mod_poly_zero(g);//we will assume dishonest sent zeroes. Source will have to open later.
						}
					} else {//we know at this point that transfer source is corrupt, but we don't mark it yet because other honest do not know.
						fmpz_mod_poly_zero(g);//we will assume dishonest sent zeroes. Source will have to open later.
					}
				}
				transToUs = t;
			}
		}
		if (!transToUs.error) {
			for (ulong i = 1; i <= D; ++i) {//commit to each coefficient received
				fmpz_mod_poly_get_coeff_fmpz(value, g, i);
				commit(value, getCoeffCommitIdForTransfer(transToUs.transferedCommitId, transToUs.transferSource, transToUs.transferTarget, i));//INTERACTIVE
			}
		} else {
			for (ulong i = 1; i <= D; ++i) {//Participate in other's commitments (We make dummy commitments to 0)
				fmpz_zero(value);
				commit(value);//INTERACTIVE
			}
		}
		/*
		 * No more commitments will happen during transfers. Mark all transfers with failed commits if
		 * source or target is corrupt. (Failed 'commit's mark parties as corrupt. Equivalently, we could
		 * check 'isSuccess' for each coefficient commitment.)
		 */
		for (auto& t : vecTrans) {
			if (isCorrupt(t.transferTarget) || isCorrupt(t.transferSource)) {
				t.error = true;
			}
		}
		/*
		 * Now every coefficient is committed to, and every party can locally form commitments
		 * to <f(k)> = cid + Ʃ ( k^i . <cid_coeff_i> ) using 'add' and 'scalarMult'.
		 * (We are going to do 'designatedOpen's next, and before we can do that every party
		 * must form these commitment records, at least for the shares they will receive.)
		 */
		CommitmentTransfer transFromUs;
		for (auto& t : vecTrans) {
			if (!t.error) {
				for (PartyId k = 1; k <= N; ++k) {
					t.addFkx(k, combineCoeffCommitsForTransfer(t.commitId, k, t.transferSource, t.transferTarget));
					t.addGkx(k, combineCoeffCommitsForTransfer(t.transferedCommitId, k, t.transferSource, t.transferTarget));
				}
			}
			if (t.transferSource == pid) {
				transFromUs = t;
			}
			if (t.transferTarget == pid) {
				transToUs = t;
			}
		}
		//for the commitment we are trying to transfer, open the commitments to corresponding parties
		for (ulong i = 1; i < N; ++i) {//we start from i = 1, so that we don't open to self
			PartyId k = getTargetForIteration(i);
			if (!transFromUs.error) {
				designatedOpen(transFromUs.getFkx(k), k);//INTERACTIVE
			} else {//We are not the source for any transfer. We will not 'designatedOpen' anything, but will participate in other's 'designatedOpen's.
				designatedOpen(NONE, k);//INTERACTIVE
			}
		}
		//for the commitment being transfered to us, open the commitments to corresponding parties
		for (ulong i = 1; i < N; ++i) {//we start from i = 1, so that we don't open to self
			PartyId k = getTargetForIteration(i);
			if (!transToUs.error) {
				designatedOpen(transToUs.getGkx(k), k);//INTERACTIVE
			} else {//We are not the target for any transfer. We will not 'designatedOpen' anything, but will participate in other's 'designatedOpen's.
				designatedOpen(NONE, k);//INTERACTIVE
			}
		}
		fmpz_mod_poly_clear(g);
	}
	{//Step 4 - For each ongoing transfer of commitment, check consistency of commitments opened to us.
		MessagePtr bmReject = newMsg();
		bmReject->setDebugInfo("transfer commitment step 4 - rejected transfers");
		for (auto& t : vecTrans) {
			if (!t.error) {
				CommitmentRecord* crFk = commitments->getRecord(t.getFkx(pid));
				CommitmentRecord* crGk = commitments->getRecord(t.getGkx(pid));
				bool openOk = (crFk->isValueOpenToUs() && crGk->isValueOpenToUs());
				if (!openOk || fmpz_equal(crFk->getOpenedValue(), crGk->getOpenedValue()) == 0) {//NOT EQUAL
					//We only send information about rejected. We are only interested in 'reject's by honest parties.
					bmReject->addTransfer(t.commitId, t.transferSource, t.transferTarget);
				}
#ifdef TRANSFER_REJECT_VALID_TRANSFER
				if(dishonest && t.transferSource == 3 && t.transferTarget == 1) {//dishonest checker(Party 2) rejects transfer (from Party 3 to Party 1)
					//TEST CASE OK : Each party checks the shares made public (previously 'designated open'ed to Party 2). They match and transfer succeeds.
					bmReject->addTransfer(t.commitId, t.transferSource, t.transferTarget);
				}
#endif
			}
		}
		broadcast->broadcast(bmReject);
	}
	interact();
	{
		//first update vecTrans with broadcast information
		for (ulong i = 0; i < N; ++i) {
			PartyId sender = i + 1;
			if (broadcast->hasMsg(sender)) {
				vector<CommitmentTransfer> vecReject = broadcast->recv(sender)->getTransfers();
				if (vecReject.size() > vecTrans.size()) {
					addCorrupt(sender);
					continue;//continue with next sender's message
				}
				for (auto& t : vecTrans) {
					if (!t.error) {
						for (auto const& tReject : vecReject) {
							const bool match = (t.commitId == tReject.commitId) && (t.transferSource == tReject.transferSource) && (t.transferTarget == tReject.transferTarget);
							if (match) {
								t.addRejecter(sender);
								break;//continue with next yet unrejected (by this sender) transfer
							}
						}
					}
				}
			}
		}
		//Next, we open the commitments for the rejected transfers for which we are either the source or the target,
		//and we also participate in the open's of other parties
		unordered_map<PartyId, ulong> numberOfOpensPerParty;// 'open's are done in parallel to reduce number of rounds
		CommitmentTransfer transFromUs, transToUs;
		for (auto& t : vecTrans) {
			if (t.transferSource == pid) {
				transFromUs = t;
			}
			if (t.transferTarget == pid) {
				transToUs = t;
			}
			if (!t.error) {
				auto const& is = numberOfOpensPerParty.insert(make_pair(t.transferSource, 0));
				auto const& it = numberOfOpensPerParty.insert(make_pair(t.transferTarget, 0));
				is.first->second += t.rejecters.size();
				it.first->second += t.rejecters.size();
			}
		}
		ulong maxNumberOfOpensPerParty = 0;
		for (auto const& p : numberOfOpensPerParty) {
			if (p.second > maxNumberOfOpensPerParty) {
				maxNumberOfOpensPerParty = p.second;
			}
		}
		if (!transFromUs.error) {
			for (auto const& k : transFromUs.rejecters) {
				open(transFromUs.getFkx(k));//INTERACTIVE
			}
		}
		if (!transToUs.error) {
			for (auto const& k : transToUs.rejecters) {
				open(transToUs.getGkx(k));//INTERACTIVE
			}
		}
		ulong participations = (maxNumberOfOpensPerParty - ((transFromUs.error ? 0 : transFromUs.rejecters.size()) + (transToUs.error ? 0 : transToUs.rejecters.size())));
		for (ulong i = 0; i < participations; ++i) {
			open();//INTERACTIVE
		}
		/*
		 * Now that all rejected have been opened, every party can compare the opened values
		 * for all rejections, and tell whether dishonest parties reported false rejections
		 * or one (or both) of (transferSource, transferTarget) was dishonest.
		 */
		for (auto& t : vecTrans) {
			if (!t.error && t.isRejected()) {
				for (auto const& k : t.rejecters) {
					CommitmentRecord* crF = commitments->getRecord(t.getFkx(k));
					CommitmentRecord* crG = commitments->getRecord(t.getGkx(k));
					bool sourceOpenOk = (crF != nullptr && crF->isOpened());
					bool targetOpenOk = (crG != nullptr && crG->isOpened());
					if (!sourceOpenOk) {
						addCorrupt(t.transferSource);
					}
					if (!targetOpenOk) {
						addCorrupt(t.transferTarget);
					}
					bool openOk = sourceOpenOk && targetOpenOk;
					bool openedValuesOk = false;
					if (openOk) {
						openedValuesOk = (fmpz_equal(crF->getOpenedValue(), crG->getOpenedValue()) != 0);//if source & target opened the same values, rejection was due to dishonest checker
					}
					if (!openOk || !openedValuesOk) {
						t.error = true;
						break;//continue with next transfer
					} else if (!isCorrupt(t.transferSource) && !isCorrupt(t.transferTarget)) {//this means both source and target also succeeded in designated opens in step 3
						addCorrupt(k);//hence, rejecter must have lied (source&target could not have been first dishonest, then honest)
					}
				}
			}
		}
	}
	{//Step 5 - At this point, 'error' flag is set to true for all transfers in which one or both of (source, target) is corrupt. All honest will agree on the flag values.
		CommitmentTransfer transFromUs, transToUs;
		for (auto& t : vecTrans) {
			if (t.transferSource == pid) {
				transFromUs = t;
			}
			if (t.transferTarget == pid) {
				transToUs = t;
			}
		}
		//if transfer from us is marked as erronous, we open our commitment (note : this will not prove that we were honest before. but it will enable a public commitment by honest parties)
		if (transFromUs.error) {
#ifdef TRANSFER_SOURCE_DO_NOT_OPEN_ERRONEOUS
				if(dishonest) {//dishonest transfer source refuses to open commitment
					//TEST CASE OK : Transfer source is marked as corrupt. Consequently, 0 is used instead of shares distributed by her for the ongoing VSS.
					open();//INTERACTIVE
				} else
#endif
			open(transFromUs.commitId);//INTERACTIVE
		} else {//participate in 'open's of others
			open();//INTERACTIVE
		}
		for (auto& t : vecTrans) {
			if (t.error) {
				CommitmentRecord* cr = commitments->getRecord(t.commitId);
				if (cr != nullptr && cr->isOpened()) {
					t.transferedCommitId = getTransferedCommitId(t);
					if(!commitments->exists(t.transferedCommitId)) {
						commitments->addRecord(t.transferTarget, t.transferedCommitId);
					}
					CommitmentRecord* crTransfered = commitments->getRecord(t.transferedCommitId);
					publicCommit(crTransfered, cr->getOpenedValue());
				} else {
					addCorrupt(t.transferSource);//transfer fails. all honest will use 0 shares instead of what this source sent.
				}
			}
		}
	}
	interact();
}

/**
 * Protocol 'Perfect Commitment Multiplication'
 *
 * A new commitment is made to the value,
 * which is the product of values of commitments cid1 and cid2.
 * Note : Other parties will simultaneously run multiplyCommit(CommitmentId cid1', CommitmentId cid2')
 */
CommitmentId Party::multiplyCommitments(CommitmentId cid1, CommitmentId cid2) {
	CommitmentRecord* cr1 = nullptr;
	CommitmentRecord* cr2 = nullptr;
	CommitmentId cid3 = NONE;
	{//Step1
		//Check preconditions
		cr1 = commitments->getRecord(cid1);
		if (cr1 == nullptr || cr1->getOwner() != pid) {
			throw PceasException("Bad commitid in multiplyCommit :"+cid1);
		}
		cr2 = commitments->getRecord(cid2);
		if (cr2 == nullptr || cr2->getOwner() != pid) {
			throw PceasException("Bad commitid in multiplyCommit :"+cid2);
		}
		//commit to the product
		fmpz_mul(value, cr1->getOpenedValue(), cr2->getOpenedValue());
		cid3 = getMultipliedCommitId(cid1, cid2);
#ifdef MULTIPLICATION_COMMIT_TO_DIFFERENT_VALUE
			if (dishonest) { //dishonest will commit to a value different than the product of values for cid1 and cid2.
				//TEST CASE OK : Multiplication is rejected, and this party is identified as corrupt. Other multiplications are not affected.
				fmpz_add_ui(value, value, 1); // commit to value + 1 instead of value
			}
#endif
		commit(value, cid3);//INTERACTIVE

		MessagePtr bm = newMsg();
		bm->setDebugInfo("multiply commitments step 1 : " + cid1 + " * " + cid2);
		//Let other parties know about our commitment multiplication
		bm->addMultiplication(cid1, cid2, cid3, pid);
		broadcast->broadcast(bm);
	}
	interact();
	vector<CommitmentMult> vecMult;
	{//Step2
		for (ulong i = 0; i < N; ++i) {
			PartyId sender = i + 1;
			if (broadcast->hasMsg(sender)) {
				vector<CommitmentMult> received = broadcast->recv(sender)->getMultiplications();
				if (received.size() == 1) {
					CommitmentMult receivedMult = received.back();
					CommitmentRecord* cr1_i = commitments->getRecord(receivedMult.cid1);
					CommitmentRecord* cr2_i = commitments->getRecord(receivedMult.cid2);
					CommitmentRecord* cr3_i = commitments->getRecord(receivedMult.cid3);
					bool rec1Ok = (cr1_i != nullptr && cr1_i->getOwner() == sender);
					bool rec2Ok = (cr2_i != nullptr && cr2_i->getOwner() == sender);
					bool rec3Ok = (cr3_i != nullptr && cr3_i->getOwner() == sender);
					if (rec1Ok && rec2Ok && rec3Ok) {
						CommitmentMult newMult(receivedMult.cid1, receivedMult.cid2, receivedMult.cid3, sender);//ignore anything else the sent message contains
						vecMult.push_back(newMult);
					} else {
						if (isCorrupt(pid)) {//we want to keep corrupt parties running until the end for debugging test cases. Here we prevent corrupted marking others as corrupt due to previously failed transfers.
							continue;
						}
						addCorrupt(sender);//all honest will agree
					}
				} else {
					addCorrupt(sender);//all honest will agree
				}
			} else {
				addCorrupt(sender);//all honest will agree
			}
		}
		fmpz_mod_poly_t f, g, h;
		fmpz_mod_poly_init(f, FIELD_PRIME);
		fmpz_mod_poly_init(g, FIELD_PRIME);
		fmpz_mod_poly_init(h, FIELD_PRIME);
		mu->sampleUnivariate(f, cr1->getOpenedValue(), D);
		mu->sampleUnivariate(g, cr2->getOpenedValue(), D);
		fmpz_mod_poly_mul(h, f, g);
		if (!MathUtil::degreeCheckEQ(h, 2*D)) {//sanity check
			throw PceasException("Bad polynomial in commitment multiplication.");
		}
		//we commit to all coefficients(except x^0 coefficients val1, val2, val1*val2) of all 3 polynomials
		for (ulong i = 1; i <= D; ++i) {
			fmpz_mod_poly_get_coeff_fmpz(value, f, i);
			commit(value, getCoeffCommitIdForMult(POLY_F, cid1, cid2, i));//INTERACTIVE
		}
		for (ulong i = 1; i <= D; ++i) {
			fmpz_mod_poly_get_coeff_fmpz(value, g, i);
			commit(value, getCoeffCommitIdForMult(POLY_G, cid1, cid2, i));//INTERACTIVE
		}
		for (ulong i = 1; i <= 2*D; ++i) {
			fmpz_mod_poly_get_coeff_fmpz(value, h, i);
			commit(value, getCoeffCommitIdForMult(POLY_H, cid1, cid2, i));//INTERACTIVE
		}
		//No more commits will happen. We don't need to consider multiplications of corrupt players (for ex. those with with failed commitments). Mark their multiplications.
		for (auto& m : vecMult) {
			if (isCorrupt(m.owner)) {
				m.error = true;
			}
		}
		fmpz_mod_poly_clear(f);
		fmpz_mod_poly_clear(g);
		fmpz_mod_poly_clear(h);
	}
	{//Step3
		CommitmentMult myMult;
		/*
		 * Now every coefficient is committed to, and every party can locally form commitments
		 * to <f(k)> = cid + Ʃ ( k^i . <cid_coeff_i> ) using 'add' and 'scalarMult'.
		 * (We are going to do 'designatedOpen's next, and before we can do that every party
		 * must form these commitment records, at least for the shares they will receive.)
		 */
		for (auto& m : vecMult) {
			if (!m.error) {
				for (PartyId k = 1; k <= N; ++k) {
					m.addFkx(k, combineCoeffCommitsForMult(POLY_F, m.cid1, m.cid1, m.cid2, k, D));
					m.addGkx(k, combineCoeffCommitsForMult(POLY_G, m.cid2, m.cid1, m.cid2, k, D));
					m.addHkx(k, combineCoeffCommitsForMult(POLY_H, m.cid3, m.cid1, m.cid2, k, 2*D));
				}
			}
			if (m.owner == pid) {
				myMult = m;
			}
		}
		//'designatedOpen' shares for our multiplication
		for (ulong i = 1; i < N; ++i) {//we start from i = 1, so that we don't open to self
			PartyId k = getTargetForIteration(i);
			if (!myMult.error) {
				designatedOpen(myMult.getFkx(k), k);//INTERACTIVE
				designatedOpen(myMult.getGkx(k), k);//INTERACTIVE
				designatedOpen(myMult.getHkx(k), k);//INTERACTIVE
			} else {//We will not 'designatedOpen' anything, but will participate in other's 'designatedOpen's.
				designatedOpen(NONE, k);//INTERACTIVE
				designatedOpen(NONE, k);//INTERACTIVE
				designatedOpen(NONE, k);//INTERACTIVE
			}
		}
	}
	{//Step 4
		//For each ongoing multiplication of commitments, we will check consistency of commitments opened to us.
		MessagePtr bmReject = newMsg();
		bmReject->setDebugInfo("multiply commitments step 4 - rejected multiplications");
		for (auto& m : vecMult) {
			if (!m.error) {
				CommitmentRecord* crFk = commitments->getRecord(m.getFkx(pid));
				CommitmentRecord* crGk = commitments->getRecord(m.getGkx(pid));
				CommitmentRecord* crHk = commitments->getRecord(m.getHkx(pid));
				bool openOk = (crFk->isValueOpenToUs() && crGk->isValueOpenToUs() && crHk->isValueOpenToUs());
				bool valuesOk = false;
				if (openOk) {
					fmpz_mul(value, crFk->getOpenedValue(), crGk->getOpenedValue());
					fmpz_mod(value, value, FIELD_PRIME);
					if (fmpz_equal(value, crHk->getOpenedValue()) != 0) {//EQUAL
						valuesOk = true;//f(pid) * g(pid) == h(pid)
					}
				}
				if (!openOk || !valuesOk) {
					//We only send information about rejected. We are only interested in 'reject's by honest parties.
					bmReject->addMultiplication(m.cid1, m.cid2, m.cid3, m.owner);
				}
#ifdef MULTIPLICATION_REJECT_VALID_MULTIPLICATION
				if(dishonest && m.owner == 3) {//dishonest checker(Party 2) rejects (legitimate) multiplication of Party 3.
					//TEST CASE OK : Party 3 'open's previously 'designatedOpen'ed shares. Each party checks the shares made public. They match and transfer of Party 3 succeeds.
					bmReject->addMultiplication(m.cid1, m.cid2, m.cid3, m.owner);
				}
#endif
			}
		}
		broadcast->broadcast(bmReject);
	}
	interact();
	{//Step5
		//first update vecMult with broadcast information
		for (ulong i = 0; i < N; ++i) {
			PartyId sender = i + 1;
			if (broadcast->hasMsg(sender)) {
				vector<CommitmentMult> vecReject = broadcast->recv(sender)->getMultiplications();
				if (vecReject.size() > vecMult.size()) {
					addCorrupt(sender);
					continue;//continue with next sender's message
				}
				for (auto& m : vecMult) {
					if (!m.error) {
						for (auto const& mReject : vecReject) {
							const bool match = (m.cid1 == mReject.cid1) && (m.cid2 == mReject.cid2) && (m.cid3 == mReject.cid3);
							if (match) {
								m.addRejecter(sender);
								break;//continue with next yet unrejected (by this sender) multiplication
							}
						}
					}
				}
			}
		}
		//For any party who rejected our multiplication, we 'open' the shares which we 'designatedOpen'ed to them previously
		//and we also participate in the open's of other parties
		ulong maxNumberOfOpensPerParty = 0;// 'open's are done in parallel to reduce number of rounds (3 distinct opens for f,g,h counted as 1)
		CommitmentMult myMult;
		for (auto& m : vecMult) {
			if (m.owner == pid) {
				myMult = m;
			}
			if (!m.error) {
				if (m.rejecters.size() > maxNumberOfOpensPerParty) {
					maxNumberOfOpensPerParty = m.rejecters.size();
				}
			}
		}
		if (!myMult.error) {
			for (auto const& k : myMult.rejecters) {
				open(myMult.getFkx(k));//INTERACTIVE
				open(myMult.getGkx(k));//INTERACTIVE
				open(myMult.getHkx(k));//INTERACTIVE
			}
		}
		ulong participations = (maxNumberOfOpensPerParty - (myMult.error ? 0 : myMult.rejecters.size()));
		for (ulong i = 0; i < participations; ++i) {
			open();//INTERACTIVE
			open();//INTERACTIVE
			open();//INTERACTIVE
		}
		/*
		 * Now that all rejected have been opened, every party can compare the opened values
		 * for all rejections, and tell whether dishonest parties reported false rejections
		 * or the party which did the commitment multiplication was dishonest.
		 */
		for (auto& m : vecMult) {
			if (!m.error && m.isRejected()) {
				for (auto const& k : m.rejecters) {
					CommitmentRecord* crF = commitments->getRecord(m.getFkx(k));
					CommitmentRecord* crG = commitments->getRecord(m.getGkx(k));
					CommitmentRecord* crH = commitments->getRecord(m.getHkx(k));
					bool fOpenOk = (crF != nullptr && crF->isOpened());
					bool gOpenOk = (crG != nullptr && crG->isOpened());
					bool hOpenOk = (crH != nullptr && crH->isOpened());
					bool openOk = fOpenOk && gOpenOk && hOpenOk;
					bool valuesOk = false;
					if (openOk) {
						fmpz_mul(value, crF->getOpenedValue(), crG->getOpenedValue());
						fmpz_mod(value, value, FIELD_PRIME);
						if (fmpz_equal(value, crH->getOpenedValue()) != 0) {//EQUAL
							valuesOk = true;//f(k) * g(k) == h(k)
						}
					}
					if (!openOk || !valuesOk) {
						addCorrupt(m.owner);
						m.error = true;
						break;//continue with next transfer
					} else if (!isCorrupt(m.owner)) {//this means owner also succeeded in designated opens in step 3
						addCorrupt(k);//hence, rejecter must have lied (owner could not have been first dishonest, then honest)
					}
				}
			}
		}
	}
	interact();
	return cid3;
}

/**
 * A helper method to combine commitments to coefficients of a polynomial f(x)
 * to get a commitment to the party's share <f(k)> = cid + Ʃ ( k^i . <cid_coeff_i> )
 */
CommitmentId Party::combineCoeffCommitsForTransfer(CommitmentId cid, ulong k, ulong transferSource, ulong transferTarget) {
	CommitmentId combined = cid;//initialized with term_0
	fmpz_t scalar;
	fmpz_init_set_ui(scalar, 1);
	for (ulong i = 1; i <= D; ++i) {
		fmpz_mul_ui(scalar,scalar,k); // k^i
		fmpz_mod(scalar, scalar, FIELD_PRIME); // k^i
		string cid_coeff_i = getCoeffCommitIdForTransfer(cid, transferSource, transferTarget, i);
		string term_i = constMultCommitment(scalar, cid_coeff_i); // k^i . <cid_coeff_i>
		combined = addCommitments(combined, term_i); // Ʃ
	}
	fmpz_clear(scalar);
	return combined;
}

CommitmentId Party::combineCoeffCommitsForMult(string polyName, CommitmentId cid, CommitmentId cid1, CommitmentId cid2, ulong k, ulong degree) {
	CommitmentId combined = cid;//initialized with term_0
	fmpz_t scalar;
	fmpz_init_set_ui(scalar, 1);
	for (ulong i = 1; i <= degree; ++i) {
		fmpz_mul_ui(scalar,scalar,k); // k^i
		fmpz_mod(scalar, scalar, FIELD_PRIME); // k^i
		string cid_coeff_i = getCoeffCommitIdForMult(polyName, cid1, cid2, i);
		string term_i = constMultCommitment(scalar, cid_coeff_i); // k^i . <cid_coeff_i>
		combined = addCommitments(combined, term_i); // Ʃ
	}
	fmpz_clear(scalar);
	return combined;
}

CommitmentId Party::combineCoeffCommitsForSharing(CommitmentId cid, ulong k) {
	CommitmentId combined = cid;//initialized with term_0
	fmpz_t scalar;
	fmpz_init_set_ui(scalar, 1);
	for (ulong i = 1; i <= D; ++i) {
		fmpz_mul_ui(scalar,scalar,k); // k^i
		fmpz_mod(scalar, scalar, FIELD_PRIME); // k^i
		string cid_coeff_i = getCoeffCommitIdForSharing(cid, i);
		string term_i = constMultCommitment(scalar, cid_coeff_i); // k^i . <cid_coeff_i>
		combined = addCommitments(combined, term_i); // Ʃ
	}
	fmpz_clear(scalar);
	return combined;
}

/**
 * A helper method, which simulates 'combineCoeffCommits' to create the commitment ID that would be
 * output by that function when called with same arguments. Does not create any commitments.
 */
CommitmentId Party::getCombinedCoeffCommitIdForTransfer(CommitmentId cid, ulong k, ulong transferSource, ulong transferTarget) const {
	CommitmentId combined = cid;//initialized with term_0
	fmpz_t scalar;
	fmpz_init_set_ui(scalar, 1);
	for (ulong i = 1; i <= D; ++i) {
		fmpz_mul_ui(scalar,scalar,k); // k^i
		fmpz_mod(scalar, scalar, FIELD_PRIME); // k^i
		CommitmentId cid_coeff_i = getCoeffCommitIdForTransfer(cid, transferSource, transferTarget, i);
		CommitmentId term_i = getConstMultCommitId(scalar, cid_coeff_i);
		combined = getAddedCommitId(combined, term_i);
	}
	fmpz_clear(scalar);
	return combined;
}

CommitmentId Party::getCombinedCoeffCommitIdForMult(string polyName, CommitmentId cid, CommitmentId cid1, CommitmentId cid2, ulong k, ulong degree) const {
	CommitmentId combined = cid;//initialized with term_0
	fmpz_t scalar;
	fmpz_init_set_ui(scalar, 1);
	for (ulong i = 1; i <= degree; ++i) {
		fmpz_mul_ui(scalar,scalar,k); // k^i
		fmpz_mod(scalar, scalar, FIELD_PRIME); // k^i
		CommitmentId cid_coeff_i = getCoeffCommitIdForMult(polyName, cid1, cid2, i);
		CommitmentId term_i = getConstMultCommitId(scalar, cid_coeff_i);
		combined = getAddedCommitId(combined, term_i);
	}
	fmpz_clear(scalar);
	return combined;
}

CommitmentId Party::getCombinedCoeffCommitIdForSharing(CommitmentId cid, ulong k) const {
	CommitmentId combined = cid;//initialized with term_0
	fmpz_t scalar;
	fmpz_init_set_ui(scalar, 1);
	for (ulong i = 1; i <= D; ++i) {
		fmpz_mul_ui(scalar,scalar,k); // k^i
		fmpz_mod(scalar, scalar, FIELD_PRIME); // k^i
		CommitmentId cid_coeff_i = getCoeffCommitIdForSharing(cid, i);
		CommitmentId term_i = getConstMultCommitId(scalar, cid_coeff_i);
		combined = getAddedCommitId(combined, term_i);
	}
	fmpz_clear(scalar);
	return combined;
}

CommitmentId Party::getCoeffCommitIdForTransfer(CommitmentId cid, PartyId source, PartyId target, ulong coeff) const {
	CommitmentId commitid = "_(";
	commitid += cid+"_trans_coeff_";
	commitid += to_string(source)+"_";
	commitid += to_string(target)+"_";
	commitid += to_string(coeff);
	commitid += ")_";
	return commitid;
}

CommitmentId Party::getCoeffCommitIdForMult(string polyName, CommitmentId cid1, CommitmentId cid2, ulong coeff) const {
	auto p = getSortedPair(cid1, cid2);
	CommitmentId commitid = "_(";
	commitid += p.first;
	commitid += "_mult_coeff_";
	commitid += p.second;
	commitid += "_";
	commitid += polyName;//f, g, or h
	commitid += to_string(coeff);
	commitid += ")_";
	return commitid;
}

CommitmentId Party::getCoeffCommitIdForSharing(CommitmentId cid, ulong coeff) const {
	CommitmentId commitid = "_(";
	commitid += cid+"_share_coeff_";
	commitid += to_string(coeff);
	commitid += ")_";
	return commitid;
}

CommitmentId Party::getMultipliedCommitId(CommitmentId cid1, CommitmentId cid2) const {
	auto p = getSortedPair(cid1, cid2);
	CommitmentId commitid = "_(";
	commitid += p.first;
	commitid += "_*_";
	commitid += p.second;
	commitid += ")_";
	return commitid;
}

CommitmentId Party::getAddedCommitId(CommitmentId cid1, CommitmentId cid2) const {
	auto p = getSortedPair(cid1, cid2);
	CommitmentId commitid = "_(";
	commitid += p.first;
	commitid += "_+_";
	commitid += p.second;
	commitid += ")_";
	return commitid;
}

CommitmentId Party::getConstMultCommitId(fmpz_t const& c, CommitmentId cid) const {
	CommitmentId commitid = "_(";
	commitid += MathUtil::fmpzToStr(c);
	commitid += "_._";
	commitid += cid;
	commitid += ")_";
	return commitid;
}

CommitmentId Party::getTransferedCommitId(CommitmentId id, PartyId source, PartyId target) const {
	CommitmentId commitid = "_(transfered_";
	commitid += id+"_";
	commitid += to_string(source)+"-->";
	commitid += to_string(target);
	commitid += ")_";
	return commitid;
}

CommitmentId Party::getTransferedCommitId(CommitmentTransfer const& ct) const {
	return getTransferedCommitId(ct.commitId, ct.transferSource, ct.transferTarget);
}

/**
 * For commutative operations like addition and multiplication, we did not care
 * which share gets assigned to which wire. We use lexical ordering to ensure
 * that same commitment ID is generated for any assigning order.
 */
pair<CommitmentId, CommitmentId> Party::getSortedPair(CommitmentId cid1, CommitmentId cid2) const {
	if (cid1 < cid2) {
		return pair<CommitmentId, CommitmentId>(cid1, cid2);
	} else {
		return pair<CommitmentId, CommitmentId>(cid2, cid1);
	}
}

/**
 * All honest parties will rename shares according to the same scheme :
 * 	1. to keep size of commitment IDs managable
 * 	2. to maintain a correspondance between shares of different parties
 * 	   (A party performs local computations on other parties shares. See
 * 	   operation of addition and constant mult. gates)
 * 	3. to be able to more easily handle commitment conflicts(We aim to prevent dishonest
 *     from creating a commitment, whose ID will be used by some honest party in the future,
 *     by enforcing an ID generation scheme.)
 *
 *  uniqueSuffix must be something all honest parties can agree on. (For ex. isInput, gateNumber, input sharing round)
 */
CommitmentId Party::makeShareName(PartyId distributer, PartyId receiver, string uniqueSuffix, bool input, bool mulTriple, bool assigned) const {
	string prefix = SHARE_PREFIX;
	if (input) {
		prefix += "(input)";
	}
	if (mulTriple) {
		prefix += "(multiplication_triple)";
	}
	if (assigned) {
		prefix += "(assigned)";
	}
	return makeShareName(prefix, distributer, receiver, uniqueSuffix);
}

CommitmentId Party::makeShareName(string prefix, PartyId distributer, PartyId receiver, string uniqueSuffix) const {
	CommitmentId commitid = prefix;
	commitid += SEPERATOR;
	commitid += to_string(distributer);
	commitid += SEPERATOR;
	commitid += to_string(receiver);
	commitid += SEPERATOR;
	commitid += uniqueSuffix;
	return commitid;
}

CommitmentId Party::getShareNameFor(PartyId k, CommitmentId cid) const {
	if (k == pid) {
		return cid;
	}
	auto const& tokens = splitShareName(cid);
	string prefix = tokens.at(0);
	PartyId distributor = stoul(tokens.at(1));
	PartyId receiver = stoul(tokens.at(2));
	if (receiver != pid) {
		throw PceasException("Given share belongs to other party.");
	}
	string suffix = tokens.at(3);
	return makeShareName(prefix, distributor, k, suffix);
}

vector<string> Party::splitShareName(CommitmentId cid) const {
	vector<string> tokens;
	const char* str = cid.c_str();
	do {
		const char *begin = str;
		while(*str != SEPERATOR && *str) {
			str++;
		}
		tokens.push_back(string(begin, str));
	} while (0 != *str++);
	if (tokens.size() != 4) {
		throw PceasException("Malformed share name");
	}
	return tokens;
}

CommitmentId Party::makeTripleName(PartyId owner, string type, GateNumber gn) const {
	CommitmentId commitid = TRIPLE_PREFIX;
	commitid += SEPERATOR;
	commitid += to_string(owner);
	commitid += SEPERATOR;
	commitid += type;
	commitid += SEPERATOR;
	commitid += to_string(gn);
	return commitid;
}

CommitmentId Party::addCommitments(CommitmentId cid1, CommitmentId cid2) {
	CommitmentRecord* cr1 = commitments->getRecord(cid1);
	CommitmentRecord* cr2 = commitments->getRecord(cid2);
	if (cr1 != nullptr && cr2 != nullptr) {
		if (cr1->getOwner() != cr2->getOwner()) {
			throw PceasException("Trying to add commitments with different owners.");
		}
		PartyId owner = cr1->getOwner();
		CommitmentId cid3 = getAddedCommitId(cid1, cid2);
		if (!commitments->exists(cid3)) {
			commitments->addRecord(owner, cid3);
		}
		CommitmentRecord* cr3 = commitments->getRecord(cid3);
		fmpz_t temp;
		fmpz_init_set_ui(temp, 0);
		fmpz_add(temp, cr1->getShare(), cr2->getShare());
		fmpz_mod(temp, temp, FIELD_PRIME);//reduce
		cr3->setShare(temp);
		if (owner == pid) {
			fmpz_mod_poly_t tempPoly;
			fmpz_mod_poly_init(tempPoly, FIELD_PRIME);
			fmpz_mod_poly_add(tempPoly, cr1->getfx_0(), cr2->getfx_0());
			cr3->setfx_0(tempPoly);
			fmpz_mod_poly_clear(tempPoly);
			cr3->setOpenedValue(calculateZeroShare(cr3->getfx_0()));
			// opened value could also be set as follows :
//			fmpz_add(temp, cr1->getOpenedValue(), cr2->getOpenedValue());
//			cr3->setOpenedValue(temp);
		}
		cr3->setDone(cr1->isSuccess() && cr2->isSuccess());
		fmpz_clear(temp);
		return cid3;
	}
	throw PceasException("Trying addition with nonexisting commitment : "+cid1+"+"+cid2);
}

CommitmentId Party::constMultCommitment(fmpz_t const& c, CommitmentId cid) {
	CommitmentRecord* cr = commitments->getRecord(cid);
	if (cr != nullptr) {
		CommitmentId cid3 = getConstMultCommitId(c, cid);
		if (!commitments->exists(cid3)) {
			commitments->addRecord(cr->getOwner(), cid3);
		}
		CommitmentRecord* cr3 = commitments->getRecord(cid3);
		fmpz_t temp;
		fmpz_init_set_ui(temp, 0);
		fmpz_mul(temp, c, cr->getShare());
		fmpz_mod(temp, temp, FIELD_PRIME);//reduce
		cr3->setShare(temp);
		if (cr->getOwner() == pid) {
			fmpz_mod_poly_t tempPoly;
			fmpz_mod_poly_init(tempPoly, FIELD_PRIME);
			fmpz_mod_poly_scalar_mul_fmpz(tempPoly, cr->getfx_0(), c);
			cr3->setfx_0(tempPoly);
			fmpz_mod_poly_clear(tempPoly);
			cr3->setOpenedValue(calculateZeroShare(cr3->getfx_0()));
			// opened value could also be set as follows :
//			fmpz_mul(temp, c, cr->getOpenedValue());
//			cr3->setOpenedValue(temp);
		}
		cr3->setDone(cr->isSuccess());
		fmpz_clear(temp);
		return cid3;
	}
	throw PceasException("Trying scalar multiplication with nonexisting commitment : "+cid);
}

CommitmentId Party::constAddCommitment(fmpz_t const& c, CommitmentId cid) {
	CommitmentRecord* cr = commitments->getRecord(cid);
	if (cr != nullptr) {
		CommitmentId tempCid = commitments->addRecord(cr->getOwner());
		CommitmentRecord* tempCr = commitments->getRecord(tempCid);
		publicCommit(tempCr, c);
		tempCr->setDone(true);
		return addCommitments(cid, tempCid);
	}
	throw PceasException("Trying scalar addition with nonexisting commitment : "+cid);
}

CommitmentId Party::substractCommitments(CommitmentId cid1, CommitmentId cid2) {
	fmpz_t minusOne;
	fmpz_init_set_si(minusOne, -1);
	CommitmentId minusSecond = constMultCommitment(minusOne, cid2);
	CommitmentId result = addCommitments(cid1, minusSecond);
	fmpz_clear(minusOne);
	return result;
}

/**
 * Calculates share for a single party
 */
void Party::calculatePartyShare(ulong i, fmpz_mod_poly_t const& f) {
	fmpz_t partyIndex;
	fmpz_init(partyIndex);
	fmpz_set_ui(partyIndex, i+1); //array index 0 for Party 1, etc.
	fmpz_mod_poly_evaluate_fmpz(shares+i, f, partyIndex); //share of Party i is f(i)
	fmpz_clear(partyIndex);
}

/**
 * Uses multipoint evaluation to calculate shares for all parties
 */
void Party::calculatePartyShares(fmpz_mod_poly_t const& f) {
	_fmpz_vec_zero(shares, N);
	fmpz_mod_poly_evaluate_fmpz_vec_fast(shares, f, multipointVec, N);
}

fmpz_t const& Party::calculateZeroShare(fmpz_mod_poly_t const& f) {
	fmpz_t z;
	fmpz_init_set_ui(z, 0);
	fmpz_mod_poly_evaluate_fmpz(value, f, z);
	fmpz_clear(z);
	return value;
}

/**
 * Calculates the Lagrange basis polynomial with given index
 */
void Party::calculateDelta(fmpz_mod_poly_t& delta, PartyId i) {
	fmpz_t d;
	fmpz_init_set_ui(d, 1);
	fmpz_mod_poly_t mono;
	fmpz_mod_poly_init(mono, FIELD_PRIME);
	fmpz_mod_poly_set_coeff_ui(mono, 1, 1); // mono(x) = x
	fmpz_mod_poly_zero(delta);
	fmpz_mod_poly_set_coeff_ui(delta, 0, 1); // delta(x) = 1
	fmpz_t temp;
	fmpz_init(temp);
	for (PartyId j = 1; j <= N; ++j) {
		if (j != i && !isCorrupt(j)) {
			fmpz_set_ui(temp, j);
			fmpz_neg(temp, temp); // temp = -j
			fmpz_mod(temp, temp, FIELD_PRIME); // temp = -j (mod P)
			fmpz_mod_poly_set_coeff_fmpz(mono, 0, temp); // mono(x) = x - j
			fmpz_mod_poly_mul(delta ,delta ,mono);
			fmpz_add_ui(temp, temp, i);
			fmpz_mod(temp, temp, FIELD_PRIME);
			fmpz_mul(d, d, temp);
		}
	}
	fmpz_invmod(d, d, FIELD_PRIME); // we need inverse of d mod P, as delta is Poduct[1/d_j * mono_j(x)]
	fmpz_mod_poly_scalar_mul_fmpz(delta, delta, d);
	fmpz_clear(d);
	fmpz_mod_poly_clear(mono);
	fmpz_clear(temp);
}

void Party::setRecombinationVector() {
	fmpz_t temp, z;
	fmpz_init(temp);
	fmpz_init_set_ui(z, 0);
	fmpz_mod_poly_t delta;
	fmpz_mod_poly_init(delta, FIELD_PRIME);
	_fmpz_vec_zero(recombinationVector, N);
	for (PartyId i = 1; i <= N; ++i) {
		if (!isCorrupt(i)) {//we don't need the vector elements for corrupt parties
			calculateDelta(delta, i);
			fmpz_mod_poly_evaluate_fmpz(temp, delta, z);
			fmpz_set(recombinationVector+i-1, temp);
		}
	}
	fmpz_clear(temp);
	fmpz_clear(z);
	fmpz_mod_poly_clear(delta);
}

/**
 * If the given verifiable share f_k(x) can be used to compute f(x,y),
 * evaluate f_k and return false if it conflicts the value associated with f(x,y).
 */
bool Party::checkConsistency(const PartyId x, const PartyId y, fmpz_t const& val, const PartyId k, fmpz_mod_poly_t const& f_k) {
	if (x == k || y == k) {//we can check a broadcast point (an opened disputed value) for consistency, only if this condition holds
		ulong indexArg;
		if (x == k) {
			//calculate f_k(y) = f(k, y)
			indexArg = y-1;
		} else {
			//calculate f_k(x) = f(k, x)
			indexArg = x-1;
		}
		calculatePartyShare(indexArg, f_k);

#ifdef VERBOSE
		fmpz_t tmp;
		fmpz_init(tmp);
		fmpz_set(tmp, shares+indexArg);
		cout << "Consistency Check of Party " << to_string(k) << "for dispute " << to_string(x) << "->" << to_string(y)
				<< "Calculated : " << MathUtil::fmpzToStr(tmp) << " Owner opened : " << MathUtil::fmpzToStr(val) << endl;
#endif

		if (fmpz_equal(val, shares+indexArg) == 0) {//NOT EQUAL
			return false;
		}
	}
	return true;
}

void Party::addSecret(string label, ulong val) {
	secrets->addSecret(label, val);
}

void Party::sanityChecks() {
	if (N < 3) {
		throw PceasException("Number of computing parties must be greater than 2.");
	}
	if (running == PCEAS || running == PCEAS_WITH_CIRCUIT_RANDOMIZATION) {
		//solving for max # corrupt parties C from C <= D and C < N-2D
		const ulong C = (N % 3 == 0) ? ((N - 1) / 3) : (N / 3);//C : maximum (expected) number of corrupt parties
		/*
		 * We enforce a threshold(Threshold T = D+1) for the worst case. Note that, by considering the worst case,
		 * we prevent runs for some (N,T) combinations, which would work fine when number of actively corrupted
		 * parties is less than C.
		 */
		if ((N - 2 * D) <= C) {
			throw PceasException("Threshold too large for max. number of corrupt parties.");
		}
		if (D < C) {
			throw PceasException("Threshold too small for max. number of corrupt parties.");
		}
		/*
		 * Note : If number of dishonest parties (detected) exceeds C at any point during
		 * execution, all honest will complain and stop execution.
		 * For example, for N = 6, T = 3, we can tolerate at most 1 dishonest party. When 2 parties
		 * are set as dishonest, computation will not complete.
		 */
		maxDishonest = C;
	}
	/**
	 * Because we use Shamir's secret sharing scheme, field must be of size at least N + 1.
	 * More general schemes which allow field size independent of N exist. See Chapter 6 of
	 * Secure Multiparty Computation And Secret Sharing. Cambridge University Press, 2015
	 */
	if (fmpz_cmp_ui(FIELD_PRIME, N) <= 0) {
		throw PceasException("Field size must be greater than number of computing parties.");
	}
	if (fmpz_is_probabprime(FIELD_PRIME) == 0) {//there is a possibility that a composite value is not caught by this check
		throw PceasException("Supplied field prime is a composite.");
	}
	if (!broadcast) {
		throw PceasException("No secure broadcast.");
	}
	if (!circuit) {
		throw PceasException("Circuit not set.");
	}
	if (circuit->getOutputCount() != 1) {//circuit must have single unconnected wire
		/*
		 * A simplifying assumption. To extend to circuits with multiple outputs,
		 * one simply needs to repeat 'Step 3 of 3 : output reconstruction' for each output.
		 * (Note that a gate CAN have multiple 'output' wires)
		 */
		throw PceasException("Function must have single output.");
	}
}

void Party::setBroadcast(ConsensusBroadcast* cb) {
	broadcast = cb;
}

void Party::setChannelTo(ulong i, SecureChannel* sc) {
	channels[i] = sc;
}

void Party::interact() {
	unique_lock<mutex> lk(m);
	interactive = true;
	cv.notify_one();
	cv.wait(lk, [this]{return messagesReady;});
	messagesReady = false;
}

void Party::end() {
	unique_lock<mutex> lk(m);
	done = true;
	interactive = true;//let synchronizer progress and terminate itself
	cv.notify_one();
}

void Party::runDummyInteractiveProtocol(uint i) { // A dummy protocol for testing synchronization
	const int TOTAL_ROUNDS = 10;
	for (int round = 0; round < TOTAL_ROUNDS; ++round) {
		//running local computations...
		this_thread::sleep_for(chrono::milliseconds(1000*i));
		interact();
	}
	end();
}

void Party::print(stringstream& ss) {
	//print commitments table
	commitments->print(ss);
	//print set of corrupted parties
	if (!corrupted.empty()) {
		cout << endl << "Corrupt : ";
		for (auto const& p : corrupted) {
			cout << to_string(p) << "\t";
		}
		cout << endl;
	}
	//print recombination vector
	cout << endl << "Recombination vector : ";
	for (ulong i = 0; i < N; ++i) {
		fmpz_t temp;
		fmpz_init_set(temp, recombinationVector+i);
		cout << MathUtil::fmpzToStr(temp) << "\t";
		fmpz_clear(temp);
	}
	cout << endl;
	//print shares array
//	cout << endl << "Shares : ";
//	for (ulong i = 0; i < N; ++i) {
//		fmpz_t temp;
//		fmpz_init_set(temp, shares+i);
//		cout << MathUtil::fmpzToStr(temp) << "\t";
//		fmpz_clear(temp);
//	}
//	cout << endl;
}

MessagePtr Party::newMsg() const {
	return MessagePtr(new Message(pid, FIELD_PRIME));
}

bool Party::isCorrupt(PartyId p) const {
	auto it = corrupted.find(p);
	return (it != corrupted.end());
}

void Party::addCorrupt(PartyId p) {
	corrupted.insert(p);
	if (corrupted.size() > maxDishonest) {
		throw PceasException("More corrupted parties than the protocol can handle.");
	}
	setRecombinationVector();//recalculate recombination vector
}

/**
 * To run 'transfersCommitment's and 'designatedOpen's in parallel,
 * Party 1 transfers to Party 1+i, Party 2 transfers to Party 2+i, ...
 * Note : Such a scheme is required because each party can receive
 * at most one private message (and one broadcast) from any other party
 * in a single round (by definition of 'round').
 */
PartyId Party::getTargetForIteration(ulong i) const {
	PartyId k = (pid + i);
	if (k > N) {
		k = k - N;
	}
	return k;
}

/**
 * Find source from given 'target', using 'sampleSource' and 'sampleTarget',
 * such that both sample and newly found source-target pairs comply with the
 * target choosing scheme given in 'getTargetForIteration'.
 */
PartyId Party::getSourceFromTarget(PartyId target, PartyId sampleSource, PartyId sampleTarget)  const{
	const ulong positiveDiff = (sampleTarget > sampleSource) ? (sampleTarget - sampleSource) : ((N + sampleTarget) - sampleSource);
	PartyId source = (target > positiveDiff) ? (target - positiveDiff) : ((N + target) - positiveDiff);
	return source;
}

PartyId Party::getTargetFromSource(PartyId source, PartyId sampleSource, PartyId sampleTarget) const {
	return getSourceFromTarget(source, sampleTarget, sampleSource);
}

const string Party::SHARE_PREFIX = "share";
const string Party::TRIPLE_PREFIX = "triple";

} /* namespace pceas */
