#ifndef CM_H  
#define CM_H

#include "Input.h"
#include "GurSolver.h"
#include "Heuristic_CM.h"
#include "Algo.h"
#include "Input.h"
#include "MiaoAlgo.h"

#include <filesystem>

const vector<string>InstancesTTP = {"BRA24", "CIRC40", "CON40", "GAL40", "INCR40", "LINE40", "N16", "NFL32", "SUP12"};

std::string FolderPath(const InputData& data);


void SaveSolutionXML(std::ofstream& output_file, Solution& sol);
void SaveSolution(std::ofstream& output_file, Solution& sol);
void SolveHeuristic(Input& in, vector<int>& TimeStamps, const string FolderPath, const InputData& data);
void SolveIP(Input& in, vector<int>& TimeStamps, const string FolderPath, const InputData& data);
void TestCostMinimization(InputData& data);
void BoundsTTP();
void BoundsTTP_OneInstance(InputData& data);
void BoundsTTP_All(const InputData& data);
void ReadSolution(const string path, Solution& sol);
void ReadSolutionXML(const string path, Solution& sol);

#endif
