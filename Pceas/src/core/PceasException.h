/*
 * CommitmentException.h
 *
 *  Created on: Feb 20, 2017
 *      Author: m3r7
 */

#ifndef PCEASEXCEPTION_H_
#define PCEASEXCEPTION_H_

#include <stdexcept>
#include <string>

namespace pceas {

class PceasException: public std::runtime_error {
public:
	PceasException();
	PceasException(std::string message);
	virtual ~PceasException();
};

} /* namespace pceas */

#endif /* PCEASEXCEPTION_H_ */
