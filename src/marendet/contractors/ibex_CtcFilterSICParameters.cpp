//============================================================================
//                                  I B E X                                   
// File        : ibex_CtcFilterSICParameters.cpp
// Author      : Antoine Marendet, Gilles Chabert
// Copyright   : Ecole des Mines de Nantes (France)
// License     : See the LICENSE file
// Created     : May 4, 2018
// Last Update : May 4, 2018
//============================================================================

#include "ibex_CtcFilterSICParameters.h"

#include "system/ibex_SIConstraintCache.h"

#include "ibex_Cell.h"
#include "ibex_Function.h"
#include "ibex_Interval.h"
#include "ibex_SICPaving.h"

#include <algorithm>
#include <iterator>
#include <vector>

namespace ibex {

CtcFilterSICParameters::CtcFilterSICParameters(const SIPSystem& system) :
		CellCtc(system.ext_nb_var), system_(system) {

}

CtcFilterSICParameters::~CtcFilterSICParameters() {
}

void CtcFilterSICParameters::contractCell(Cell& cell) {
	NodeData& node_data = cell.get<NodeData>();
	for (int i = 0; i < system_.sic_constraints_.size(); ++i) {
		//node_data.sic_constraints_caches[i].update_cache(*system_.sic_constraints_[i].function_, cell.box);
		//contractOneConstraint(i, node_data, cell.box);
		simplify_paving(system_.sic_constraints_[i], node_data.sic_constraints_caches[i], cell.box, true);
	}
}

void CtcFilterSICParameters::contractOneConstraint(size_t i, NodeData& node_data, const IntervalVector& box) {
	auto& list = node_data.sic_constraints_caches[i].parameter_caches_;
	auto it = list.begin();
	IntervalVector paramBoxUnion = node_data.sic_constraints_caches[i].initial_box_;
	sort(list.begin(), list.end(), [](const ParameterEvaluationsCache& p1, const ParameterEvaluationsCache& p2) {
		return p1.evaluation.ub() > p2.evaluation.ub();
	});
	while (it != list.end()) {
		bool keepInVector = true;
		for (int paramCount = 0; paramCount < it->parameter_box.size(); ++paramCount) {
			int paramIndex = paramCount + system_.ext_nb_var;
			if (it->full_gradient[paramIndex].lb() > 0) {
				it->parameter_box[paramCount] = it->parameter_box[paramCount].ub();
				if (it->parameter_box[paramCount].ub() < paramBoxUnion[paramCount].ub()) {
					keepInVector = false;
				} else {
					// TODO Maybe update only at the end
					//_updateMemoryBox(*it);
				}
			} else if (it->full_gradient[paramIndex].ub() < 0) {
				it->parameter_box[paramCount] = it->parameter_box[paramCount].lb();
				if (it->parameter_box[paramCount].lb() > paramBoxUnion[paramCount].lb()) {
					keepInVector = false;
				} else {
					//_updateMemoryBox(*it);
				}
			}
		}
		if (!keepInVector) {
			it = list.erase(it);
		} else {
			//_updateMemoryBox(*it);
			++it;
		}
	}
	it = list.begin();
	while (it != list.end()) {
		if (system_.sic_constraints_[i].evaluate(box, it->parameter_box).ub() <= 0) {
			it = list.erase(it);
		} else {
			it++;
		}
	}
	sort(list.begin(), list.end(), [](const ParameterEvaluationsCache& p1, const ParameterEvaluationsCache& p2) {
		return p1.evaluation.ub() > p2.evaluation.ub();
	});
}
} // end namespace ibex
