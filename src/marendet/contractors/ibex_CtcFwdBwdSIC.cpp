//============================================================================
//                                  I B E X                                   
// File        : ibex_CtcFwdBwdSIC.cpp
// Author      : Antoine Marendet, Gilles Chabert
// Copyright   : Ecole des Mines de Nantes (France)
// License     : See the LICENSE file
// Created     : May 4, 2018
// Last Update : May 4, 2018
//============================================================================

#include "ibex_CtcFwdBwdSIC.h"

#include "system/ibex_SIConstraint.h"
#include "system/ibex_SIConstraintCache.h"

#include "ibex_BitSet.h"
#include "ibex_Function.h"
#include "ibex_Vector.h"

using namespace std;

namespace ibex {

CtcFwdBwdSIC::CtcFwdBwdSIC(const SIConstraint& constraint) :
		Ctc(constraint.variable_count_), constraint_(constraint), backward_domain_(Interval::NEG_REALS) {
	init();
}

CtcFwdBwdSIC::~CtcFwdBwdSIC() {
	delete input;
	delete output;
}

void CtcFwdBwdSIC::init() {

	input = new BitSet(nb_var);
	output = new BitSet(nb_var);
	for (BitSet::const_iterator it=constraint_.function_->used_vars.begin(); it!=constraint_.function_->used_vars.end(); it++) {
		if (it < nb_var) {
			output->add(it);
			input->add(it);
		}
	}
}

void CtcFwdBwdSIC::contract(IntervalVector &box) {
	IntervalVector full_box(constraint_.function_->nb_var());
	full_box.put(0, box);
	bool fixpoint = true;
	for (const auto& parameter_box : constraint_.cache_->parameter_caches_) {
		full_box.put(nb_var, parameter_box.parameter_box.mid());
		if (!constraint_.function_->backward(backward_domain_, full_box)) {
			fixpoint = false;
		}
		if (full_box.is_empty())
			break;
	}
	if (!full_box.is_empty()) {
		for (const auto& box : constraint_.cache_->best_blankenship_points_) {
			full_box.put(nb_var, box);
			if (!constraint_.function_->backward(backward_domain_, full_box)) {
				fixpoint = false;
			}
			if (full_box.is_empty())
				break;
			/*if (!constraint_.function_->backward(backward_domain_, full_box)) {
				fixpoint = false;
			}
			if (full_box.is_empty())
				break;*/
		}
	}

	box = full_box.subvector(0, nb_var - 1);
	//if(full_box.is_empty())
	//	box.set_empty();
	if (fixpoint) {
		set_flag(FIXPOINT);
		set_flag(INACTIVE);
	}

	if (box.is_empty()) {
		set_flag(FIXPOINT);
	}

}
} // end namespace ibex
