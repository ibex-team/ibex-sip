//============================================================================
//                                  I B E X                                   
// File        : ibex_LoupFinderDefault.cpp
// Author      : Antoine Marendet, Gilles Chabert
// Copyright   : Ecole des Mines de Nantes (France)
// License     : See the LICENSE file
// Created     : May 4, 2018
// Last Update : May 4, 2018
//============================================================================

#include "ibex_Cell.h"
#include "ibex_LoupFinderSIPDefault.h"
#include "ibex_Vector.h"


using namespace std;

namespace ibex {

LoupFinderSIPDefault::LoupFinderSIPDefault(
		const SIPSystem& system) :
		system_(system) {
}

LoupFinderSIPDefault::~LoupFinderSIPDefault() {
}

std::pair<IntervalVector, double> LoupFinderSIPDefault::find(
		const Cell& cell, const IntervalVector& loup_point,
		double loup) {
    Vector loup_point_plus_goal(cell.box.mid());
	if(check(system_, loup_point_plus_goal, loup, false)) {
		return std::make_pair(loup_point_plus_goal.subvector(0, cell.box.size()-2), loup);
	}
	throw NotFound();
}

} // end namespace ibex
