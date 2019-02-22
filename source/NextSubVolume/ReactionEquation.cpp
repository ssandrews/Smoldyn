/*
 * ReactionEquation.cpp
 *
 *  Created on: 9 Nov 2012
 *      Author: robinsonm
 */

#include "ReactionEquation.h"

namespace Kairos {
ReactionComponent operator*(const int mult, Species& s) {
	return ReactionComponent(mult,s,0);
}

ReactionSide operator+(const ReactionComponent& arg1, const ReactionComponent& arg2) {
	return ReactionSide(arg1,arg2);
}

ReactionSide operator+(Species& arg1, const ReactionComponent& arg2) {
	return ReactionSide(ReactionComponent(1,arg1,0),arg2);
}

ReactionSide operator+(const ReactionComponent& arg1, Species& arg2) {
	return ReactionSide(arg1,ReactionComponent(1,arg2,0));
}

ReactionSide operator+(Species& arg1, Species& arg2) {
	return ReactionSide(ReactionComponent(1,arg1,0),ReactionComponent(1,arg2,0));
}

ReactionSide& operator+(ReactionSide& side, const ReactionComponent& comp) {
	side.push_back(comp);
	return side;
}

ReactionSide& operator+(ReactionSide& side, Species& s) {
	side.push_back(ReactionComponent(1,s,0));
	return side;
}

ReactionEquation operator>>(const ReactionSide& lhs, const ReactionSide& rhs) {
	ReactionSide* newlhs = new ReactionSide(lhs);
	ReactionSide* newrhs = new ReactionSide(rhs);
	return ReactionEquation(*newlhs,*newrhs);
}

ReactionEquation operator>>(const ReactionSide& lhs, const ReactionComponent& rhs) {
	ReactionSide* newlhs = new ReactionSide(lhs);
	ReactionSide* newrhs = new ReactionSide(rhs);
	return ReactionEquation(*newlhs,*newrhs);
}

ReactionEquation operator>>(const ReactionSide& lhs, Species& rhs) {
	ReactionSide* newlhs = new ReactionSide(lhs);
	ReactionSide* newrhs = new ReactionSide(ReactionComponent(1,rhs,0));
	return ReactionEquation(*newlhs,*newrhs);
}

ReactionEquation operator>>(const ReactionSide& lhs, const int rhs) {
	ASSERT(rhs==0,"null species is always 0");
	ReactionSide* newlhs = new ReactionSide(lhs);
	ReactionSide* newrhs = new ReactionSide();
	return ReactionEquation(*newlhs,*newrhs);
}

ReactionEquation operator>>(const ReactionComponent& lhs, const ReactionSide& rhs) {
	ReactionSide* newlhs = new ReactionSide(lhs);
	ReactionSide* newrhs = new ReactionSide(rhs);
	return ReactionEquation(*newlhs,*newrhs);
}

ReactionEquation operator>>(const ReactionComponent& lhs, const ReactionComponent& rhs) {
	ReactionSide* newlhs = new ReactionSide(lhs);
	ReactionSide* newrhs = new ReactionSide(rhs);
	return ReactionEquation(*newlhs,*newrhs);
}
ReactionEquation operator>>(const ReactionComponent& lhs, Species& rhs) {
	ReactionSide* newlhs = new ReactionSide(lhs);
	ReactionSide* newrhs = new ReactionSide(ReactionComponent(1,rhs,0));
	return ReactionEquation(*newlhs,*newrhs);
}
ReactionEquation operator>>(const ReactionComponent& lhs, const int rhs) {
	ASSERT(rhs==0,"null species is always 0");
	ReactionSide* newlhs = new ReactionSide(lhs);
	ReactionSide* newrhs = new ReactionSide();
	return ReactionEquation(*newlhs,*newrhs);
}

ReactionEquation operator>>(Species& lhs, const ReactionSide& rhs) {
	ReactionSide* newlhs = new ReactionSide(ReactionComponent(1,lhs,0));
	ReactionSide* newrhs = new ReactionSide(rhs);
	return ReactionEquation(*newlhs,*newrhs);
}

ReactionEquation operator>>(Species& lhs, const ReactionComponent& rhs) {
	ReactionSide* newlhs = new ReactionSide(ReactionComponent(1,lhs,0));
	ReactionSide* newrhs = new ReactionSide(rhs);
	return ReactionEquation(*newlhs,*newrhs);
}
ReactionEquation operator>>(Species& lhs, Species& rhs) {
	ReactionSide* newlhs = new ReactionSide(ReactionComponent(1,lhs,0));
	ReactionSide* newrhs = new ReactionSide(ReactionComponent(1,rhs,0));
	return ReactionEquation(*newlhs,*newrhs);
}
ReactionEquation operator>>(Species& lhs, const int rhs) {
	ASSERT(rhs==0,"null species is always 0");
	ReactionSide* newlhs = new ReactionSide(ReactionComponent(1,lhs,0));
	ReactionSide* newrhs = new ReactionSide();
	return ReactionEquation(*newlhs,*newrhs);
}

ReactionEquation operator>>(const int lhs, const ReactionSide& rhs) {
	ASSERT(lhs==0,"null species is always 0");
	ReactionSide* newlhs = new ReactionSide();
	ReactionSide* newrhs = new ReactionSide(rhs);
	return ReactionEquation(*newlhs,*newrhs);
}

ReactionEquation operator>>(const int lhs, const ReactionComponent& rhs) {
	ASSERT(lhs==0,"null species is always 0");
	ReactionSide* newlhs = new ReactionSide();
	ReactionSide* newrhs = new ReactionSide(rhs);
	return ReactionEquation(*newlhs,*newrhs);
}
ReactionEquation operator>>(const int lhs, Species& rhs) {
	ASSERT(lhs==0,"null species is always 0");
	ReactionSide* newlhs = new ReactionSide();
	ReactionSide* newrhs = new ReactionSide(ReactionComponent(1,rhs,0));
	return ReactionEquation(*newlhs,*newrhs);
}

std::ostream& operator<< (std::ostream& out, const ReactionSide &side) {
	const int n = side.size();
	for (int i = 0; i < n; ++i) {
		out << side[i].multiplier << "("<<side[i].species->id<<")";
		if (i != n-1) {
			out << " + ";
		}
	}
	return out;
}
std::ostream& operator<< (std::ostream& out, const ReactionEquation &eq) {
	return out << eq.lhs << " >> " << eq.rhs;
}
}
