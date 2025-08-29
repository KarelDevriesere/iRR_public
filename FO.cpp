#include "FO.h"
#include <algorithm>

FO::FO(Input& in, const std::unordered_map<FO_move, string>& moves, // moves, weights and in are defined in main
           const std::unordered_map<FO_move, double>& weights, const int seed): GurSolver(in), SA<FO_move>(moves, weights, seed){
	x_fixed = vector<vector<vector<bool>>>(getNrTeams(), vector<vector<bool>>(getNrTeams(), vector<bool>(getNrRounds(), false)));
	x_value = vector<vector<vector<bool>>>(getNrTeams(), vector<vector<bool>>(getNrTeams(), vector<bool>(getNrRounds(), false)));
}

FO::~FO(){}

void FO::InitializeModel(Solution& sol){
	const bool HA = true;
	const bool relax_x = false;
    build_all(HA, relax_x);
	setBoundCapacityViolations();
    AddObj(true, false);
    Set_x_value_from_sol(sol);
}

void FO::unfix_all(){
	for (int r = 0; r < getNrRounds(); ++r){
		for (int i = 0; i < getNrTeams(); ++i){
			for (int j = 0; j < getNrTeams(); ++j){
				x_fixed[i][j][r] = false;
			}
		}
	}
}

void FO::fix_all(){
	for (int r = 0; r < getNrRounds(); ++r){
		for (int i = 0; i < getNrTeams(); ++i){
			for (int j = 0; j < getNrTeams(); ++j){
				x_fixed[i][j][r] = true;
			}
		}
	}
}

void FO::FixVariables(){
	for (int i = 0; i < getNrTeams(); ++i){
		for (int j = 0; j < getNrTeams(); ++j){
			if (isEligible(i, j)){
				for (int r = 0; r < getNrRounds(); ++r){
					if (x_fixed[i][j][r]){
						if (x_value[i][j][r] > 0.9){
							x[i][j][r].set(GRB_DoubleAttr_LB, 1.0);
						}
						else{
							x[i][j][r].set(GRB_DoubleAttr_UB, 0.0);
						}
					}
				}
			}
		}
	}
}

void FO::FreeTeams(){
	for (int i = 0; i < getNrTeams(); ++i){
		double rd = RandomNumber();
		if (rd < PercFreeTeams){
			for (int r = 0; r < getNrRounds(); ++r){
				for (int j = 0; j < getNrTeams(); ++j){
					x_fixed[i][j][r] = false;
					x_fixed[j][i][r] = false;
				}
			}
			if (RandomNumber() < PercentageHapsFixed[FO_move::T]){
				HapFixed[i] = true; 
			}
			else{
				HapFixed[i] = false;	
			}
		}
	}
}

FO_move MoveNrRounds(const int nr, const bool consecutive){
	if (nr == 1){
		return FO_move::R1;
	}
	else if (nr == 2){
		if (consecutive){
			return FO_move::R2C;
		}
		else{
			return FO_move::R2NC;
		}
	}
	else{
		assert(nr == 3);
		if (consecutive){
			return FO_move::R3C;
		}
		else{
			return FO_move::R3NC;
		}
	}
}

void FO::FreeRounds(const int nr, const bool consecutive){
	vector<int>Rounds(getNrRounds());
	for (int r = 0; r < Rounds.size(); ++r){
		Rounds[r] = r;
	}
	if (!consecutive){
		unsigned seed = 42;
    	std::shuffle(Rounds.begin(), Rounds.end(), engine); // engine is inherited from SA
	}
	std::uniform_int_distribution<> dist(0, getNrRounds()-nr);
	const int start = dist(gen);
	for (int i = 0; i < getNrTeams(); ++i){
		double rd = RandomNumber();
		if (rd < PercentageHapsFixed[MoveNrRounds(nr, consecutive)]){
			HapFixed[i] = true; 
		}
		else{
			HapFixed[i] = false;
		}
		for (int r = start; r < start+nr; ++r){
			for (int j = 0; j < getNrTeams(); ++j){
				x_fixed[i][j][r] = false;
			}
		}
	}
}

void FO::UpdateSizeFixedVariables(const FO_move move, const bool optimal){
	if (optimal){
		// if solved to optimality, we can free more HAPs
		if (move == FO_move::T && RandomNumber() < 0.5 && PercFreeTeams < 1.0){
			PercFreeTeams += 0.01;
		}
		else{
			if (PercentageHapsFixed.at(move) > 0.01){
				PercentageHapsFixed.at(move) -= 0.01;
			}
		}
	}
	else{
		// if not, fix more haps for the rounds
		// for the teams: either more haps or fix more teams
		if (move == FO_move::T && RandomNumber() < 0.5 && PercFreeTeams > 0.01){
			PercFreeTeams -= 0.01;
		}
		else{
			if (PercentageHapsFixed.at(move) < 1.0){
				PercentageHapsFixed.at(move) += 0.01;
			}
		}
	}
}

void FO::Validate(){
	// HAPS
	for (int i = 0; i < getNrTeams(); ++i){
		int nr_breaks = 0;
		for (int r = 1; r < getNrRounds(); ++r){
			if (Orientation[i][r-1] == Orientation[i][r]){
				nr_breaks++;
			}
			if (r == 1 || r == getNrRounds()-1){
				if (getHAP_requirement(HAP_requirement_name::NoBreakBeginningEnd)){
					assert(Orientation[i][r-1] != Orientation[i][r]);
				}
			}
			if (r >= 2){
				if (getHAP_requirement(HAP_requirement_name::NoThreeConsecutive)){
					assert(!(Orientation[i][r-2] == Orientation[i][r-1] && Orientation[i][r-1] == Orientation[i][r]));
				}
			}
		}
		if (getHAP_requirement(HAP_requirement_name::BreakLimit)){
			assert(nr_breaks <= getBreakLimit());
		}
	}
}

void FO::Set_x_value_from_sol(Solution& sol){
	for (int i = 0; i < getNrTeams(); ++i){
		for (int j = 0; j < getNrTeams(); ++j){
			if (isEligible(i, j)){
				for (int r = 0; r < getNrRounds(); ++r){
					x[i][j][r].set(GRB_DoubleAttr_LB, 0.0);
					x[i][j][r].set(GRB_DoubleAttr_UB, 1.0);
					if (sol.MatchColor[i][j] == r){
						x_value[i][j][r] = 1;
						assert(sol.Orientation[i][r] == HA::H);
						assert(sol.Orientation[j][r] == HA::A);
						Orientation[i][r] = HA::H;
						Orientation[j][r] = HA::A;
					}
					else{
						x_value[i][j][r] = 0;
					}
				}
			}
		}
	}
	Validate();
}

void FO::Store_x_value(){
	for (int i = 0; i < getNrTeams(); ++i){
		for (int j = 0; j < getNrTeams(); ++j){
			if (isEligible(i, j)){
				for (int r = 0; r < getNrRounds(); ++r){
					// x[i][j][r].set(GRB_DoubleAttr_LB, 0.0);
					// x[i][j][r].set(GRB_DoubleAttr_UB, 1.0);
					if (x[i][j][r].get(GRB_DoubleAttr_X) > 0.9){
						x_value[i][j][r] = 1;
						Orientation[i][r] = HA::H;
						Orientation[j][r] = HA::A;
					}
					else{
						x_value[i][j][r] = 0;
					}
				}
			}
		}
	}
}


void FO::FreeVariables(){
	for (int i = 0; i < getNrTeams(); ++i){
		for (int j = 0; j < getNrTeams(); ++j){
			if (isEligible(i, j)){
				for (int r = 0; r < getNrRounds(); ++r){
					x[i][j][r].set(GRB_DoubleAttr_LB, 0.0);
					x[i][j][r].set(GRB_DoubleAttr_UB, 1.0);
					/*
					if (x[i][j][r].get(GRB_DoubleAttr_X) > 0.9){
						x_value[i][j][r] = 1;
						Orientation[i][r] = HA::H;
						Orientation[j][r] = HA::A;
					}
					else{
						x_value[i][j][r] = 0;
					}
						*/
				}
			}
		}
	}
}

void FO::solve(Input& in, Solution& sol){
	cout << "Initialize model" << endl;
	UpdateBestSolution(sol);
	InitializeModel(sol);
	cout << "Start F&O" << endl;
	fix_all();
	StartTime = std::chrono::high_resolution_clock::now();
	bool consecutive = true;
	best_obj = sol.ComputeTotalCost();
	current_obj = best_obj;
	int it = 0;
	while (!STOP){
		double rd = RandomNumber();
		auto iterator = WeightsCumul.upper_bound(rd);
		CurrentMove = iterator->second;
		if (CurrentMove == FO_move::R1){
			// cout << "R1" << endl;
			consecutive = true;
			FreeRounds(1, consecutive);
		}
		else if (CurrentMove == FO_move::R2C){
			// cout << "R2C" << endl;
			consecutive = true;
			FreeRounds(2, consecutive);
		}
		else if (CurrentMove == FO_move::R3C){
			// cout << "R3C" << endl;
			consecutive = true;
			FreeRounds(3, consecutive);
		}
		else if (CurrentMove ==FO_move::R2NC){
			// cout << "R2C" << endl;
			consecutive = false;
			FreeRounds(2, consecutive);
		}
		else if (CurrentMove == FO_move::R3NC){
			// cout << "R3C" << endl;
			consecutive = false;
			FreeRounds(3, consecutive);
		}
		else{
			// cout << "T" << endl;
			FreeTeams();
		}
		setTimeLimit(TimeLimit.at(CurrentMove));
		NrChosen.at(CurrentMove)++;
		FixHAP(sol);
		FixVariables();
		auto time_before_solve = std::chrono::high_resolution_clock::now();
		int obj = GurSolver::solve();
		time_diff = std::chrono::high_resolution_clock::now() - time_before_solve;
		FreeVariables();
		// Moeten ook x_value updaten!!!
		if (obj < best_obj){
			Store_x_value();
		}
		Update(sol, obj); // TODO: in this function, we do not update sol but the x variables in GurSolver
		// Hence updating the best sequence of matches is unnecessary

        auto duration_gap = std::chrono::duration_cast<std::chrono::seconds>(time_diff);
		bool optimal = true;
		if ((int)duration_gap.count() > TimeLimit.at(CurrentMove)){
            optimal = false;
        }
        UpdateSizeFixedVariables(CurrentMove, optimal);

		if (++it > 100){
			it = 0;
			// UpdateSelectionProbabilities(); // TODO
		}

		fix_all();
	}
	SaveSolution(sol);
}
