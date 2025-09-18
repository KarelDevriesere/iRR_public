#include <iostream>
#include <fstream>
#include "assert.h"
#include <map>
#include <filesystem>
#include <array>
#include <vector>
#include <algorithm>

#include "Analyze.h"

using namespace std;

const map<string, pair<int, string>> BestReportedValuesMiaoConstant = {
    {"Ti-0",  {1400, "IP"}},
    {"Ti-1",  {1143, "RF"}},
    {"Ti-2",  {1130, "RF"}},
    {"Ti-3",  {1128, "IP"}},

    {"S-0",   {4324, "RF"}},
    {"S-1",   {3611, "Meta"}},
    {"S-2",   {3446, "IP"}},
    {"S-3",   {3407, "IP"}},

    {"U21-0", {2784, "RF"}},
    {"U21-1", {2427, "IP"}},
    {"U21-2", {2314, "RF"}},
    {"U21-3", {2314, "IP"}},

    {"U17-0", {3918, "Meta"}},
    {"U17-1", {3482, "Meta"}},
    {"U17-2", {3406, "Meta"}},
    {"U17-3", {3403, "Meta"}},

    {"U13-0", {4359, "Meta"}},
    {"U13-1", {3908, "Meta"}},
    {"U13-2", {3831, "Meta"}},
    {"U13-3", {3793, "Meta"}},

    {"U15-0", {2993, "Meta"}},
    {"U15-1", {2754, "Meta"}},
    {"U15-2", {2668, "Meta"}},
    {"U15-3", {2658, "Meta"}},

    {"M-0", {-1, "-"}},
    {"M-1", {-1, "-"}},
    {"M-2", {-1, "-"}},
    {"M-3", {-1, "-"}}
};

const map<string, pair<int, string>> BestReportedValuesMiaoNC1 = {
    {"Ti-0",  {1400, "IP"}},
    {"Ti-1",  {1204, "IP"}},
    {"Ti-2",  {1138, "IP"}},
    {"Ti-3",  {1145, "IP"}},

    {"S-0", {4538, "IP"}},
    {"S-1", {3604, "RF"}},
    {"S-2", {3422, "RF"}},
    {"S-3", {3409, "RF"}},

    {"U21-0",   {3120, "IP"}},
    {"U21-1",   {2464, "RF"}},
    {"U21-2",   {2332, "RF"}},
    {"U21-3",   {2298, "RF"}},

    {"U17-0",   {4204, "RF"}},
    {"U17-1",   {3627, "Meta"}},
    {"U17-2",   {3435, "RF"}},
    {"U17-3",   {3410, "RF"}},

    {"U13-0",   {4492, "Meta"}},
    {"U13-1",   {4049, "RF"}},
    {"U13-2",   {3983, "Meta"}},
    {"U13-3",   {3923, "Meta"}},

    {"U15-0",   {3101, "Meta"}},
    {"U15-1",   {2848, "Meta"}},
    {"U15-2",   {2755, "Meta"}},
    {"U15-3",   {2766, "Meta"}},

    {"M-0", {-1, "-"}},
    {"M-1", {-1, "-"}},
    {"M-2", {-1, "-"}},
    {"M-3", {-1, "-"}}
};

const map<string, pair<int, string>> BestReportedValuesMiaoNC2 = {
    {"Ti-0",  {1400, "IP"}},
    {"Ti-1",  {1483, "IP"}},
    {"Ti-2",  {1584, "IP"}},
    {"Ti-3",  {1584, "IP"}},

    {"S-0", {4758, "IP"}},
    {"S-1", {4465, "RF"}},
    {"S-2", {4256, "IP"}},
    {"S-3", {4256, "IP"}},

    {"U21-0",   {3196, "IP"}},
    {"U21-1",   {3063, "RF"}},
    {"U21-2",   {2958, "IP"}},
    {"U21-3",   {2958, "IP"}},

    {"U17-0",   {4498, "RF"}},
    {"U17-1",   {4400, "RF"}},
    {"U17-2",   {4250, "RF"}},
    {"U17-3",   {4236, "RF"}},

    {"U13-0",   {4692, "RF"}},
    {"U13-1",   {4554, "RF"}},
    {"U13-2",   {4380, "RF"}},
    {"U13-3",   {4338, "IP"}},

    {"U15-0",   {3542, "RF"}},
    {"U15-1",   {3363, "RF"}},
    {"U15-2",   {3153, "RF"}},
    {"U15-3",   {3150, "RF"}},

    {"M-0", {-1, "-"}},
    {"M-1", {-1, "-"}},
    {"M-2", {-1, "-"}},
    {"M-3", {-1, "-"}}
};

const array<string,6>Algos = {"IP_Karel", "IP_Miao", "RF_Miao", "Meta_Miao", "FO_Karel", "Meta_Karel"};
const map<int,string>MiaoInstanceName = {{1, "S"}, {2, "U13"}, {3, "U15"}, {4, "U17"}, {5, "U21"}, {6, "M"}, {7, "Ti"}};

string MiaoInstance(const int Inst, const int NrBreaks){
    return MiaoInstanceName.at(Inst) + "-" + to_string(NrBreaks);
}

string replaceUnderscores(std::string input) {
    replace(input.begin(), input.end(), '_', ' ');
    return input;
}

map<string,map<string,map<string,int>>> ConstructTable(const int InstanceSet){
    map<string,map<string,map<string,int>>>Table;
    vector<string>Instances;
    if (InstanceSet == 0){
        for (int i = 1; i < 8; i++){
            for (int b = 0; b < 4; b++){
                Instances.push_back(MiaoInstance(i,b));
            }
        }
    }
    else if (InstanceSet == 1){
        for (int i = 1; i < 27; ++i){
            Instances.push_back(to_string(i));
        }
    }
    for (string i: Instances){
        for (string algo: Algos){
            vector<string>Metrics = {"BestObj", "BestTime"};
            if (algo == "Meta_Miao" || algo == "FO_Karel" || algo == "Meta_Karel"){
                Metrics.push_back("AvgObj");
                Metrics.push_back("AvgTime");
            }
            for (string m: Metrics){
                Table[i][algo][m] = -1;
            }
        }
    }
    return Table;
}

void AnalyzeResults(const int InstanceSet, const bool ConstantCap, const int CapSetting){

    map<string,map<string,map<string,int>>>Table = ConstructTable(InstanceSet);

    string file_path_results_base = "Results";
    if (InstanceSet == 0){
        cout << "Analyze results Miao instances ";
        file_path_results_base += (std::string(PATHSEP) + "Miao");
    }
    else if (InstanceSet == 1){
        cout << "Analyze results Hockey instances" << endl;
        file_path_results_base += (std::string(PATHSEP) + "Hockey");
    }
    else if (InstanceSet == 2){
        cout << "Analyze results Miao instances" << endl;
        file_path_results_base += (std::string(PATHSEP) + "Test");
    }
    else{
        cout << "InstanceSet should be 0,1 or 2" << endl;
        std::exit(0);
    }

    if (InstanceSet == 0){
        if (ConstantCap){
            cout << " with constant capacity" << endl;
            file_path_results_base += (std::string(PATHSEP) + "ConstantCapacity");
            cout << "new file path = " << file_path_results_base << endl;
        }
        else{
            assert(CapSetting == 1 || CapSetting == 2);
            cout << " with variable capacity and setting " << CapSetting << endl;
            file_path_results_base += (std::string(PATHSEP) + "VariableCapacity/Setting" + to_string(CapSetting));
        }

        for (int b = 0; b < 4; ++b){
            std::string file_path_results = file_path_results_base + std::string(PATHSEP) + "b" + to_string(b) + std::string(PATHSEP) + "Objectives";
            try {
                for (const auto& entry : std::filesystem::directory_iterator(file_path_results)) {

                    std::string fname = entry.path().filename().string();
                    const std::string suffix = "TS_test.txt";

                    if (entry.is_regular_file() && fname.size() >= suffix.size() && fname.compare(fname.size() - suffix.size(), suffix.size(), suffix) == 0) { // TS_test: last results
                        // std::cout << "Opening file: " << entry.path() << '\n';

                        std::ifstream file(entry.path());
                        if (!file) {
                            std::cerr << "Error opening file: " << entry.path() << '\n';
                            continue;
                        }

                        std::string firstLine;
                        std::string token;
                        if (std::getline(file, firstLine)) {
                            std::stringstream ss(firstLine);
                            
                            std::getline(ss, token, ','); // first field = string
                            string algo = token;

                            std::getline(ss, token, ',');
                            int Inst = std::stoi(token);

                            std::getline(ss, token, ',');
                            int BestObj = std::stoi(token);

                            std::getline(ss, token, ',');
                            int BestTime = std::stoi(token);

                            std::getline(ss, token, ',');
                            int AvgObj = std::stoi(token);

                            std::getline(ss, token, ',');
                            int AvgTime = std::stoi(token);

                            string ConvertedInst;
                            if (InstanceSet == 0){
                                ConvertedInst = MiaoInstance(Inst, b);
                            }
                            else if (InstanceSet == 2){
                                ConvertedInst = to_string(Inst);
                            }
                            Table.at(ConvertedInst).at(algo).at("BestObj") = BestObj;
                            Table.at(ConvertedInst).at(algo).at("BestTime") = BestTime;
                            if (algo == "Meta_Miao" || algo == "FO_Karel" || algo == "Meta_Karel"){
                                Table.at(ConvertedInst).at(algo).at("AvgObj") = AvgObj;
                                Table.at(ConvertedInst).at(algo).at("AvgTime") = AvgTime;
                            }

                        } else {
                            std::cout << "[File " << entry.path() << " is empty]\n";
                            std::exit(0);
                        }
                    }
                }
            } catch (const std::exception& e) {
                std::cerr << "Error: " << e.what() << '\n';
            }
        }
    }
    // Save results in latex table
    std::string file_path_table = file_path_results_base + "/Results.txt";
    std::ofstream results_file(file_path_table);
    cout << "Store results table in " << file_path_table << endl;

    auto it = Table.begin();
    results_file << "\\begin{landscape}\n";
    results_file << "\\begin{table}[h!]\n";
    results_file << "\\centering\n";
    results_file << "\\caption{Travel distance results using ";
    string ref;
    if (ConstantCap){
        results_file << "constant capacity.";
        ref = "ResultsInstancesMiaoConstantCap";
    }
    else{
        if (CapSetting == 1){
            results_file << "capacity setting 1.";
            ref = "ResultsInstancesMiaoCapSetting1";
        }
        else{
            results_file << "capacity setting 2.";
            ref = "ResultsInstancesMiaoCapSetting2";
        }
    }
    results_file << "}\n";

    results_file << "\\label{tab:" << ref << "}\n";
    results_file << "\\begin{threeparttable}\n";
    results_file << "\\begin{tabular}{ccccccccccccccccccccccccccc}\n";
    results_file << "Instance & \\multicolumn{2}{c}{Paper Miao} & & ";

    for (auto& algo: {"IP Karel", "IP Miao", "RF Miao"}){
        results_file << " \\multicolumn{" << 2 << "}" << "{c}{" << algo << "} & & ";
    }
    for (auto& algo: {"FO Karel", "Meta Karel"}){
        results_file << " \\multicolumn{" << 4 << "}" << "{c}{" << algo << "} & & ";
    }
    results_file << " \\multicolumn{" << 4 << "}" << "{c}{" << "Meta Miao" << "}";
    results_file << "\\\\ \n";
    results_file << "\\bottomrule \n";

    results_file << " & $d_i$ & Method & & ";
    for (int i = 0; i < 3; ++i){
        results_file << " $d_i$ & t & & ";
    }
    for (int i = 0; i < 2; ++i){
        results_file << " $d_i$ & t & $\\Delta d_i$ & $\\Delta t$ & & ";
    }
    results_file << " $d_i$ & t & $\\Delta d_i$ & $\\Delta t$";
    results_file << "\\\\ \n";
    results_file << "\\cline{2-3}\\cline{5-6}\\cline{8-9}\\cline{11-12}\\cline{14-17}\\cline{19-22}\\cline{24-27} \n";

    for (auto&[inst, algo_results]: Table){
        results_file << inst << " & ";

        int BestFoundValue = 2147483647; // INT_MAX
        int WorstFoundValue = 0;
        for (string algo: {"IP_Karel", "IP_Miao", "RF_Miao", "FO_Karel", "Meta_Karel", "Meta_Miao"}){
            if (Table.at(inst).at(algo).at("BestObj") > 0 && Table.at(inst).at(algo).at("BestObj") < BestFoundValue){ // distance minimization so lower is better
                BestFoundValue = Table.at(inst).at(algo).at("BestObj");
            }
            else if (Table.at(inst).at(algo).at("BestObj") > 0 && Table.at(inst).at(algo).at("BestObj") > WorstFoundValue){
                WorstFoundValue = Table.at(inst).at(algo).at("BestObj");
            }
        }

        // --------- Miao column ------------- //
        
        int value;
        string method;
        if (ConstantCap){
            value = BestReportedValuesMiaoConstant.at(inst).first;
            method =  BestReportedValuesMiaoConstant.at(inst).second;
        }
        else{
            if (CapSetting == 1){
                value = BestReportedValuesMiaoNC1.at(inst).first;
                method =  BestReportedValuesMiaoNC1.at(inst).second;
            }
            else {
                value = BestReportedValuesMiaoNC2.at(inst).first;
                method =  BestReportedValuesMiaoNC2.at(inst).second;
            }
        }

        if (value > 0 && value <= BestFoundValue){
            BestFoundValue = value;
            results_file << "\\cellcolor{green!25}" << value << " & " << method << " & ";
        }
        else if (value > 0 && value >= WorstFoundValue){
            WorstFoundValue = value;
            results_file << "\\cellcolor{red!25}" << value << " & " << method << " & ";
        }
        else{
            results_file << value << " & " << method << " & ";
        }

        // --------- Miao column ------------- //

        for (string algo: {"IP_Karel", "IP_Miao", "RF_Miao", "FO_Karel", "Meta_Karel", "Meta_Miao"}){
            vector<string>Metrics;
            if (algo == "IP_Karel" || algo == "IP_Miao" || algo == "RF_Miao"){
                Metrics = {"BestObj", "BestTime"};
            }
            else{
                Metrics = {"BestObj", "BestTime", "AvgObj", "AvgTime"};
            }
            for (auto& metric: Metrics){
                int m = Table.at(inst).at(algo).at(metric);
                if (m < 0 || m > 7300){ // max time limit is 7200
                    results_file << " & - ";
                }
                else{
                    if (metric == "BestObj" && Table.at(inst).at(algo).at("BestObj") == BestFoundValue){
                        results_file << " & " << "\\cellcolor{green!25}" << m;
                    }
                    else if (metric == "BestObj" && Table.at(inst).at(algo).at("BestObj") == WorstFoundValue){
                        results_file << " & " << "\\cellcolor{red!25}" << m;
                    }
                    else{
                        results_file << " & " << m;
                    }
                }
            }
            if (algo != "Meta_Miao"){
                results_file << " & ";
            }
        }
        if (inst.back() == '3' && inst != std::prev(Table.end())->first){
            results_file << "\\\\ \\hdashline \n";
        }
        else{
            results_file << "\\\\ \n";
        }
    }

    results_file << "\\bottomrule \n";
    results_file << "\\end{tabular}\n";
    results_file << "\\end{threeparttable}\n";
    results_file << "\\end{table}\n";
    results_file << "\\end{landscape}\n";

    results_file.close();
}
