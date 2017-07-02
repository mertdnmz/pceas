/*
 * SecureChannel.cpp
 *
 *  Created on: Feb 13, 2017
 *      Author: m3r7
 */

#include "SecureChannel.h"
#include "../core/PceasException.h"

namespace pceas {

SecureChannel::SecureChannel() {
}

SecureChannel::~SecureChannel() {
}

void SecureChannel::send(MessagePtr m) {
	future = m;
}

MessagePtr SecureChannel::recv() {
	if (message == nullptr) {
		throw PceasException("No message.");
	}
	return message;
}

bool SecureChannel::hasMsg() const {
	return message != nullptr;
}

/**
 * Called by synchronizer
 */
void SecureChannel::swapToFuture() {
	message = nullptr;//discard past
	swap(message, future);
}

} /* namespace pceas */
