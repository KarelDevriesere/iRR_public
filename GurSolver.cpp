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
	env.start();
	env.set(GRB_IntParam_Threads, 8); // Very important when using HPC!!!
#ifdef PRINT
#if PRINT == 0
	env.set(GRB_IntParam_LogToConsole, 0); // surpress all output
#endif
#endif
	return GRBModel(env);
  }

void GurSolver::WarmStart(Solution& sol){
	assert(!x.empty());
	for (int r = 0; r < getNrRounds(); ++r){
		vector<bool>NodeSeen(getNrTeams(), false);
		for (int i = 0; i < getNrTeams(); ++i){
			if (!NodeSeen[i]){
				int j = sol.TeamColorOpp[i][r];
				if (sol.Orientation[i][r] == HA::H){
					assert(sol.Orientation[j][r] == HA::A);
					x[i][j][r].set(GRB_DoubleAttr_Start, 1.0);
				}
				else{
					assert(sol.Orientation[j][r] == HA::H);
					assert(sol.Orientation[i][r] == HA::A);
					x[j][i][r].set(GRB_DoubleAttr_Start, 1.0);
				}
				NodeSeen[j] = true;
			}
		}
	}
}

pair<vector<vector<int>>,vector<int>>GurSolver::GenerateTrips(const int t, const vector<int>& TeamsList){
	const int N = getNrTeams();
	vector<vector<vector<int>>>TripleSeen(N, vector<vector<int>>(N, vector<int>(N, false)));
	vector<vector<int>>Trips;
	vector<int>CostTrips;
	int i,j,k,dist; 
	for (i = 0; i < N-1; ++i) {
        for (j = i+1; j < N-1; ++j) {
            for (k = 0; k < N-1; ++k) {
                if (k == i || k == j){
					continue;
				}
				Trips.push_back({TeamsList[i], TeamsList[k], TeamsList[j]});
				if (t == 10){
					cout << "{" << TeamsList[i] << "," << TeamsList[k] << "," << TeamsList[j] << "}" << endl;
				}
				dist = getDistanceTeams(t,TeamsList[i]) + getDistanceTeams(TeamsList[i],TeamsList[k]) + getDistanceTeams(TeamsList[k],TeamsList[j]) + getDistanceTeams(TeamsList[j],t);
				CostTrips.push_back(dist);
			}
			Trips.push_back({TeamsList[i], TeamsList[j]});
			dist = getDistanceTeams(t,TeamsList[i]) + getDistanceTeams(TeamsList[i],TeamsList[j]) + getDistanceTeams(TeamsList[j],t);
			CostTrips.push_back(dist);
        }
		Trips.push_back({TeamsList[i]});
		dist = 2*getDistanceTeams(t,TeamsList[i]);
		CostTrips.push_back(dist);
    }
	assert(Trips.size() == (N-1) + ((N-1)*(N-2))/2 + ((N-1)*(N-2)*(N-3))/2);
	return {Trips,CostTrips};
}


pair<vector<vector<int>>,vector<int>>GurSolver::GenerateTrips_TripModel(const int t, const vector<int>& TeamsList, const int MinTripLength){
	// Here, the order of the teams matters!!!!
	const int N = getNrTeams();
	vector<vector<vector<int>>>TripleSeen(N, vector<vector<int>>(N, vector<int>(N, false)));
	vector<vector<int>>Trips;
	vector<int>CostTrips;
	int i,j,k,dist; 
	for (i = 0; i < N-1; ++i) {
        for (j = i+1; j < N-1; ++j) {
            for (k = 0; k < N-1; ++k) {
                if (k == i || k == j){
					continue;
				}
				if (getNrRounds() >= 6){
					Trips.push_back({TeamsList[i], TeamsList[k], TeamsList[j]});
					Trips.push_back({TeamsList[j], TeamsList[k], TeamsList[i]});
					dist = getDistanceTeams(t,TeamsList[i]) + getDistanceTeams(TeamsList[i],TeamsList[k]) + getDistanceTeams(TeamsList[k],TeamsList[j]) + getDistanceTeams(TeamsList[j],t);
					CostTrips.push_back(dist);
					CostTrips.push_back(dist);
				}
			}
			if (MinTripLength < 3){
				Trips.push_back({TeamsList[i], TeamsList[j]});
				Trips.push_back({TeamsList[j], TeamsList[i]});
				dist = getDistanceTeams(t,TeamsList[i]) + getDistanceTeams(TeamsList[i],TeamsList[j]) + getDistanceTeams(TeamsList[j],t);
				CostTrips.push_back(dist);
				CostTrips.push_back(dist);
			}
        }
		if (MinTripLength < 2){
			Trips.push_back({TeamsList[i]});
			dist = 2*getDistanceTeams(t,TeamsList[i]);
			CostTrips.push_back(dist);
		}
    }
	int SumTripLength = 0;
	if (getNrRounds() >= 6 && MinTripLength <= 3){
		SumTripLength += ((N-1)*(N-2)*(N-3));
	}
	if (MinTripLength <= 2){
		SumTripLength += ((N-1)*(N-2));
	}
	if (MinTripLength <= 1){
		SumTripLength += (N-1);
	}
	assert(Trips.size() == SumTripLength);
	return {Trips,CostTrips};
}

bool IsTeamInTrip(const int i, vector<int>& trip){
	for (auto& opp: trip){
		if (opp == i){
			return true;
		}
	}
	return false;
}

void GurSolver::iTTP_TripModel(){

	model.set(GRB_DoubleParam_MemLimit, 32.0);

	cout << "build trip model" << endl;

	TripModelTTP = true;

	const int N = getNrTeams();
	const int R = getNrRounds();
	int NrHaps = getNrHAPs();
	cout << "NrHaps included in model = " << NrHaps << endl;
 	int t,i,j,r,s,h,L;

	// Compute the least number of consecutive A's: this will be the minimum trip length
	// Convenient when, for example, doing BRA24_6: HHHAAA and AAAHHH: take only trips of lenght 3 into account

	int MinTripLength = 3;
	int TripLength = 3;
	for (h = 0; h < NrHaps; ++h){
		if (getModeHAPRound(h,0)==HA::A){
			TripLength = 1;
		}
		for (r = 1; r < R; ++r){
			if (getModeHAPRound(h,r)==HA::A){
				TripLength++;
			}
			else{
				if (getModeHAPRound(h,r-1)==HA::A){
					if (TripLength < MinTripLength){
						MinTripLength = TripLength;
						if (MinTripLength == 1){
							break;
						}
					}
					TripLength = 0;
				}
			}
		}
		if (MinTripLength == 1){
			break;
		}
	}
	cout << "MinTripLength = " << MinTripLength << endl;

	vector<pair<vector<vector<int>>,vector<int>>>Trips_CostTrips(N);
	Trips = vector<vector<vector<int>>>(N);
	CostTrips = vector<vector<int>>(N);
	for (t = 0; t < N; ++t){
		vector<int>TeamsList(N-1);
		for (i = 0, j = -1; i < N; ++i){
			if (i != t){
				TeamsList[++j] = i;
			}
		}
		Trips_CostTrips[t] = GenerateTrips_TripModel(t, TeamsList, MinTripLength);
		Trips[t] = Trips_CostTrips[t].first;
		CostTrips[t] = Trips_CostTrips[t].second;
		NrTrips = CostTrips[t].size();
		if (t > 0){
			assert(NrTrips ==  CostTrips[t-1].size());
		}
	}

	vector<vector<int>>LastStartRoundTrip = vector<vector<int>>(N, vector<int>(NrTrips, -1));
	for (t = 0; t < N; ++t){
		for (r = 0; r < NrTrips; ++r){
			L = Trips[t][r].size();
			LastStartRoundTrip[t][r] = R-L;
		}
	}

	cout << "NrTrips = " << NrTrips << endl;

	z_trs = vector<vector<vector<GRBVar>>>(N, vector<vector<GRBVar>>(NrTrips, vector<GRBVar>(R)));
	for (t = 0; t < N; ++t){
		for (r = 0; r < NrTrips; ++r){
			for (s = 0; s < LastStartRoundTrip[t][r]; ++s){
				z_trs[t][r][s] = model.addVar(0, 1, 0.0, GRB_BINARY/*, "z[" + to_string(t) + "," + to_string(r) + "," + to_string(s) + "]"*/);
			}
		}
	}

	cout << "Nr of trip variables = " << N*R*NrTrips << endl;

	y = vector<vector<GRBVar>>(N, vector<GRBVar>(NrHaps));
	for (t = 0; t < N; ++t){
		for (h = 0; h < NrHaps; ++h){
			y[t][h] = model.addVar(0, 1, 0.0, GRB_BINARY/*,"y[" + to_string(t) + "," + to_string(h) + "]"*/);
		}
	}

	cout << "Nr of y variables = " << N*NrHaps << endl;

	cout << "c1" << endl;

	for (t = 0; t < N; ++t){
		GRBLinExpr sum_h = 0;
		for (h = 0; h < NrHaps; ++h){
			sum_h += y[t][h];
		}
		model.addConstr(sum_h == 1/*, "c1"*/); // each team is assigned to exactly 1 hap
	}

	z_trs = vector<vector<vector<GRBVar>>>(N, vector<vector<GRBVar>>(NrTrips, vector<GRBVar>(R)));
	for (t = 0; t < N; ++t){
		for (r = 0; r < NrTrips; ++r){
			int L = Trips[t][r].size();
			for (s = 0; s < R; ++s){
				z_trs[t][r][s] = model.addVar(0, 1, 0.0, GRB_CONTINUOUS, "z[" + to_string(t) + "," + to_string(r) + "," + to_string(s) + "]");
				if (s+L > R){
					z_trs[t][r][s].set(GRB_DoubleAttr_UB, 0.0); // trip cannot start in this round
				}
			}
		}
	}

	cout << "c2" << endl;

	for (t = 0; t < N; ++t){
		for (i = t+1; i < N; ++i){
			GRBLinExpr sum_rs = 0;
			for (r = 0; r < NrTrips; r++){
				bool team_found = false;
				for (auto& opp: Trips[i][r]){
					if (opp == t){
						team_found = true;
						break;
					}
				}
				if (team_found){
					for (s = 0; s < LastStartRoundTrip[i][r]; s++){
						sum_rs += z_trs[i][r][s];
					}
				}
				team_found = false;
				for (auto& opp: Trips[t][r]){
					if (opp == i){
						team_found = true;
						break;
					}
				}
				if (team_found){
					for (s = 0; s < LastStartRoundTrip[t][r]; s++){
						sum_rs += z_trs[t][r][s];
					}
				}
			}
			model.addConstr(sum_rs <= 1/*, "c2"*/);
		}
	}

	cout << "c3" << endl;

	for (t = 0; t < N; ++t){
		for (s = 0; s < R; s++){
			GRBLinExpr sum_rsh = 0;
			for (r = 0; r < NrTrips; ++r){
				int L = Trips[t][r].size();
				for (int s_ = max(s-L+1, 0); s_ <= s; s_++){
					if (s_ > LastStartRoundTrip[t][r]){
						break;
					}
					sum_rsh += z_trs[t][r][s_];
				}
			}
			for (h = 0; h < NrHaps; ++h){
				if (getModeHAPRound(h,s) == HA::A){
					sum_rsh -= y[t][h];
				}
			}
			model.addConstr(sum_rsh == 0/*, "c3"*/);
		}
	}

	cout << "c$" << endl;

	for (t = 0; t < N; ++t){
		for (s = 0; s < R; s++){
			GRBLinExpr sum_rsh = 0;
			for (i = 0; i < N; ++i){
				for (r = 0; r < NrTrips; ++r){
					int L = Trips[i][r].size();
					for (int s_ = max(s-L+1, 0); s_ <= s; s_++){
						if (s_ > LastStartRoundTrip[i][r]){
							break;
						}
						for (int l = 0; l < L; ++l){
							if (Trips[i][r][l] == t && s_+l == s){
								sum_rsh += z_trs[i][r][s_];
							}
						}
					}
				}
			}
			for (h = 0; h < NrHaps; ++h){
				if (getModeHAPRound(h,s) == HA::H){
					sum_rsh -= y[t][h];
				}
			}
			model.addConstr(sum_rsh == 0/*, "c4"*/);
		}
	}

	/*
	// Can't we leave this constraint because of Triangle inequality????

	for (t = 0; t < getNrTeams(); ++t){
		for (r = 0; r < NrTrips; ++r){
			for (s = 1; s < R; ++s){
				GRBLinExpr sum_h = 0;
				for (h = 0; h < getNrHAPs(); ++h){
					if (getModeHAPRound(h,s-1) == HA::H){
						sum_h += y[t][h];
					}
				}
				model.addConstr(z_trs[t][r][s] <= sum_h, "c5");
			}
		}
	}
	*/

	cout << "objective" << endl;

	Objective = 0;
	for (t = 0; t < N; ++t){
		for (r = 0; r < NrTrips; ++r){
			for (s = 0; s < LastStartRoundTrip[t][r]; ++s){
				Objective += CostTrips[t][r]*z_trs[t][r][s];
			}
		}
	}
	
	model.setObjective(Objective, GRB_MINIMIZE);

	cout << "done" << endl;
}

void GurSolver::BoundTTP_AllTeams(const int minTrips){

	const int N = getNrTeams();

	int t,r,i;
	
	// Nog eens nakijken of model wel klopt!!!!
	// Trip r is verschillend voor elk team, is dit een probleem??

	vector<pair<vector<vector<int>>,vector<int>>>Trips_CostTrips(N);
	vector<vector<vector<int>>>Trips(N);
	vector<vector<int>>CostTrips(N);
	int NrTrips;
	for (t = 0; t < N; ++t){
		vector<int>TeamsList(N-1);
		int i,j,r;
		for (i = 0, j = -1; i < N; ++i){
			if (i != t){
				TeamsList[++j] = i;
			}
		}
		Trips_CostTrips[t] = GenerateTrips(t, TeamsList);
		Trips[t] = Trips_CostTrips[t].first;
		CostTrips[t] = Trips_CostTrips[t].second;
		NrTrips = CostTrips[t].size();
		if (t > 0){
			assert(NrTrips ==  CostTrips[t-1].size());
		}
	}

	vector<vector<GRBVar>>z_tr = vector<vector<GRBVar>>(N, vector<GRBVar>(NrTrips));
	for (t = 0; t < N; ++t){
		for (r = 0; r < NrTrips; ++r){
			z_tr[t][r] = model.addVar(0, 1, 0.0, GRB_BINARY);
		}
	}

	vector<vector<GRBVar>>y_ti = vector<vector<GRBVar>>(N, vector<GRBVar>(N));
	for (t = 0; t < N; ++t){
		for (i = 0; i < N; ++i){
			y_ti[t][i] = model.addVar(0, 1, 0.0, GRB_BINARY);
		}
	}

	for (i = 0; i < N; ++i){
		model.addConstr(y_ti[i][i] == 0);
	}

	// SRR constraint
	for (i = 0; i < N; ++i){
		for (t = i+1; t < N; ++t){
			model.addConstr(y_ti[t][i] + y_ti[i][t] <= 1);
		}
	}

	for (t = 0; t < N; ++t){
		GRBLinExpr sum_ti = 0;
		GRBLinExpr sum_it = 0;
		for (i = 0; i < N; ++i){
			sum_ti += y_ti[t][i];
			sum_it += y_ti[i][t];
			GRBLinExpr sum_r = 0;
			for (r = 0; r < NrTrips; ++r){
				// auto it = std::find(Trips[r].begin(), Trips[r].end(), TeamsList[i]);
				bool TeamFound = false;
				for (auto& opp: Trips[t][r]){
					if (opp == i){
						TeamFound = true;
						break;
					}
				}
				if (TeamFound){
					sum_r += z_tr[t][r];
				}
			}
			model.addConstr(sum_r == y_ti[t][i]);
		}
		model.addConstr(sum_ti == getNrRounds()/2);
		model.addConstr(sum_it == getNrRounds()/2);
	}

	GRBLinExpr sum_tr = 0;
	for (t = 0; t < N; ++t){
		for (r = 0; r < NrTrips; ++r){
			sum_tr += (Trips[t][r].size()+1)*z_tr[t][r];
		}
	}
	model.addConstr(sum_tr >= minTrips);

	Objective = 0;
	for (t = 0; t < N; ++t){
		for (r = 0; r < NrTrips; ++r){
			Objective += CostTrips[t][r]*z_tr[t][r];
		}
	}
	model.setObjective(Objective, GRB_MINIMIZE);

	/*
	model.optimize();

	cout << "Solution for team " << t << endl;
	for (r = 0; r < NrTrips; ++r){
		if (z_r[r].get(GRB_DoubleAttr_X) > 0.9){
			cout << "This trip has a cost of " << CostTrips[r] << endl;
			cout << t << " -> " << Trips[r][0] << ": " << getDistanceTeams(t,Trips[r][0]) << endl;
			for (int k = 1; k < Trips[r].size(); ++k){
				cout << Trips[r][k-1] << " -> " << Trips[r][k] << ": " << getDistanceTeams(Trips[r][k-1], Trips[r][k]) << endl;
			}
			cout << Trips[r].back() << " -> " << t << ": " << getDistanceTeams(t,Trips[r].back()) << endl;
		}
	}
	cin.get();
	*/
}

void GurSolver::BoundTTP(const int t){

	const int N = getNrTeams();

	vector<int>TeamsList(N-1);
	int i,j,r;
	for (i = 0, j = -1; i < N; ++i){
		if (i != t){
			TeamsList[++j] = i;
		}
	}

	pair<vector<vector<int>>,vector<int>>Trips_CostTrips = GenerateTrips(t, TeamsList);
	vector<vector<int>>Trips = Trips_CostTrips.first;
	vector<int>CostTrips = Trips_CostTrips.second;
	const int NrTrips = CostTrips.size();

	vector<GRBVar>z_r = vector<GRBVar>(NrTrips);
	for (r = 0; r < NrTrips; ++r){
		z_r[r] = model.addVar(0, 1, 0.0, GRB_BINARY);
	}

	vector<GRBVar>y_i = vector<GRBVar>(NrTrips);
	for (i = 0; i < N-1; ++i){
		y_i[i] = model.addVar(0, 1, 0.0, GRB_BINARY);
	}

	GRBLinExpr sum_i = 0;
	for (i = 0; i < N-1; ++i){
		sum_i += y_i[i];
		GRBLinExpr sum_r = 0;
		for (r = 0; r < NrTrips; ++r){
			// auto it = std::find(Trips[r].begin(), Trips[r].end(), TeamsList[i]);
			bool TeamFound = false;
			for (auto& opp: Trips[r]){
				if (opp == TeamsList[i]){
					TeamFound = true;
					break;
				}
			}
			if (TeamFound){
				sum_r += z_r[r];
			}
		}
		model.addConstr(sum_r == y_i[i]);
	}
	model.addConstr(sum_i == getNrRounds()/2);

	Objective = 0;
	for (r = 0; r < NrTrips; ++r){
		Objective += CostTrips[r]*z_r[r];
	}
	model.setObjective(Objective, GRB_MINIMIZE);

	/*
	model.optimize();

	cout << "Solution for team " << t << endl;
	for (r = 0; r < NrTrips; ++r){
		if (z_r[r].get(GRB_DoubleAttr_X) > 0.9){
			cout << "This trip has a cost of " << CostTrips[r] << endl;
			cout << t << " -> " << Trips[r][0] << ": " << getDistanceTeams(t,Trips[r][0]) << endl;
			for (int k = 1; k < Trips[r].size(); ++k){
				cout << Trips[r][k-1] << " -> " << Trips[r][k] << ": " << getDistanceTeams(Trips[r][k-1], Trips[r][k]) << endl;
			}
			cout << Trips[r].back() << " -> " << t << ": " << getDistanceTeams(t,Trips[r].back()) << endl;
		}
	}
	cin.get();
	*/
}

void GurSolver::iTTP(){

	int t,i,j,r;
	const bool HA = true;
	const bool relax_x = false;
	build_base(HA, relax_x); // all base constraints

	if (getNrRounds() < 4){
		cout << "WARNING: iTTP constraints not needed because NrRounds is " << getNrRounds() << endl;

		// Objective function:
		Objective = 0;
		for (i = 0; i < getNrTeams(); ++i){
			for (j = 0; j < getNrTeams(); ++j){
				if (i == j){
					continue;
				}
				for (r = 0; r < getNrRounds(); ++r){
					Objective += (2*getDistanceTeams(i,j))*x[i][j][r];
				}
			}
		}
		model.setObjective(Objective, GRB_MINIMIZE);

		return;
	}

	// z_tij = 1 if t travels from the home venue of i to that of j
	z = vector<vector<vector<GRBVar>>>(getNrTeams(), vector<vector<GRBVar>>(getNrTeams(), vector<GRBVar>(getNrTeams())));
	for (t = 0; t < getNrTeams(); ++t){
		for (i = 0; i < getNrTeams(); ++i){
			for (j = 0; j < getNrTeams(); ++j){
				std::string varName = "z_" + std::to_string(t) + "_" + std::to_string(i) + "_" + std::to_string(j);
				z[t][i][j] = model.addVar(0, 1, 0.0, GRB_BINARY, varName);	   
			}
		}
    }

	// C1: link x-variables with z variable
	// t travels from i to j in r if it played A vs i in r-1 and A vs j in r
	for (t = 0; t < getNrTeams(); ++t){
		for (i = 0; i < getNrTeams(); ++i){
			for (j = 0; j < getNrTeams(); ++j){
				if (t == i || t == j || i == j){
					continue;
				}
				for (r = 1; r < getNrRounds(); ++r){
					model.addConstr(z[t][i][j] >= x[i][t][r-1] + x[j][t][r] - 1);
				}
			}
		}
	}

	// C2: Team t travels from i to its own venue in r if it plays A vs i in r-1 but H in round r

	for (t = 0; t < getNrTeams(); ++t){
		for (i = 0; i < getNrTeams(); ++i){
			if (t == i){
				continue;
			}
			for (r = 1; r < getNrRounds(); ++r){
				GRBLinExpr sum_j = 0;;
				for (j = 0; j < getNrTeams(); ++j){
					if (j == t){
						continue;
					}
					sum_j += x[t][j][r];
				}
				model.addConstr(z[t][i][t] >= x[i][t][r-1] + sum_j - 1);
			}
		}
	}

	// C3: opposite to that of C2, a team t travels from its own location to i in r if it played H in r-1 but away against i in r

	for (t = 0; t < getNrTeams(); ++t){
		for (i = 0; i < getNrTeams(); ++i){
			if (t == i){
				continue;
			}
			for (r = 1; r < getNrRounds(); ++r){
				GRBLinExpr sum_j = 0;;
				for (j = 0; j < getNrTeams(); ++j){
					if (j == t){
						continue;
					}
					sum_j += x[t][j][r-1];
				}
				model.addConstr(z[t][t][i] >= x[i][t][r] + sum_j - 1);
			}
		}
	}

	// C4 and C5: control first and last rounds 

	for (t = 0; t < getNrTeams(); ++t){
		for (i = 0; i < getNrTeams(); ++i){
			if (t == i){
				continue;
			}
			model.addConstr(z[t][t][i] >= x[i][t][0]);
			model.addConstr(z[t][i][t] >= x[i][t][getNrRounds()-1]);
		}
	}

	// C6 and C7: In any 4 consecutive time slots, a team cannot play more than 3 A games or less than 1 H game

	for (t = 0; t < getNrTeams(); ++t){
		for (r = 0; r <= getNrRounds()-4; ++r){
			GRBLinExpr sum_kj = 0;
			for (int k = r; k < r+4; ++k){
				for (j = 0; j < getNrTeams(); ++j){
					if (t == j){
						continue;
					}
					sum_kj += x[j][t][k];
				}
			}
			model.addConstr(sum_kj <= 3);
			model.addConstr(sum_kj >= 1);
		}
	}

	// Objective function:
	Objective = 0;
	for (t = 0; t < getNrTeams(); ++t){
		for (i = 0; i < getNrTeams(); ++i){
			for (j = 0; j < getNrTeams(); ++j){
				if (i == j){
					continue;
				}
				Objective += getDistanceTeams(i,j)*z[t][i][j];
			}
		}
	}
	model.setObjective(Objective, GRB_MINIMIZE);
}

void GurSolver::AddLowerBoundiTTP(const int LB){
	GRBLinExpr sum_LB = 0;
	int t,i,j;
	for (t = 0; t < getNrTeams(); ++t){
		for (i = 0; i < getNrTeams(); ++i){
			for (j = 0; j < getNrTeams(); ++j){
				if (i == j){
					continue;
				}
				sum_LB += getDistanceTeams(i,j)*z[t][i][j];
			}
		}
	}
	model.addConstr(sum_LB >= LB);

}

void GurSolver::Fix_x(Solution& sol){
	for (int r = 0; r < getNrRounds(); ++r){
		vector<bool>NodeSeen(getNrTeams(), false);
		for (int i = 0; i < getNrTeams(); ++i){
			if (NodeSeen[i]){
				continue;
			}
			int j = sol.TeamColorOpp[i][r];
			assert(sol.isEligible(i,j));
			NodeSeen[j] = true; 
			if (sol.Orientation[i][r] == HA::H){
				model.addConstr(x[i][j][r] == 1);
			}
			else{
				model.addConstr(x[j][i][r] == 1);
			}
		}
	}
}

void GurSolver::FixHAP(Solution& sol){
	// cout << "Fix haps" << endl;
	const int Half = getNrRounds()/2;
	for (int i = 0; i < getNrTeams(); ++i){
		if (!HapFixed[i]){
			continue;
		}
		int nr_H = 0;
		int nr_A = 0;
		for (int r = 0; r < getNrRounds(); ++r){
			assert(sol.Orientation[i][r] == HA::H || sol.Orientation[i][r] == HA::A);
			int nr_opp = 0;
			if (sol.Orientation[i][r] == HA::H){
				nr_H++;
				for (int j = 0; j < getNrTeams(); ++j){
					if (!isEligible(i, j)){
						continue;
					}
					// cout << "fix " << j << "-" << i << endl;
					// x[j][i][r].set(GRB_DoubleAttr_UB, 0.0);
					string consName = "x[" + to_string(j) + "][" + to_string(i) + "][" + to_string(r) + "] == 0";
					model.addConstr(x[j][i][r] == 0, consName);
				}
			}
			else {
				nr_A++;
				for (int j = 0; j < getNrTeams(); ++j){
					if (!isEligible(i, j)){
						continue;
					}
					// x[i][j][r].set(GRB_DoubleAttr_UB, 0.0);
					string consName = "x[" + to_string(i) + "][" + to_string(j) + "][" + to_string(r) + "] == 0";
					model.addConstr(x[i][j][r] == 0, consName);
				}
			}
			// assert(nr_opp == 1);
		}
		assert(nr_H == Half && nr_A == Half);
	}
}


void GurSolver::build_base_league(const bool HA, const bool relax_x, const int l){ // l = league

	assert(getNrTeamsLeague(l) % 2 == 0);
	assert(getNrTeamsLeague(l)-1 >= getNrRounds());

	int i,j,r,i_,j_;

	x = vector<vector<vector<GRBVar>>>(getNrTeamsLeague(l), vector<vector<GRBVar>>(getNrTeamsLeague(l), vector<GRBVar>(getNrRounds())));

	for (i = 0; i < getNrTeamsLeague(l); ++i){
		for (j = 0; j < getNrTeamsLeague(l); ++j){
		   if (true /*isEligible(i_, j_)*/){
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

   for (i = 0; i < getNrTeamsLeague(l); ++i){
		for (j = 0; j < getNrTeamsLeague(l); ++j){
		   if (i != j){
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
			   if (getNrRounds() < getNrTeamsLeague(l)-1){ // then not every team can play against any other team
				model.addConstr(sum_r <= 1, consName);
			   }
			   else{
				model.addConstr(sum_r == 1, consName);
			   }
		   }
		}
		for (r = 0; r < getNrRounds(); ++r){
			std::string consName = "c2_" + std::to_string(i) + "_" + std::to_string(r);
			 GRBLinExpr sum_j = 0;
			 for (j = 0; j < getNrTeamsLeague(l); ++j){
				if (i != j){
					sum_j += (x[i][j][r] + x[j][i][r]);
				}
			 }
			 // Each team plays exactly once in each round
			 model.addConstr(sum_j == 1, consName);
		}
   }


    if (!SRR){
		array<pair<int,int>,2>Range = {{{0, getNrRounds()/2}, {getNrRounds()/2, getNrRounds()}}};
		for (i = 0; i < getNrTeamsLeague(l); ++i){
			// Make DRR phased: every team sees an opponent at most once in a half
			for (j = 0; j < getNrTeamsLeague(l); ++j){
				// if (isEligible(i, j)){
					for (auto&[start, end]: Range){
						GRBLinExpr sum_r = 0;
						std::string consName = "c3_" + std::to_string(i) + "_" + std::to_string(j) + "_" + std::to_string(start) + "_" + std::to_string(end);
						for (r = start; r < end; ++r){
							sum_r += (x[i][j][r] + x[j][i][r]);
						}
						model.addConstr(sum_r <= 1, consName);
					}
				// }
			}
		}
	}

	for (i = 0; i < getNrTeamsLeague(l); ++i){
		for (j = 0; j < getNrTeamsLeague(l); ++j){
			i_ = getGlobalIndexTeam(l,i), j_ = getGlobalIndexTeam(l,j);
			if (!isEligible(i_, j_)){
				for (r = 0; r < getNrRounds(); ++r){
					model.addConstr(x[i][j][r] == 0);
				}
			}
		}
	}

	if (!HA){
		return;
	}

	assert(getNrRounds() % 2 == 0);

	int nrH = getNrRounds() / 2;
	int nrA = nrH;
	for (i = 0; i < getNrTeamsLeague(l); ++i){
		GRBLinExpr sum_jr_H = 0;
		// GRBLinExpr sum_jr_A = 0;
		for (j = 0; j < getNrTeamsLeague(l); ++j){
			if (true){
				for (r = 0; r < getNrRounds(); ++r){
					sum_jr_H += x[i][j][r];
					// sum_jr_A += x[j][i][r];
				}
			}
		}
		model.addConstr(sum_jr_H == nrH, "c_3H");
		// model.addConstr(sum_jr_A == nrA, "c_3A");
	}

	for (i = 0; i < getNrTeamsLeague(l); ++i){
		for (r = 2; r < getNrRounds(); ++r){
			GRBLinExpr sum_H = 0, sum_A = 0;
			for (j = 0; j < getNrTeamsLeague(l); ++j){
				sum_H += (x[i][j][r-2] + x[i][j][r-1] + x[i][j][r]);
				sum_A += (x[j][i][r-2] + x[j][i][r-1] + x[j][i][r]);
			}
			model.addConstr(sum_H <= 2, "c_4H");
			model.addConstr(sum_A <= 2, "c_4A");
		}
	}

	// cout << "base done" << endl;

}

void GurSolver::build_base(const bool HA, const bool relax_x){ 

	assert(getNrTeams() % 2 == 0);
	assert(getNrTeams()-1 >= getNrRounds());

	int i,j,r;

	x = vector<vector<vector<GRBVar>>>(getNrTeams(), vector<vector<GRBVar>>(getNrTeams(), vector<GRBVar>(getNrRounds())));

	// cout << "x" << endl;

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

   // cout << "c1" << endl;

   for (i = 0; i < getNrTeams(); ++i){
		for (j = 0; j < getNrTeams(); ++j){
		   if (isEligible(i, j)/*i != j*/){
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
			   if (getNrRounds() < getNrTeams()-1){ // then not every team can play against any other team
				model.addConstr(sum_r <= 1, consName);
			   }
			   else{
				model.addConstr(sum_r == 1, consName);
			   }
		   }
		}
		for (r = 0; r < getNrRounds(); ++r){
			std::string consName = "c2_" + std::to_string(i) + "_" + std::to_string(r);
			 GRBLinExpr sum_j = 0;
			 for (j = 0; j < getNrTeams(); ++j){
				if (/*i != j*/ isEligible(i, j)){
					sum_j += (x[i][j][r] + x[j][i][r]);
				}
			 }
			 // Each team plays exactly once in each round
			 model.addConstr(sum_j == 1, consName);
		}
   }

    // cout << "c3" << endl;

    if (!SRR){
		array<pair<int,int>,2>Range = {{{0, getNrRounds()/2}, {getNrRounds()/2, getNrRounds()}}};
		for (i = 0; i < getNrTeams(); ++i){
			// Make DRR phased: every team sees an opponent at most once in a half
			for (j = 0; j < getNrTeams(); ++j){
				// if (isEligible(i, j)){
					for (auto&[start, end]: Range){
						GRBLinExpr sum_r = 0;
						std::string consName = "c3_" + std::to_string(i) + "_" + std::to_string(j) + "_" + std::to_string(start) + "_" + std::to_string(end);
						for (r = start; r < end; ++r){
							sum_r += (x[i][j][r] + x[j][i][r]);
						}
						model.addConstr(sum_r <= 1, consName);
					}
				// }
			}
		}
	}

	if (!HA){
		return;
	}

	// cout << "c4" << endl;

	assert(getNrRounds() % 2 == 0);

	int nrH = getNrRounds() / 2;
	int nrA = nrH;
	for (i = 0; i < getNrTeams(); ++i){
		GRBLinExpr sum_jr_H = 0;
		// GRBLinExpr sum_jr_A = 0;
		for (j = 0; j < getNrTeams(); ++j){
			if (isEligible(i, j) /*true*/){
				for (r = 0; r < getNrRounds(); ++r){
					sum_jr_H += x[i][j][r];
					// sum_jr_A += x[j][i][r];
				}
			}
		}
		model.addConstr(sum_jr_H == nrH, "c_3H");
		// model.addConstr(sum_jr_A == nrA, "c_3A");
	}

	// cout << "base done" << endl;
}

void GurSolver::build_league(const bool HA, const bool relax_x){

	build_base(HA, relax_x);

	int i,j,r;

	if (IsMaxSameClubConstraint()){
		for (i = 0; i < getNrTeams(); ++i){
		// Each team plays a maximum nr of times against a team from the same club
		const int c = getTeamClub(i);
		GRBLinExpr sum_same_club = 0;
		std::string consName = "c4_" + std::to_string(i);
		for (auto& j: getTeamsClub(c)){
			if (/*isEligible(i,j) && */ !isTeamDummy(j)){
				for (r = 0; r < getNrRounds(); ++r){
					sum_same_club += (x[i][j][r] + x[j][i][r]);
				}
			}
		}
		model.addConstr(sum_same_club <= getMaxSameClub());
	}
	}

   // if we outcomment this, check whether isEligible is put again in all the constraints!!!
   // Much faster for Tiny-Constant
   /*
   for (int i = 0; i < getNrTeams(); ++i){
	for (int j = i+1; j < getNrTeams(); ++j){
		if (!isEligible(i,j)){
			for (r = 0; r < getNrRounds(); ++r){
				x[i][j][r].set(GRB_DoubleAttr_UB, 0.0);
				x[j][i][r].set(GRB_DoubleAttr_UB, 0.0);
			}
		}
	}
   }
   */

   if (HA){
	build_HAP_constraints();
   }
}

void GurSolver::build_HAP_constraints(){

	int i,j,r;

    if (getHAP_requirement(HAP_requirement_name::NoThreeConsecutive)){
		cout << "Add constraint NoThreeConsecutive to model" << endl;
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
	cout << "Add constraint NoBreakBeginningEnd to model" << endl;
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
	 cout << "Add constraint BreakLimit to model with " << getBreakLimit() << " breaks" << endl;
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
	cout << "Add constraint QuarterBalanced to model" << endl;
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

void GurSolver::build_capacity_constraint_league(Solution& sol, const int l){
	v = vector<vector<GRBVar>>(getNrClubs(), vector<GRBVar>(getNrRounds()));

	for (int c = 0; c < getNrClubs(); ++c){ // NrClubs == clubs without dummy
		for (int r = 0; r < getNrRounds(); ++r){
			assert(getNrTeamsClub(c) >= sol.getCapacityClub(c, r));
			int ub_var = getNrTeamsClub(c)-sol.getCapacityClub(c, r);
			v[c][r] = model.addVar(0, ub_var, 0.0, GRB_INTEGER);

			GRBLinExpr sum_tj = 0;
			for (int i = 0; i < getNrTeamsLeague(l); ++i){
				int i_ = getGlobalIndexTeam(l,i);
				if (getTeamClub(i_) != c){
					continue;
				}
				for (int j = 0; j < getNrTeamsLeague(l); ++j){
					int j_ = getGlobalIndexTeam(l,j);
					sum_tj += x[i][j][r];
				}
			}
			// cout << "Capacity of club " << c << " in round " << r << " = " << getCapacityClub(c, r) << endl;
			model.addConstr(sum_tj <= getCapacityClub(c, r) - sol.ComputeCapacityClubRound(c, r) + 0.1 + v[c][r], "Capacity");
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
				// cout << "Capacity of club " << c << " in round " << r << " = " << getCapacityClub(c, r) << endl;
				model.addConstr(sum_tj <= getCapacityClub(c, r) + 0.1 + v[c][r], "Capacity");
			}
		}
	}

	// std::cout << "done building model" << std::endl;

   return 1;
}

void GurSolver::setTimeLimit(const int time_limit){
	model.set(GRB_DoubleParam_TimeLimit, time_limit);
}

void GurSolver::AddObjGeneralCosts(){
	Objective = 0;
	for (int i = 0; i < getNrTeams(); ++i){
		for (int j = 0; j < getNrTeams(); ++j){
			if (/*isEligible(i, j)*/ true){
				for (int r = 0; r < getNrRounds(); ++r){
					Objective += getCostMatchRound(i,j,r)*x[i][j][r];
				}
			}
		}
	}
	model.setObjective(Objective, GRB_MINIMIZE);
}

void GurSolver::AddObjMinBreaks(){
	Objective = 0;
	for (int i = 0; i < getNrTeams(); ++i){
		for (int r = 0; r < getNrRounds(); ++r){
			Objective += b[i][r];
		}
	}
	model.setObjective(Objective, GRB_MINIMIZE);
}

void GurSolver::AddObjMinTravelLeague(const int l){
	Objective = 0;
	for (int i = 0; i < getNrTeamsLeague(l); ++i){
		for (int j = 0; j < getNrTeamsLeague(l); ++j){
			int i_ = getGlobalIndexTeam(l,i), j_ = getGlobalIndexTeam(l,j);
			if (isEligible(i_, j_)){
				for (int r = 0; r < getNrRounds(); ++r){
					Objective += getDistanceTeams(i_, j_)*x[i][j][r];
				}
			}
		}
	}
	model.setObjective(Objective, GRB_MINIMIZE);
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
		for (int c = 0; c < getNrClubs(); ++c){
			if (c == getIndexDummyClub()){
				assert(false);
				continue;
			}
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
	for (int c = 0; c < getNrClubs(); ++c){
		for (int r = 0; r < getNrRounds(); ++r){
			c_cap += v[c][r];
		}
	}
	model.addConstr(c_cap <= getAllowedNrCapacityViolations());
}

/*
void GurSolver::addCallbackToTrackTime(){
	StartTimeGurSolver = std::chrono::high_resolution_clock::now();
    MyCallback cb(TimeTillBestSolutionGurSolverOuter, StartTimeGurSolver);
	model.setCallback(&cb);
}
*/

// unique pointer version
void GurSolver::addCallbackToTrackTime() {
    StartTimeGurSolver = std::chrono::high_resolution_clock::now();
    cb = std::make_unique<MyCallback>(TimeTillBestSolutionGurSolverOuter, StartTimeGurSolver, CurrentTimeStampIndexOuter, TimeStampsOuter, TimeStampSolutionOuter);
    model.setCallback(cb.get());  // Gurobi needs a raw pointer
}

int GurSolver::solve(){


	if (TrackTimeBestSolution){
		addCallbackToTrackTime();
	}

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
		return -1;
	}
	return std::round(model.get(GRB_DoubleAttr_ObjVal)); // In case of time limit this returns the best found objective
}

int GurSolver::getBestObjValue(){
	return model.get(GRB_DoubleAttr_ObjVal);
}

double GurSolver::getMipGap(){
	return model.get(GRB_DoubleAttr_MIPGap);
}

int GurSolver::getBestBound(){
	return (int)model.get(GRB_DoubleAttr_ObjBound);
}

void GurSolver::SaveSolution(Solution& sol){
	sol.clear();
	if (TripModelTTP){
		vector<int>dist_team(getNrTeams(),0);

		for (int t = 0; t < getNrTeams(); ++t){
			// cout << "Trips team " << t << ": " << endl;
			for (int r = 0; r < NrTrips; ++r){
				for (int s = 0; s < getNrRounds(); ++s){
					if (z_trs[t][r][s].get(GRB_DoubleAttr_X) > 0.9){
						// cout << "-------" << endl;
						// cout << "start trip in " << s << endl;
						// cout << "cost = " << CostTrips[t][r] << endl;
						dist_team[t] += CostTrips[t][r];
						// cout << "length = " << Trips[t][r].size() << endl;
						for (int l = 0; l < Trips[t][r].size(); ++l){
							int j = Trips[t][r][l];
							// cout << j << endl;
							sol.TeamColorOpp[t][s+l] = j;
							sol.TeamColorOpp[j][s+l] = t;
							sol.MatchColor[t][j] = s+l;
							if (SRR){
								sol.MatchColor[j][t] = s+l;
							}
							sol.Orientation[j][s+l] = HA::H;
							sol.Orientation[t][s+l] = HA::A;
							int count_hap_t = 0;
							int count_hap_j = 0;
							for (int h = 0; h < getNrHAPs(); ++h){
								if (y[t][h].get(GRB_DoubleAttr_X) > 0.9){
									assert(getModeHAPRound(h,s+l) == HA::A);
									count_hap_t++;
								}
								if (y[j][h].get(GRB_DoubleAttr_X) > 0.9){
									assert(getModeHAPRound(h,s+l) == HA::H);
									count_hap_j++;
								}
							}
							assert(count_hap_t == 1);
							assert(count_hap_j == 1);
						}
						// cout << "-------" << endl;
					}
				}
			}
		}
		for (int t = 0; t < getNrTeams(); ++t){
			if (dist_team[t] != sol.ComputeTravelCostTeamTTP(t)){
				cout << "dist_team " << t << " = " << dist_team[t] << " but real travel dist = " << sol.ComputeTravelCostTeamTTP(t) << endl;
			}
		}
	}
	else{
		for (int r = 0; r < getNrRounds(); ++r){ 
			// cout << "-------" << endl;
			// cout << "Round " << r << endl;
			// cout << "-------" << endl;
			for (int i = 0; i < getNrTeams(); ++i){
				for (int j = 0; j < getNrTeams(); ++j){
					if (!isEligible(i,j)){
						continue;
					}
					if (x[i][j][r].get(GRB_DoubleAttr_X) > 0.9)
					{
						sol.TeamColorOpp[j][r] = i;
						sol.TeamColorOpp[i][r] = j;
						sol.MatchColor[i][j] = r;
						if (SRR){
							sol.MatchColor[j][i] = r;
						}
						sol.Orientation[i][r] = HA::H;
						sol.Orientation[j][r] = HA::A;
						// cout << i << " - " << j << endl;
					}
				}
			}
		}
	}
	// cin.get();
}

void GurSolver::SaveSolutionLeague(Solution& sol, const int l){
	for (int i = 0; i < getNrTeamsLeague(l); ++i){
		for (int r = 0; r < getNrRounds(); ++r){
			for (int j = 0; j < getNrTeamsLeague(l); ++j){
				int i_ = getGlobalIndexTeam(l,i), j_ = getGlobalIndexTeam(l,j);
				assert(!x.empty());
				if (x[i][j][r].get(GRB_DoubleAttr_X) > 0.9){
					assert(isEligible(i_,j_));
					sol.TeamColorOpp[j_][r] = i_;
					sol.TeamColorOpp[i_][r] = j_;
					sol.MatchColor[i_][j_] = r;
					if (SRR){
						sol.MatchColor[j_][i_] = r;
					}
					sol.Orientation[i_][r] = HA::H;
					sol.Orientation[j_][r] = HA::A;
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
				// int ub_var = getNrTeamsClub(c)-getCapacityClub(c, r);
				v[c][r] = model.addVar(0, GRB_INFINITY, 0.0, GRB_CONTINUOUS);
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

	if (getSetting() != Setting::TTP){
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
				model.addConstr(sum_th <= getCapacityClub(c, r) + v[c][r]); // capacity
			}
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

	// cout << "patterns done" << endl;
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

void GurSolver::AddMiaoSymmetryConstraint(){
	// Because teams from the same club is not enough, they also need to have the same strength!!
	for (int c = 0; c < getNrClubs(); ++c){
		for (int i = 0; i < getTeamsClub(c).size(); ++i){
			int i_ = getTeamsClub(c)[i];
			if (!isTeamDummy(i_)){
				for (int j = i+1; j < getTeamsClub(c).size(); ++j){
					int j_ = getTeamsClub(c)[j];
					if (!isTeamDummy(j_) && (getStrenghtTeam(j_) == getStrenghtTeam(i_))){
						GRBLinExpr sum_h = 0;
						for (int h = 0; h < getNrHAPs(); ++h){
							sum_h += (h*(y[i_][h]-y[j_][h]));
						}
						model.addConstr(sum_h >= 0);
					}
				}
			}
		}
	}
	// cout << "added symmetry constraint" << endl;
}

void GurSolver::BuildMiaoFormulation(const bool relax_x, const bool min_travel, const bool min_capacity_violations){
	const bool HA = false;
	build_all(HA, relax_x);
	if (min_travel){
		setBoundCapacityViolations();
	}
	BuildPatternFormulation();
	// link the assigned patterns with the opponent schedule
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

	AddObj(min_travel, min_capacity_violations);

	// AddMiaoSymmetryConstraint(); // does not work with warm start
	
	// Constraint for the dummy teams:
	/*
	for (int i = 0; i < getNrTeams(); i++){
		if (!isTeamDummy(i)){
			GRBLinExpr sum_j = 0;
			for (int j = 0; j < getNrTeams(); ++j){
				if (isTeamDummy(j)){
					for (int r = 0; r < getNrRounds(); ++r){
						sum_j += (x[i][j][r] + x[j][i][r]);
					}
				}
			}
			model.addConstr(sum_j <= 2);
			// In paper of Miao it is stated that "all teams originally assigned to leagues of size m-1
			// in the solution of Toffolo can play against a dummy team at most 2x", but should this not
			// be required for every team??
		}
	}
		*/

	// Limit total nr of dummy games 

	/*
	if (getNrTeams() == 216){
		// this is the U15 instance in Miao
		// Only instance where some leagues have more than 2 dummy teams
		GRBLinExpr sum_ij = 0;
		for (int i = 0; i < getNrTeams(); ++i){
			for (int j = i+1; j < getNrTeams(); ++j){
				if (isTeamDummy(i) && isTeamDummy(j)){
					for (int r = 0; r < getNrRounds(); ++r){
						sum_ij += (x[i][j][r] + x[j][i][r]);
					}
				}
			}
		}
		model.addConstr(sum_ij == 46);
	}
	*/
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
		assert(y[t][sol.getHAPIndexTeam(t)].get(GRB_DoubleAttr_X) > 0.9);
		vector<HA>HAP = sol.getHAP(sol.getHAPIndexTeam(t)); 
		if (printHAPs){
			cout << "Team " << t << " is assigned HAP " << sol.getHAPIndexTeam(t) << " : ";
		}
		for (int h = 0; h < HAP.size(); ++h){
			assert(HAP[h] == HA::H || HAP[h] == HA::A);
			if (HAP[h] == HA::H){
				assert(sol.getModeHAPRound(sol.getHAPIndexTeam(t),h) == HA::H);
				sol.Orientation[t][h] = HA::H;
				if (printHAPs){
					cout << "H";
				}
			}
			else{
				assert(sol.getModeHAPRound(sol.getHAPIndexTeam(t),h) == HA::A);
				sol.Orientation[t][h] = HA::A;
				if (printHAPs){
					cout << "A";
				}
			}
		}
		if (printHAPs){
			cout << endl;
		}
	}
	// cin.get();

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

void GurSolver::PrintVariables(){
	if (!x.empty()){
		for (int r = 0; r < getNrRounds(); ++r){
			cout << "-------" << endl;
			cout << "Round " << r << endl;
			cout << "-------" << endl;
			for (int i = 0; i < getNrTeams(); ++i){
				for (int j = i+1; j < getNrTeams(); ++j){
					if (x[i][j][r].get(GRB_DoubleAttr_X) > 0.01){
						cout << "x[" << i << "][" << j << "][" << r << "] = " << x[i][j][r].get(GRB_DoubleAttr_X) << endl;
					}
					if (x[j][i][r].get(GRB_DoubleAttr_X) > 0.01){
						cout << "x[" << j << "][" << i << "][" << r << "] = " << x[j][i][r].get(GRB_DoubleAttr_X) << endl;
					}
				}
			}
		}
	}
	if (!z.empty()){
		cout << "-----" << endl;
		cout << "TRIP variables: " << endl;
		cout << "-----" << endl;
		for (int t = 0; t < getNrTeams(); ++t){
			for (int i = 0; i < getNrTeams(); ++i){
				for (int j = 0; j < getNrTeams(); ++j){
					if (z[t][i][j].get(GRB_DoubleAttr_X) > 0.01){
						cout << "z[" << t << "][" << i << "][" << j << "] = " << z[t][i][j].get(GRB_DoubleAttr_X) << endl;
					}	   
				}
			}
		}
	}
}

void GurSolver::AssignHAPsToTeams(Solution& sol){

	// cout << "Assign HAPs to teams" << endl;

	BuildPatternFormulation();
	// setBoundCapacityViolations();
	// HAP set!

	const int NrHaps = getNrHAPs();

	const bool min_travel = false;
	const bool min_capacity_violations = true; // set to true!!!
	AddObj(min_travel, min_capacity_violations);

	if (min_capacity_violations){
		setBoundCapacityViolations();
	}

	int obj = solve();

	// cout << "obj = " << obj << " but allowed nr of capacity violations = " << getAllowedNrCapacityViolations() << endl;
	// cin.get();
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
