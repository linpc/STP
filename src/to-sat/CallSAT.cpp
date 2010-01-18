// -*- c++ -*-
/********************************************************************
 * AUTHORS: Vijay Ganesh
 *
 * BEGIN DATE: November, 2005
 *
 * LICENSE: Please view LICENSE file in the home dir of this Program
 ********************************************************************/
#include "ToSAT.h"

namespace BEEV
{
  //Bucketize clauses into buckets of size 1,2,...CLAUSAL_BUCKET_LIMIT
  static ClauseBuckets * Sort_ClauseList_IntoBuckets(ClauseList * cl)
  {
    ClauseBuckets * cb = new ClauseBuckets();
    
    //Sort the clauses, and bucketize by the size of the clauses
    for(ClauseList::iterator it = cl->begin(), itend = cl->end(); 
        it!=itend; it++)
      {
        ClausePtr cptr = *it;
        int cl_size = cptr->size();
        if(cl_size >= CLAUSAL_BUCKET_LIMIT)
          {
            cl_size = CLAUSAL_BUCKET_LIMIT;
          }
// 	else
// 	  {
// 	    cl_size = CLAUSAL_BUCKET_LIMIT-1;
// 	  }

        //If no clauses of size cl_size have been seen, then create a
        //bucket for that size
        if(cb->find(cl_size) == cb->end())
          {
            ClauseList * cllist = new ClauseList();
            cllist->push_back(cptr);
            (*cb)[cl_size] = cllist;
          }
        else
          {
            ClauseList * cllist = (*cb)[cl_size];
            cllist->push_back(cptr);
          }
      }
    
    return cb;
  } //End of SortClauseList_IntoBuckets()

  bool ToSAT::CallSAT_On_ClauseBuckets(MINISAT::Solver& SatSolver,
                                       ClauseBuckets * cb)
  {
    ClauseBuckets::iterator it = cb->begin();
    ClauseBuckets::iterator itend = cb->end();
    
    bool sat = false;
    for(int count=1;it!=itend;it++, count++)
      {
        ClauseList *cl = (*it).second;
// 	if(CLAUSAL_BUCKET_LIMIT == count)
// 	  {
// 	    sat = toSATandSolve(SatSolver,*cl, false, true);
// 	  }
// 	else
	  {
	    sat = toSATandSolve(SatSolver,*cl);
	  }
        if(!sat)
          {
            return sat;
          }
      }
    return sat;
  }

  //Call the SAT solver, and check the result before returning. This
  //can return one of 3 values, SOLVER_VALID, SOLVER_INVALID or
  //SOLVER_UNDECIDED
  bool ToSAT::CallSAT(MINISAT::Solver& SatSolver, 
                      const ASTNode& modified_input,
                      const ASTNode& original_input)
  {
    bm->GetRunTimes()->start(RunTimes::BitBlasting);
    BitBlaster BB(bm);
    ASTNode BBFormula = BB.BBForm(modified_input);
    bm->ASTNodeStats("after bitblasting: ", BBFormula);
    bm->GetRunTimes()->stop(RunTimes::BitBlasting);

    CNFMgr* cm = new CNFMgr(bm);
    ClauseList* cl = cm->convertToCNF(BBFormula);    

#if defined CRYPTOMINISAT || defined CRYPTOMINISAT2
    ClauseList* xorcl = cm->ReturnXorClauses();
#endif

    ClauseBuckets * cb = Sort_ClauseList_IntoBuckets(cl);    
    bool sat = CallSAT_On_ClauseBuckets(SatSolver, cb);    
    //bool sat = toSATandSolve(SatSolver, *cl);

    for (ClauseBuckets::iterator it = cb->begin(); it != cb->end(); it++)
    	delete it->second;
    delete cb;

    if(!sat)
      {
        cm->DELETE(cl);
        cm->DELETE(xorcl);
        delete cm;
    	return sat;
      }

#if defined CRYPTOMINISAT || defined CRYPTOMINISAT2
    if(!xorcl->empty())
      { 
        sat = toSATandSolve(SatSolver, *xorcl, true);
      }
#endif 

    cm->DELETE(cl);
    cm->DELETE(xorcl);
    delete cm;
    return sat;
  }
  
  //##################################################
  //##################################################

  /*******************************************************************
   * Helper Functions
   *******************************************************************/

  //This function prints the output of the STP solver
  void ToSAT::PrintOutput(SOLVER_RETURN_TYPE ret)
  {
    bool true_iff_valid = (SOLVER_VALID == ret);

    if (bm->UserFlags.print_output_flag)
      {
        if (bm->UserFlags.smtlib_parser_flag)
          {
            if (true_iff_valid && 
                (input_status == TO_BE_SATISFIABLE))
              {
                cerr << "Warning. Expected satisfiable,"\
                  " FOUND unsatisfiable" << endl;
              }
            else if (!true_iff_valid && 
                     (input_status == TO_BE_UNSATISFIABLE))
              {
                cerr << "Warning. Expected unsatisfiable,"\
                  " FOUND satisfiable" << endl;
              }
          }
      }

    if (true_iff_valid)
      {
        bm->ValidFlag = true;
        if (bm->UserFlags.print_output_flag)
          {
            if (bm->UserFlags.smtlib_parser_flag)
              cout << "unsat\n";
            else
              cout << "Valid.\n";
          }
      }
    else
      {
        bm->ValidFlag = false;
        if (bm->UserFlags.print_output_flag)
          {
            if (bm->UserFlags.smtlib_parser_flag)
              cout << "sat\n";
            else
              cout << "Invalid.\n";
          }
      }
  } //end of PrintOutput()
};//end of namespace
