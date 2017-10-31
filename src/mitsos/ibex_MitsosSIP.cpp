	/* ============================================================================
 * I B E X - ibex_MitsosSIP.cpp
 * ============================================================================
 * Copyright   : IMT Atlantique (FRANCE)
 * License     : This program can be distributed under the terms of the GNU LGPL.
 *               See the file COPYING.LESSER.
 *
 * Author(s)   : Gilles Chabert
 * Created     : Sep 18, 2017
 * ---------------------------------------------------------------------------- */

#include "ibex_MitsosSIP.h"
#include "ibex_BD_Factory.h"
#include "ibex_LLP_Factory.h"
#include "ibex_DefaultOptimizer.h"

#include <sstream>

using namespace std;

namespace ibex {

MitsosSIP::MitsosSIP(System& sys, const std::vector<const ExprSymbol*>& vars,  const std::vector<const ExprSymbol*>& params, const BitSet& is_param, bool shared_discretization) :
			SIP(sys, vars, params, is_param), p_domain(p_arg),
			trace(1), l_max(20), LBD_samples(new vector<double>[p]),
			UBD_samples(shared_discretization? LBD_samples : new vector<double>[p]),
			ORA_samples(shared_discretization? LBD_samples : new vector<double>[p]),
			shared_discretization(shared_discretization) {

	// Initialize the domains of parameters
	for (int J=0; J<p_arg; J++) {
		p_domain.set_ref(J,*new Domain(params[J]->dim));
	}

	// =========== initial sample values ==================
	for (int j=0; j<p; j++) {
		LBD_samples[j].push_back(param_init_domain[j].mid());
		if (!shared_discretization) {
			UBD_samples[j].push_back(param_init_domain[j].mid());
			ORA_samples[j].push_back(param_init_domain[j].mid());
		}
	}

}

MitsosSIP::~MitsosSIP() {
	for (int J=0; J<p_arg; J++) {
		delete &p_domain[J];
	}

	delete[] LBD_samples;

	if (!shared_discretization) {
		delete[] UBD_samples;
		delete[] ORA_samples;
	}
}

void MitsosSIP::optimize(double eps_f) {
	Vector x_LBD(sys.nb_var);
	Vector x_UBD(sys.nb_var);
	Vector x_opt(sys.nb_var);
	Vector x_ORA(sys.nb_var+1);

	double f_LBD=NEG_INFINITY;
	double f_UBD=POS_INFINITY;
	double f_RES=POS_INFINITY;

	double LBD_uplo, LBD_loup;
	double UBD_uplo, UBD_loup;
	double ORA_uplo, ORA_loup;

	double eps_g=0.1;

	double r_g=1.5;
	double r_LLP=1.5;

	// Note: we must have eps_f >> eps_LBD + eps_UBD,
	//       see Theorem 1 in Djelassi & Mitsos, 2016
	double eps_NLP=eps_f/10;
	double eps_LBD=eps_NLP;
	double eps_UBD=eps_NLP;
	double eps_ORA=eps_NLP;

	double eps_LLP=eps_NLP;

	// Enclosure of max g_i(x_UBD).
	// the upper bound will be used in the ORA problem
	// for the domain of the objective (eta)
	Interval g_max_UBD=Interval::ALL_REALS;

	if (trace>=1) {
		cout << "=========================" << endl;
		cout << "INIT : " << endl;
		cout << " eps_g=" << eps_g << endl;
		if (trace>=1) {
			cout << " eps_LLP=" << eps_LLP << endl;
		}
	}

	int iteration = 0;
	clock_t start = clock();
	double last_iter_time = 0;

	while (f_UBD - f_LBD > eps_f) {
		clock_t iter_start = clock();
		iteration++;

		// ============================== LBD ==============================
		if (trace>=1) {
			cout << "=========================" << endl;
			cout << "ITERATION : " << iteration << endl;
			cout << " * LBD" << endl;
		}

		if (!solve_LBD(eps_LBD, x_LBD, LBD_uplo, LBD_loup)) {
			cout << "   infeasible problem";
			exit(0);
		}

		if (LBD_uplo < f_LBD) {
			stringstream msg;
			msg << "[SIP]: uplo is decreasing (uplo=" << f_LBD << "  LBD_uplo=" << LBD_uplo << ")";
			ibex_warning(msg.str());

			// note: in this case, do we really have to solve LLP?
			// this may add useless parameter values
			// (the optimum x* being less "good")
			//				eps_ibex = eps_ibex/2.0;
			//				cout << "eps_ibex=" << eps_ibex << endl;
		} else {

			if (LBD_uplo > f_LBD) {
				f_LBD = LBD_uplo;
				if (trace>=1) cout << "\033[33m   f_LBD = " << f_LBD << "\033[0m" << endl;
			} else {

			}
		}

		Interval g_max_LBD=solve_LLP(true, x_LBD, eps_LLP);

		if (g_max_LBD.ub()<=0) {
			//cout << " found feasible x_LBD!! --->";
			if (LBD_loup < f_UBD) {
				//cout << " decreases loup!\n";
				f_UBD=LBD_loup;
				x_opt=x_LBD;
			} else {
				ibex_warning("[SIP]: LBD finds a SIP-feasible point that does not decrease the UB!");
				//cout << " don't decrease loup...\n";
				//eps_ibex = eps_ibex/2.0;
			}
		} else if (g_max_LBD.lb()<=0) {
			eps_LLP = g_max_LBD.diam() / r_LLP;
			if (trace>=1) cout << "   eps_LLP=" << eps_LLP << endl;
		}


		// ============================== UBD ==============================
		bool UBD_changed=true ;

		while (UBD_changed) {

			if (trace>=1) cout << " * UBD" << endl;
			while (UBD_changed) {

				UBD_changed=false;

				if (solve_UBD(eps_g, eps_UBD, x_UBD, UBD_uplo, UBD_loup)) {

					if (eps_LLP > eps_g) {
						eps_LLP = eps_g/r_g;
						if (trace>=1) cout << "   eps_LLP=" << eps_LLP << endl;
					}

					g_max_UBD=solve_LLP(false, x_UBD, eps_LLP);

					if (g_max_UBD.ub()<=0) {
						// x_UBD is feasible: update the loup
						if (UBD_loup < f_UBD) {
							f_UBD=UBD_loup;
							if (trace>=1) cout << "\033[32m   f_UBD = " << f_UBD << "\033[0m" << endl;
							x_opt=x_UBD;
							UBD_changed=true;
						}
						eps_g = eps_g/r_g;
						if (trace>=1) cout << "   eps_g=" << eps_g << endl;
					} else {
						// ====== added by chabs =============
						if (g_max_UBD.lb()<=0) {
							eps_LLP = g_max_UBD.diam() / r_LLP;
							if (trace>=1) cout << "   eps_LLP=" << eps_LLP << endl;
						}
					}
				} else {
					// if UBD is infeasible
					eps_g = eps_g/r_g;
					if (trace>=1) cout << "   infeasible" << endl;
					if (trace>=1) cout << "   eps_g=" << eps_g << endl;
				}
			}

			// ============================== ORA ==============================
			if (f_LBD==NEG_INFINITY || f_UBD==POS_INFINITY) break;

			if (trace>=1) cout << " * ORA" << endl;

			bool LBD_changed=true;
			int l=0;

			while ((l<l_max) && LBD_changed && (f_UBD - f_LBD > eps_f)) {

				l++;
				LBD_changed=false;

				double f_RES=0.5*(f_UBD + f_LBD);

				// note: the oracle is called several times
				// consecutively only if the lower bound has changed
				// so x_UBD does actually correspond to the last LLP
				// program solved (the SIP-feasibility of x_ORA has not
				// been called yet), and g_max_UBD.ub() is a valid upper
				// bound for eta.
				if (!solve_ORA(f_RES, x_LBD, -g_max_UBD.ub(), eps_ORA, x_ORA, ORA_uplo, ORA_loup)) {
					// if happens => bug
					ibex_error("[SIP]: the oracle problem is infeasible");
				}

				double eta_lb=-ORA_loup;
				double eta_ub=-ORA_uplo;

				if (eta_ub<0) {
					f_LBD = f_RES;
					x_LBD = x_ORA;
					LBD_changed = true;
					if (trace>=1) cout << "\033[33m   f_LBD = " << f_LBD << "\033[0m" << endl;
					continue;
				}

				if (eta_lb<=0) break;

				// *** eta_lb>0 ***
				// SIP-feasibility test of x_ORA
				Interval g_max_ORA=solve_LLP(false, x_ORA, eps_LLP);

				if (g_max_ORA.ub()<=0) {
					UBD_loup = sys.goal_ub(x_ORA);
					// x_ORA is feasible: update the loup
					if (UBD_loup > f_UBD) {
						// if happens => bug
						cout << "UBD_loup=" << UBD_loup;
						ibex_error("[SIP]: the oracle problem found a SIP-feasible point above the loup.");
					}

					if (eta_ub/r_g < eps_g) {
						eps_g = eta_ub/r_g;
						if (trace>=1) cout << "   eps_g=" << eps_g << endl;
					}

					f_UBD = UBD_loup;
					if (trace>=1) cout << "\033[32m   f_UBD = " << f_UBD << "\033[0m" << endl;
					x_opt = x_ORA;
					UBD_changed=true;

				} else {
					// ====== added by chabs =============
					if (g_max_ORA.lb()<=0) {
						eps_LLP = g_max_ORA.diam() / r_LLP;
						if (trace>=1) cout << "   eps_LLP=" << eps_LLP << endl;
					}
				}
			}
		}

		last_iter_time = (clock() - iter_start)/(double) CLOCKS_PER_SEC;
	}
	double time = (clock() - start)/(double) CLOCKS_PER_SEC;

	cout << endl << endl << "f* in [" << f_LBD << "," << f_UBD << "]" << endl;
	cout << endl << endl << "x*=" << x_opt << endl << endl;
	cout << endl << endl << iteration << " iterations" << endl;
	cout << time << "s (real time)" << endl;
	cout << time/iteration << "s per iteration" << endl;
	cout << "Last iteration time: " << last_iter_time << endl;
}

bool MitsosSIP::solve_LBD(double eps, Vector& x_opt, double& uplo, double& loup) {
	System LBD_sys(LBD_Factory(*this));
	return solve(LBD_sys, eps, x_opt, uplo, loup);
}

bool MitsosSIP::solve_UBD(double eps_g, double eps, Vector& x_opt, double& uplo, double& loup) {
	System UBD_sys(UBD_Factory(*this,eps_g));
	return solve(UBD_sys, eps, x_opt, uplo, loup);
}

bool MitsosSIP::solve_ORA(double f_RES, const Vector& x_LBD, double eta_ub, double eps, Vector& x_opt, double& uplo, double& loup) {
	System ORA_sys(ORA_Factory(*this,f_RES));

	// compute initial domain of eta
	IntervalVector g_x_LBD = ORA_sys.f_ctrs.eval_vector(x_LBD);
	double eta_lb=-(g_x_LBD[0].ub());
	for (int i=1; i<g_x_LBD.size(); i++)
		eta_lb=std::min(eta_lb, -g_x_LBD[i].ub());

	Interval eta_domain(eta_lb, eta_ub);

	if (eta_domain.is_unbounded()) {
		eta_domain = Interval(-1e8,1e8);
		ibex_warning("[SIP] unbounded domain for objective variable in the \"oracle\" problem (replaced by [-1e8,1e8])");
	}

	return solve(ORA_sys, eps, x_opt, uplo, loup);
}

bool MitsosSIP::solve(const System& sub_sys, double eps, Vector& x_opt, double& uplo, double& loup) {

	//DefaultOptimizer o(sub_sys,eps,eps);
	DefaultOptimizer o(sub_sys,eps,eps);

	Optimizer::Status status=o.optimize(sub_sys.box);

	//o.report();

	if (status!=Optimizer::SUCCESS && status!=Optimizer::INFEASIBLE) {
		ibex_error("[SIP] system solving failed");
	}

	uplo = o.get_uplo();
	loup = o.get_loup();
	x_opt = o.get_loup_point().lb().subvector(0,sub_sys.nb_var-1);

	return status==Optimizer::SUCCESS;
}

Interval MitsosSIP::solve_LLP(bool LBD, const Vector& x_opt, double eps) {

	cout << "   LLP: ";

	double lb=NEG_INFINITY;
	double ub=NEG_INFINITY;

	for (int c=0; c<sys.nb_ctr; c++) {
		try {
			LLP_Factory lpp_fac(*this,c,x_opt);

			System max_sys(lpp_fac);
            //cout << max_sys << endl;

			// Mitsos algorithm works with absolute precision
			DefaultOptimizer o(max_sys,0,eps);

			VarSet param_LLP_var(p, lpp_fac.param_LLP_var);

			IntervalVector param_box=param_LLP_var.var_box(param_init_domain);

			//cout << "param box=" << param_box << endl;

			Optimizer::Status status=o.optimize(param_box);
			//o.report();

			if (status!=Optimizer::SUCCESS) {
				ibex_error("LLP failed");
			}

			// Note: LLP is actually min -g_i(x)
			if (-o.get_uplo()>ub)
				ub=-o.get_uplo();

			if (-o.get_loup()>lb)
				lb=-o.get_loup();

			if (-o.get_loup()<=0) continue; // satisfied constraint

			Vector y_opt=o.get_loup_point().lb().subvector(0,max_sys.nb_var-1);
			int j2=0;
			for (int j=0; j<p; j++) {
				if (lpp_fac.param_LLP_var[j]) {
					//cout << "param n°" << j << " : add sample value " << y_opt[j2] << endl;
					if (LBD)
						LBD_samples[j].push_back(y_opt[j2++]);
					else
						UBD_samples[j].push_back(y_opt[j2++]);
				}
			}

		} catch(LLP_Factory::ParameterFreeConstraint&) {

		}
	}

	if (trace>=1) {
		if (ub<=0)
			cout << " SIP-feasible!" << endl;
		else {
			cout << " samples size=[";
			for (int j=0; j<p; j++) {
				if (j>0) cout << ' ';
				cout << (LBD? LBD_samples[j].size() : UBD_samples[j].size());
			}
			cout << "]\n";
		}
	}
	return Interval(lb,ub);
}

} // namespace ibex
