/*
 * ConsensusBroadcast.h
 *
 *  Created on: Feb 13, 2017
 *      Author: m3r7
 */

#ifndef CONSENSUSBROADCAST_H_
#define CONSENSUSBROADCAST_H_

#include <mutex>
#include <unordered_map>
#include "../message/Message.h"

namespace pceas {

/**
 * This class simulates Consensus Broadcast ideal functionality.
 */
class ConsensusBroadcast {
public:
	ConsensusBroadcast();
	virtual ~ConsensusBroadcast();

	void broadcast(MessagePtr m);
	MessagePtr recv(PartyId sender);
	bool hasMsg(PartyId sender);
	void swapToFuture();
private:
	unordered_map<PartyId, MessagePtr> messages;
	unordered_map<PartyId, MessagePtr> future;
	mutex mut;
};

} /* namespace pceas */

#endif /* CONSENSUSBROADCAST_H_ */
