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
 * CommitmentTransfer.h
 *
 *  Created on: Feb 25, 2017
 *      Author: m3r7
 */

#ifndef COMMITMENTTRANSFER_H_
#define COMMITMENTTRANSFER_H_

#include <unordered_map>
#include <unordered_set>
#include <sstream>
#include "../core/Pceas.h"

namespace pceas {

using namespace std;

struct CommitmentTransfer {
public:
	CommitmentTransfer();
	CommitmentTransfer(bool err);
	CommitmentTransfer(CommitmentId commitId, PartyId transferSource, PartyId transferTarget);
	virtual ~CommitmentTransfer();
	CommitmentId commitId;
	PartyId transferSource;
	PartyId transferTarget;
	bool error;//something went wrong with this transfer. We ignore it until Step 5.
	unordered_set<PartyId> rejecters;//steps 3-4
	CommitmentId transferedCommitId;
	unordered_map<PartyId, CommitmentId> fkx;//coefficients combined for source
	unordered_map<PartyId, CommitmentId> gkx;//coefficients combined for target

	void addFkx(PartyId k, CommitmentId c);
	CommitmentId getFkx(PartyId k) const;
	void addGkx(PartyId k, CommitmentId c);
	CommitmentId getGkx(PartyId k) const;
	void addRejecter(PartyId p);
	bool isRejected() const;

	void print(stringstream& ss) const;
};

} /* namespace pceas */

#endif /* COMMITMENTTRANSFER_H_ */
