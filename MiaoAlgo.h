#include "GurSolver.h"
#include "Input.h"
#include "Operators.h"
#include "algo.h"

enum class HAP_operator{InterClubSwap, IntraClubSwap, RandomSwap, ComplementInsertion};

class MiaoAlgo{
    private:
        void MakeForbiddenEdges(const int l, const int r, Solution& sol);
        // see page 216 in Miao
        // The weighs change depending on the instance
        array<HAP_operator, 4>HAP_operators = {HAP_operator::InterClubSwap, HAP_operator::IntraClubSwap, HAP_operator::RandomSwap, HAP_operator::ComplementInsertion};
        std::unordered_map<HAP_operator, double>Weights = {{HAP_operator::InterClubSwap, 1.0/3.0}, {HAP_operator::IntraClubSwap, 1.0/3.0}, {HAP_operator::RandomSwap, 1.0/6.0}, {HAP_operator::ComplementInsertion, 1.0/6.0}};
        unordered_map<HAP_operator, double>WeightsCumul;

        int best_obj;

    public:
        MiaoAlgo(Input& in);
        ~MiaoAlgo();

        vector<int>Rounds;

        void solve(Input& in, Solution& sol);
        bool SchedulePhase(Solution& sol);
        void ReAssignHAPs(Solution& sol);
        void Reset(Solution& sol); // delete all opponents
        void Update(Solution& sol);
};