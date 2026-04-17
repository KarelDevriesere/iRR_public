#include "FO.h"
#include <algorithm>

FO::FO(Input& in, std::unique_ptr<MetaBase<FO_move>> strategy): GurSolver(in), MetaH(std::move(strategy)){
	x_fixed = vector<vector<vector<bool>>>(getNrTeams(), vector<vector<bool>>(getNrTeams(), vector<bool>(getNrRounds(), false)));
	x_value = vector<vector<vector<bool>>>(getNrTeams(), vector<vector<bool>>(getNrTeams(), vector<bool>(getNrRounds(), false)));
	LeagueFree = vector<bool>(getNrLeagues(), true);

	DisFreeTeams = std::make_unique<Randomizer<double>>(0, 1, MetaH->gen);
	DisFreeHAPs = std::make_unique<Randomizer<double>>(0, 1, MetaH->gen);
}


FO::~FO(){}

void FO::FreeVariablesPerturb(){
	for (int i = 0; i < getNrTeams(); ++i){
		for (int j = 0; j < getNrTeams(); ++j){
			if (isEligible(i, j)){
				for (int r = 0; r < getNrRounds(); ++r){
					if (DisFreeTeams->Sample() < PercFreeVariables){
						x_fixed[i][j][r] = false;
						x[i][j][r].set(GRB_DoubleAttr_LB, 0.0);
						x[i][j][r].set(GRB_DoubleAttr_UB, 1.0);
					}
					else{
						x_fixed[i][j][r] = true;
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

int FO::Perturb(Solution& sol){
	FreeVariablesPerturb();
	const bool HA = true;
	const bool relax_x = false;
	AddObjPerturb(x_fixed, x_value);
#ifdef PRINT
#if PRINT == 1
	cout << "PERTURB" << endl;
	cout << "previous solution = " << sol.ComputeTotalCost() << endl;
#endif 
#endif
	auto time_before_solve = std::chrono::high_resolution_clock::now();
	GurSolver::solve();
	auto time_diff = std::chrono::high_resolution_clock::now() - time_before_solve;
	auto duration_gap = std::chrono::duration_cast<std::chrono::seconds>(time_diff);
	if ((int)duration_gap.count() > TimeLimitPerturb){
		if (PercFreeVariables - 0.01 >= 0.01){
			PercFreeVariables -= 0.01;
		}
	}
	else{
		if (PercFreeVariables + 0.01 <= 1.0){
			PercFreeVariables += 0.01;
		}
	}
	SaveSolution(sol);
	int obj = sol.ComputeTotalCost();
#ifdef PRINT
#if PRINT == 1
	cout << "new solution = " << obj << endl;
#endif 
#endif
	Store_x_value();
	// cin.get();
	return obj;
}

void FO::InitializeModel(Solution& sol, const InputData& data){
	if (data.TTP){
		iTTP();
		Set_x_value_from_sol(sol);
	}
	else{
		const bool HA = true;
		const bool relax_x = false;
		build_all(HA, relax_x);
		setBoundCapacityViolations();
		AddObj(true, false);
		Set_x_value_from_sol(sol);
	}
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

void FO::FreeLeagues(){
	if (getNrLeagues() == 1){
		return;
	}
	vector<int>Leagues(getNrLeagues());
	int l;
	for (l = 0; l < getNrLeagues(); ++l){
		Leagues[l] = l;
		LeagueFree[l] = false;
	}
	std::shuffle(Leagues.begin(), Leagues.end(), MetaH->gen);
	for (l = 0; l < NrLeaguesFree.at(MetaH->CurrentMove); ++l){
		LeagueFree[Leagues[l]] = true;
	}
}

void FO::FreeTeams(){
	for (int l = 0; l < getNrLeagues(); ++l){
		if (!LeagueFree[l]){
			continue;
		}
		for (int i : getTeamsLeague(l)){
			double rd = DisFreeTeams->Sample();
			if (rd < PercFreeTeams){
				for (int r = 0; r < getNrRounds(); ++r){
					for (int j : getTeamsLeague(l)){
						x_fixed[i][j][r] = false;
						x_fixed[j][i][r] = false;
					}
				}
				if (DisFreeHAPs->Sample() < PercentageHapsFixed[FO_move::T]){
					HapFixed[i] = true; 
				}
				else{
					HapFixed[i] = false;	
				}
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
    	std::shuffle(Rounds.begin(), Rounds.end(), MetaH->gen); // gen is inherited from SA
	}
	std::uniform_int_distribution<> dist(0, getNrRounds()-nr);
	const int start = dist(MetaH->gen);
	for (int l = 0; l < getNrLeagues(); ++l){
		if (!LeagueFree[l]){
			continue;
		}
		for (int i : getTeamsLeague(l)){
			double rd = DisFreeHAPs->Sample();
			if (rd < PercentageHapsFixed[MoveNrRounds(nr, consecutive)]){
				HapFixed[i] = true; 
			}
			else{
				HapFixed[i] = false;
			}
			for (int r = start; r < start+nr; ++r){
				for (int j : getTeamsLeague(l)){
					x_fixed[i][j][r] = false;
				}
			}
		}
	}
}

void FO::UpdateSizeFreeLeagues(const FO_move move, const bool optimal){
	if (optimal && NrLeaguesFree.at(move)+1 < getNrLeagues()){
		++NrLeaguesFree.at(move);
	}
	else if (!optimal && NrLeaguesFree.at(move)-1 > 0){
		--NrLeaguesFree.at(move);
	}
}

void FO::UpdateSizeFixedVariables(const FO_move move, const bool optimal){
	if (optimal){
		// if solved to optimality, we can free more HAPs
		if (move == FO_move::T && DisFreeHAPs->Sample() < 0.5 && PercFreeTeams < 1.0){
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
		if (move == FO_move::T && DisFreeHAPs->Sample() < 0.5 && PercFreeTeams > 0.01){
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
			if (!sol.isEligible(i,j)){
				continue;
			}
			for (int r = 0; r < getNrRounds(); ++r){
				x[i][j][r].set(GRB_DoubleAttr_LB, 0.0);
				x[i][j][r].set(GRB_DoubleAttr_UB, 1.0);
				if (sol.MatchColor[i][j] == r){
					if (sol.Orientation[i][r] == HA::H){
						x_value[i][j][r] = 1;
						assert(sol.Orientation[j][r] == HA::A);
						Orientation[i][r] = HA::H;
						Orientation[j][r] = HA::A;
					}
					else{
						assert(sol.Orientation[j][r] == HA::H);
						assert(sol.Orientation[i][r] == HA::A);
						x_value[j][i][r] = 1;
						Orientation[i][r] = HA::A;
						Orientation[j][r] = HA::H;
					}
				}
				else{
					x_value[i][j][r] = 0;
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

void FO::DoMove(){
	bool consecutive = true;
	// TODO: free a subset of the leagues
	// cout << "do " << MetaH->Moves.at(MetaH->CurrentMove) << endl;
	FreeLeagues();
	if (MetaH->CurrentMove == FO_move::R1){
		// cout << "R1" << endl;
		consecutive = true;
		FreeRounds(1, consecutive);
	}
	else if (MetaH->CurrentMove == FO_move::R2C){
		// cout << "R2C" << endl;
		consecutive = true;
		FreeRounds(2, consecutive);
	}
	else if (MetaH->CurrentMove == FO_move::R3C){
		// cout << "R3C" << endl;
		consecutive = true;
		FreeRounds(3, consecutive);
	}
	else if (MetaH->CurrentMove ==FO_move::R2NC){
		// cout << "R2C" << endl;
		consecutive = false;
		FreeRounds(2, consecutive);
	}
	else if (MetaH->CurrentMove == FO_move::R3NC){
		// cout << "R3C" << endl;
		consecutive = false;
		FreeRounds(3, consecutive);
	}
	else{
		// cout << "T" << endl;
		FreeTeams();
	}
}

void FO::solve(Input& in, Solution& sol, const InputData& data){
	MetaH->Initialize(sol);
	MetaH->SetExecutor(this);
	MetaH->UpdateBestSolution(sol);
	fix_all();
	MetaH->StartTime = std::chrono::high_resolution_clock::now();
	bool consecutive = true;
	MetaH->best_obj = sol.ComputeTotalCost();
	double prev_obj = MetaH->best_obj;
	MetaH->current_obj = MetaH->best_obj;
	MetaH->it = 0;
	while (MetaH->getTimeDiff() <= MetaH->TIME_LIMIT){
		if (++MetaH->it < MaxNrRandomIterations){
			MetaH->CurrentMove = MetaH->SelectNB();
		}
		else{
			MetaH->CurrentMove = MetaH->SelectNB_MAB();
		}
		DoMove();
		setTimeLimit(TimeLimit.at(MetaH->CurrentMove));
		FixHAP(sol);
		FixVariables();
		auto time_before_solve = std::chrono::high_resolution_clock::now();
		int obj = GurSolver::solve();
		auto time_diff = std::chrono::high_resolution_clock::now() - time_before_solve;
		FreeVariables();
		MetaH->NrChosen.at(MetaH->CurrentMove)++;
		/*
		if (prev_obj < obj){
			cout << "prev_obj = " << prev_obj << ", obj = " << obj << endl;
		}
		*/
		assert(prev_obj >= obj);
		MetaH->Reward.at(MetaH->CurrentMove) += (prev_obj-obj);
		MetaH->UpdateSelectionProbabilities(MetaH->CurrentMove);
		auto duration_gap = std::chrono::duration_cast<std::chrono::seconds>(time_diff);
		bool optimal = true;
		if ((int)duration_gap.count() > TimeLimit.at(MetaH->CurrentMove)){
            optimal = false;
        }
		// Moeten ook x_value updaten!!!
		if (obj < prev_obj){
#ifdef PRINT
#if PRINT == 1
			cout << "Improved previous obj = " << obj << endl;
#endif 
#endif
			Store_x_value();
			SaveSolution(sol);
			optimal = true; // if we find a better solution: still increase the size
			MetaH->it_idle = 0;
			if (obj < MetaH->best_obj){
				MetaH->UpdateBest(sol, obj);
#ifdef PRINT
#if PRINT == 1
				cout << "New best obj = " << obj << endl;
#endif 
#endif
			}
		}
		else{
			++MetaH->it_idle;
		}
		 // TODO: in this function, we do not update sol but the x variables in GurSolver
		// Hence updating the best sequence of matches is unnecessary
        UpdateSizeFixedVariables(MetaH->CurrentMove, optimal);
		UpdateSizeFreeLeagues(MetaH->CurrentMove, optimal);

		if (MetaH->it % 100 == 0){
			++TimeLimitPerturb;
		}

		if (MetaH->it_idle >= MetaH->MAX_IT){
			obj = Perturb(sol);
			MetaH->it_idle = 0;
			if (data.TTP){
				AddObj_iTTP();// set normal objective again
			}
			else{
				AddObj(true, false); // set normal objective again 
			}
			FreeVariables();
		}

		MetaH->UpdateTimeStamps();

		fix_all();
		prev_obj = obj;
	}
	MetaH->SaveBestSolution(sol);
}
