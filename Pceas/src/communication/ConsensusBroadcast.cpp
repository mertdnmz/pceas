/*
 * ConsensusBroadcast.cpp
 *
 *  Created on: Feb 13, 2017
 *      Author: m3r7
 */

#include "ConsensusBroadcast.h"
#include "../core/PceasException.h"

namespace pceas {

ConsensusBroadcast::ConsensusBroadcast() {
}

ConsensusBroadcast::~ConsensusBroadcast() {
}

void ConsensusBroadcast::broadcast(MessagePtr m) {
	lock_guard<mutex> guard(mut);// multiple threads will try to broadcast
	future.insert(pair<PartyId,MessagePtr>(m->getSender(),m));
}

MessagePtr ConsensusBroadcast::recv(PartyId sender) {
	lock_guard<mutex> guard(mut);
	auto it = messages.find(sender);
	if (it == messages.end()) {
		throw PceasException("1 brodcast message expected, 0 found.");
	}
	return it->second;
}

bool ConsensusBroadcast::hasMsg(PartyId sender) {
	lock_guard<mutex> guard(mut);
	auto it = messages.find(sender);
	return (it != messages.end());
}

/**
 * Called by synchronizer
 */
void ConsensusBroadcast::swapToFuture() {
	messages.clear();//discard past
	swap(messages, future);
}

} /* namespace pceas */
