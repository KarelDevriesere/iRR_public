#ifndef CM_H  
#define CM_H

#include "Input.h"
#include "GurSolver.h"
#include "Heuristic.h"
#include "Algo.h"
#include "Input.h"
#include "GreedyMatching.h"

#include <filesystem>

std::string FolderPath(const InputData& data);


void SaveSolutionXML(std::ofstream& output_file, Solution& sol);
void SaveSolution(std::ofstream& output_file, Solution& sol);
void SolveHeuristic(Input& in, vector<int>& TimeStamps, const string FolderPath, const InputData& data);
void SolveIP(Input& in, vector<int>& TimeStamps, const string FolderPath, const InputData& data);
void SelectAlgo(InputData& data);
void BoundsTTP();
void BoundsTTP_OneInstance(InputData& data);
void ReadSolution(const string path, Solution& sol);
void ReadSolutionXML(const string path, Solution& sol);

#endif
