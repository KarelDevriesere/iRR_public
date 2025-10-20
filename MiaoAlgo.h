#include "GurSolver.h"
#include "Input.h"
#include "Operators.h"
#include "Algo.h"
#include "SA.h"

enum class HAP_operator{InterClubSwap, IntraClubSwap, RandomSwap, ComplementInsertion, Initial};

class MiaoAlgo: public SA<HAP_operator>{
    private:
        void MakeForbiddenEdges(const int l, const int r, Solution& sol);
        // see page 216 in Miao
        // The weighs change depending on the instance

        int team1; // if InterClubSwap, IntraClubSwap or RandomSwap, the haps of team1 and team2 are switched
        int team2;
        int hap_index1;
        int hap_index2;

    public:
        MiaoAlgo(const std::unordered_map<HAP_operator, string>& moves, const std::unordered_map<HAP_operator, double>& weights, Input& in, std::mt19937& g);
        ~MiaoAlgo();

        bool InitialOnly = false;
        bool InitialSolutionGiven = false;

        vector<int>Rounds;

        bool InterClubSwap(Solution& sol);
        bool IntraClubSwap(Solution& sol);
        bool RandomSwap(Solution& sol);
        bool ComplementInsertion(Solution& sol);
        bool SchedulePhase(Solution& sol);
        void ReAssignHAPs(Solution& sol);
        void Reset(Solution& sol); // delete all opponents
        void SaveBestSequenceMatches(Solution& sol);
        void ReverseMove(Solution& sol);

        // Custom functions already declared in SA:
        void solve(Input& in, Solution& sol) override;

        void SolveGivenSeqeuence(Input& in, Solution& sol);
};
