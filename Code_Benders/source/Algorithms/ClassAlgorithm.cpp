#include "ClassAlgorithm.h"

/// constructor
Algorithm::Algorithm(Instance* i, int timeLimit, std::string solName): instance(i), timeLimit(timeLimit), solName(solName){
}

/// desturctor
Algorithm::~Algorithm(){}

/// Solve
void Algorithm::solve(){}
