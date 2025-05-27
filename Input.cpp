#include "Input.h"
#include <iostream>
#include <fstream> // read txt file
#include <sstream> // split numbers of txt file
#include <string>
#include <assert.h>

Input::Input(){}
Input::~Input(){}

int Input::read(const std::string& file_path){

    std::ifstream file(file_path);
    if (!file.is_open()) {
        std::cerr << "Error opening the file!";
        return 0;
    }

    std::string line;

    bool ConstantCapacity = true;
    int i = 0, k = 0, t = 0, l = 0;
    int IndexDummyClub;
    while (getline(file, line)){
        // std::cout << line << std::endl;
        std::istringstream iss(line);  // 
        int num;
        int j = 0;
        if (i == 0){
            while (iss >> num) {  
                if (j == 0){
                    NrTeams = num;
                    Teams = vector<int>(NrTeams);
                    TeamStrength = vector<int>(NrTeams);
                    TeamClub = vector<int>(NrTeams);
                    TeamLeague = vector<int>(NrTeams); // TODO: multiple leagues
                }
                else if (j == 1){
                    NrLeagues = num;
                    LeagueTeams = vector<vector<int>>(NrLeagues);
                    // assert(NrLeagues == 1); // TODO
                }
                else if (j == 2){
                    NrClubs = num; // +1 because we are going to create a dummy club!!
                    IndexDummyClub = NrClubs;
                    ClubTeams = vector<vector<int>>(NrClubs+1);
                    DistanceClubs = vector<vector<int>>(NrClubs+1, vector<int>(NrClubs+1));
                    DistanceClubs[IndexDummyClub] = vector<int>(NrClubs+1, 0);
                }
                else if (j == 3){
                    NrRounds = num;
                    ClubCapacity = vector<vector<int>>(NrClubs+1, vector<int>(NrRounds, -1));
                }
                else if (j == 4){
                    if (num == 1){
                        ConstantCapacity = false;
                    }
                }
                else{
                    std::cerr << "If i is 0 j should be <= 5" << std::endl;
                }
                ++j;
            }
        }
        else if (i < 1+NrClubs){ 
            if (i == 1){
                k = 0; // k = index of the club
                t = 0; // t = index of the team
            }
            while (iss >> num) {  
                // 0 is the size of the club
                if (j == 0){} // Nr of teams in the club
                else if (j == 1){
                    if (ConstantCapacity){
                        std::fill(ClubCapacity[k].begin(), ClubCapacity[k].end(), num);
                    }
                }
                else {
                    Teams[t] = num;
                    TeamClub[num] = k;
                    ClubTeams[k].push_back(num);
                }
                ++j;
            }
            ++k;
        }
        else if (i < 1+2*NrClubs){ 
            if (i == 1+NrClubs){
                k = 0;
            }
            assert(k != IndexDummyClub);
            DistanceClubs[k] = vector<int>(NrClubs+1);
            while (iss >> num) { 
                DistanceClubs[k][j++] = num;
            }
            assert(j == IndexDummyClub);
            DistanceClubs[k][j] = 0;
            ++k;
        }
        else if (i < 1+2*NrClubs+NrLeagues){
            if (i == 1+2*NrClubs){
                t = 0; // t = index team
            }
            // vector<int>LeagueIndexCount(NrLeagues, 0);
            while (iss >> num) { 
                assert(num >= 0);
                if (NrLeagues == 1){
                    assert(l == 0);
                }
                TeamStrength[t] = num-1;
                // cout << "add " << t << " to league " << l << endl;
                LeagueTeams[l].push_back(t); // TODO: only 1 league now, with eligible opponents
                TeamLeague[t] = l;
                // LeagueIndex[t] += LeagueIndexCount[num-1];

                // ++LeagueIndexCount[num-1];
                ++t;
            }
            // after reading the line: new league!
            ++l;
        }
        else{
            if (ConstantCapacity){
                continue;
            }
            int c = i - (1+2*NrClubs+NrLeagues);
            cout << "Nr of teams of club " << c << " = " << getNrTeamsClub(c) << endl;
            int r = 0;
            while (iss >> num) { 
                if (j == 0){} // no meaning
                else if (r < NrRounds) {
                    ClubCapacity[c][r] = num;
                    // cout << "Capacity of club " << c << " in round " << r << " = " << ClubCapacity[c][r] << endl;
                    ++r;
                }
                else{
                    break;
                }
                ++j;
            }
        }
        ++i;
    }
    cout << "done" << endl;

    // Add the dummy teams
    int DummyCapacity = 0;
    for (l = 0; l < getNrLeagues(); ++l){
        if ((int)LeagueTeams[l].size() % 2 != 0){
            cout << "add dummy" << endl;
            Teams.push_back(NrTeams);
            TeamStrength.push_back(l);
            TeamClub.push_back(IndexDummyClub);

            ClubTeams[IndexDummyClub].push_back(NrTeams);
            LeagueTeams[l].push_back(NrTeams);
            ++DummyCapacity;

            ++NrTeams;
        }
    }
    std::fill(ClubCapacity[IndexDummyClub].begin(), ClubCapacity[IndexDummyClub].end(), DummyCapacity);

    for (int c = 0; c < NrClubs; ++c){
        if (ClubTeams[c].size() == 1){
            SingleTeamClubs.push_back(c);
        }
        else if (ClubTeams[c].size() > 1){
            MultiTeamClubs.push_back(c);
        }
    }

    // New: eligible opponents
    int i_, j_;
    Eligible = vector<vector<bool>>(getNrTeams(), vector<bool>(getNrTeams(), false));
    for (l = 0; l < getNrLeagues(); ++l){
        for (i = 0; i < getNrTeamsLeague(l); ++i){ // TODO: more than 1 league
            i_ = getGlobalIndexTeam(l, i);
            for (int j = i+1; j < getNrTeamsLeague(l); ++j){
                j_ = getGlobalIndexTeam(l, j);
                if (abs(TeamStrength[i_]-TeamStrength[j_]) < 2){
                    Eligible[i_][j_] = true;
                    Eligible[j_][i_] = true;  
                }
            }
        }
    }

    TeamLeagueIndex = vector<int>(NrTeams);
    for (int t = 0; t < NrTeams; ++t){
        int l = TeamLeague[t];
        for (int j = 0; j < getNrTeamsLeague(l); ++j){
            if (LeagueTeams[l][j] == t){
                TeamLeagueIndex[t] = j;
                break;
            }
        }
    }

    // ++NrClubs;
    std::cout << "NrTeams = " << NrTeams << ", NrLeagues = " << NrLeagues << ", NrClubs = " << NrClubs << ", NrRounds = " << NrRounds << std::endl;
    for (l = 0; l < NrLeagues; ++l){
        std::cout << "League " << l << " has " << LeagueTeams[l].size() << " teams" << std::endl;
    }

    /*
    std::cout << "## club composition ##" << std::endl;
    for (int c = 0; c < NrClubs; c++){
        std::cout << ClubCapacity[c] << ", ";
        for (auto& t: ClubTeams[c]){
            std::cout << t << ", ";
        }
        std::cout << std::endl;
    }
    std::cout << "## DISTANCE MATRIX ##" << std::endl;
    for (auto& row: DistanceClubs){
        for (auto& cell: row){
            std::cout << cell << ", ";
        }
        std::cout << std::endl;
    }
    std::cout << "## Strength of teams ##" << std::endl;
    for (int t = 0; t < NrTeams; t++){
        std::cout << TeamStrength[t] << ", ";
    }
    std::cout << std::endl;
    */

    /*
    cout << "ConstantCapacity = " << ConstantCapacity << endl;
    for (int c = 0; c < NrClubs; ++c){
        for (int r = 0; r < NrRounds; ++r){
            cout << "Capacity of club " << c << " in round " << r << " = " << ClubCapacity[c][r] << endl;
        }
    }
    cin.get();
    */

    file.close();
    std::cout << "done reading input" << std::endl;
    return 1;
}

void Input::readAllowedNrCapacityViolations(const int num){
    AllowedNrCapacityViolations = num;
}

int Input::read_HAPs(){
    // std::string file_path = "C:\\Users\\kardvrie\\C++\\VSprojects\\test2\\Patterns\\patterns_" + to_string(NrRounds) + "_";
    std::string file_path = "Patterns\\patterns_" + to_string(NrRounds) + "_";
    if (HAP_requirements.at(HAP_requirement_name::BreakLimit)){
        file_path += to_string(BreakLimit) + ".txt";
    }
    else{
        file_path += "all.txt";
    }
    cout << "read " << file_path << endl;
    std::ifstream file(file_path);
    if (!file.is_open()) {
        std::cerr << "No pattern available for " << NrRounds << " rounds and ";
        if (HAP_requirements.at(HAP_requirement_name::BreakLimit)){
            cout << BreakLimit;
        }
        else{
            cout << "no ";
        }
        cout << " breaks" << endl;
        return 0;
    }

    std::string line;
    int num, i;
    int h = 0;
    HA mode;
    vector<vector<HA>>HAPs_even;
    vector<vector<HA>>HAPs_odd;
    while (getline(file, line)){
        // std::cout << line << std::endl;
        std::istringstream iss(line); 
        i = 0; 
        vector<HA>HAP(NrRounds);
        while (iss >> num) {
            assert(num == 0 || num == 1);
            if (num == 1){
                mode = HA::H;
            }
            else{
                mode = HA::A;
            }
            HAP[i++] = mode;
        }
        if (h % 2 == 0){
            HAPs_even.push_back(HAP);
        }
        else{
            HAPs_odd.push_back(HAP);
        }
        ++h;
    }
    ComplementHAP = vector<int>(h);
    int index = 0;
    for (h = 0; h < HAPs_even.size(); ++h){
        HAPs.push_back(HAPs_even[h]);
        HAPs.push_back(HAPs_odd[h]);
        ComplementHAP[index] = index+1;
        ComplementHAP[index+1] = index;
        index += 2;
    }
    TeamsHAP = vector<int>(NrTeams);

    return 1;

}
