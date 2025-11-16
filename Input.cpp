#include "Input.h"
#include <iostream>
#include <fstream> // read txt file
#include <sstream> // split numbers of txt file
#include <string>
#include <cmath>
#include <numeric> // for iota
#include <algorithm> // for sorting

#include "rapidxml/rapidxml.hpp"
#include "rapidxml/rapidxml_utils.hpp"
#include "rapidxml/rapidxml_print.hpp"

using namespace rapidxml;

Input::Input(){}
Input::~Input(){}

void Input::setBaseAlgo(){
    // Base setting: consider whole SRR but take only costs of first NrRounds into account!!
    BaseAlgo = true;
    NrRoundsBaseAlgo = NrRounds;
    // Eerste idee van Base: niet doen!!
    /*
    NrRounds = NrTeams-1;
    cout << "NrRounds = " << NrRounds << endl;
    cout << "NrRoundsBaseAlgo = " << NrRoundsBaseAlgo << endl;

    // extend club capacity
    ClubCapacity = vector<vector<int>>(NrClubs, vector<int>(NrRounds, 1));
    for (auto& CapacityVector: ClubCapacity){
        CapacityVector.insert(CapacityVector.end(), NrRounds-CapacityVector.size(), NrTeams);
    }
        */
}

int Input::getDistanceTeams(const int i, const int j){
    if (i > IsTeamDummy.size() || j > IsTeamDummy.size() || i < 0 || j < 0){
        cout << "i = " << i << ", j = " << j << " in getDistanceTeams()" << endl;
        std::abort();
    }
    if (isTeamDummy(i) || isTeamDummy(j)){
        return 0;
    }
    else{
        return DistanceClubs[TeamClub[i]][TeamClub[j]];
    }
}

void Input::SetDefault(const int NrTeams){
    // All teams same strength, every team has its own club, no dummy teams, all teams in same league
    // In solution: modify everything such that instead of travel cost, we compute the costs

    Teams = vector<int>(NrTeams);
    iota(Teams.begin(), Teams.end(), 0);
    TeamStrength = vector<int>(NrTeams, 1);
    TeamLeague = vector<int>(NrTeams, 1);
    TeamClub = vector<int>(NrTeams);
    iota(TeamClub.begin(), TeamClub.end(), 0);

    NrLeagues = 1;
    LeagueTeams = vector<vector<int>>(1, vector<int>(NrTeams, 0));
    iota(LeagueTeams[0].begin(), LeagueTeams[0].end(), 0);

    NrClubs = NrTeams;
    ClubTeams = vector<vector<int>>(NrTeams);
    ClubTeamsLeague = vector<vector<vector<int>>>(NrTeams, vector<vector<int>>(NrLeagues));
    for (int i = 0; i < NrTeams; ++i){
        ClubTeams.at(i) = {i};
        ClubTeamsLeague.at(i).at(0) = {i};
    }
    ClubCapacity = vector<vector<int>>(NrClubs, vector<int>(NrRounds, 1));
    iota(SingleTeamClubs.begin(), SingleTeamClubs.end(), 0);
    
    Eligible = vector<vector<bool>>(getNrTeams(), vector<bool>(getNrTeams(), true));
    TeamLeagueIndex = vector<int>(NrTeams); // index of the team in its league
    iota(TeamLeagueIndex.begin(), TeamLeagueIndex.end(), 0);
    IsTeamDummy = vector<bool>(NrTeams, false);
}

int Input::read_TTP(const std::string file_path){
    Setting_ = Setting::TTP;
    cout << "Read TTP files" << endl;
    file<> xmlFile(file_path.c_str()); // replace with your file path
    xml_document<> doc;
    doc.parse<0>(xmlFile.data());

    // Get root node <Instance>
    xml_node<> *root = doc.first_node("Instance");
    if (!root) {
        std::cerr << "No <Instance> node found!" << std::endl;
        return 1;
    }

    // ---- Read MetaData ----
    xml_node<> *meta = root->first_node("MetaData");
    if (meta) {
        InstanceName = meta->first_node("InstanceName")->value();
        std::string dataType = meta->first_node("DataType")->value();
        std::string contributor = meta->first_node("Contributor")->value();

        int day = 0;
        int month = 0;
        int year = 0;

        xml_node<> *dateNode = meta->first_node("Date");
        if (auto* dayAttr = dateNode->first_attribute("day")) {
            day = std::stoi(dayAttr->value());
        }
        if (auto* monthAttr = dateNode->first_attribute("month")) {
            month = std::stoi(monthAttr->value());
        }
        if (auto* yearAttr = dateNode->first_attribute("year")) {
            year = std::stoi(yearAttr->value());
        }

        std::cout << "Instance: " << InstanceName << ", Type: " << dataType
                  << ", Contributor: " << contributor << std::endl;
        std::cout << "Date: " << day << "/" << month << "/" << year << std::endl;
    }

    // ---- Read Teams ----
    NrTeams = 0;
    xml_node<> *teamsNode = root->first_node("Resources")->first_node("Teams");
    if (teamsNode) {
        for (xml_node<> *team = teamsNode->first_node("team"); team; team = team->next_sibling("team")) {
            int id = std::stoi(team->first_attribute("id")->value());
            // std::string name = team->first_attribute("name")->value();
            int league = std::stoi(team->first_attribute("league")->value());
            // std::cout << "Team " << id << ": " << name << " (League " << league << ")" << std::endl;
            NrTeams++;
        }
    }

    // ---- Read Rounds ----
    NrRounds = 0;
    xml_node<> *slotsNode = root->first_node("Resources")->first_node("Slots");
    if (slotsNode) {
        for (xml_node<> *slot = slotsNode->first_node("slot"); slot; slot = slot->next_sibling("slot")) {
            //int id = std::stoi(slot->first_attribute("id")->value());
	    //std::cout << "Id: " << id << std::endl;
            NrRounds++;
        }
    }

    if(NrRounds >= NrTeams){
	    std::cout << "Too many slots: " << NrRounds << " vs. " << NrTeams <<". Impossible to play i1RR" << std::endl;
    }

    // ---- Read Distances ----
    MaxEdgeCost = 0;
    DistanceClubs = vector<vector<int>>(NrTeams, vector<int>(NrTeams, 0));
    CostMatchRound = vector<vector<vector<int>>>(NrTeams,vector<vector<int>>(NrTeams,vector<int>(NrRounds, 0)));
    xml_node<> *distancesNode = root->first_node("Data")->first_node("Distances");
    if (distancesNode) {
        for (xml_node<> *dist = distancesNode->first_node("distance"); dist; dist = dist->next_sibling("distance")) {
            int t1 = std::stoi(dist->first_attribute("team1")->value());
            int t2 = std::stoi(dist->first_attribute("team2")->value());
            DistanceClubs[t1][t2] = std::stoi(dist->first_attribute("dist")->value());
            // std::cout << "Distance between team " << t1 << " and " << t2 << " = " << DistanceClubs[t1][t2] << std::endl;
            if (MaxEdgeCost < DistanceClubs[t1][t2]){
                MaxEdgeCost = DistanceClubs[t1][t2];
            }
            for (int r = 0; r < NrRounds; ++r){
                CostMatchRound[t1][t2][r] = DistanceClubs[t1][t2];
                CostMatchRound[t2][t1][r] = DistanceClubs[t1][t2];
            }
        }
    }

    cout << "NrTeams = " << NrTeams << ", NrRounds =  " << NrRounds  << endl;

    SetDefault(NrTeams);

    return 1;
}

int Input::read_CostMinimization(const std::string file_path, InstanceSetCM inst){
    Setting_ = Setting::CM;
    cout << "This function is intended ONLY for cost minimization!!" << endl;

    std::ifstream file(file_path);
    if (!file.is_open()) {
        std::cerr << "Error opening the file!";
        return 0;
    }

    std::string line;
    char comma;
    getline(file, line);
    std::istringstream iss(line);  
    if (inst == InstanceSetCM::Karel){
        iss >> NrTeams >> comma >> NrRounds;
    }
    else if (inst == InstanceSetCM::Jasper){
        iss >> NrTeams;
        NrRounds = NrTeams-1;
    }
    else if (inst == InstanceSetCM::Uthus){
        iss >> NrTeams;
        NrRounds = NrTeams-1;
    }
    else{
        std::cerr << "Instance set should be Karel, Jasper or Uthus!!";
        return 0;
    }
    cout << "NrTeams = " << NrTeams << ", NrRounds =  " << NrRounds  << endl;
    CostMatchRound = vector<vector<vector<int>>>(NrTeams,vector<vector<int>>(NrTeams,vector<int>(NrRounds, 0)));
    int i,j,r,c;
    MaxEdgeCost = 0;
    while (getline(file, line)){
        std::istringstream iss(line);
        if (inst == InstanceSetCM::Karel){
            iss >> i >> comma >> j >> comma >> r >> comma >> c;
        }
        else if (inst == InstanceSetCM::Jasper){
            iss >> i >> j >> r >> c;
        }
        else if (inst == InstanceSetCM::Uthus){
            iss >> i >> j >> r >> c;
            r -= 1;
        }
        if (r < NrRounds){
            CostMatchRound.at(i).at(j).at(r) = c; 
            // cout << "cost of " << i << " vs " << j << " in " << r << " = " << c << endl;
            if (c > MaxEdgeCost){
                MaxEdgeCost = c;
            }
        }
    }
    
    SetDefault(NrTeams);
    return 1;
}

MiaoInstance Input::getMiaoInstance(){
    if (NrTeams == NrTeamsMiaoInstances.at(MiaoInstance::S).first){
        return MiaoInstance::S;
    }
    else if (NrTeams == NrTeamsMiaoInstances.at(MiaoInstance::U13).first){
        return MiaoInstance::U13;
    }
    else if (NrTeams == NrTeamsMiaoInstances.at(MiaoInstance::U15).first){
        return MiaoInstance::U15;
    }
    else if (NrTeams == NrTeamsMiaoInstances.at(MiaoInstance::U17).first){
        return MiaoInstance::U17;
    }
    else if (NrTeams == NrTeamsMiaoInstances.at(MiaoInstance::U21).first){
        return MiaoInstance::U21;
    }
    else if (NrTeams == NrTeamsMiaoInstances.at(MiaoInstance::M).first){
        return MiaoInstance::M;
    }
    else if (NrTeams == NrTeamsMiaoInstances.at(MiaoInstance::Tiny).first){
        return MiaoInstance::Tiny;
    }
    else{
        cout << "NrTeams = " << NrTeams << " did not match any instance of Miao, abort" << endl;
        std::exit(0);
    }
}

void Input::setAllowedNrCapacityViolations1RR(const InputData& data){
    string FilePath = "Instances" + string(PATHSEP);
    if (data.Miao){
        FilePath += "Miao" + string(PATHSEP) + "Vcr" + string(PATHSEP);
        FilePath += data.Instance + "_s" + to_string(data.CapacitySetting) + "_b" + to_string(data.MaxNrBreaks) + ".txt";
    }
    else if (data.Hockey){
        FilePath += "Hockey" + string(PATHSEP) + "Vcr" + string(PATHSEP) + data.Instance + ".txt";
    }
    else{
        cout << "Capacity violations only for Miao or Hockey!" << endl;
        std::abort();
    }
    std::ifstream file(FilePath);
    if (!file.is_open()) {
        std::cerr << "Error opening the file " << FilePath;
        return;
    }
    std::string line;
    std::getline(file, line);

    // Read only first line
    std::getline(file, line);
    std::stringstream ss(line);
    string i;
    int s,b,v;
    char comma;
    getline(ss, i, ',');
    if (data.Miao){
        if (ss >> s >> comma >> b >> comma >> v) {
            AllowedNrCapacityViolations = v;
        }
    }
    else{
        if (ss >> v) {
            AllowedNrCapacityViolations = v;
        }
    }
}

void Input::setAllowedNrCapacityViolations2RR(){
    if (HAP_requirements.at(HAP_requirement_name::BreakLimit)){
        assert(BreakLimit == 0 || BreakLimit == 1 || BreakLimit == 2 || BreakLimit == 3);
    }
    if (!ConstantCapacity){
        assert(MiaoHAPSetting == 1 || MiaoHAPSetting == 2);
    }
    if (ConstantCapacity){ // constant
        if (InstanceMiao == MiaoInstance::U15){ 
            AllowedNrCapacityViolations = 14;
        }
        else if (InstanceMiao == MiaoInstance::M){
            AllowedNrCapacityViolations = 175;
        }
        else{
            AllowedNrCapacityViolations = 0;
        }
    }
    else{
        if (MiaoHAPSetting == 2){
            if (InstanceMiao == MiaoInstance::Tiny){ // Tiny
                if (BreakLimit == 0){
                    AllowedNrCapacityViolations = 28;
                }
                else if (BreakLimit == 1){
                    AllowedNrCapacityViolations = 4;
                }
                else{
                    AllowedNrCapacityViolations = 0;
                }
            }
            else{
                AllowedNrCapacityViolations = 0;
            }
        }
        else{
            if (InstanceMiao == MiaoInstance::Tiny){
                if (BreakLimit == 0){
                    AllowedNrCapacityViolations = 44;
                }
                else if (BreakLimit == 1){
                    AllowedNrCapacityViolations = 31;
                }
                else if (BreakLimit == 2) {
                    AllowedNrCapacityViolations = 24;
                }
                else if (BreakLimit == 3) {
                    AllowedNrCapacityViolations = 21;
                }
            }
            else if (InstanceMiao == MiaoInstance::S){ // S
                if (BreakLimit == 0){
                    AllowedNrCapacityViolations = 167;
                }
                else if (BreakLimit == 1){
                    AllowedNrCapacityViolations = 140;
                }
                else if (BreakLimit == 2){
                    AllowedNrCapacityViolations = 108;
                }
                else if (BreakLimit == 3){
                    AllowedNrCapacityViolations = 96;
                }
            }
            else if (InstanceMiao == MiaoInstance::U21){ // U21
                if (BreakLimit == 0){
                    AllowedNrCapacityViolations = 164;
                }
                else if (BreakLimit == 1){
                    AllowedNrCapacityViolations = 127;
                }
                else if (BreakLimit == 2){
                    AllowedNrCapacityViolations = 100;
                }
                else if (BreakLimit == 3){
                    AllowedNrCapacityViolations = 87;
                }
            }
            else if (InstanceMiao == MiaoInstance::U17){ // U17
                if (BreakLimit == 0){
                    AllowedNrCapacityViolations = 359;
                }
                else if (BreakLimit == 1){
                    AllowedNrCapacityViolations = 271;
                }
                else if (BreakLimit == 2){
                    AllowedNrCapacityViolations = 224;
                }
                else if (BreakLimit == 3){
                    AllowedNrCapacityViolations = 194;
                }
            }
            else if (InstanceMiao == MiaoInstance::U13){ // U13
                if (BreakLimit == 0){
                    AllowedNrCapacityViolations = 453;
                }
                else if (BreakLimit == 1){
                    AllowedNrCapacityViolations = 333;
                }
                else if (BreakLimit == 2){
                    AllowedNrCapacityViolations = 267;
                }
                else if (BreakLimit == 3){
                    AllowedNrCapacityViolations = 233;
                }
            }
            else if (InstanceMiao == MiaoInstance::U15){ // U15
                if (BreakLimit == 0){
                    AllowedNrCapacityViolations = 497;
                }
                else if (BreakLimit == 1){
                    AllowedNrCapacityViolations = 376;
                }
                else if (BreakLimit == 2){
                    AllowedNrCapacityViolations = 301;
                }
                else if (BreakLimit == 3){
                    AllowedNrCapacityViolations = 264;
                }
            }
            else {
                assert(InstanceMiao == MiaoInstance::M);
                AllowedNrCapacityViolations = 807;
            }
        }
    }
}

int Input::read_Miao_Hockey(const std::string file_path, const bool Miao){

    cout << "This function is intended ONLY for Hockey and Miao instances!!" << endl;

    if (Miao){
        Setting_ = Setting::Miao;
    }
    else{
        Setting_ = Setting::Hockey;
    }

    std::ifstream file(file_path);
    if (!file.is_open()) {
        std::cout << "Error opening the file!" << endl;
        return 0;
    }

    std::string line;

    ConstantCapacity = true;
    int i = 0, k = 0, t = 0, l = 0;
    int MaxDistanceClubs = 0;
    while (getline(file, line)){
        // std::cout << line << std::endl;
        std::istringstream iss(line);  
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
                    if (Miao){
                        InstanceMiao = getMiaoInstance();
                    }
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
                    ClubTeamsLeague = vector<vector<vector<int>>>(NrClubs+1);
                    for (int clb = 0; clb < NrClubs; ++clb){
                        ClubTeamsLeague[clb] = vector<vector<int>>(NrLeagues);
                    }
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
                // cout << "start saving info of clubs" << endl;
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
                // cout << "start saving info of travel distances" << endl;
                k = 0;
            }
            assert(k != IndexDummyClub);
            DistanceClubs[k] = vector<int>(NrClubs+1);
            while (iss >> num) { 
                if (!(Miao && InstanceMiao == MiaoInstance::M)){
                    DistanceClubs[k][j++] = num;
                }
                else if (k < NrTeamsMiaoInstances.at(MiaoInstance::U13).first-NrTeamsMiaoInstances.at(MiaoInstance::U13).second
                    && j < NrTeamsMiaoInstances.at(MiaoInstance::U13).first-NrTeamsMiaoInstances.at(MiaoInstance::U13).second){
                    // cout << "dist = " << DistanceClubs[TeamClub[k]][TeamClub[j]] << endl;
                    if (DistanceClubs[TeamClub[k]][TeamClub[j]] > 0){
                        /*
                        cout << "-------"  << endl;
                        cout << "Club of " << k << " = " << TeamClub[k] << endl;
                        cout << "Club of " << j << " = " << TeamClub[j] << endl;
                        cout << "dist is " << DistanceClubs[TeamClub[k]][TeamClub[j]] << " but num is " << num << endl;
                        cout << "-------" << endl;
                        */
                        assert(DistanceClubs[TeamClub[k]][TeamClub[j]] ==  num);
                    }
                    DistanceClubs[TeamClub[k]][TeamClub[j++]] =  num;
                }
                if (num > MaxDistanceClubs){
                    MaxDistanceClubs = num;
                }
            }
            if (!(Miao && InstanceMiao == MiaoInstance::M)){
                assert(j == IndexDummyClub);
                DistanceClubs[k][j] = 0;
            }
            ++k;
        }
        else if (i < 1+2*NrClubs+NrLeagues){
            // cout << "Populate league " << l << endl;
            if (i == 1+2*NrClubs){
                t = 0; // t = index team
            }
            // vector<int>LeagueIndexCount(NrLeagues, 0);
            while (iss >> num) { 
                assert(num >= 0);
                assert(t < NrTeams);
                if (NrLeagues == 1){
                    assert(l == 0);
                }
                TeamStrength[t] = num-1;
                if (!(Miao && InstanceMiao == MiaoInstance::M)){
                    LeagueTeams[l].push_back(t); // TODO: only 1 league now, with eligible opponents
                    TeamLeague[t] = l;
                }
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
            // cout << "Nr of teams of club " << c << " = " << getNrTeamsClub(c) << endl;
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
    // cout << "done" << endl;

    // Add the dummy teams; only when not doing Miao instances (the dummy teams are hidden under the normal teams)
    // go over the teams and specify which teams are dummy teams and which are not!
    int NrNonDummyTeams = 0;
    if (Miao){
        NrNonDummyTeams = NrTeams - NrTeamsMiaoInstances.at(InstanceMiao).second;
    }
    else{
        NrNonDummyTeams = NrTeams;
    }
    int DummyCapacity = 0;
    for (l = 0; l < getNrLeagues(); ++l){
        cout << "League " << l << " has size " << LeagueTeams[l].size() << endl;
        if ((int)LeagueTeams[l].size() % 2 != 0){
            assert(false); // All instances should have leagues of even size!!!
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

    if (Miao && InstanceMiao == MiaoInstance::M){
        LeagueTeams.resize(4);
        // for the Merged instance in Miao, we need something else!
        for (int p = 0; p < NrTeams; ++p){
            // Teams are ordered in this order: U13->U15->U17->U21
            if (p < NrTeamsMiaoInstances.at(MiaoInstance::U13).first - NrTeamsMiaoInstances.at(MiaoInstance::U13).second){
                // Put teams into first league
                LeagueTeams[0].push_back(t); 
                TeamLeague[t] = 0;
            } 
            else if (p < NrTeamsMiaoInstances.at(MiaoInstance::U13).first){
                LeagueTeams[0].push_back(t); 
                TeamLeague[t] = 0;
                IsTeamDummy[t] = true;
            }
            if (p < NrTeamsMiaoInstances.at(MiaoInstance::U15).first - NrTeamsMiaoInstances.at(MiaoInstance::U15).second){
                LeagueTeams[1].push_back(t); 
                TeamLeague[t] = 1;
            } 
            else if (p < NrTeamsMiaoInstances.at(MiaoInstance::U15).first){
                LeagueTeams[1].push_back(t); 
                TeamLeague[t] = 1;
                IsTeamDummy[t] = true;
            }
            if (p < NrTeamsMiaoInstances.at(MiaoInstance::U17).first - NrTeamsMiaoInstances.at(MiaoInstance::U17).second){
                LeagueTeams[2].push_back(t); 
                TeamLeague[t] = 2;
            } 
            else if (p < NrTeamsMiaoInstances.at(MiaoInstance::U17).first){
                LeagueTeams[2].push_back(t); 
                TeamLeague[t] = 2;
                IsTeamDummy[t] = true;
            }
            if (p < NrTeamsMiaoInstances.at(MiaoInstance::U21).first - NrTeamsMiaoInstances.at(MiaoInstance::U21).second){
                LeagueTeams[3].push_back(t); 
                TeamLeague[t] = 3;
            } 
            else if (p < NrTeamsMiaoInstances.at(MiaoInstance::U21).first){
                LeagueTeams[3].push_back(t); 
                TeamLeague[t] = 3;
                IsTeamDummy[t] = true;
            }
        }
    }
    else{
        IsTeamDummy = vector<bool>(NrTeams, false);
        for (int p = NrNonDummyTeams; p < IsTeamDummy.size(); p++){
            IsTeamDummy[p] = true;
        }   
    }

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
    Eligible = vector<vector<bool>>(getNrTeams(), vector<bool>(getNrTeams(), false)); // TODO
    for (l = 0; l < getNrLeagues(); ++l){
        for (i = 0; i < getNrTeamsLeague(l); ++i){ // TODO: more than 1 league
            i_ = getGlobalIndexTeam(l, i);
            for (int j = i+1; j < getNrTeamsLeague(l); ++j){
                j_ = getGlobalIndexTeam(l, j);
                if (abs(TeamStrength[i_]-TeamStrength[j_]) < 2){
                    Eligible[i_][j_] = true;
                    Eligible[j_][i_] = true;  
                }
                else if (isTeamDummy(i_) || isTeamDummy(j_)){
                    Eligible[i_][j_] = true;
                    Eligible[j_][i_] = true;
                }
            }
        }
    }

    TeamLeagueIndex = vector<int>(NrTeams);
    for (int t = 0; t < NrTeams; ++t){
        int l = TeamLeague[t];
        bool index_found = false;
        for (int j = 0; j < getNrTeamsLeague(l); ++j){
            if (LeagueTeams[l][j] == t){
                TeamLeagueIndex[t] = j;
                index_found = true;
                break;
            }
        }
        assert(index_found);
    }

    SingleTeamClubsLeague = vector<vector<int>>(NrLeagues);
    MultiTeamClubsLeague = vector<vector<int>>(NrLeagues);

    for (int l = 0; l < NrLeagues; ++l){
        for (int c = 0; c < NrClubs; ++c){
            int count = 0;
            for (int i = 0; i < getNrTeamsLeague(l); ++i){
                int i_ = getGlobalIndexTeam(l,i);
                if (getTeamClub(i_) == c){
                    ClubTeamsLeague[c][l].push_back(i_);
                    count++;
                }
            }
            if (count == 1){
                SingleTeamClubsLeague[l].push_back(c);
            }
            else if (count > 1){
                MultiTeamClubsLeague[l].push_back(c);
            }
        }
        cout << "Size SingleTeamClubs in league " << l << " = " << SingleTeamClubsLeague[l].size() << endl;
        cout << "Size MultiTeamClubs in league " << l << " = " << MultiTeamClubsLeague[l].size() << endl;
    }

    // set Maximum cost of an edge )> for maximum weight matching
    MaxEdgeCost = MaxDistanceClubs;

    // In Matching: always use CostMatchRound from now on!!
    CostMatchRound = vector<vector<vector<int>>>(NrTeams,vector<vector<int>>(NrTeams,vector<int>(NrRounds, 0)));
    for (int r = 0; r < NrRounds; ++r){
        for (int i = 0; i < NrTeams; ++i){
            for (int j = i; j < NrTeams; ++j){
                CostMatchRound[i][j][r] = getDistanceTeams(i,j);
                CostMatchRound[j][i][r] = getDistanceTeams(i,j);
            }
        }
    }

    // ++NrClubs;

    std::cout << "NrTeams = " << NrTeams << ", NrLeagues = " << NrLeagues << ", NrClubs = " << NrClubs << ", NrRounds = " << NrRounds << std::endl;

    /*
    for (l = 0; l < NrLeagues; ++l){
        std::cout << "League " << l << " has " << LeagueTeams[l].size() << " teams" << std::endl;
    }
    std::cout << "Nr of dummy clubs = " << NrTeams - NrNonDummyTeams << endl;
    */

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
    // std::cout << "done reading input" << std::endl;
    return 1;
}

void Input::readAllowedNrCapacityViolations(const int num){
    AllowedNrCapacityViolations = num;
}

bool Input::HAP_satisfies_all_requirements(const vector<HA>& HAP){
    if (HAP_requirements.at(HAP_requirement_name::NoThreeConsecutive)){
        for (int h = 2; h < HAP.size(); ++h){
            if (HAP[h] == HAP[h-1] && HAP[h-1] == HAP[h-2]){
                // cout << "HHH or AAA detected in HAP" << endl;
                // cin.get();
                return false;
            }
        }
    }
    if (HAP_requirements.at(HAP_requirement_name::NoBreakBeginningEnd)){
        if (HAP[0] == HAP[1] || HAP[(int)HAP.size()-1] == HAP[(int)HAP.size()-2]){
            // cout << "break beginning or end in HAP" << endl;
            // cin.get();
            return false;
        }
    }
    if (HAP_requirements.at(HAP_requirement_name::BreakLimit)){
        int nr_breaks = 0;
        for (int h = 1; h < HAP.size(); ++h){
            if (HAP[h] == HAP[h-1]){
                nr_breaks++;
            }
        }
        if (nr_breaks > BreakLimit){
            // cout << "Nr breaks higher than limit in HAP" << endl;
            // cin.get();
            return false;
        }
    }
    if (HAP_requirements.at(HAP_requirement_name::QuarterBalanced)){
        int nr_H1 = 0;
        int nr_H2 = 0;
        int lb = floor(HAP.size()/4);
        int ub = lb+1;
        for (int h = 0; h < HAP.size()/2; ++h){
            if (HAP[h] == HA::H){
                nr_H1++;
            }
        }
        for (int h = HAP.size()/2; h < HAP.size(); ++h){
            if (HAP[h] == HA::H){
                nr_H2++;
            }
        }
        if (nr_H1 < lb || nr_H2 < lb || nr_H1 > ub || nr_H2 > ub){
            // cout << "HAP not quarter balanced" << endl;
            // cin.get();
            return false;
        }
    }
    return true;
}

int Input::read_HAPs(){
    // std::string file_path = "C:\\Users\\kardvrie\\C++\\VSprojects\\test2\\Patterns\\patterns_" + to_string(NrRounds) + "_";
    std::string file_path = "Patterns" + std::string(PATHSEP) + "patterns_" + to_string(NrRounds) + "_";
    if (InstanceMiao == MiaoInstance::M){
        file_path += "c.txt"; // always chose canoncial for this instance
        BreakLimit = 3; // Teams have max 3 breaks
    }
    else if (HAP_requirements.at(HAP_requirement_name::BreakLimit)){
        file_path += to_string(BreakLimit) + ".txt";
    }
    else{
        file_path += "all.txt";
    }
    // cout << "read " << file_path << endl;
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
            HAPs_even.push_back(HAP); // line i and line i+1 with i even are complementary!!
        }
        else{
            HAPs_odd.push_back(HAP);
        }
        ++h;
    }
    int index = 0;
    cout << "Nr of HAPs listed = " << h << endl;
    for (h = 0; h < HAPs_even.size(); ++h){
        if (InstanceMiao != MiaoInstance::M && !HAP_satisfies_all_requirements(HAPs_even[h])){ // preprocess the haps!
            // Do do not preprocess for the canoncial HAP set
            // cout << "HAP with index " << h << " not satisfactory " << endl;
            continue;
        }
        HAPs.push_back(HAPs_even[h]);
        HAPs.push_back(HAPs_odd[h]);
        ComplementHAP.push_back(index+1);
        ComplementHAP.push_back(index);
        index += 2;
    }
    TeamsHAP = vector<int>(NrTeams);
    cout << HAPs.size() << " satisfactory haps" << endl;

    return 1;

}

void Input::DeleteNonPromisingHAPsTTP(const int NrHaps){
    // first, sort the HAPs based on nr of breaks
    vector<pair<int, int>>HapIndexNrBreaks;
    int h,r,NrBreaks;
    for (h = 0; h < HAPs.size(); ++h){
        NrBreaks = 0;
        for (r = 1; r < NrRounds; ++r){
            if (HAPs[h][r-1] == HAPs[h][r]){
                ++NrBreaks;
            }
        }
        HapIndexNrBreaks.emplace_back(h, NrBreaks);
    }

    std::sort(HapIndexNrBreaks.begin(), HapIndexNrBreaks.end(),
              [](const auto &a, const auto &b) {
                  return a.second > b.second;  // descending by value
              });

    vector<vector<HA>>NewHAPs;
    NewHAPs.reserve(NrHaps);
    int i = 0;
    vector<bool>HAPSeen(HAPs.size(), false);
    for (const auto &[h, b] : HapIndexNrBreaks){
        if (HAPSeen.at(h)){
            continue;
        }
        NewHAPs.push_back(HAPs[h]);
        NewHAPs.push_back(HAPs[getComplementIndexHAP(h)]);
        HAPSeen.at(getComplementIndexHAP(h)) = true;
        // std::cout << "HAP " << h << " has " << b << " breaks" << "\n";
        i += 2;
        if (i >= NrHaps){
            break;
        }
    }
    HAPs = std::move(NewHAPs);
    cout << "Nr of HAPs that will be used = " << HAPs.size() << endl;

    AllHAPsIncluded = false;

}
