#include "InitialSolution.h"
#include <assert.h>
#include <iostream>
#include <fstream>
#include <filesystem>
#include <numeric> // for iota
#include "GurSolver.h"

#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/edge_coloring.hpp>
#include <boost/graph/properties.hpp>

void setHaps(const int c1, const int c2, Solution& sol){
	// The idea is that even 2 disjoint perfect matchings decompose into even cycles, so we can always find an orientation where the
	// nr of H games = nr A games
	vector<bool>NodeSeen(sol.getNrTeams(), false);
	bool stop = false;
	int StartNode = 0;
	int i,j;
	while (true){
		i = StartNode;
		do{
			NodeSeen[i] = true;
			j = sol.TeamColorOpp[i][c1];
			sol.Orientation[i][c1] = HA::H;
			sol.Orientation[j][c1] = HA::A;
			sol.MatchColor[i][j] = c1;
			if (sol.SRR){
				sol.MatchColor[j][i] = c1;
			}
			i = j;
			NodeSeen[i] = true;
			j = sol.TeamColorOpp[i][c2];
			sol.Orientation[i][c2] = HA::H;
			sol.Orientation[j][c2] = HA::A;
			sol.MatchColor[i][j] = c2;
			if (sol.SRR){
				sol.MatchColor[j][i] = c2;
			}
			i = j;
		}
		while (i != StartNode);

		bool EmptyNodeFound = false;
		for (i = 0; i < sol.getNrTeams(); ++i){
			if (!NodeSeen[i]){
				EmptyNodeFound = true;
				StartNode = i;
				break;
			}
		}
		if (!EmptyNodeFound){
			break;
		}
	}
}

void RepairBalanceHA(Solution& sol){
	// Only takes into account that teams play an equal nr of H and A games!!
	/*
	INZICHT: als we dit zo doen, voldoen we altijd aan de volgende constraints:
		1) No 3 consecutive H or A games
		2) No break in beginning and end
		3) No imbalance in the halfs
	Aantal breaks kunnen wel zeer slecht worden
	*/
	int c1 = 0;
	int c2;
	int R = sol.getNrRounds();
	if (R % 2 != 0){
		R -= 1;
	}
	while (c1 < R){
		c2 = c1+1;
		setHaps(c1, c2, sol);
		c1 += 2;
	}
	// Leave the last round as it is

	if (R != sol.getNrRounds()){
		assert(R == sol.getNrRounds()-1);
		c1 = R;
		vector<bool>NodeSeen(sol.getNrTeams(), false);
		for (int i = 0; i < sol.getNrTeams(); ++i){
			if (!NodeSeen[i]){
				int j = sol.TeamColorOpp[i][c1];
				sol.Orientation[i][c1] = HA::H;
				sol.Orientation[j][c1] = HA::A;
				NodeSeen[i] = true;
				NodeSeen[j] = true;
			}
		}
	}
}

void CircleMethod(vector<int>& Teams, Solution& sol){

    int temp, a, b;
    int N = sol.getNrTeams();
    assert(N % 2 == 0);
	assert(N == Teams.size());

	vector<int>circle(N);
	for (int c = 0; c < N; ++c)
		circle[c] = Teams[c];

	for (int c = 0; c < N - 1; ++c)
	{
		if (c % 2 == 0){
			a = circle[N - 1];
			b = circle[0];
		}
		else{
			b = circle[N - 1];
			a = circle[0];
		}
		sol.SetColorMatch(a, b, c);
		for (int k = 1; k < N / 2; ++k)
		{
			if (c % 2 == 0){
				a = circle[k];
				b = circle[N - 1 - k];
			}
			else{
				b = circle[k];
				a = circle[N - 1 - k];
			}
			sol.SetColorMatch(a, b, c);
		}
		temp = circle[0];
		for (int j = 0; j < N - 1; j++)
		{
			circle[j] = circle[j + 1];
		}
		circle[N - 2] = temp;
	}

	// Repair the HAPs
	RepairBalanceHA(sol);
}

typedef pair<int, int>E;
typedef boost::adjacency_list< boost::vecS, boost::vecS, boost::undirectedS, boost::no_property, size_t,boost::no_property >BGraph;

pair<BGraph,vector<E>> Vizing(const int N, const int seed){

	vector<E>edge_vector;

	// To get a feasible color, leave node N out at first
	int i,j;
	for (i = 0; i < N-1; ++i){
		for (j = i+1; j < N-1; ++j){
			edge_vector.push_back(E(i,j));
		}
	}

	// shuffle the edge vector to get truly random edge colorings
	std::mt19937 gen(seed);
    std::shuffle(edge_vector.begin(), edge_vector.end(), gen);

    typedef boost::adjacency_list< boost::vecS, boost::vecS, boost::undirectedS, boost::no_property, size_t,boost::no_property >BGraph;

    BGraph g(N-1); 
    for (std::size_t j = 0; j < edge_vector.size(); ++j){
        boost::add_edge(edge_vector[j].first, edge_vector[j].second, g);
    }
	size_t colors = boost::edge_coloring(g, get(boost::edge_bundle, g));

	return {g, edge_vector};
}

vector<vector<bool>>VizingRegularColorableGraph(const int N, const int R, const int seed){ // returns a 2D vector A where A[i][j] = true means that i and j are adjacent (so returns the induced opponent graph)
	vector<vector<bool>>A(N, vector<bool>(N));
	pair<BGraph,vector<E>>Output = Vizing(N, seed);
	BGraph g = Output.first;
	vector<E>edge_vector = Output.second;

	vector<vector<bool>>ColorLeft(N-1, vector<bool>(N-1, true));

	int i,j,c;
    for (size_t k = 0; k < edge_vector.size(); ++k)
    {
		i = edge_vector[k].first;
		j = edge_vector[k].second;
		c = g[boost::edge(i, j, g).first];
		ColorLeft[i][c] = false;
		ColorLeft[j][c] = false;
		if (c < R){
			A[i][j] = true;
			A[j][i] = true;
		}
    }

	int LastNode = N-1;

	for (i = 0; i < N-1; ++i){
		c = 0;
		while (!ColorLeft[i][c]){
			++c;
		}
		if (c < R){
			A[i][LastNode] = true;
			A[LastNode][i] = true;
		}
	}

	return A;
}


void VizingConstruction(Solution& sol, const int seed){ 
	const int N = sol.getNrTeams();

	pair<BGraph,vector<E>>Output = Vizing(N, seed);
	BGraph g = Output.first;
	vector<E>edge_vector = Output.second;

	// Now, for assign colors to the edges with node N

	vector<vector<bool>>ColorLeft(N-1, vector<bool>(N-1, true));

	vector<int>ShuffledTeams(N);
	iota(ShuffledTeams.begin(), ShuffledTeams.end(), 0);
	std::mt19937 gen(seed);
	std::shuffle(ShuffledTeams.begin(), ShuffledTeams.end(), gen);

	// cout << "set Colorleft" << endl;
	int i,j,c,i_,j_;
    for (size_t k = 0; k < edge_vector.size(); ++k)
    {
		i = edge_vector[k].first;
		j = edge_vector[k].second;
		c = g[boost::edge(i, j, g).first];
		ColorLeft[i][c] = false;
		ColorLeft[j][c] = false;
		if (c < sol.getNrRounds()){
			i_ = ShuffledTeams[i];
			j_ = ShuffledTeams[j];
			sol.TeamColorOpp[i_][c] = j_;
    		sol.TeamColorOpp[j_][c] = i_;
		}
    }

	int LastNode = N-1;
	int LastNode_ = ShuffledTeams[LastNode];

	for (i = 0; i < N-1; ++i){
		c = 0;
		while (!ColorLeft[i][c]){
			++c;
		}
		i_ = ShuffledTeams[i];
		if (c < sol.getNrRounds()){
			sol.TeamColorOpp[i_][c] = LastNode_;
    		sol.TeamColorOpp[LastNode_][c] = i_;
		}
	}

	// set MatchColors in repair balance!!

	RepairBalanceHA(sol);

	return;
}
