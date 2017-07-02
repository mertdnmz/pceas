/*
 * CommitmentException.cpp
 *
 *  Created on: Feb 20, 2017
 *      Author: m3r7
 */

#include "PceasException.h"

namespace pceas {

PceasException::PceasException():runtime_error("Unspecified error while running protocol."){}
PceasException::PceasException(std::string message):runtime_error(message.c_str()){}
PceasException::~PceasException() {
}

} /* namespace pceas */
