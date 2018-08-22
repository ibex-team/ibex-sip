//============================================================================
//                                  I B E X                                   
// File        : ibex_utils.cpp
// Author      : Antoine Marendet, Gilles Chabert
// Copyright   : Ecole des Mines de Nantes (France)
// License     : See the LICENSE file
// Created     : May 4, 2018
// Last Update : May 4, 2018
//============================================================================

#include "ibex_utils.h"

#include "ibex_Interval.h"

#include <cmath>
#include <utility>

using namespace std;

namespace ibex {

Interval centeredFormEval(const Function& function, const IntervalVector& arg) {
	/*Interval natural_extension = function.eval(arg);
	Interval centered_form = function.eval(arg.mid()) + function.gradient(arg) * (arg - arg.mid());*/
	IntervalVector new_arg_ub(arg);
	IntervalVector new_arg_lb(arg);
	IntervalVector grad = function.gradient(arg);
	for(int i = 0; i < arg.size(); ++i) {
		if(grad[i].lb() > 0) {
			new_arg_ub[i] = arg[i].ub();
			new_arg_lb[i] = arg[i].lb();
		} else if(grad[i].ub() < 0) {
			new_arg_ub[i] = arg[i].lb();
			new_arg_lb[i] = arg[i].ub();
		}
	}
	Interval natural_extension = function.eval(arg);
	Interval centered_form = function.eval(arg.mid()) + grad * (arg - arg.mid());
	Interval ub_form = function.eval(arg.ub()) + grad * (arg - arg.ub());
	Interval lb_form = function.eval(arg.lb()) + grad * (arg - arg.lb());
	Interval res = natural_extension & centered_form & ub_form & lb_form;
	res &= Interval(function.eval(new_arg_lb).lb(), function.eval(new_arg_ub).ub());
	return res;
}

std::string print_mma(const Vector& iv) {
	std::string res = "{";
	for(int i = 0; i < iv.size()-1; ++i) {
		res += std::to_string(iv[i]) + ", ";
	}
	res += std::to_string(iv[iv.size()-1]) + "}";
	return res;
}

std::string print_mma(const IntervalVector& iv) {
	std::string res = "{";
	for(int i = 0; i < iv.size()-1; ++i) {
		res += "{" + std::to_string(iv[i].lb()) + ", " + std::to_string(iv[i].ub()) + "}, ";
	}
	res += "{" + std::to_string(iv[iv.size()-1].lb()) + ", " + std::to_string(iv[iv.size()-1].ub()) + "}}";
	return res;
}

bool isfinite(const Vector& v) {
	for (int i = 0; i < v.size(); ++i) {
		if (!std::isfinite(v[i]))
			return false;
	}
	return true;
}

std::vector<IntervalVector> bisectAllDim(const IntervalVector& iv) {
	vector<IntervalVector> res;
	res.emplace_back(iv);
	for (int i = 0; i < iv.size(); ++i) {
		const int res_size = res.size();
		for (int j = 0; j < res_size; ++j) {
			if (res[j][i].is_bisectable()) {
				auto pair = res[j].bisect(i);
				res[j] = pair.first;
				res.emplace_back(pair.second);
			}
		}
	}
	return res;
}
} // end namespace ibex
