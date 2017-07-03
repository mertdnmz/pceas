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
