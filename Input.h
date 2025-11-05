#ifndef INPUT_H  
#define INPUT_H

#include <string>
#include <vector>
#include <unordered_map>
#include <assert.h>
#include <limits.h>

using namespace std;

enum class Move{TS,PTS,RS,PRS,C,NC,
    MinCost_BM, Random_BM,
    PTS_MinCost_PR, PTS_Random_PR,
    MinCost_M_MinCost_PR, MinCost_M_Random_PR, Random_M_MinCost_PR, Random_M_Random_PR}; 

enum class Setting{Miao,Hockey,CM,TTP};
enum class InstanceSetCM{Karel, Jasper, Uthus}; // Instances Cost Minimization
enum class HAP_requirement_name{NoThreeConsecutive, NoBreakBeginningEnd, BreakLimit, QuarterBalanced};
enum class MiaoInstance{S, U13, U15, U17, U21, M, Tiny}; // Instances paper Miao

enum class HA{H, A, BYE};

struct InputData{
    // All input that we get from the command line
    // If nothing specified, we get the default value
    const unordered_map<Move,bool>IsMoveInBase = {{Move::TS, true}, 
    {Move::PTS, true}, 
    {Move::RS, true}, 
    {Move::PRS, true}, 
    {Move::C, true}, 
    /*{Move::NC, false},*/
    {Move::MinCost_BM, false}, 
    {Move::Random_BM, false}, 
    {Move::PTS_MinCost_PR, false}, 
    {Move::PTS_Random_PR, false}, 
    {Move::MinCost_M_MinCost_PR, false}, 
    {Move::MinCost_M_Random_PR, false},
    {Move::Random_M_MinCost_PR, false}, 
    {Move::Random_M_Random_PR, false}};

    const unordered_map<Move,string>Moves = {{Move::TS, "TS"}, 
    {Move::PTS, "PTS"}, 
    {Move::RS, "RS"}, 
    {Move::PRS, "PRS"}, 
    {Move::C, "C"}, 
    /*{Move::NC, "NC"},*/
    {Move::MinCost_BM, "MinCost_BM"}, 
    {Move::Random_BM, "Random_BM"}, 
    {Move::PTS_MinCost_PR, "PTS_MinCost_PR"}, 
    {Move::PTS_Random_PR, "PTS_Random_PR"}, 
    {Move::MinCost_M_MinCost_PR, "MinCost_M_MinCost_PR"}, 
    {Move::MinCost_M_Random_PR, "MinCost_M_Random_PR"},
    {Move::Random_M_MinCost_PR, "Random_M_MinCost_PR"}, 
    {Move::Random_M_Random_PR, "Random_M_Random_PR"}}; 

    string Instance;
    int seed = 0;
    bool Heuristic = 1;
    bool HistoryLengthProvided = false;
    int HistoryLength = 1;
    int NrTeams = 36; // CM
    int NrRounds = 4; // TTP
    int k = 5; // CM
    int inst = 0; // CM
    int TimeLimit = 7200;
    int MaxIt = 1000000;
    bool CM = true; // CM
    bool TTP = false; // TTP
    bool Base = false;
    long ConstrViolationCost = 100000;
    unordered_map<Move, double>InputWeights;
    unordered_map<Move, bool>MoveSeen;

    bool Miao = false; // Miao
    bool ConstantCapacity = true; // Miao
    int MaxNrBreaks = 0; // Miao
    int CapacitySetting = 1; // Miao

    bool MinCost = false;
    string OutputFolder;

    bool HillClimbingFirst = false;
    double LowerBoundGap = 1.0;
};

class Input
{
    private:
        int NrTeams = 0, NrLeagues = 0, NrClubs = 0, NrRounds = 0; 
        bool ConstantCapacity;
        vector<vector<int>>ClubTeams;
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
        vector<int>TeamLeagueIndex;
        vector<bool>IsTeamDummy;
        vector<vector<bool>>Eligible; // stores whether 2 teams are eligible to play vs each other
        int AllowedNrCapacityViolations = 0; // default TODO TODO
        int MaxSameClub = 2;
        int IndexDummyClub;
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

        bool HAP_satisfies_all_requirements(const vector<HA>& HAP);

        MiaoInstance InstanceMiao;
        // pair<TotalNrTeams,NrDummyTeams>
        std::unordered_map<MiaoInstance, std::pair<int,int>>NrTeamsMiaoInstances = {{MiaoInstance::S, {50,0}}, {MiaoInstance::U13, {184,18}}, {MiaoInstance::U15, {216,49}},
            {MiaoInstance::U17, {144,14}}, {MiaoInstance::U21, {64,6}}, {MiaoInstance::M, {608,87}}, {MiaoInstance::Tiny, {16,1}}};

        vector<vector<vector<int>>>CostMatchRound;

        Setting Setting_;
        
    public:
        Input();
        ~Input();
        int read_TTP(const std::string file_path);
        int read_CostMinimization(const string file_path, InstanceSetCM inst);
        int read_CostMinimizationJasper(const string file_path);
        int read_Miao_Hockey(const string file_path, const bool Miao);
        void readAllowedNrCapacityViolations(const int num);
        int getNrTeams()const{return NrTeams;}
        int getNrLeagues()const{return NrLeagues;}
        int getNrTeamsLeague(const int l)const{return (int)LeagueTeams[l].size();}
        int getNrRounds()const{return NrRounds;}
        int getNrClubs()const{return NrClubs;}
        int getDistanceClubs(const int c1, const int c2)const{return DistanceClubs[c1][c2];}
        int getDistanceTeams(const int i, const int j)const{
            if (isTeamDummy(i) || isTeamDummy(j)){
                return 0;
            }
            else{
                return DistanceClubs[TeamClub[i]][TeamClub[j]];
            }
        }
        vector<int> getTeamsClub(const int c)const{return ClubTeams[c];}
        vector<int> getSingleTeamClubs()const{return SingleTeamClubs;};
        vector<int> getMultiTeamClubs()const{return MultiTeamClubs;};
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
        void setAllowedNrCapacityViolations();
        int getAllowedNrCapacityViolations()const{return AllowedNrCapacityViolations;};
        int getMaxSameClub()const{return MaxSameClub;};
        void setMaxSameClub(const int max){MaxSameClub = max;};
        bool IsCapacityConstant(){return ConstantCapacity;};
        void setMiaoHAPSetting(const int nr){assert(nr == 1 || nr == 2);MiaoHAPSetting = nr;};
        int getMiaoHAPSetting()const{return MiaoHAPSetting;};

        bool SRR = false;

        int read_HAPs();
        int getNrHAPs()const{return (int)HAPs.size();};
        vector<HA> getHAP(const int h)const{return HAPs[h];}; // 0: H, 1: A
        HA getModeHAPRound(const int h, const int r)const{return HAPs[h][r];};
        void setHAPIndexTeam(const int i, const int h){TeamsHAP[i] = h;};
        int getHAPIndexTeam(const int i)const{return TeamsHAP[i];};
        int getComplementIndexHAP(const int h)const{return ComplementHAP[h];};

        Setting getSetting()const{return Setting_;};

        MiaoInstance getMiaoInstance();

        void SetDefault(const int NrTeams);

        int getCostMatchRound(const int i, const int j, const int r)const{return CostMatchRound[i][j][r];};

        void setBaseAlgo();
        bool BaseAlgo = false;
        int NrRoundsBaseAlgo = -1;
        bool IsBaseAlgo()const{return BaseAlgo;};
        int getNrRoundsBaseAlgo()const{return NrRoundsBaseAlgo;};

        string getInstanceName()const{return InstanceName;};
};

#endif
