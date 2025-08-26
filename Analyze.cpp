#include <iostream>
#include <fstream>
#include "assert.h"
#include <map>
#include <filesystem>
#include <array>

#include "Analyze.h"

using namespace std;

const array<string,6>Algos = {"IP_Karel", "IP_Miao", "RF_Miao", "Meta_Miao", "FO_Karel", "Meta_Karel"};
const map<int,string>MiaoInstanceName = {{1, "S"}, {2, "U13"}, {3, "U15"}, {4, "U17"}, {5, "U21"}, {6, "M"}, {7, "Ti"}};

constexpr string MiaoInstance(const int Inst, const int NrBreaks){
    return MiaoInstanceName.at(Inst) + "-" + to_string(NrBreaks);
}

string replaceUnderscores(std::string input) {
    std::replace(input.begin(), input.end(), '_', ' ');
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
        file_path_results_base += "/Miao";
    }
    else if (InstanceSet == 1){
        cout << "Analyze results Hockey instances" << endl;
        file_path_results_base += "/Hockey";
    }
    else if (InstanceSet == 2){
        cout << "Analyze results Miao instances" << endl;
        file_path_results_base += "/Test";
    }
    else{
        cout << "InstanceSet should be 0,1 or 2" << endl;
        std::exit(0);
    }

    if (InstanceSet == 0){
        if (ConstantCap){
            cout << " with constant capacity" << endl;
            file_path_results_base += "/ConstantCapacity";
        }
        else{
            assert(CapSetting == 1 || CapSetting == 2);
            cout << " with variable capacity and setting " << CapSetting << endl;
            file_path_results_base += ("/VariableCapacity/Setting" + to_string(CapSetting));
        }

        for (int b = 0; b < 4; ++b){
            std::string file_path_results = file_path_results_base + "/b" + to_string(b) + "/Objectives";
            try {
                for (const auto& entry : std::filesystem::directory_iterator(file_path_results)) {
                    if (entry.is_regular_file() && entry.path().extension() == ".txt") {
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
    results_file << "\n";

    results_file << "\\label{tab:" << ref << "}\n";
    results_file << "\\begin{threeparttable}\n";
    results_file << "\\begin{tabular}{cccccccccccccccccccccccccc}\n";
    results_file << "Instance & LB & & ";

    for (auto& algo: {"IP Karel", "IP Miao", "RF Miao"}){
        results_file << " \\multicolumn{" << 2 << "}" << "{c}{" << algo << "} & & ";
    }
    for (auto& algo: {"FO Karel", "Meta Karel"}){
        results_file << " \\multicolumn{" << 4 << "}" << "{c}{" << algo << "} & & ";
    }
    results_file << " \\multicolumn{" << 4 << "}" << "{c}{" << "Meta Miao" << "}";
    results_file << "\\\\ \n";
    results_file << "\\bottomrule \n";

    results_file << " & & & ";
    for (int i = 0; i < 3; ++i){
        results_file << " $d_i$ & t & & ";
    }
    for (int i = 0; i < 2; ++i){
        results_file << " $d_i$ & t & $\\Delta d_i$ & $\\Delta t$ & & ";
    }
    results_file << " $d_i$ & t & $\\Delta d_i$ & $\\Delta t$";
    results_file << "\\\\ \n";
    results_file << "\\cline{4-5}\\cline{7-8}\\cline{10-11}\\cline{13-16}\\cline{18-21}\\cline{23-26} \n";

    for (auto&[inst, algo_results]: Table){
        results_file << inst << " & & ";
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
                    results_file << " & " << m;
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