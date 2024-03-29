/*
 * Use the Bitblasted encoding as a propagator.
 */

#ifndef BBASPROP_H_
#define BBASPROP_H_

#include "../sat/core/Solver.h"


class BBAsProp
{
public:

  SATSolver::vec_literals assumptions;
  ToSATAIG aig;
  MinisatCore<Minisat::Solver>* ss;
  ASTNode i0, i1,r;
  ToSAT::ASTNodeToSATVar m;

  BBAsProp(Kind k, STPMgr* mgr, int bits):
  aig(mgr, GlobalSTP->arrayTransformer)
  {

    const bool term = is_Term_kind(k);

    i0 = mgr->CreateSymbol("i0", 0, bits);
    i1 = mgr->CreateSymbol("i1", 0, bits);

    ASTNode p, eq;

    if (term)
      {
        p = mgr->CreateTerm(k, bits, i0, i1);
        r = mgr->CreateSymbol("r", 0, bits);
        eq = mgr->CreateNode(EQ, p, r);
      }
    else
      {
        p = mgr->CreateNode(k, i0, i1);
        r = mgr->CreateSymbol("r", 0, 0);
        eq = mgr->CreateNode(IFF, p, r);
      }

    ss = new MinisatCore<Minisat::Solver>(mgr->soft_timeout_expired);

    aig.CallSAT(*ss, eq, false);
    m = aig.SATVar_to_SymbolIndexMap();
  }

  bool
  unitPropagate()
  {
    return ss->unitPropagate(assumptions);
  }

  void
  toAssumptions(FixedBits& a, FixedBits& b, FixedBits& output)
  {
    assumptions.clear();
    int bits = a.getWidth();

    for (int i = 0; i < bits; i++)
      {
        if (a[i] == '1')
          {
            assumptions.push(SATSolver::mkLit(m.find(i0)->second[i], false));
          }
        else if (a[i] == '0')
          {
            assumptions.push(SATSolver::mkLit(m.find(i0)->second[i], true));
          }

        if (b[i] == '1')
          {
            assumptions.push(SATSolver::mkLit(m.find(i1)->second[i], false));
          }
        else if (b[i] == '0')
          {
            assumptions.push(SATSolver::mkLit(m.find(i1)->second[i], true));
          }
      }
    for (int i = 0; i < output.getWidth(); i++)
      {
        if (output[i] == '1')
          {
            assumptions.push(SATSolver::mkLit(m.find(r)->second[i], false));
          }
        else if (output[i] == '0')
          {
            assumptions.push(SATSolver::mkLit(m.find(r)->second[i], true));
          }
      }
  }

  int
  fixedCount()
  {
    return addToClauseCount(i0) +addToClauseCount(i1) + addToClauseCount(r);
  }

  int
  addToClauseCount(const ASTNode n)
  {
    int result =0;
    const int bits = std::max(1U, n.GetValueWidth());
    for (int i = bits - 1; i >= 0; i--)
      {
        SATSolver::lbool r = ss->value(m.find(n)->second[i]);

        if (r == ss->true_literal() || r == ss->false_literal())
          result++;
      }
    return result;
  }


};

#endif /* BBASPROP_H_ */
