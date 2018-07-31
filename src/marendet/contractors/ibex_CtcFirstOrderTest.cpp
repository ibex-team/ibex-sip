//============================================================================
//                                  I B E X                                   
// File        : ibex_CtcFirstOrderTest.cpp
// Author      : Antoine Marendet, Gilles Chabert
// Copyright   : Ecole des Mines de Nantes (France)
// License     : See the LICENSE file
// Created     : May 4, 2018
// Last Update : May 4, 2018
//============================================================================

#include "ibex_CtcFirstOrderTest.h"

#include "ibex_Cell.h"
#include "ibex_Function.h"
#include "ibex_IntervalMatrix.h"
#include "ibex_IntervalVector.h"
#include "ibex_Linear.h"
#include "ibex_LinearException.h"
#include "ibex_Vector.h"

#include <vector>

using namespace std;

namespace ibex {

CtcFirstOrderTest::CtcFirstOrderTest(const SIPSystem& system) :
		CellCtc(system.ext_nb_var), system_(system) {

}

CtcFirstOrderTest::~CtcFirstOrderTest() {
}

void CtcFirstOrderTest::contractCell(Cell& cell) {
	if(cell.box.size() == 2) {
		return;
	}
	vector<IntervalVector> gradients;
	for (int i = 0; i < system_.normal_constraints_.size() - 1; ++i) {
		if (!system_.normal_constraints_[i].isSatisfied(cell.box)) {
			gradients.push_back(system_.normal_constraints_[i].gradient(cell.box));
		}
	}
	for (int i = 0; i < system_.sic_constraints_.size(); ++i) {
		if (!system_.sic_constraints_[i].isSatisfied(cell.box)) {
			gradients.push_back(system_.sic_constraints_[i].gradient(cell.box));
		}
	}

	// Without the goal variable
	IntervalMatrix matrix(nb_var - 1, gradients.size() + 1);
	matrix.set_col(0, system_.goal_function_->gradient(cell.box.subvector(0, nb_var - 2)));
	for (int i = 0; i < gradients.size(); ++i) {
		matrix.set_col(i + 1, gradients[i].subvector(0, nb_var - 2));
	}
	bool testfailed = true;
	if (matrix.nb_cols() == 1) {
		if (matrix.col(0).contains(Vector::zeros(nb_var - 2))) {
			testfailed = false;
		}
	} else {
		int* pr = new int[matrix.nb_rows()];
		int* pc = new int[matrix.nb_cols()];
		IntervalMatrix LU(matrix.nb_rows(), matrix.nb_cols());
		testfailed = true;
		try {
			interval_LU(matrix, LU, pr, pc);
		} catch(SingularMatrixException&) {
			testfailed = false;
		}
		delete[] pr;
		delete[] pc;
	}
	if(testfailed) {
		cell.box.set_empty();
	}

}
} // end namespace ibex
