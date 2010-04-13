#ifndef SAT_H_
#define SAT_H_

#ifdef CRYPTOMINISAT
#include "cryptominisat/Solver.h"
#include "cryptominisat/SolverTypes.h"
#endif

#ifdef CRYPTOMINISAT2
#include "cryptominisat2/Solver.h"
#include "cryptominisat2/SolverTypes.h"
#endif

#ifdef CORE
#include "core/Solver.h"
#include "core/SolverTypes.h"
#include "simp/SimpSolver.h"
#endif

#endif
