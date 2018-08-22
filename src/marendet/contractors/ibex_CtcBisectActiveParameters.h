//============================================================================
//                                  I B E X                                   
// File        : ibex_CtcBisectActiveParameters.h
// Author      : Antoine Marendet
// Copyright   : Ecole des Mines de Nantes (France)
// License     : See the LICENSE file
// Created     : July 11, 2018
// Last Update : July 11, 2018
//============================================================================

#ifndef __SIP_IBEX_CTC_BISECT_ACTIVE_PARAMETERS_H_
#define __SIP_IBEX_CTC_BISECT_ACTIVE_PARAMETERS_H_

#include "ibex_CellCtc.h"
#include "ibex_RelaxationLinearizerSIP.h"

namespace ibex {

class CtcBisectActiveParameters : public CellCtc
{
private:
    const SIPSystem& sys_;
    RelaxationLinearizerSIP linearizer_;
    LPSolver lp_solver_;
public:
    CtcBisectActiveParameters(const SIPSystem& system);
    virtual ~CtcBisectActiveParameters();

    void contractCell(Cell& cell);
};

}  // namespace ibex



#endif  // __SIP_IBEX_CTC_BISECT_ACTIVE_PARAMETERS_H_
