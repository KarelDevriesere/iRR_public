// Include std
#include <string>

#ifndef ALGORITHM_H
#define ALGORITHM_H

// Include other
#include "../validator/Globals.h"
#include "../validator/ClassSlot.h"
#include "../validator/ClassSlotGroup.h"
#include "../validator/ClassTeam.h"
#include "../validator/ClassTeamGroup.h"
#include "../validator/ClassInstance.h"
#include "../include/ClassInterface.h"


/** Abstract-data type for a sports timetabling generic solver
 * Other solvers are inherited from this class to have different parameters,...
 * Recevies a sports scheduling instance object and tries to optimize this instance.
 */
class Algorithm
{
private:
	

public: // Public member functions
	/// Constructors
	Algorithm(Instance* i, int timeLimit, std::string solName);
	virtual ~Algorithm();

	/// Solve the instance
	virtual void solve();

protected: // Protected member variables
	/// Instance object for which we try to construct a solution
	Instance* instance;
	/// Time limit to solve problem instnace
	int timeLimit;
	/// Name of solutin xml file to be saved
	std::string solName;
};

#endif /* ALGORITHM_H */
