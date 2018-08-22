//============================================================================
//                                  I B E X                                   
// File        : ibex_SIPOptimizer.h
// Author      : Antoine Marendet, Gilles Chabert
// Copyright   : Ecole des Mines de Nantes (France)
// License     : See the LICENSE file
// Created     : May 4, 2018
// Last Update : May 4, 2018
//============================================================================

#ifndef __SIP_IBEX_SIPOPTIMIZER_H__
#define __SIP_IBEX_SIPOPTIMIZER_H__

#include "ibex_CellCtc.h"
#include "ibex_LoupFinderSIP.h"

#include "ibex_Bsc.h"
#include "ibex_Cell.h"
#include "ibex_CellBufferOptim.h"
#include "ibex_Interval.h"
#include "ibex_IntervalVector.h"

#include <iostream>

namespace ibex {
class SIPOptimizer {

public:
	enum Status {
		success,
		timeout,
		infeasible,
		no_feasible_found,
		unbounded_obj,
		unreached_prec
	};

	static const double default_rel_eps_f;
	static const double default_abs_eps_f;
	static const double default_lf_loop_ratio;


	SIPOptimizer(CellCtc& ctc, Bsc& bisector,
			LoupFinderSIP& loup_finder, LoupFinderSIP* loup_finder2, CellBufferOptim& buffer,
			double abs_eps, double rel_eps, double timeout = -1, double eps_x = 0, int maxiter_ = -1, double lf_loop_ratio=default_lf_loop_ratio);

	SIPOptimizer::Status minimize(const IntervalVector& box, double obj_init_bound =
	POS_INFINITY);

	void report(bool verbose = true);
	double get_loup() const;
	double get_uplo() const;
	SIPOptimizer::Status get_status() const;
	IntervalVector get_loup_point() const;
	double get_time() const;
	double get_nb_cells() const;
	double get_obj_rel_prec() const;
	double get_obj_abs_prec() const;

	int trace_ = 0;
	double timeout_;

private:
	double compute_ymax();
	void handleCell(Cell& cell);
	void updateUplo();
	bool updateLoup(const Cell& cell);
	bool updateLoup2(const Cell& cell);

	void updateUploEpsboxes(double ymin);

	CellCtc& ctc_;
	Bsc& bisector_;
	LoupFinderSIP& loup_finder_;
	LoupFinderSIP* loup_finder2_;
	CellBufferOptim& buffer_;

	const double obj_rel_prec_f_;
	const double obj_abs_prec_f_;
	double eps_x_;
	int maxiter_;
	double lf_loop_ratio_;

	SIPOptimizer::Status status_ = SIPOptimizer::Status::success;
	double uplo_ = NEG_INFINITY;
	double loup_ = POS_INFINITY;
	double initial_loup_ = POS_INFINITY;
	double uplo_epsboxes = POS_INFINITY;
	double time_ = 0;
	IntervalVector loup_point_ = IntervalVector(1);
	int nb_cells_ = 0;
	bool loup_changed_ = false;
	int n = 0; // dimension of search space

};

inline
    std::ostream& operator<<(std::ostream& os, SIPOptimizer::Status status) {
        switch(status) {
        case SIPOptimizer::Status::success:
            os << "SUCCESS";
            break;
        case SIPOptimizer::Status::timeout:
            os << "TIMEOUT";
            break;
        case SIPOptimizer::Status::infeasible:
            os << "INFEASIBLE";
            break;
        case SIPOptimizer::Status::no_feasible_found:
            os << "NO FEASIBLE POINT FOUND";
            break;
        case SIPOptimizer::Status::unreached_prec:
            os << "UNREACHED PREC";
            break;
        case SIPOptimizer::Status::unbounded_obj:
            os << "UBOUNDED OBJ";
            break;
        }
        return os;
    }

} // end namespace ibex

#endif // __SIP_IBEX_SIPOPTIMIZER_H__
