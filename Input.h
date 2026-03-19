#ifndef INPUT_H  
#define INPUT_H

#include <string>
#include <vector>
#include <unordered_map>
#include <assert.h>
#include <limits.h>
#include <iostream>

using namespace std;

enum class Move{TS,PTS,RS,PRS,C,NC,
    MinCost_BM, Random_BM,
    iPTS_MinCost_PR, iPTS_Random_PR,
    MinCost_M_MinCost_PR, MinCost_M_Random_PR, Random_M_MinCost_PR, Random_M_Random_PR,
    InterClubSwap, IntraClubSwap, RandomSwap, ComplementInsertion, // Li et al HAP operators
    HomeAwaySwap}; 

enum class Setting{Football,Hockey,TTP};
enum class HAP_requirement_name{NoThreeConsecutive, NoBreakBeginningEnd, BreakLimit, QuarterBalanced};
enum class FootballInstance{S, U13, U15, U17, U21, M, Tiny}; // Instances paper Li et al

const std::unordered_map<Move, string>GreedyMatchingMoves = {{Move::InterClubSwap, "InterClubSwap"}, {Move::IntraClubSwap, "IntraClubSwap"}, {Move::RandomSwap, "RandomSwap"}, {Move::ComplementInsertion, "ComplementInsertion"}};
const std::unordered_map<Move, double>GreedyMatchingWeights = {{Move::InterClubSwap, 1.0/3.0}, {Move::IntraClubSwap, 1.0/3.0}, {Move::RandomSwap, 1.0/6.0}, {Move::ComplementInsertion, 1.0/6.0}};

enum class HA{H, A, BYE};

struct InputData{
    // All input that we get from the command line
    // If nothing specified, we get the default value
    const unordered_map<Move,bool>IsMoveInBase = {{Move::TS, true}, 
    /*{Move::PTS, true},*/ 
    {Move::RS, true}, 
    {Move::PRS, true}, 
    {Move::C, false}, 
    /*{Move::NC, false},*/
    {Move::MinCost_BM, false}, 
    {Move::Random_BM, false}, 
    /*{Move::iPTS_MinCost_PR, false},*/
    {Move::iPTS_Random_PR, false}, 
    /*{Move::MinCost_M_MinCost_PR, false},*/ 
    /*{Move::MinCost_M_Random_PR, false},*/
    /*{Move::Random_M_MinCost_PR, false},*/
    {Move::Random_M_Random_PR, false}};

    const unordered_map<Move,string>Moves = {{Move::TS, "TS"}, 
    /*{Move::PTS, "PTS"},*/ 
    {Move::RS, "RS"}, 
    {Move::PRS, "PRS"}, 
    {Move::C, "C"}, 
    /*{Move::NC, "NC"},*/
    {Move::MinCost_BM, "MinCost_BM"}, 
    {Move::Random_BM, "Random_BM"}, 
    /*{Move::iPTS_MinCost_PR, "iPTS_MinCost_PR"},*/
    {Move::iPTS_Random_PR, "iPTS_Random_PR"}, 
    /*{Move::MinCost_M_MinCost_PR, "MinCost_M_MinCost_PR"},*/ 
    /*{Move::MinCost_M_Random_PR, "MinCost_M_Random_PR"},*/
    /*{Move::Random_M_MinCost_PR, "Random_M_MinCost_PR"},*/
    {Move::Random_M_Random_PR, "Random_M_Random_PR"}}; 

    string Instance;
    int seed = 0;
    bool Heuristic = 1;
    bool HistoryLengthProvided = false;
    int HistoryLength = 1;
    double PerturbeIncrease = 0.005;
    int NrRounds = 4; // TTP
    int TimeLimit = 7200;
    long MaxIt = 100000;
    bool TTP = false; // TTP
    long ConstrViolationCost = 100000;
    unordered_map<Move, double>InputWeights;
    bool addMinTripConstraint = false;
    bool addColoringConstraint = false;

    bool Football = false; // YSTP, football
    bool ConstantCapacity = true; // YSTP, football
    int MaxNrBreaks = 100; // YSTP, football
    int CapacitySetting = 0; // YSTP, football

    bool MinCost = false;
    string OutputFolder;

    string startSol = "";

    bool RunGM = false;
    bool RunRF = false;

    bool Hockey = false;
    int PercentageHAPs = 100;

    bool SolveTripModel = false; // iTTP
    bool TripModelHAP_Fixed = false; //iTTP
};

class Input
{
    private:
        int NrTeams = 0, NrLeagues = 0, NrClubs = 0, NrRounds = 0; 
        bool ConstantCapacity;
        vector<vector<int>>ClubTeams;
        vector<vector<vector<int>>>ClubTeamsLeague;
        vector<vector<int>>LeagueTeams;
        // vector<int>LeagueIndex;
        vector<vector<int>>ClubCapacity;
        vector<vector<int>>DistanceClubs;
        vector<int>TeamLeague;
        vector<int>TeamStrength;
        vector<int>Teams;
        vector<int>TeamClub;
        vector<int>SingleTeamClubs;
        vector<int>MultiTeamClubs;
        vector<vector<int>>SingleTeamClubsLeague;
        vector<vector<int>>MultiTeamClubsLeague;
        vector<int>TeamLeagueIndex;
        vector<bool>IsTeamDummy;
        vector<vector<bool>>Eligible; // stores whether 2 teams are eligible to play vs each other
        int AllowedNrCapacityViolations = 0; // default TODO TODO
        bool MaxSameClubConstraint = false;
        int MaxSameClub = 100;
        int IndexDummyClub = -1;
        int MaxEdgeCost;
        string InstanceName;

        std::unordered_map<HAP_requirement_name, bool>HAP_requirements{
            {HAP_requirement_name::NoThreeConsecutive, false},
            {HAP_requirement_name::NoBreakBeginningEnd, false},
            {HAP_requirement_name::QuarterBalanced, false},
            {HAP_requirement_name::BreakLimit, false}};

        int BreakLimit;
        vector<vector<HA>>HAPs; 
        vector<int>TeamsHAP;
        vector<int>ComplementHAP;
        int MiaoHAPSetting = -1;

        FootballInstance InstanceFootball = FootballInstance::S;
        // pair<TotalNrTeams,NrDummyTeams>
        std::unordered_map<FootballInstance, std::pair<int,int>>NrTeamsFootballInstances = {{FootballInstance::S, {50,0}}, {FootballInstance::U13, {184,18}}, {FootballInstance::U15, {216,49}},
            {FootballInstance::U17, {144,14}}, {FootballInstance::U21, {64,6}}, {FootballInstance::M, {608,87}}, {FootballInstance::Tiny, {16,1}}};

        vector<vector<vector<int>>>CostMatchRound;

        Setting Setting_;
        
    public:
        Input();
        ~Input();
        int read_TTP(const std::string file_path);
        int read_CostMinimizationJasper(const string file_path);
        int read_YSTP(const string file_path, const bool Miao);
        void readAllowedNrCapacityViolations(const int num);
        int getNrTeams()const{return NrTeams;}
        int getNrLeagues()const{return NrLeagues;}
        int getNrTeamsLeague(const int l)const{return (int)LeagueTeams[l].size();}
        int getNrRounds()const{return NrRounds;}
        int getNrClubs()const{return NrClubs;}
        int getDistanceClubs(const int c1, const int c2)const{return DistanceClubs[c1][c2];}
        int getDistanceTeams(const int i, const int j)const;
        vector<int> getTeamsClub(const int c)const{return ClubTeams[c];}
        vector<int> getTeamsClubLeague(const int c, const int l)const{return ClubTeamsLeague[c][l];}
        vector<int> getSingleTeamClubs()const{return SingleTeamClubs;};
        vector<int> getMultiTeamClubs()const{return MultiTeamClubs;};
        vector<int> getSingleTeamClubsLeague(const int l)const{return SingleTeamClubsLeague[l];};
        vector<int> getMultiTeamClubsLeague(const int l)const{return MultiTeamClubsLeague[l];};
        vector<int> getTeamsLeague(const int l)const{return LeagueTeams[l];};
        int getNrTeamsClub(const int c)const{return ClubTeams[c].size();};
        int getTeamClub(const int i)const{return TeamClub[i];}
        int getCapacityClub(const int c, const int r)const{return ClubCapacity[c][r];}
        int getStrenghtTeam(const int t)const{return TeamStrength[t];}
        int getLeagueTeam(const int t)const{return TeamLeague[t];}
        int getIndexInLeague(const int t)const{return TeamLeagueIndex[t];};
        int getGlobalIndexTeam(const int l, const int i)const{return LeagueTeams[l][i];}
        int getIndexDummyClub()const{return IndexDummyClub;};
        bool isEligible(const int i, const int j)const{return Eligible[i][j];};
        int getMaxEdgeCost()const{return MaxEdgeCost;};

        bool isTeamDummy(const int i)const{return IsTeamDummy[i];};
        
        void setHAP_requirements(const bool NoThreeConsecutive, const bool NoBreakBeginningEnd, const bool QuarterBalanced, const bool BreakLimit, const int Limit){
            HAP_requirements.at(HAP_requirement_name::NoThreeConsecutive) = NoThreeConsecutive;
            HAP_requirements.at(HAP_requirement_name::NoBreakBeginningEnd) = NoBreakBeginningEnd;
            HAP_requirements.at(HAP_requirement_name::BreakLimit) = BreakLimit;
            HAP_requirements.at(HAP_requirement_name::QuarterBalanced) = QuarterBalanced;
            this->BreakLimit = Limit;
        }

        bool getHAP_requirement(const HAP_requirement_name req){
            return HAP_requirements.at(req);
        }

        int getBreakLimit()const{return BreakLimit;}; // nr breaks per team
        void setAllowedNrCapacityViolations1RR(const InputData& data);
        void setAllowedNrCapacityViolations2RR();
        int getAllowedNrCapacityViolations()const{return AllowedNrCapacityViolations;};
        int getMaxSameClub()const{return MaxSameClub;};
        void setMaxSameClub(const int max){
            MaxSameClubConstraint = true;
            MaxSameClub = max;
        };
        bool IsCapacityConstant(){return ConstantCapacity;};
        void setMiaoHAPSetting(const int nr){assert(nr == 1 || nr == 2);MiaoHAPSetting = nr;};
        int getMiaoHAPSetting()const{return MiaoHAPSetting;};
        bool IsMaxSameClubConstraint()const{return MaxSameClubConstraint;};

        bool SRR = false;

        int read_HAPs();
        int getNrHAPs()const{return (int)HAPs.size();};
        vector<HA> getHAP(const int h)const{return HAPs[h];}; // 0: H, 1: A
        HA getModeHAPRound(const int h, const int r)const{return HAPs[h][r];};
        void setHAPIndexTeam(const int i, const int h){TeamsHAP[i] = h;};
        int getHAPIndexTeam(const int i)const{return TeamsHAP[i];};
        int getComplementIndexHAP(const int h)const{return ComplementHAP[h];};
        void AddHAPWithComplement(const vector<HA>NewHAP, const vector<HA>NewHAP_c){
            int index = getNrHAPs();
            HAPs.push_back(NewHAP);
            HAPs.push_back(NewHAP_c);
            ComplementHAP.push_back(index+1);
            ComplementHAP.push_back(index);
        }
        bool HAP_satisfies_all_requirements(const vector<HA>& HAP);

        Setting getSetting()const{return Setting_;};

        FootballInstance getFootballInstance();

        void SetDefault(const int NrTeams);

        int getCostMatchRound(const int i, const int j, const int r)const{return CostMatchRound[i][j][r];};

        void setBaseAlgo();
        bool BaseAlgo = false;
        int NrRoundsBaseAlgo = -1;
        bool IsBaseAlgo()const{return BaseAlgo;};
        int getNrRoundsBaseAlgo()const{return NrRoundsBaseAlgo;};

        string getInstanceName()const{return InstanceName;};

        bool AllHAPsIncluded = true;
        void DeleteNonPromisingHAPsTTP(const int NrHaps);
};

#endif
