#ifndef INPUT_H  
#define INPUT_H

#include <string>
#include <vector>
#include <unordered_map>
#include <assert.h>

enum class HAP_requirement_name{NoThreeConsecutive, NoBreakBeginningEnd, BreakLimit, QuarterBalanced};
enum class MiaoInstance{S, U13, U15, U17, U21, M, Tiny};

enum class HA{H, A, BYE};

using namespace std;

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
        
    public:
        Input();
        ~Input();
        int read(const string file_path, const bool Miao);
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

        MiaoInstance getMiaoInstance();
};

#endif