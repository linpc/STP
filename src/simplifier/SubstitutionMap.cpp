/*
 * substitutionMap.cpp
 *
 */

#include "SubstitutionMap.h"
#include "simplifier.h"
#include "../AST/ArrayTransformer.h"

namespace BEEV
{

SubstitutionMap::~SubstitutionMap()
{
	delete SolverMap;
}

// if false. Don't simplify while creating the substitution map.
// This makes it easier to spot how long is spent in the simplifier.
const bool simplify_during_create_subM = false;

//if a is READ(Arr,const) and b is BVCONST then return 1.
//if a is a symbol SYMBOL, return 1.
//if b is READ(Arr,const) and a is BVCONST then return -1
// if b is a symbol return -1.
//
//else return 0 by default
int TermOrder(const ASTNode& a, const ASTNode& b)
{
  const Kind k1 = a.GetKind();
  const Kind k2 = b.GetKind();

  if (k1 == SYMBOL)
  	return 1;

  if (k2 == SYMBOL)
  	return -1;


  //a is of the form READ(Arr,const), and b is const, or
  if ((k1 == READ
       && a[0].GetKind() == SYMBOL
       && a[1].GetKind() == BVCONST
       && (k2 == BVCONST)))
    return 1;

  //b is of the form READ(Arr,const), and a is const, or
  //b is of the form var, and a is const
  if ((k1 == BVCONST)
      && ((k2 == READ
           && b[0].GetKind() == SYMBOL
           && b[1].GetKind() == BVCONST)))
    return -1;

  return 0;
} //End of TermOrder()



//This finds conjuncts which are one of: (= SYMBOL BVCONST), (= BVCONST (READ SYMBOL BVCONST)),
// (IFF SYMBOL TRUE), (IFF SYMBOL FALSE), (IFF SYMBOL SYMBOL), (=SYMBOL SYMBOL)
// or (=SYMBOL BVCONST).
// It tries to remove the conjunct, storing it in the substitutionmap. It replaces it in the
// formula by true.
// The bvsolver puts expressions of the form (= SYMBOL TERM) into the solverMap.

ASTNode SubstitutionMap::CreateSubstitutionMap(const ASTNode& a,  ArrayTransformer*at)
{
  if (!bm->UserFlags.wordlevel_solve_flag)
    return a;

  ASTNode output = a;
  //if the variable has been solved for, then simply return it
  if (CheckSubstitutionMap(a, output))
    return output;

  if (!alreadyVisited.insert(a.GetNodeNum()).second)
  {
    return a;
  }


  //traverse a and populate the SubstitutionMap
  const Kind k = a.GetKind();
  if (SYMBOL == k && BOOLEAN_TYPE == a.GetType())
    {
      bool updated = UpdateSubstitutionMap(a, ASTTrue);
      output = updated ? ASTTrue : a;
      return output;
    }
  if (NOT == k && SYMBOL == a[0].GetKind())
    {
      bool updated = UpdateSubstitutionMap(a[0], ASTFalse);
      output = updated ? ASTTrue : a;
      return output;
    }

  if (IFF == k || EQ == k)
    {
	  ASTVec c = a.GetChildren();
	  SortByArith(c);

	  if (c[0] == c[1])
		  return ASTTrue;

      ASTNode c1;
      if (EQ == k)
    	  c1 = simplify_during_create_subM ? simp->SimplifyTerm(c[1]) : c[1];
      else// (IFF == k )
    	  c1= simplify_during_create_subM ?  simp->SimplifyFormula_NoRemoveWrites(c[1], false) : c[1];

      bool updated = UpdateSubstitutionMap(c[0], c1);
      output = updated ? ASTTrue : a;

      if (updated)
        {
          //fill the arrayname readindices vector if e0 is a
          //READ(Arr,index) and index is a BVCONST
          int to;
          if ((to =TermOrder(c[0],c1))==1 && c[0].GetKind() == READ)
              at->FillUp_ArrReadIndex_Vec(c[0], c1);
          else if (to ==-1 && c1.GetKind() == READ)
                    at->FillUp_ArrReadIndex_Vec(c1,c[0]);
        }

      return output;
    }

  if (XOR == k)
  {
	  if (a.Degree() !=2)
		  return output;

	  int to = TermOrder(a[0],a[1]);
	  if (0 == to)
		  return output;

	  ASTNode symbol,rhs;
	  if (to==1)
	  {
		  symbol = a[0];
		  rhs = a[1];
	  }
	  else
	  {
		  symbol = a[0];
		  rhs = a[1];
	  }

	  assert(symbol.GetKind() == SYMBOL);

	  // If either side is already solved for.
	  if (CheckSubstitutionMap(symbol) || CheckSubstitutionMap(rhs))
		  return output;

	  if (UpdateSolverMap(symbol, bm->CreateNode(NOT, rhs)))
			return ASTTrue;
		else
			return output;
  }


  if (AND == k)
    {
      ASTVec o;
      ASTVec c = a.GetChildren();
      for (ASTVec::iterator
             it = c.begin(), itend = c.end();
           it != itend; it++)
        {
          simp->UpdateAlwaysTrueFormSet(*it);
          ASTNode aaa = CreateSubstitutionMap(*it,at);

          if (ASTTrue != aaa)
            {
              if (ASTFalse == aaa)
                return ASTFalse;
              else
                o.push_back(aaa);
            }
        }
      if (o.size() == 0)
        return ASTTrue;

      if (o.size() == 1)
        return o[0];

      return bm->CreateNode(AND, o);
    }

  //printf("I gave up on kind: %d node: %d\n", k, a.GetNodeNum());
  return output;
} //end of CreateSubstitutionMap()


ASTNode SubstitutionMap::applySubstitutionMap(const ASTNode& n)
{
	bm->GetRunTimes()->start(RunTimes::ApplyingSubstitutions);
	ASTNodeMap cache;
	ASTNode result =  replace(n,*SolverMap,cache,bm->defaultNodeFactory, false);
	bm->GetRunTimes()->stop(RunTimes::ApplyingSubstitutions);
	return result;
}

ASTNode SubstitutionMap::applySubstitutionMapUntilArrays(const ASTNode& n)
{
	bm->GetRunTimes()->start(RunTimes::ApplyingSubstitutions);
	ASTNodeMap cache;
	ASTNode result = replace(n,*SolverMap,cache,bm->defaultNodeFactory, true);
	bm->GetRunTimes()->stop(RunTimes::ApplyingSubstitutions);
	return result;
}

ASTNode SubstitutionMap::replace(const ASTNode& n, ASTNodeMap& fromTo,
                ASTNodeMap& cache, NodeFactory * nf)
{
    return replace(n,fromTo,cache,nf,false);
}

// NOTE the fromTo map is changed as we traverse downwards.
// We call replace on each of the things in the fromTo map aswell.
// This is in case we have a fromTo map: (x maps to y), (y maps to 5),
// and pass the replace() function the node "x" to replace, then it
// will return 5, rather than y.

ASTNode SubstitutionMap::replace(const ASTNode& n, ASTNodeMap& fromTo,
		ASTNodeMap& cache, NodeFactory * nf, bool stopAtArrays)
{

        ASTNodeMap::const_iterator it;
	if ((it = cache.find(n)) != cache.end())
		return it->second;

	if ((it = fromTo.find(n)) != fromTo.end())
	{
		if (n.GetIndexWidth() != 0)
		{
			const ASTNode& r = it->second;
			assert(r.GetIndexWidth() == n.GetIndexWidth());
			assert(BVTypeCheck(r));
			ASTNode replaced = replace(r, fromTo, cache,nf,stopAtArrays);
			if (replaced != r)
				fromTo[n] = replaced;

			return replaced;
		}
		ASTNode replaced = replace(it->second, fromTo, cache,nf,stopAtArrays);
		if (replaced != it->second)
			fromTo[n] = replaced;

		return replaced;
	}

	// These can't be created like regular nodes are. Skip 'em for now.
	if (n.isConstant() || n.GetKind() == SYMBOL /*|| n.GetKind() == WRITE*/)
		return n;

         if (stopAtArrays && n.GetType() == ARRAY_TYPE)
            {
           return n;
            }

	ASTVec children;
	children.reserve(n.GetChildren().size());
	for (unsigned i = 0; i < n.GetChildren().size(); i++)
	{
		children.push_back(replace(n[i], fromTo, cache,nf,stopAtArrays));
	}

	// This code short-cuts if the children are the same. Nodes with the same children,
	// won't have necessarily given the same node if the simplifyingNodeFactory is enabled
	// now, but wasn't enabled when the node was created. Shortcutting saves lots of time.

	ASTNode result;
	if (n.GetType() == BOOLEAN_TYPE)
	{
		assert(children.size() > 0);
		if (children != n.GetChildren()) // short-cut.
		{
			result = nf->CreateNode(n.GetKind(), children);
		}
		else
			result = n;
	}
	else
	{
		assert(children.size() > 0);
		if (children != n.GetChildren()) // short-cut.
		{
			// If the index and value width aren't saved, they are reset sometimes (??)
			const unsigned int indexWidth = n.GetIndexWidth();
			const unsigned int valueWidth = n.GetValueWidth();
			result = nf->CreateArrayTerm(n.GetKind(),indexWidth, n.GetValueWidth(),
					children);
			assert(result.GetValueWidth() == valueWidth);

		}
		else
			result = n;
	}

	if (n != result)
	{
		assert(BVTypeCheck(result));
		assert(result.GetValueWidth() == n.GetValueWidth());
		assert(result.GetIndexWidth() == n.GetIndexWidth());
	}

	cache[n] = result;
	return result;
}

// Adds to the dependency graph that n0 depends on the variables in n1.
// It's not the transitive closure of the dependencies. Just the variables in the expression "n1".
// This is only needed as long as all the substitution rules haven't been written through.
void SubstitutionMap::buildDepends(const ASTNode& n0, const ASTNode& n1)
{
	if (n0.GetKind() != SYMBOL)
		return;

	if (n1.isConstant())
		return;

	vector<Symbols*> av;
	vars.VarSeenInTerm(vars.getSymbol(n1), rhs_visited, rhs, av);

	sort(av.begin(), av.end());
	for (int i =0; i < av.size();i++)
	{
		if (i!=0 && av[i] == av[i-1])
			continue; // Treat it like a set of Symbol* in effect.

		ASTNodeSet* sym = (vars.TermsAlreadySeenMap.find(av[i])->second);
		if(rhsAlreadyAdded.find(sym) != rhsAlreadyAdded.end())
			continue;
		rhsAlreadyAdded.insert(sym);

		//cout << loopCount++ << " ";
		//cout << "initial" << rhs.size() << " Adding: " <<sym->size();
		rhs.insert(sym->begin(), sym->end());
		//cout << "final:" << rhs.size();
		//cout << "added:" << sym << endl;

	}

	assert (dependsOn.find(n0) == dependsOn.end());
	dependsOn.insert(make_pair(n0,vars.getSymbol(n1)));
}

// Take the transitive closure of the varsToCheck. Storing the result in visited.
void SubstitutionMap::loops_helper(const set<ASTNode>& varsToCheck, set<ASTNode>& visited)
{
	set<ASTNode>::const_iterator visitedIt = visited.begin();

	set<ASTNode> toVisit;
	vector<ASTNode> visitedN;

	// for each variable.
	for (set<ASTNode>::const_iterator varIt = varsToCheck.begin(); varIt != varsToCheck.end(); varIt++)
	{
		while (*visitedIt < *varIt && visitedIt != visited.end())
			visitedIt++;

		if (*visitedIt == *varIt)
			continue;

		visitedN.push_back(*varIt);
		DependsType::iterator it;
		if ((it = dependsOn.find(*varIt)) != dependsOn.end())
		{
			Symbols* s= it->second;
			bool destruct;
			ASTNodeSet* varsSeen = vars.SetofVarsSeenInTerm(s,destruct);
			toVisit.insert(varsSeen->begin(), varsSeen->end());

			if (destruct)
				delete varsSeen;
		}
	}

	visited.insert(visitedN.begin(), visitedN.end());

	visitedN.clear();

	if (toVisit.size() !=0)
		loops_helper(toVisit, visited);
}


// If n0 is replaced by n1 in the substitution map. Will it cause a loop?
// i.e. will the dependency graph be an acyclic graph still.
// For example, if we have x = F(y,z,w), it would make the substitutionMap loop
// if there's already z = F(x).
bool SubstitutionMap::loops(const ASTNode& n0, const ASTNode& n1)
	{
	if (n0.GetKind() != SYMBOL)
		return false; // sometimes this function is called with constants on the lhs.

	if (n1.isConstant())
		return false; // constants contain no variables. Can't loop.

	// We are adding an edge FROM n0, so unless there is already an edge TO n0,
	// there is no change it can loop. Unless adding this would add a TO and FROM edge.
	if (rhs.find(n0) == rhs.end())
	{
		return vars.VarSeenInTerm(n0,n1);
	}

	if (n1.GetKind() == SYMBOL && dependsOn.find(n1) == dependsOn.end() )
		return false; // The rhs is a symbol and doesn't appear.

	if (debug_substn)
		cout << loopCount++  << endl;

	bool destruct = true;
	ASTNodeSet* dependN = vars.SetofVarsSeenInTerm(n1,destruct);

	if (debug_substn)
	{
		cout << n0 << " " <<  n1.GetNodeNum(); //<< " Expression size:" << bm->NodeSize(n1,true);
		cout << "Variables in expression: "<< dependN->size() << endl;
	}

	set<ASTNode> depend(dependN->begin(), dependN->end());

	if (destruct)
		delete dependN;

	set<ASTNode> visited;
	loops_helper(depend,visited);

	bool loops = visited.find(n0) != visited.end();

	if (debug_substn)
		cout << "Visited:" << visited.size() << "Loops:" << loops << endl;

	return (loops);
	}

};
