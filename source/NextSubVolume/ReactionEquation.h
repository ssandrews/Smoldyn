/*
 * ReactionEquation.h
 *
 * Copyright 2012 Martin Robinson
 *
  * This file is part of RD_3D.
 *
 * RD_3D is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * RD_3D is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with RD_3D.  If not, see <http://www.gnu.org/licenses/>.
 *
 *  Created on: 9 Nov 2012
 *      Author: robinsonm
 */

#ifndef REACTIONEQUATION_H_
#define REACTIONEQUATION_H_

#include "Species.h"
#include <limits>

namespace Kairos {
struct ReactionComponent {
	ReactionComponent(int multiplier, Species& species_ref, int compartment_index):
		multiplier(multiplier),
		species(&species_ref),
		compartment_index(compartment_index) {}
	ReactionComponent(const ReactionComponent& arg):
		multiplier(arg.multiplier),
		species(arg.species),
		compartment_index(arg.compartment_index),
		tmp(arg.tmp){}

	bool operator==(const ReactionComponent& arg) const {
		return (multiplier == arg.multiplier) &&
				(species == arg.species) &&
				(compartment_index == arg.compartment_index);
	}
	bool operator<(const ReactionComponent& arg) const {
		return species < arg.species;
	}
	bool operator>(const ReactionComponent& arg) const {
		return species > arg.species;
	}
	int multiplier;
	Species* species;
	int compartment_index;
	double tmp;
};

class ReactionSide: public std::vector<ReactionComponent> {
public:
	ReactionSide() {}
	ReactionSide(const ReactionComponent& arg1, const ReactionComponent& arg2) {
		this->push_back(arg1);
		this->push_back(arg2);
	}
	ReactionSide(const ReactionComponent& arg) {
		this->push_back(arg);
	}
	~ReactionSide() {
		this->clear();
	}
	void set_compartment_index(const int i) {
		//for (auto& rc : *this) {
		for (std::vector<ReactionComponent>::iterator rc=this->begin();rc!=this->end();rc++) {
            //preserve negative compartment indicies
            if (rc->compartment_index < 0) {
                if (i == 0) {
                    rc->compartment_index = -std::numeric_limits<int>::max();
                } else  {
			        rc->compartment_index = -i;
                }
            } else {
			    rc->compartment_index = i;
            }
		}
	}
	int get_num_reactants() {
		int num_reactants = 0;
		//for (auto& rc : *this) {
		for (std::vector<ReactionComponent>::iterator rc=this->begin();rc!=this->end();rc++) {
			//num_reactants += rc.multiplier;
			num_reactants += rc->multiplier;
		}
		return num_reactants;
	}
};

struct ReactionEquation {
	void react() {
		//for (auto& rc : lhs) {
		for (std::vector<ReactionComponent>::iterator rc=lhs.begin();rc!=lhs.end();rc++) {
			//rc.species->copy_numbers[rc.compartment_index] -= rc.multiplier;
			//ASSERT(rc.species->copy_numbers[rc.compartment_index] < 0, "After reaction, copy number in compartment is less than zero!!!");
			rc->species->copy_numbers[rc->compartment_index] -= rc->multiplier;
			ASSERT(rc->species->copy_numbers[rc->compartment_index] < 0, "After reaction, copy number in compartment is less than zero!!!");
		}
		//for (auto& rc : rhs) {
		for (std::vector<ReactionComponent>::iterator rc=rhs.begin();rc!=rhs.end();rc++) {
			rc->species->copy_numbers[rc->compartment_index] += rc->multiplier;
		}
	}

	ReactionEquation(ReactionSide& lhs, ReactionSide& rhs):
		lhs(lhs),rhs(rhs) {}
	ReactionSide& lhs;
	ReactionSide& rhs;
};

std::ostream& operator<< (std::ostream& out, const ReactionSide &side);
std::ostream& operator<< (std::ostream& out, const ReactionEquation &eq);

ReactionComponent operator*(const int mult, Species& s);
ReactionSide operator+(const ReactionComponent& arg1, const ReactionComponent& arg2);
ReactionSide operator+(Species& arg1, const ReactionComponent& arg2);
ReactionSide operator+(const ReactionComponent& arg1, Species& arg2);
ReactionSide operator+(Species& arg1, Species& arg2);
ReactionSide& operator+(ReactionSide& side, const ReactionComponent& comp);
ReactionSide& operator+(ReactionSide& side, Species& s);
ReactionEquation operator>>(const ReactionSide& lhs, const ReactionSide& rhs);
ReactionEquation operator>>(const ReactionSide& lhs, const ReactionComponent& rhs);
ReactionEquation operator>>(const ReactionSide& lhs, Species& rhs);
ReactionEquation operator>>(const ReactionSide& lhs, const int rhs);
ReactionEquation operator>>(const ReactionComponent& lhs, const ReactionSide& rhs);
ReactionEquation operator>>(const ReactionComponent& lhs, const ReactionComponent& rhs);
ReactionEquation operator>>(const ReactionComponent& lhs, Species& rhs);
ReactionEquation operator>>(const ReactionComponent& lhs, const int rhs);
ReactionEquation operator>>(Species& lhs, const ReactionSide& rhs);
ReactionEquation operator>>(Species& lhs, const ReactionComponent& rhs);
ReactionEquation operator>>(Species& lhs, Species& rhs);
ReactionEquation operator>>(Species& lhs, const int rhs);
ReactionEquation operator>>(const int lhs, const ReactionSide& rhs);
ReactionEquation operator>>(const int lhs, const ReactionComponent& rhs);
ReactionEquation operator>>(const int lhs, Species& rhs);

}
#endif /* REACTIONEQUATION_H_ */
