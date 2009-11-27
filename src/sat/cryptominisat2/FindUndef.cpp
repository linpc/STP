#include "FindUndef.h"

#include "Solver.h"
#include <algorithm>

namespace MINISAT
{

FindUndef::FindUndef(Solver& _S) :
    S(_S)
    , isPotentialSum(0)
{
    dontLookAtClause.resize(S.clauses.size(), false);
    isPotential.resize(S.nVars(), false);
    fillPotential();
    satisfies.resize(S.nVars(), 0);
}

void FindUndef::fillPotential()
{
    int trail = S.decisionLevel()-1;
    
    while(trail > 0) {
        assert(trail < S.trail_lim.size());
        uint at = S.trail_lim[trail];
        
        assert(at > 0);
        Var v = S.trail[at].var();
        isPotential[v] = true;
        isPotentialSum++;
        
        trail--;
    }
    
    for (XorClause** it = S.xorclauses.getData(), **end = it + S.xorclauses.size(); it != end; it++) {
        XorClause& c = **it;
        for (Lit *l = c.getData(), *end = l + c.size(); l != end; l++) {
            if (isPotential[l->var()]) {
                isPotential[l->var()] = false;
                isPotentialSum--;
            }
            assert(!S.value(*l).isUndef());
        }
    }
    
    vector<Var> replacingVars = S.toReplace->getReplacingVars();
    for (Var *it = &replacingVars[0], *end = it + replacingVars.size(); it != end; it++) {
        if (isPotential[*it]) {
            isPotential[*it] = false;
            isPotentialSum--;
        }
    }
}

void FindUndef::unboundIsPotentials()
{
    for (uint i = 0; i < isPotential.size(); i++)
        if (isPotential[i])
            S.assigns[i] = l_Undef;
}

const uint FindUndef::unRoll()
{
    while(!updateTables()) {
        assert(isPotentialSum > 0);
        
        uint32_t maximum = 0;
        Var v;
        for (uint i = 0; i < isPotential.size(); i++) {
            if (isPotential[i] && satisfies[i] >= maximum) {
                maximum = satisfies[i];
                v = i;
            }
        }
        
        isPotential[v] = false;
        isPotentialSum--;
        
        std::fill(satisfies.begin(), satisfies.end(), 0);
    }
    
    unboundIsPotentials();
    
    return isPotentialSum;
}

bool FindUndef::updateTables()
{
    bool allSat = true;
    
    uint i = 0;
    for (Clause** it = S.clauses.getData(), **end = it + S.clauses.size(); it != end; it++, i++) {
        if (dontLookAtClause[i])
            continue;
        
        Clause& c = **it;
        bool definitelyOK = false;
        Var v;
        uint numTrue = 0;
        for (Lit *l = c.getData(), *end = l + c.size(); l != end; l++) {
            if (S.value(*l) == l_True) {
                if (!isPotential[l->var()]) {
                    dontLookAtClause[i] = true;
                    definitelyOK = true;
                    break;
                } else {
                    numTrue ++;
                    v = l->var();
                }
            }
        }
        if (definitelyOK)
            continue;
        
        if (numTrue == 1) {
            isPotential[v] = false;
            isPotentialSum--;
            dontLookAtClause[i] = true;
            continue;
        }
        
        allSat = false;
        for (Lit *l = c.getData(), *end = l + c.size(); l != end; l++) {
            if (S.value(*l) == l_True)
                satisfies[l->var()]++;
        }
    }
    
    return allSat;
}
};
