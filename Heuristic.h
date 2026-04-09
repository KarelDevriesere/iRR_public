#include <iostream>
#include <fstream>
#include <array>
#include <vector>
#include <chrono>
#include <map>

#include "Meta.h"
#include "Solution.h"
#include "Operators.h"

using namespace std;

class Heuristic: public Operator, public MoveExecutor
{
    private:
        std::unique_ptr<MetaBase<Move>> MetaH;

        array<int,3>SelectTwoTeamsAndColorMinCost();
        array<int,3>SelectTwoTeamsAndColor();
        pair<int,int>SelectTwoTeams();
        pair<int,int>SelectTwoRounds();
        // Moves for cost minimization
        void SelectTS();
        void SelectiPTS();
        void SelectRS();
        void SelectPRS();
        void SelectiPRS(const bool bipartite);
        void SelectBalancedCycle();
        void SelectGPTS();

        bool MinCostPR = false; // MinCost Path
        bool MinCostAC = false; // MinCost Alternating Cycle
        bool MinCostC = false; // MinCost cycle

        // Helpers (construct these once to avoid overhead):
        pair<int,int>PairOfTeams;
        pair<int,int>PairOfRounds;
        array<int,3>PairOfTeamsAndColor;
        vector<int>ColoredRoundsLantern; // iPTS
        vector<HA>OrientationCopy_i; // iPTS
        vector<HA>OrientationCopy_j; // iPTS
        vector<int>CostBeforeTTPTeams; // Unbalanced iPRS

        // Only if orientation changed since last time does it make sense to do this
        bool LineGraphUsefull = false;

        // Random distributions:
        std::unique_ptr<Randomizer<int>>DisL;
        std::unique_ptr<Randomizer<int>>DisR2;
        vector<std::unique_ptr<Randomizer<int>>>DisTL1;
        vector<std::unique_ptr<Randomizer<int>>>DisTL2;

    public:
        explicit Heuristic(Solution& current_sol, std::unique_ptr<MetaBase<Move>> strategy);
        ~Heuristic();

        void PushLocalOptimum();
        
        void DoMove() override;

	    void TeamSwapper();

        void solve(Input& in, Solution& sol){
            if (MetaH) {
                MetaH->solve(in, sol);
            }
        }

        void saveTimeStamps(std::ofstream& output_file) { // wrapper to save time stamps
            if (MetaH) {
                MetaH->SaveSolutionsTimeStamps(output_file);
            }
        }
};
