#include "GurSolver.h"
#include <assert.h>
#include <iostream>
#include <chrono>
#include <map>
#include <numeric>

using namespace std;

GurSolver::GurSolver(const Input& in) : Input(in), env(true), model(createModel(env)) {
	Constraints_fixed_variables = vector<vector<vector<GRBConstr>>>(getNrTeams(), vector<vector<GRBConstr>>(getNrRounds()));
	HapFixed = vector<bool>(getNrTeams(), false);
	Orientation = vector<vector<HA>>(getNrTeams(), vector<HA>(getNrRounds(), HA::BYE));
}
GurSolver::~GurSolver(){}

GRBModel GurSolver::createModel(GRBEnv& env) {
	// env.set(GRB_IntParam_OutputFlag, 0);
	env.start();
	return GRBModel(env);
  }

void build_single_league_withoutHA(){
	// In this case, we just need to select r perfect matchings such that their total sum is optimal!!
	// Can be done greedily if we know fur sure we cannot get stuck!!
	// The insight is that, if we only have travel distance, the order of the rounds does matter, so all
	// we need to do is to find a number of (edge-disjoint) perfect matchings
	// UPDATE 14/03/2025: tried this idee with matching formulation of Jasper's code, DOES NOT SEEM TO WORK
	// Scip is way too slow compared to Gurobi, i.e. Gurobi finds optimal solution within a second for i01 and i05 and within 9 seconds for i06

	// Question: can we make abstraction of the round? E.g. just select k Matchings?

	// New idee: Dantzig Wolfe for multi-competition problem
	// 
}

void GurSolver::unfix_all(){
	for (int r = 0; r < getNrRounds(); ++r){
		for (int i = 0; i < getNrTeams(); ++i){
			for (int j = 0; j < getNrTeams(); ++j){
				x_fixed[i][j][r] = false;
			}
		}
	}
}

void GurSolver::fix_all(){
	for (int r = 0; r < getNrRounds(); ++r){
		for (int i = 0; i < getNrTeams(); ++i){
			for (int j = 0; j < getNrTeams(); ++j){
				x_fixed[i][j][r] = true;
			}
		}
	}
}

void GurSolver::build_league(const bool HA, const bool relax_x){

	assert(getNrTeams() % 2 == 0);
	assert(getNrTeams()-1 >= getNrRounds());

	int i,j,r;

	x = vector<vector<vector<GRBVar>>>(getNrTeams(), vector<vector<GRBVar>>(getNrTeams(), vector<GRBVar>(getNrRounds())));
	x_fixed = vector<vector<vector<bool>>>(getNrTeams(), vector<vector<bool>>(getNrTeams(), vector<bool>(getNrRounds(), false)));
	x_value = vector<vector<vector<bool>>>(getNrTeams(), vector<vector<bool>>(getNrTeams(), vector<bool>(getNrRounds(), false)));

	for (i = 0; i < getNrTeams(); ++i){
		for (j = 0; j < getNrTeams(); ++j){
		   if (isEligible(i, j)){
			// cout << i << " and " << j << " of strength " << getStrenghtTeam(i) << " and " << getStrenghtTeam(j) << " can play vs each other" << endl;
			   for (r = 0; r < getNrRounds(); ++r){
				   std::string varName = "x_" + std::to_string(i) + "_" + std::to_string(j) + "_" + std::to_string(r);
				   if (!relax_x){
						x[i][j][r] = model.addVar(0, 1, 0.0, GRB_BINARY, varName);
				   }
				   else{
						x[i][j][r] = model.addVar(0, 1, 0.0, GRB_CONTINUOUS, varName);
				   }
				   
			  }
		   }
		}
   }

   for (i = 0; i < getNrTeams(); ++i){
		for (j = 0; j < getNrTeams(); ++j){
		   if (isEligible(i, j)){
			   std::string consName = "c1_" + std::to_string(i) + "_" + std::to_string(j);
			   GRBLinExpr sum_r = 0;
			   for (r = 0; r < getNrRounds(); ++r){
				if (SRR){
					// If SRR: a team sees an opponent at most once
					sum_r += (x[i][j][r] + x[j][i][r]);
				}
				else{
					// If DRR: a game (i,j) happens at most once (if 2x same opponent, the other game needs to be (j,i))
					sum_r += x[i][j][r];
				}  
			   }
			   model.addConstr(sum_r <= 1, consName);
		   }
		}
		for (r = 0; r < getNrRounds(); ++r){
			std::string consName = "c2_" + std::to_string(i) + "_" + std::to_string(r);
			 GRBLinExpr sum_j = 0;
			 for (j = 0; j < getNrTeams(); ++j){
			   if (isEligible(i, j)){
				   sum_j += (x[i][j][r] + x[j][i][r]);
			   }
			 }
			 // Each team plays exactly once in each round
			 model.addConstr(sum_j == 1, consName);
		}
   }

    if (!SRR){
		array<pair<int,int>,2>Range = {{{0, getNrRounds()/2}, {getNrRounds()/2, getNrRounds()}}};
		for (i = 0; i < getNrTeams(); ++i){
			// Make DRR phased: every team sees an opponent at most once in a half
			for (j = 0; j < getNrTeams(); ++j){
				if (isEligible(i, j)){
					for (auto&[start, end]: Range){
						GRBLinExpr sum_r = 0;
						for (r = start; r < end; ++r){
							sum_r += (x[i][j][r] + x[j][i][r]);
						}
						model.addConstr(sum_r <= 1);
					}
				}
			}
		}
	}

   for (i = 0; i < getNrTeams(); ++i){
	// Each team plays a maximum nr of times against a team from the same club
	const int c = getTeamClub(i);
	GRBLinExpr sum_same_club = 0;
	for (auto& j: getTeamsClub(c)){
		if (isEligible(i,j)){
			for (r = 0; r < getNrRounds(); ++r){
				sum_same_club += (x[i][j][r] + x[j][i][r]);
			}
		}
	}
	model.addConstr(sum_same_club <= getMaxSameClub());
   }

   if (!HA){
	return;
   }

   assert(getNrRounds() % 2 == 0);

   int nrH = getNrRounds() / 2;
   int nrA = nrH;
   for (i = 0; i < getNrTeams(); ++i){
	   GRBLinExpr sum_jr_H = 0;
	   // GRBLinExpr sum_jr_A = 0;
	   for (j = 0; j < getNrTeams(); ++j){
		   if (isEligible(i, j)){
			   for (r = 0; r < getNrRounds(); ++r){
				   sum_jr_H += x[i][j][r];
				   // sum_jr_A += x[j][i][r];
			   }
		   }
	   }
	   model.addConstr(sum_jr_H == nrH, "c_3H");
	   // model.addConstr(sum_jr_A == nrA, "c_3A");
   }

   if (getHAP_requirement(HAP_requirement_name::NoThreeConsecutive)){
	for (i = 0; i < getNrTeams(); ++i){
		for (r = 2; r < getNrRounds(); ++r){
			GRBLinExpr sum_H = 0, sum_A = 0;
			for (j = 0; j < getNrTeams(); ++j){
				if (isEligible(i, j)){
					sum_H += (x[i][j][r-2] + x[i][j][r-1] + x[i][j][r]);
					sum_A += (x[j][i][r-2] + x[j][i][r-1] + x[j][i][r]);
				}
			}
			model.addConstr(sum_H <= 2, "c_4H");
			model.addConstr(sum_A <= 2, "c_4A");
		}
	}
   }

   if (getHAP_requirement(HAP_requirement_name::NoBreakBeginningEnd)){
	int R = getNrRounds();
	for (i = 0; i < getNrTeams(); ++i){
		GRBLinExpr H_break_beginning = 0, A_break_beginning = 0;
		GRBLinExpr H_break_end = 0, A_break_end = 0;
		for (j = 0; j < getNrTeams(); ++j){
			if (isEligible(i, j)){
				H_break_beginning += (x[i][j][0] + x[i][j][1]);
				A_break_beginning += (x[j][i][0] + x[j][i][1]);
				H_break_end += (x[i][j][R-1] + x[i][j][R-2]);
				A_break_end += (x[j][i][R-1] + x[j][i][R-2]);
			}
		}
		model.addConstr(H_break_beginning <= 1, "c_5Hb");
		model.addConstr(A_break_beginning <= 1, "c_5Ab");
		model.addConstr(H_break_end <= 1, "c_5Hc");
		model.addConstr(A_break_end <= 1, "c_5Ad");
	}
   }

   if (getHAP_requirement(HAP_requirement_name::BreakLimit)){
	 b = vector<vector<GRBVar>>(getNrTeams(), vector<GRBVar>(getNrRounds()));
	 for (i = 0; i < getNrTeams(); ++i){
		for (r = 0; r < getNrRounds(); ++r){
			std::string varName = "b_" + std::to_string(i) + "_"  + std::to_string(r);
			b[i][r] = model.addVar(0, 1, 0.0, GRB_BINARY, varName);
		}
	 }

	 for (i = 0; i < getNrTeams(); ++i){
		GRBLinExpr break_limit = 0;
		for (r = 1; r < getNrRounds(); ++r){
			GRBLinExpr break_H = 0, break_A = 0; 
			for (j = 0; j < getNrTeams(); ++j){
				if (isEligible(i, j)){
					break_H += (x[i][j][r-1] + x[i][j][r]);
					break_A += (x[j][i][r-1] + x[j][i][r]);
				}
			}
			model.addConstr(break_H-1 <= b[i][r], "c6_break1");
			model.addConstr(break_A-1 <= b[i][r], "c6_break2");
			break_limit += b[i][r];
		}
		model.addConstr(break_limit <= getBreakLimit(), "c6_break_limit");
	 }
   }

   if (getHAP_requirement(HAP_requirement_name::QuarterBalanced)){
	const int R = getNrRounds();
	const int Half = R/2;
	const vector<pair<int,int>>Halves = {{0, Half}, {Half, R}};
	const int lb = floor((double)Half/2.0);
    const int ub = lb+1;
	for (const auto&[Start, End]: Halves){
		for (i = 0; i < getNrTeams(); ++i){
			GRBLinExpr sumH = 0;
			// GRBLinExpr sumA = 0;
			for (j = 0; j < getNrTeams(); ++j){
				if (isEligible(i, j)){
					for (r = Start; r < End; ++r){
						sumH += x[i][j][r];
						// sumA += x[j][i][r];
					}
				}
			}
			model.addConstr(sumH <= ub, "c_7H_ub");
			// model.addConstr(sumA <= ub, "c_7A_ub");
			model.addConstr(lb <= sumH, "c_7H_lb");
			// model.addConstr(lb <= sumA, "c_7A_lb");
		}
	}
   }
}

int GurSolver::build_all(const bool HA, const bool relax_x){
    
	Objective = 0;

	build_league(HA, relax_x); // We now have 1 single "league"
	// teams of different leagues cannot play vs each other bc of eligible opponents

	if (HA){
		v = vector<vector<GRBVar>>(getNrClubs(), vector<GRBVar>(getNrRounds()));

		for (int c = 0; c < getNrClubs(); ++c){ // NrClubs == clubs without dummy
			for (int r = 0; r < getNrRounds(); ++r){
				assert(getNrTeamsClub(c) >= getCapacityClub(c, r));
				int ub_var = getNrTeamsClub(c)-getCapacityClub(c, r);
				v[c][r] = model.addVar(0, ub_var, 0.0, GRB_INTEGER);

				GRBLinExpr sum_tj = 0;
				for (auto& i: getTeamsClub(c)){
					assert(getTeamClub(i) == c);
					for (int j = 0; j < getNrTeams(); ++j){
						if (isEligible(i, j)){
							sum_tj += x[i][j][r];
						}
					}
				}
				model.addConstr(sum_tj <= getCapacityClub(c, r) + 0.1 + v[c][r], "Capacity");
			}
		}
	}

	std::cout << "done building model" << std::endl;

   return 1;
}

void GurSolver::setTimeLimit(const int time_limit){
	model.set(GRB_DoubleParam_TimeLimit, time_limit);
}

void GurSolver::AddObj(const bool min_travel, const bool min_capacity_violations){
	if (min_travel){
		Objective = 0;
		for (int i = 0; i < getNrTeams(); ++i){
			for (int j = 0; j < getNrTeams(); ++j){
				if (isEligible(i, j)){
					for (int r = 0; r < getNrRounds(); ++r){
						Objective += getDistanceTeams(i, j)*x[i][j][r];
					}
				}
			}
		}	
	}
	else if (min_capacity_violations){
		Objective = 0;
		for (int c = 0; c < getNrClubs()-1; ++c){ // Last club is a dummy club!!
			for (int r = 0; r < getNrRounds(); ++r){
				Objective += v[c][r];
			}
		}
	}
	else{
		Objective = 0;
	}
	model.setObjective(Objective, GRB_MINIMIZE);
}

void GurSolver::setBoundCapacityViolations(){

	GRBLinExpr c_cap = 0;
	for (int c = 0; c < getNrClubs(); ++c){ // Last club is a dummy club!!
		for (int r = 0; r < getNrRounds(); ++r){
			c_cap += v[c][r];
		}
	}
	model.addConstr(c_cap <= getAllowedNrCapacityViolations());
}

void GurSolver::FixHAP(Solution& sol){
	// cout << "Fix haps" << endl;
	for (int i = 0; i < getNrTeams(); ++i){
		if (!HapFixed[i]){
			continue;
		}
		for (int r = 0; r < getNrRounds(); ++r){
			int nr_opp = 0;
			if (sol.Orientation[i][r] == HA::H){
				for (int j = 0; j < getNrTeams(); ++j){
					if (!isEligible(i, j)){
						continue;
					}
					// cout << "fix " << j << "-" << i << endl;
					x[j][i][r].set(GRB_DoubleAttr_UB, 0.0);
					if (x_value[i][j][r] > 0.9){
						++nr_opp;
						assert(Orientation[j][r] == HA::A);
					}
				}
			}
			else{
				for (int j = 0; j < getNrTeams(); ++j){
					if (!isEligible(i, j)){
						continue;
					}
					x[i][j][r].set(GRB_DoubleAttr_UB, 0.0);
					if (x_value[j][i][r] > 0.9){
						++nr_opp;
						assert(Orientation[j][r] == HA::H);
					}
				}
			}
			// assert(nr_opp == 1);
		}
	}
}

int GurSolver::ComputeObjValue(){
	int obj_value = 0;
	for (int i = 0; i < getNrTeams(); ++i){
		for (int j = 0; j < getNrTeams(); ++j){
			if (isEligible(i, j)){
				for (int r = 0; r < getNrRounds(); ++r){
					if (x_value[i][j][r] > 0.9){
						obj_value += getDistanceTeams(i,j);
					}
				}
			}
		}
	}
	return obj_value;
}

void GurSolver::FixVariables(){
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

void GurSolver::FreeTeams(){
	for (int i = 0; i < getNrTeams(); ++i){
		double rd = (double)rand()/RAND_MAX;
		if (rd < PercFreeTeams){
			for (int r = 0; r < getNrRounds(); ++r){
				for (int j = 0; j < getNrTeams(); ++j){
					x_fixed[i][j][r] = false;
					x_fixed[j][i][r] = false;
				}
			}
			if ((double)rand()/RAND_MAX < PercentageHapsFixed[nb_name::T]){
				HapFixed[i] = true; 
			}
			else{
				HapFixed[i] = false;
			}
		}
	}
}

nb_name MoveNrRounds(const int nr, const bool consecutive){
	if (nr == 1){
		return nb_name::R1;
	}
	else if (nr == 2){
		if (consecutive){
			return nb_name::R2C;
		}
		else{
			return nb_name::R2NC;
		}
	}
	else{
		assert(nr == 3);
		if (consecutive){
			return nb_name::R3C;
		}
		else{
			return nb_name::R3NC;
		}
	}
}

void GurSolver::FreeRounds(const int nr, const bool consecutive){
	vector<int>Rounds(getNrRounds());
	iota(Rounds.begin(), Rounds.end(), 0);
	if (!consecutive){
		unsigned seed = 42;
    	std::shuffle(Rounds.begin(), Rounds.end(), default_random_engine(seed));
	}
	const int start = rand()%(getNrRounds()-nr);
	for (int i = 0; i < getNrTeams(); ++i){
		double rd = (double)rand()/RAND_MAX;
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

void GurSolver::UpdateSelectionProbabilities(){
	// This is basically the same as in ILS..
	double nrm = 0;
    for (const auto& movename : Moves){
        cout << "reward / NrChosen = " << Reward.at(movename) << " /= " << NrChosen.at(movename) << endl;
        if (NrChosen.at(movename) > 0){
			Reward.at(movename) /= NrChosen.at(movename);
		}
		else{
			assert(Reward.at(movename) == 0);
		}
        nrm += Reward.at(movename);
    }
    double sum = 0;
    for (const auto& movename: Moves){
        cout << "weight = max((1.0-" << lambda << ")*" <<  Weights.at(movename) << " + " << lambda << "*" << Reward.at(movename)  << "/" << nrm << "), " << MinWeight << ")" << endl;
        if (nrm > 0){
            Weights.at(movename)  = std::max((1.0-lambda)*Weights.at(movename)  + lambda*(Reward.at(movename) /nrm), MinWeight);
            sum += Weights.at(movename) ;
			CumulWeights.at(movename)  = sum;
            cout << "Weight " << nb_name_string.at(movename) << " = " << Weights.at(movename)  << endl;
        }
        Reward.at(movename)  = 0;
        NrChosen.at(movename)  = 0;
		cout << "Percentage of HAPs fixed = " << PercentageHapsFixed.at(movename) << endl;
    }
	cout << "sum = " << sum << endl;
	if (nrm > 0){
		assert(0.99 <= sum && sum <= 1.01);
	}
}

void GurSolver::UpdateSizeFixedVariables(const nb_name move, const bool optimal){
	if (optimal){
		// if solved to optimality, we can free more HAPs
		if (move == nb_name::T && (double)rand()/RAND_MAX < 0.5 && PercFreeTeams < 1.0){
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
		if (move == nb_name::T && (double)rand()/RAND_MAX < 0.5 && PercFreeTeams > 0.01){
			PercFreeTeams -= 0.01;
		}
		else{
			if (PercentageHapsFixed.at(move) < 1.0){
				PercentageHapsFixed.at(move) += 0.01;
			}
		}
	}
}

void GurSolver::FO(Solution& sol){
	cout << "Start F&O" << endl;
	fix_all();
	start_time = std::chrono::high_resolution_clock::now();
    current_time = std::chrono::high_resolution_clock::now();
	setTimeLimit(TimeLimitSubIp);
	bool stop = false;
	bool consecutive = true;
	int best_obj = ComputeObjValue();
	int it = 0;
	while (!stop){
		double rd = (double)rand()/RAND_MAX;
		nb_name move;
		if (rd < CumulWeights.at(nb_name::R1)){
			// cout << "R1" << endl;
			move = nb_name::R1;
			consecutive = true;
			FreeRounds(1, consecutive);
		}
		else if (rd < CumulWeights.at(nb_name::R2C)){
			// cout << "R2C" << endl;
			move = nb_name::R2C;
			consecutive = true;
			FreeRounds(2, consecutive);
		}
		else if (rd < CumulWeights.at(nb_name::R3C)){
			// cout << "R3C" << endl;
			move = nb_name::R3C;
			consecutive = true;
			FreeRounds(3, consecutive);
		}
		else if (rd < CumulWeights.at(nb_name::R2NC)){
			// cout << "R2C" << endl;
			move = nb_name::R2NC;
			consecutive = false;
			FreeRounds(2, consecutive);
		}
		else if (rd < CumulWeights.at(nb_name::R3NC)){
			// cout << "R3C" << endl;
			move = nb_name::R3NC;
			consecutive = false;
			FreeRounds(3, consecutive);
		}
		else{
			move = nb_name::T;
			// cout << "T" << endl;
			FreeTeams();
		}
		NrChosen[move]++;
		FixHAP(sol);
		FixVariables();
		auto time_before_solve = std::chrono::high_resolution_clock::now();
		int obj = solve();
		FreeVariables();
		if (obj < best_obj){
			Store_x_value();
			
			Reward[move] += (best_obj - obj);
			cout << nb_name_string.at(move) << ": " << obj << endl;
			best_obj = obj;
		}

		current_time = std::chrono::high_resolution_clock::now();
        std::chrono::high_resolution_clock::duration time_diff = current_time - start_time;
        auto duration_gap = std::chrono::duration_cast<std::chrono::seconds>(time_diff);
        if ((int)duration_gap.count() > TimeLimitFO){
			cout << "F&O done" << endl;
            stop = true;
        }
		time_diff = current_time - time_before_solve;
        duration_gap = std::chrono::duration_cast<std::chrono::seconds>(time_diff);
		bool optimal = true;
		if ((int)duration_gap.count() > TimeLimitSubIp){
            optimal = false;
        }
        UpdateSizeFixedVariables(move, optimal);

		if (++it > 100){
			it = 0;
			UpdateSelectionProbabilities();
		}

		fix_all();
	}
}

void GurSolver::Validate(){
	// HAPS
	for (int i = 0; i < getNrTeams(); ++i){
		int nr_breaks = 0;
		for (int r = 1; r < getNrRounds(); ++r){
			if (Orientation[i][r-1] == Orientation[i][r]){
				nr_breaks++;
			}
			if (r == 1 || r == getNrRounds()-1){
				assert(Orientation[i][r-1] != Orientation[i][r]);
			}
			if (r >= 2){
				assert(!(Orientation[i][r-2] == Orientation[i][r-1] && Orientation[i][r-1] == Orientation[i][r]));
			}
		}
		assert(nr_breaks <= getBreakLimit());
	}
}

void GurSolver::Store_x_value(){
	for (int i = 0; i < getNrTeams(); ++i){
		for (int j = 0; j < getNrTeams(); ++j){
			if (isEligible(i, j)){
				for (int r = 0; r < getNrRounds(); ++r){
					x[i][j][r].set(GRB_DoubleAttr_LB, 0.0);
					x[i][j][r].set(GRB_DoubleAttr_UB, 1.0);
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
	Validate();
}


void GurSolver::FreeVariables(){
	for (int i = 0; i < getNrTeams(); ++i){
		for (int j = 0; j < getNrTeams(); ++j){
			if (isEligible(i, j)){
				for (int r = 0; r < getNrRounds(); ++r){
					x[i][j][r].set(GRB_DoubleAttr_LB, 0.0);
					x[i][j][r].set(GRB_DoubleAttr_UB, 1.0);
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

int GurSolver::solve(){

	try
	{
		model.optimize();

		// Check status
		int status = model.get(GRB_IntAttr_Status);
		if (status == GRB_INFEASIBLE || status == GRB_INF_OR_UNBD) { 
			model.write("gurobi_model.lp");
			std::cout << "Model is infeasible. Computing IIS...\n";

			// Compute IIS
			model.computeIIS();

			// Retrieve constraints
			GRBConstr* constraints = model.getConstrs();
			int numConstrs = model.get(GRB_IntAttr_NumConstrs);

			std::cout << "Minimal infeasible subset of constraints:\n";
			for (int i = 0; i < numConstrs; i++) {
				if (constraints[i].get(GRB_IntAttr_IISConstr) == 1) {
					std::cout << "  " << constraints[i].get(GRB_StringAttr_ConstrName) << "\n";
				}
			}

			// Clean up allocated memory
			delete[] constraints;
			return -1;
		}
	}
	catch (GRBException e)
	{
		std::cout << "Exception during building model" << std::endl;
		std::cout << "Error code = " << e.getErrorCode() << std::endl;
		std::cout << e.getMessage() << std::endl;
		std::abort();
	}
	catch (...)
	{
		std::cout << "Unknown exception during building model" << std::endl;
		std::abort();
	}
	if (model.get(GRB_IntAttr_SolCount) < 1){
		// not one feasible solution found
		return INT_MAX;
	}
	return std::round(model.get(GRB_DoubleAttr_ObjVal)); // In case of time limit this returns the best found objective
}

double GurSolver::getMipGap(){
	return model.get(GRB_DoubleAttr_MIPGap);
}

int GurSolver::getBestBound(){
	return (int)model.get(GRB_DoubleAttr_ObjBound);
}

void GurSolver::SaveSolution(Solution& sol){
	for (int i = 0; i < getNrTeams(); ++i){
		for (int r = 0; r < getNrRounds(); ++r){
			for (int j = 0; j < getNrTeams(); ++j){
				if (isEligible(i,j)){
					if (x[i][j][r].get(GRB_DoubleAttr_X) > 0.9){
						sol.TeamColorOpp[j][r] = i;
						sol.TeamColorOpp[i][r] = j;
						sol.MatchColor[i][j] = r;
						if (SRR){
							sol.MatchColor[j][i] = r;
						}
						sol.Orientation[i][r] = HA::H;
						sol.Orientation[j][r] = HA::A;
					}
				}
			}
		}
	}
}


void GurSolver::BuildPatternFormulation(){
	const int NrHaps = getNrHAPs(); 
	int t,h,c,r;

	if (v.empty()){
		v = vector<vector<GRBVar>>(getNrClubs(), vector<GRBVar>(getNrRounds()));
		for (int c = 0; c < getNrClubs(); ++c){ // NrClubs == clubs without dummy
			for (int r = 0; r < getNrRounds(); ++r){
				assert(getNrTeamsClub(c) >= getCapacityClub(c, r));
				int ub_var = getNrTeamsClub(c)-getCapacityClub(c, r);
				v[c][r] = model.addVar(0, ub_var, 0.0, GRB_CONTINUOUS);
			}
		}
	}

	y = vector<vector<GRBVar>>(getNrTeams(), vector<GRBVar>(NrHaps));
	for (t = 0; t < getNrTeams(); ++t){
		for (h = 0; h < NrHaps; ++h){
			y[t][h] = model.addVar(0, 1, 0.0, GRB_BINARY);
		}
	}

	for (t = 0; t < getNrTeams(); ++t){
		GRBLinExpr sum_h = 0;
		for (h = 0; h < NrHaps; ++h){
			sum_h += y[t][h];
		}
		model.addConstr(sum_h == 1); // each team is assigned to exactly 1 hap
	}

	for (c = 0; c < getNrClubs(); ++c){
		for (r = 0; r < getNrRounds(); ++r){
			GRBLinExpr sum_th = 0;
			for (auto& t: getTeamsClub(c)){
				for (h = 0; h < NrHaps; ++h){
					if (getModeHAPRound(h,r) == HA::H){
						sum_th += y[t][h];
					}
				}
			}
			model.addConstr(sum_th <= getCapacityClub(c, r) + v[c][r]);
		}
	}

	const int T_2 = getNrTeams() / 2;
	for (r = 0; r < getNrRounds(); ++r){
		GRBLinExpr sum_th = 0;
		for (t = 0; t < getNrTeams(); ++t){
			for (h = 0; h < NrHaps; ++h){
				if (getModeHAPRound(h,r) == HA::H){
					sum_th += y[t][h];
				}
			}
		}
		model.addConstr(sum_th == T_2); // exatly T/2 H games in each round
	}

	// I notice that I get as a starting solution that only 2 complementary haps are used for all teams
	// What happens if I put a constraint that the same hap can only be assigned to at most x teams? -> takes much more time to solve!!
	// But, this constraint seems to improve the solution..
	// No constraint: first solution = 4988, MaxSameTeam = 20: 4572, MaxSameTeam = 15: 4303, MaxSameTeam = 12: 3886, MaxSameTeam = 10: 4616...

	/*
	int MaxSameTeam = 12; // at most x teams can share the same hap
	for (h = 0; h < NrHaps; ++h){
		GRBLinExpr sum_h = 0;
		for (t = 0; t < getNrTeams(); ++t){
			sum_h += y[t][h];
		}
		model.addConstr(sum_h <= MaxSameTeam);
	}
	*/

	cout << "patterns done" << endl;
}

void GurSolver::Fix_y_Patterns(const Solution& sol){
	for (int t = 0; t < getNrTeams(); ++t){
		int count = 0;
		for (int h = 0; h < getNrHAPs(); ++h){
			if (sol.getHAPIndexTeam(t) == h){
				y[t][h].set(GRB_DoubleAttr_LB, 1.0);
				count++;
				break;
			}
			else{
				y[t][h].set(GRB_DoubleAttr_UB, 0.0);
			}
		}
		assert(count == 1);
	}
}

void GurSolver::BuildMiaoFormulation(const bool relax_x){
	const bool HA = false;
	build_all(HA, relax_x);
	BuildPatternFormulation();
	setBoundCapacityViolations();
	// link the assigned patters with the opponent schedule
	for (int i = 0; i < getNrTeams(); ++i){
		for (int r = 0; r < getNrRounds(); ++r){
			GRBLinExpr sum = 0;
			for (int j = 0; j < getNrTeams(); ++j){
				if (isEligible(i,j)){
					sum += x[i][j][r];
				}
			}
			for (int h = 0; h < getNrHAPs(); ++h){
				if (getModeHAPRound(h,r) == HA::H){
					sum -= y[i][h];
				}
			}
			model.addConstr(sum == 0);
		}
	}
}

void GurSolver::StoreHAPs(Solution& sol){
	for (int t = 0; t < getNrTeams(); ++t){
		for (int h = 0; h < getNrHAPs(); ++h){
			if (y[t][h].get(GRB_DoubleAttr_X) > 0.9){
				sol.setHAPIndexTeam(t, h);
				break;
			}
		}
	}

	bool printHAPs = false;
	for (int t = 0; t < getNrTeams(); ++t){
		assert(sol.getHAPIndexTeam(t) != -1);
		vector<HA>HAP = sol.getHAP(sol.getHAPIndexTeam(t)); 
		for (int h = 0; h < HAP.size(); ++h){
			if (HAP[h] == HA::H){
				sol.Orientation[t][h] = HA::H;
			}
			else{
				sol.Orientation[t][h] = HA::A;
			}
		}
	}

	// extra check

	int cap_viol = 0;
	for (int c = 0; c < getNrClubs(); ++c){
		for (int r = 0; r < getNrRounds(); ++r){
			int cap = 0;
			for (auto& t: getTeamsClub(c)){
				if (sol.Orientation[t][r] == HA::H){
					cap++;
				}
			}
			cap_viol += max(0, cap - getCapacityClub(c, r));
		}
	}
	assert(cap_viol <= getAllowedNrCapacityViolations());
}

void GurSolver::AssignHAPsToTeams(Solution& sol){

	cout << "Assign HAPs to teams" << endl;

	BuildPatternFormulation();

	const int NrHaps = getNrHAPs();

	const bool min_travel = false;
	const bool min_capacity_violations = true;
	AddObj(min_travel, min_capacity_violations);

	int obj = solve();

	assert(obj <= getAllowedNrCapacityViolations());
	StoreHAPs(sol);
}

/*
void GurSolver::SetDualProbs(){
	// DOES NOT WORK????
	cout << "Set Probabilities" << endl;
	cin.get();
	for (int l = 0; l < getNrLeagues(); ++l){
		for (int i = 0; i < getNrTeams(); ++i){
			for (int r = 0; r < getNrRounds(); ++r){
				for (int j = 0; j < getNrTeams(); ++j){
					if (!isEligible(i,j)){
						continue;
					}
					GRBConstr constr = model.addConstr(x[i][j][r] - x_value[l][i][j][r] == 0, "fix_cons");
					Constraints_fixed_variables[i][r].push_back(constr);
					x[i][j][r].set(GRB_CharAttr_VType, GRB_CONTINUOUS);
				}
				b[i][r].set(GRB_CharAttr_VType, GRB_CONTINUOUS);
			}
		}
	}
	for (int c = 0; c < getNrClubs()-1; ++c){ // Last club is a dummy club!!
		for (int r = 0; r < getNrRounds(); ++r){
			v[c][r].set(GRB_CharAttr_VType, GRB_CONTINUOUS);
		}
	}	
	double M_teams = 0.0, M_rounds = 0.0;
	model.update();
	assert(!model.get(GRB_IntAttr_IsMIP));
	model.optimize();
	assert(model.get(GRB_IntAttr_Status) == GRB_OPTIMAL);
	cout << "Relaxed model" << endl;
	cin.get();
	for (int l = 0; l < getNrLeagues(); ++l){
		for (int i = 0; i < getNrTeams(); ++i){
			for (int r = 0; r < getNrRounds(); ++r){
				for (const GRBConstr constr : Constraints_fixed_variables[i][r]) {
					double dual;
					try {
						dual = constr.get(GRB_DoubleAttr_Pi);
					} catch (GRBException &e) {
						cout << "Gurobi exception caught while reading dual: " << e.getMessage() << endl;
					}
					if (dual < 0){
						cout << "dual = " << dual << endl;
						WeightsRounds[r] += abs(dual);
						WeightsTeams[i] += abs(dual);
						M_teams += abs(dual);
						M_rounds += abs(dual);
					}
					model.remove(constr);
				}
				Constraints_fixed_variables[i][r].clear();

				for (int j = 0; j < getNrTeams(); ++j){
					if (!isEligible(i,j)){
						continue;
					}
					else{
						x[i][j][r].set(GRB_CharAttr_VType, GRB_BINARY);
					}
				}
			}
		}
	}
	// Normalize
	cin.get();
	cout << "Weights rounds" << endl;
	for (int r = 0; r < getNrRounds(); ++r){
		WeightsRounds[r] /= M_rounds;
		cout << "weight round " << r << " = " << WeightsRounds[r] << endl;
	}
	cin.get();
	for (int l = 0; l < getNrLeagues(); ++l){
		for (int i = 0; i < getNrTeams(); ++i){
			WeightsTeams[i] /= M_teams;
		}
	}
	model.update();
}
*/

int GurSolver::PerfectMatching(vector<vector<bool>>& GameForbidden){

	vector<vector<GRBVar>>x(getNrTeams(), vector<GRBVar>(getNrTeams()));

	Objective = 0;

	int i, j;
	for (i = 0; i < getNrTeams(); ++i){
		for (j = 0; j < getNrTeams(); j++){
			if (i != j){
				x[i][j] = model.addVar(0, 1, 0.0, GRB_BINARY);
			}
		}
	}

	for (i = 0; i < getNrTeams(); ++i){
		GRBLinExpr sum_j = 0;
		for (j = 0; j < getNrTeams(); ++j){
			if (i != j){
				sum_j += (x[i][j] + x[j][i]);

				Objective += getDistanceTeams(i, j)*x[i][j];

				if (GameForbidden[i][j]){
					x[i][j].set(GRB_DoubleAttr_UB, 0.0);
				}
			}
		}
		model.addConstr(sum_j == 1);
	}

	model.setObjective(Objective, GRB_MINIMIZE);

	model.optimize();

	for (i = 0; i < getNrTeams(); ++i){
		for (j = 0; j < getNrTeams(); ++j){
			if (i != j && x[i][j].get(GRB_DoubleAttr_X) > 0.9){
				GameForbidden[i][j] = true;
				GameForbidden[j][i] = true;
			}
		}
	}

	return std::round(model.get(GRB_DoubleAttr_ObjVal));

}

vector<vector<pair<int,int>>> GurSolver::EdgeColoring(int& C, vector<vector<bool>>& ForbiddenEdge, int& NrColorsUsed){
	typedef pair<int, int>E;
	vector<E>edge_vector;

	int e = 0;
	for (int i = 0; i < getNrTeams(); ++i){
		for (int j = i+1; j < getNrTeams(); ++j){
			if (!ForbiddenEdge[i][j]){
				edge_vector.push_back(E(i,j));
				e++;
			}
		}
	}

	int N = edge_vector.size();
	vector<vector<bool>>IncidentEdges(N, vector<bool>(N, false));
	for (int e = 0; e < N; ++e){
		for (int e2 = e+1; e2 < N; ++e2){
			if (edge_vector[e].first == edge_vector[e2].first || edge_vector[e].first == edge_vector[e2].second || edge_vector[e].second == edge_vector[e2].first || edge_vector[e].second == edge_vector[e2].second){
				IncidentEdges[e][e2] = true;
				IncidentEdges[e2][e] = true;
			}
		}
	}

	vector<GRBVar>y(C);
	vector<vector<GRBVar>>x(N, vector<GRBVar>(C));

	Objective = 0;

	// model.set(GRB_IntParam_LogToConsole, 0);

	for (int c = 0; c < C; ++c){
		y[c] = model.addVar(0, 1, 0.0, GRB_BINARY);
		Objective += y[c];
		for (int e = 0; e < N; ++e){
			x[e][c] = model.addVar(0, 1, 0.0, GRB_BINARY);
		}
	}

	for (int e = 0; e < N; ++e){
		GRBLinExpr sum_c = 0;
		for (int c = 0; c < C; ++c){
			sum_c += x[e][c];
			model.addConstr(x[e][c] <= y[c]);
		}
		model.addConstr(sum_c == 1);
	}

	for (int c = 0; c < C; ++c){
		for (int e = 0; e < IncidentEdges.size(); ++e){
			for (int e2 = e+1; e2 < IncidentEdges.size(); ++e2){
				if (IncidentEdges[e][e2]){
					model.addConstr(x[e][c] + x[e2][c] <= 1);
				}
			}
		}
	}

	model.setObjective(Objective, GRB_MINIMIZE);

	model.optimize();

	NrColorsUsed = std::round(model.get(GRB_DoubleAttr_ObjVal));
	vector<vector<pair<int,int>>>Factorization(NrColorsUsed);
	if (NrColorsUsed < C){
		assert(NrColorsUsed == C-1);
		int c_ = 0;
		int i,j;
		for (int c = 0; c < C; ++c){
			if (y[c].get(GRB_DoubleAttr_X) < 0.1){
				continue;
			} 
			for (int e = 0; e < N; ++e){
				if (x[e][c].get(GRB_DoubleAttr_X) > 0.9){
					i = edge_vector[e].first, j = edge_vector[e].second;
					// cout << i << " - " << j << " has color " << c << endl;
					Factorization[c_].push_back({i,j});
				}
			}
			c_++;
		}
	}

	return Factorization;
}