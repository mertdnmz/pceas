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
 * Pceas.h
 *
 *  Created on: Mar 3, 2017
 *      Author: m3r7
 */

#ifndef PCEAS_H_
#define PCEAS_H_

#include <string>
#include <fmpz.h>

//#define VERBOSE
//#define NO_RANDOM //Use for debugging only. Parties will pick the same randoms in each run.

#ifdef VERBOSE
#include <iostream>
#endif

/*
 * BEGIN Test Cases
 *
 * We include a few test cases, which demonstrate how protocol
 * Pceas is supposed to behave in case of dishonest behaviour.
 *
 * Brief description of the test cases can be found within
 * the code, where the corresponding macros are used.
 *
 * One or more parties can be made to exhibit dishonest behaviours,
 * via calls to 'Party::setDishonest'.
 */
#define TEST_CASE_X // Replace X with the number of test you want to run.
#define TEST_ALL // Dishonest parties will exhibit all known dishonest behaviour

#ifdef TEST_ALL
#define TEST_CASE_1
#define TEST_CASE_2
#define TEST_CASE_3
#define TEST_CASE_4
#define TEST_CASE_5
#define TEST_CASE_6
#define TEST_CASE_7
#define TEST_CASE_8
#define TEST_CASE_9
#define TEST_CASE_10
#define TEST_CASE_11
#define TEST_CASE_12
#define TEST_CASE_13
#define TEST_CASE_14
#define TEST_CASE_15
#endif

#ifdef TEST_CASE_1
#define COMMITMENT_SEND_INVALID_SHARE
#endif
#ifdef TEST_CASE_2
#define COMMITMENT_SEND_INVALID_SHARE
#define COMMITMENT_DO_NOT_OPEN_DISPUTED
#endif
#ifdef TEST_CASE_3
#define COMMITMENT_SEND_INVALID_SHARE
#define COMMITMENT_ACCUSE_HONEST_AFTER_DISPUTES_OPENED
#endif
#ifdef TEST_CASE_4
#define COMMITMENT_DISHONEST_ACCUSED
#define COMMITMENT_ACCUSED_DO_NOT_OPEN_VERIFIABLE_SHARE
#endif
#ifdef TEST_CASE_5
#define OPEN_WITH_INVALID_FX0
#endif
#ifdef TEST_CASE_6
#define OPEN_SEND_INVALID_VERIFIERS
#endif
#ifdef TEST_CASE_7
#define DESIGNATEDOPEN_WITH_INVALID_FX0
#endif
#ifdef TEST_CASE_8
#define DESIGNATEDOPEN_WITH_INVALID_FX0
#define DESIGNATEDOPEN_DO_NOT_OPEN_REJECTED
#endif
#ifdef TEST_CASE_9
#define DESIGNATEDOPEN_SEND_INVALID_VERIFIERS
#endif
#ifdef TEST_CASE_10
#define TRANSFER_TARGET_COMMITS_TO_DIFFERENT_VALUE
#endif
#ifdef TEST_CASE_11
#define TRANSFER_REJECT_VALID_TRANSFER
#endif
#ifdef TEST_CASE_12
#define TRANSFER_SOURCE_SENDS_BAD_COEFFICIENT
#endif
#ifdef TEST_CASE_13
#define TRANSFER_SOURCE_SENDS_BAD_COEFFICIENT
#define TRANSFER_SOURCE_DO_NOT_OPEN_ERRONEOUS
#endif
#ifdef TEST_CASE_14
#define MULTIPLICATION_COMMIT_TO_DIFFERENT_VALUE
#endif
#ifdef TEST_CASE_15
#define MULTIPLICATION_REJECT_VALID_MULTIPLICATION
#endif
/* END Test Cases */

typedef ulong PartyId;
typedef std::string CommitmentId;
typedef unsigned long GateNumber;
typedef ulong SecretValue;

enum Protocol {
	PROT_NONE,
	PCEPS, // Protocol 'CEPS' (Circuit Evaluation with Passive Security)
	PCEAS, // Protocol 'CEAS' (Circuit Evaluation with Active Security)
	PCEAS_WITH_CIRCUIT_RANDOMIZATION // Protocol 'CEAS' with Circuit Randomization
};

static constexpr const char* NONE = "";
static const PartyId NOPARTY = 0;

#endif /* PCEAS_H_ */
