#ifndef CM_H  
#define CM_H

#include "Input.h"
#include "GurSolver.h"
#include "Heuristic.h"
#include "InitialSolution.h"
#include "Input.h"
#include "GreedyMatching.h"
#include "Meta.h"

#include <filesystem>

void SelectAlgo(InputData& data, ParameterValues& param);

void BoundsTTP_OneInstance(InputData& data, ParameterValues& param);

#endif
