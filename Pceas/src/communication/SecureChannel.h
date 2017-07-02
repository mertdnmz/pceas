/*
 * SecureChannel.h
 *
 *  Created on: Feb 13, 2017
 *      Author: m3r7
 */

#ifndef SECURECHANNEL_H_
#define SECURECHANNEL_H_

#include "../message/Message.h"

namespace pceas {

/**
 * This class simulates Secure Channel ideal functionality.
 */
class Party;
class SecureChannel {
public:
	SecureChannel();
	virtual ~SecureChannel();

	void send(MessagePtr m);
	MessagePtr recv();
	bool hasMsg() const;
	void swapToFuture();
private:
	MessagePtr message;
	MessagePtr future;
};

} /* namespace pceas */

#endif /* SECURECHANNEL_H_ */
