/*
 * Simulator.cpp
 *
 *  Created on: Feb 10, 2016
 *      Author: m3r7
 */

#include <thread>
#include <chrono>
#include <iostream>

#include "core/Party.h"
#include "circuit/CircuitGenerator.cpp"
#include "SimulatorOptions.cpp"

using namespace std;
using namespace pceas;

/*
 * Implementing synchronous communication securely requires both
 * 1. clock synchronization (Managed by 'synchronizer')
 *  and
 * 2. an upper bound on message delivery time on the network for timeouts
 *
 * Interaction between computing parties take place in rounds.
 * Each round, parties will
 * 1. Process incoming messages
 * 2. Send outgoing messages (one per party + one broadcast)
 */
void synchronizer(Party** parties, ulong partyCount, ConsensusBroadcast* cb) {
	unsigned long rounds = 0;
	while (true) {
		bool done = true;
		for (ulong i = 0; i < partyCount; ++i) {
			done &= parties[i]->done;
		}
		if (done) {
#ifdef VERBOSE
			for (ulong i = 0; i < partyCount; ++i) {//print internal states of parties
				stringstream ss;
				parties[i]->print(ss);
				cout << ss.str() << endl;
			}
#endif
			return;
		}
		//Synchronizer will wait until all parties become 'interactive'
		for (ulong i = 0; i < partyCount; ++i) {
			unique_lock<mutex> lk(parties[i]->m);
			if (!parties[i]->done) {
				parties[i]->cv.wait(lk, [parties, i]{return parties[i]->interactive;});
			}
		}
		for (ulong i = 0; i < partyCount; ++i) {
			lock_guard<mutex> lk(parties[i]->m);
			if (!parties[i]->done) {
				parties[i]->interactive = false;
			}
		}
		rounds++;
		cout << "**************************************************************" << endl;
		cout << "Transmitting messages. Round : " << rounds << endl;
		cb->swapToFuture();//transmit broadcast messages
		for (ulong i = 0; i < partyCount; ++i) {
			for (ulong j = 0; j < partyCount; ++j) {
				parties[i]->channels[j]->swapToFuture();//transmit private messages 1/2
			}
		}
		for (ulong i = 0; i < partyCount; ++i) {
			for (ulong j = 0; j <= i; ++j) {//all parties are waiting at this point, no need for locks
				if (parties[i]->channels[j]->hasMsg()) {
					parties[i]->channels[j]->recv()->printMsg(j);//print messages
				}
				if (parties[j]->channels[i]->hasMsg() && (i != j)) {
					parties[j]->channels[i]->recv()->printMsg(i);//print messages
				}
				swap(parties[i]->channels[j], parties[j]->channels[i]);//transmit private messages 2/2
			}
			if (cb->hasMsg(i+1)) {
				cb->recv(i+1)->printMsg();//print broadcast messages
			}
		}
#ifdef VERBOSE
		const unsigned long R = 0;//print internal states of parties at the specified round
		if (rounds == R) {
			for (ulong i = 0; i < partyCount; ++i) {
				stringstream ss;
				parties[i]->print(ss);
				cout << ss.str() << endl;
			}
		}
#endif
		/**
		 * Length of a single communication round in milliseconds.
		 * Any honest party who tries to deliver a message will always have it delivered within the same round.
		 * If we don't hear from a party within a round, we can assume that party to be an active cheater.
		 */
		const chrono::milliseconds ROUND_LENGTH(0);//Simulated communication takes place 'instantly' if set to 0.
		this_thread::sleep_for(ROUND_LENGTH);
		//Synchronizer signals 'messagesReady'
		for (ulong i = 0; i < partyCount; ++i) {
			lock_guard<mutex> lk(parties[i]->m);
			if (!parties[i]->done) {
				parties[i]->messagesReady = true;
				parties[i]->cv.notify_one();
			}
		}
	}
}

int main() {
	SimulatorOptions sopt;
	CircuitGenerator cg;

	/*
	 * Data Providers :
	 * For simplicity, we will assume that the parties which provide the shares of their
	 * secrets are a subset of the computing parties.
	 * (A distinct set of parties could be set as secret owners. Each such secret owner
	 * would have to be able to run Verifiable Secret Sharing with the N computing parties,
	 * if we are to guarantee that the shares provided are consistent.)
	 * See 'Party::addSecret'
	 *
	 * Computing Parties :
	 * Parties which will perform the secure evaluation using the shares provided to them.
	 *
	 * Data Users :
	 * The party which will receive the result of secure evaluation.
	 * (We could open the result to a subset of computing parties, or to any other party.)
	 * The data user will combine the shares to obtain the result and output it to the console.
	 * See 'Party::setDataUser'
	 */
	Party** computingParties;

	//Setup
	computingParties = new Party*[sopt.N];
	thread* computingThreads = new thread[sopt.N]; // Each computing party will run on its own thread.
	ConsensusBroadcast* cb = new ConsensusBroadcast();
	for (ulong i = 0; i < sopt.N; ++i) {
		PartyId id = i+1;
		computingParties[i] = new Party(id, sopt.N, sopt.T, sopt.FIELD_PRIME);
		Circuit* testCircuit;
		if (sopt.comparator) {
			testCircuit = cg.generateComparator(sopt.bitlength, sopt.labelA, sopt.labelB, sopt.labelOne);
		} else {
			testCircuit = cg.generate(sopt.circuitDescString);
		}
		computingParties[i]->setCircuit(testCircuit);
		computingParties[i]->setProtocol(sopt.prot);
		//set consensus broadcast channel
		computingParties[i]->setBroadcast(cb);
		//create and set secure P2P channels between computing parties
		for (ulong j = 0; j < sopt.N; ++j) {
			computingParties[i]->setChannelTo(j, new SecureChannel());
		}
		//set data user
		computingParties[i]->setDataUser(sopt.dataUser);
	}
	//For the data providers, set the labels and values of secrets to be input.
	for (SimulatorOptions::Input& s : sopt.secrets) {
		computingParties[s.p-1]->addSecret(s.label, s.value);
	}
	//set dishonest parties [makes sense only for PCEAS]
	for (auto const& s : sopt.activelyCorrupted) {
		computingParties[s-1]->setDishonest();
	}

	//Start secure evaluation
	thread sync(synchronizer, computingParties, sopt.N, cb);
	for (ulong i = 0; i < sopt.N; ++i) {
		switch (sopt.prot) {
			case PCEPS:
			case PCEAS_WITH_CIRCUIT_RANDOMIZATION:
				computingThreads[i] = thread(&Party::runProtocol, computingParties[i]);
				break;
			case PCEAS:
				if (sopt.sequentialRun) {
					Circuit* nextCircuit = cg.generate(sopt.nextRunCircuitDescString);
					//note : To keep thing simple, we do next run with same secrets used for prev run (So we could skip input sharing phase for second run. But we don't.)
					computingThreads[i] = thread(&Party::runProtocolSequential, computingParties[i], sopt.labelPrevRunResult, nextCircuit);
				} else {
					computingThreads[i] = thread(&Party::runProtocol, computingParties[i]);
				}
				break;
			default:
				computingThreads[i] = thread(&Party::runDummyInteractiveProtocol, computingParties[i], i+1);
				break;
		}
	}
	sync.join();
	for (ulong i = 0; i < sopt.N; ++i) {
		computingThreads[i].join();
	}

	//Cleanup
	for (ulong i = 0; i < sopt.N; ++i) {
		delete computingParties[i];
	}
	delete cb;
	delete[] computingParties;
	delete[] computingThreads;

	cout << "End " << endl;

	return 0;
}
