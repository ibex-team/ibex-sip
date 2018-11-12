/* ============================================================================
 * I B E X - ibex_CtcFwdBwdNLC.cpp
 * ============================================================================
 * Copyright   : IMT Atlantique (FRANCE)
 * License     : This program can be distributed under the terms of the GNU LGPL.
 *               See the file COPYING.LESSER.
 *
 * Author(s)   : Antoine Marendet, Gilles Chabert
 * Created     : May 4, 2018
 * ---------------------------------------------------------------------------- */
 
#include "ibex_CtcFwdBwdNLC.h"

#include "ibex_BitSet.h"
#include "ibex_Function.h"
#include "ibex_SIPSystem.h"
namespace ibex {

CtcFwdBwdNLC::CtcFwdBwdNLC(const NLConstraint& constraint, const SIPSystem& system)
: Ctc(constraint.function_->nb_var()), function_(constraint.function_),
backward_domain_(Interval::NEG_REALS), system_(system)
{
	init();
}

CtcFwdBwdNLC::~CtcFwdBwdNLC() {
    delete input;
    delete output;
}

void CtcFwdBwdNLC::init() {
	input = new BitSet(function_->used_vars);
	output = new BitSet(function_->used_vars);
}

void CtcFwdBwdNLC::add_property(const IntervalVector& init_box, BoxProperties& map) {
    if(map[BxpNodeData::id] == nullptr) {
        map.add(new BxpNodeData(system_.getInitialNodeCaches()));
    }
}

void CtcFwdBwdNLC::contract(IntervalVector& box) {
    ContractContext context(box);
	contract(box,context);
}

void CtcFwdBwdNLC::contract(IntervalVector& box, ContractContext& context) {
    if(function_->backward(backward_domain_, box)) {
        context.output_flags.add(INACTIVE);
        context.output_flags.add(FIXPOINT);
    }

    
    if(box.is_empty()) {
        context.output_flags.add(FIXPOINT);
    }

}
} // end namespace ibex
