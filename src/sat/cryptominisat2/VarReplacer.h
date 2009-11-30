#ifndef VARREPLACER_H
#define VARREPLACER_H

#include "SolverTypes.h"
#include "Clause.h"
#include "mtl/Vec.h"

#include <sys/types.h>
#include <map>
#include <vector>

namespace MINISAT
{

using std::map;
using std::vector;

class Solver;

class VarReplacer
{
    public:
        VarReplacer(Solver* S);
        void replace(const Var var, Lit lit);
        void extendModel() const;
        void performReplace();
        const uint getNumReplacedLits() const;
        const uint getNumReplacedVars() const;
        const vector<Var> getReplacingVars() const;
        void newVar();
    
    private:
        void replace_set(vec<Clause*>& set);
        void replace_set(vec<XorClause*>& cs, const bool need_reattach);
        bool handleUpdatedClause(Clause& c, const Lit origLit1, const Lit origLit2);
        
        void setAllThatPointsHereTo(const Var var, const Lit lit);
        bool alreadyIn(const Var var, const Lit lit);
        
        vector<Lit> table;
        map<Var, vector<Var> > reverseTable;
        
        uint replacedLits;
        uint replacedVars;
        Solver* S;
};
};
#endif //VARREPLACER_H
