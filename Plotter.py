import sys
import os
import csv
import pandas as pd
import re

import seaborn as sns
import matplotlib.pyplot as plt
import numpy as np
from scipy import stats  # for Friedmans test

# HPC: module load SciPy-bundle/2023.11-gfbf-2023b

RoundSet40 = [10,20,30]
RoundSet32 = [8,16,24]
RoundSet24 = [6,12,18]
RoundSet16 = [4,8,12]
NrRoundsTTP = {"BRA24": RoundSet24, "CIRC40": RoundSet40, "CON40": RoundSet40, "GAL40": RoundSet40, "INCR40": RoundSet40, "LINE40": RoundSet40, "NL16": RoundSet16, "NFL32": RoundSet32}

def ResultsInstances(InstanceSetting, iTTP=False):
    instances = []
    PathRoot = "Instances"
    if InstanceSetting == "Miao":
        PathRoot = os.path.join(PathRoot, "Miao")
        I = ["i01", "i02", "i03", "i04", "i05", "i06"]
        S = ["0","1","2"]
        B = ["3", "100"]
        for i in I:
            for s in S:
                for b in B:
                    instances.append(i+"_s"+s+"_b"+b)
    elif InstanceSetting == "TTP":
        PathRoot = os.path.join(PathRoot, "TTP")
        for key in NrRoundsTTP.keys():
            for r in NrRoundsTTP[key]:
                instances.append(key + "_" + str(r))
    elif InstanceSetting == "Hockey":
        PathRoot = os.path.join(PathRoot, "Hockey")
        I = ["i01", "i02", "i03", "i04", "i05", "i06"]
        for i in I:
            instances.append(i)
    PathRoot = os.path.join(PathRoot, "Results")

    TableComp = {"IP_time": [], "MiaoAlgo_time": [], "IP_TripModel_HAP_fixed_time": [], "IP_TripModel_time": [], "Heuristic_time": []}
    Table = {inst: {"IP_value": -1, "IP_gap": -1, "IP_time": [], "IP_bound": -1, "Initial_Avg_value": [], "Initial_seed": [], "Initial_gap": -1, "Initial_time": [], "MiaoAlgo_Avg_value": [], "MiaoAlgo_time": [], "MiaoAlgo_seed": [], "Ip_TripModel_HAP_fixed_value": 10000000, "IP_TripModel_HAP_fixed_gap": 10000000, "IP_TripModel_HAP_fixed_time": [], "Miao_gap": -1, "IP_TripModel_value": 10000000, "IP_TripModel_gap": -1, "IP_TripModel_bound": 10000000, "IP_TripModel_time": [], "Heuristic_Avg_value": [], "Heuristic_HL": [], "Heuristic_gap": -1, "Heuristic_Avg_time": []} for inst in instances}

    TableMatchings = {inst: {"NrSuccesfullMatchings": [], "NrInfeasibleMatchings": []} for inst in instances}
    PathIP = os.path.join(PathRoot, "IP")
    PathHeuristic = os.path.join(os.path.join(PathRoot, "Heuristic"), "All") # HeuristicStartingFromMiao
    PathMiaoAlgo = os.path.join(PathRoot, "MiaoAlgo")
    if InstanceSetting == "TTP":
        PathTripModel = os.path.join(PathRoot, "IP_TripModel")
        PathTripModel_HAP_fixed = os.path.join(PathRoot, "IP_TripModel_HAP_fixed")
        Paths = [PathIP, PathMiaoAlgo, PathTripModel, PathTripModel_HAP_fixed, PathHeuristic]
    else:
        Paths = [PathIP, PathMiaoAlgo, PathHeuristic]

    for Path in Paths:
        for file_index, filename in enumerate(os.listdir(Path)):
            if not filename.endswith(".txt"):
                continue
            if "LP" in filename:
                print(f'File {filename} contains LP, skip!!')
                continue
            FilePath = os.path.join(Path, filename)
            if not os.path.exists(FilePath):
                raise FileNotFoundError(f"The file '{FilePath}' does not exist.")
                sys.exit(0)
            with open(FilePath, 'r', newline="") as file:
                reader = csv.reader(file)
                max_time = 0
                for i, row in enumerate(reader):
                    if i == 0:
                        seed = str(row[0])
                        method = row[1]
                        if InstanceSetting == "Miao":
                            inst = row[2]
                            setting = str(row[3])
                            nr_breaks = str(row[4])
                            Instance = inst + "_s" + setting + "_b" + nr_breaks
                            if method == "Heuristic": # HeuristicStartingFromMiao
                                HL = str(row[5])
                                Table[Instance]["Heuristic_HL"].append(HL)
                        elif InstanceSetting == "Hockey":
                            Instance = row[2]
                            if method == "Heuristic": # HeuristicStartingFromMiao
                                HL = str(row[3])
                                Table[Instance]["Heuristic_HL"].append(HL)
                        elif InstanceSetting == "TTP":
                            if "PercHAPs" in filename:
                                print(f'File {filename} contains PercHAPs, skip!!')
                                break
                            pattern = re.compile(r'(?:^|_)([A-Z]+[0-9]+_[0-9]+)(?:_|\.|$)')

                            match = pattern.search(filename)
                            if match:
                                Instance = match.group(1)
                                if Instance == "N16_4":
                                    Instance = "NL16_4"
                                elif Instance == "N16_8":
                                    Instance = "NL16_8"
                                elif Instance == "N16_12":
                                    Instance = "NL16_12"
                                # print(f'Extracted {Instance} from {filename}')
                            else:
                                print(f'No match for {filename} in path {FilePath}')
                                break
                            if method == "Heuristic": # HeuristicStartingFromMiao
                                HL = str(row[4])
                                Table[Instance]["Heuristic_HL"].append(HL)

                    elif i > 0 and row[0] != "Final" and row[0] != "Time" and row[0] != "NrSuccesfullMatchings":
                        if int(row[0]) > max_time:
                            max_time = int(row[0])
                    elif row[0] == "NrSuccesfullMatchings":
                        TableMatchings[Instance][row[0]].append(int(row[1]))
                        TableMatchings[Instance][row[2]].append(int(row[3]))
                        # print(f'{Instance} & {row[1]} & {row[3]} \\\\ \n')
                    if i > 1 and (row[0] == "Final"): # or method == "Heuristic" and int(row[0]) == TIME_LIMIT): # row[0] == "Final"
                        if method == "IP" and Path == PathIP:
                            Table[Instance]["IP_value"] = int(row[1])
                            Table[Instance]["IP_bound"] = int(row[2])
                            if abs(int(row[1]) - int(row[2])) > 1:
                                TableComp["IP_time"].append(172800)
                            else:
                                TableComp["IP_time"].append(max_time)
                        elif method == "IP" and Path == PathTripModel_HAP_fixed:
                            Table[Instance]["IP_TripModel_HAP_fixed_value"] = int(row[1])
                            if abs(int(row[1]) - int(row[2])) > 1:
                                TableComp["IP_TripModel_HAP_fixed_time"].append(43200)
                            else:
                                TableComp["IP_TripModel_HAP_fixed_time"].append(max_time)
                        elif method == "IP_TripModel":
                            Table[Instance]["IP_TripModel_value"] = int(row[1])
                            Table[Instance]["IP_TripModel_bound"] = int(row[2])
                            if abs(int(row[1]) - int(row[2])) > 1:
                                TableComp["IP_TripModel_time"].append(172800)
                            else:
                                TableComp["IP_TripModel_time"].append(max_time)
                        elif method == "MiaoAlgo":
                            Table[Instance]["MiaoAlgo_Avg_value"].append(int(row[1]))
                            TableComp["MiaoAlgo_time"].append(max_time)
                            Table[Instance]["MiaoAlgo_time"].append(max_time)
                            Table[Instance]["MiaoAlgo_seed"].append(seed)
                        elif method == "Heuristic": # HeuristicStartingFromMiao
                            Table[Instance]["Heuristic_Avg_value"].append(float(row[1]))
                            Table[Instance]["Heuristic_Avg_time"].append(float(row[2]))
                            TableComp["Heuristic_time"].append(max_time)
                        else:
                            print(f'Unknown method = {method}')
                            breakpoint()
                        break
                    elif i > 1 and (row[0] == "0" or row[0] == 0):
                        if method == "MiaoAlgo":
                            Table[Instance]["Initial_Avg_value"].append(int(row[1]))
                            Table[Instance]["Initial_seed"].append(seed)

    if iTTP:
        print(f"COMPUTATION TIMES:")
        print(f"& F1 & F2 & GM-c & GM-it & F2-HAP \\\\ \n")
        print(f"Min & {round(min(TableComp['IP_time'])/60.0, 2)} & {round(min(TableComp['IP_TripModel_time'])/60.0, 2)} & & {round(min(TableComp['MiaoAlgo_time'])/60.0, 2)} &  {round(min(TableComp['IP_TripModel_HAP_fixed_time'])/60.0, 2)} \\\\ \n")
        print(f"Max & {round(max(TableComp['IP_time'])/60.0, 2)} & {round(max(TableComp['IP_TripModel_time'])/60.0, 2)} & & {round(max(TableComp['MiaoAlgo_time'])/60.0, 2)} &  {round(max(TableComp['IP_TripModel_HAP_fixed_time'])/60.0, 2)} \\\\ \n")
        print(f"Average & {round(np.mean(TableComp['IP_time'])/60.0, 2)} & {round(np.mean(TableComp['IP_TripModel_time'])/60.0, 2)} & & {round(np.mean(TableComp['MiaoAlgo_time'])/60.0, 2)} &  {round(np.mean(TableComp['IP_TripModel_HAP_fixed_time'])/60.0, 2)} \\\\ \n")

    for inst in TableMatchings.keys():
        if len(TableMatchings[inst]["NrSuccesfullMatchings"]) > 0:
            avg_succes = round(sum(TableMatchings[inst]["NrSuccesfullMatchings"]) / len(TableMatchings[inst]["NrSuccesfullMatchings"]),2)
            avg_fail = round(sum(TableMatchings[inst]["NrInfeasibleMatchings"]) / len(TableMatchings[inst]["NrInfeasibleMatchings"]),2)
            print('{} & {} & {} \\\\ \n'.format(inst, avg_succes, avg_fail))
    
    if InstanceSetting == "Miao":
        OutputPath = os.path.join(os.path.join("Results", "Miao"), "Analysis.txt")   
    elif InstanceSetting == "TTP":
        if iTTP:
            OutputPath = os.path.join(os.path.join("Results", "TTP"), "Analysis_iTTP.txt")
        else:
            OutputPath = os.path.join(os.path.join("Results", "TTP"), "Analysis.txt") 
    else:
        OutputPath = os.path.join(os.path.join("Results", "Hockey"), "Analysis.txt")

    PathBoxPlot = "Results"
    if InstanceSetting == "TTP":
        PathBoxPlot = os.path.join(PathBoxPlot, "TTP")
    else:
        PathBoxPlot = os.path.join(PathBoxPlot, "Hockey")
    PathBoxPlot = os.path.join(PathBoxPlot, "DataBoxplot.csv")
    write_header = not os.path.exists(PathBoxPlot)

    with open(OutputPath, 'w') as output_file, open(PathBoxPlot, "a", newline="") as output_file_boxplot:
        if InstanceSetting == "Miao":
            output_file.write("Instance & IP_v & IP_b & IP_t & Miao_{av} & Miao_{at} & Miao_{bv} & Miao_{bt} & Heur_{av} & Heur_{at} & Heur_{bv} & Heur_{bt} & Heur_{bHL}\n")
        else:
            if iTTP:
                output_file.write("Instance & Bound & IP_v & IP_gap & IP_Trip & Ip_trip_gap & Init_v & Init_gap & Miao_{av} & Miao_{bv} & Miao_gap & IP-HAP & IP-HAP_gap\n")
            else:
                output_file.write("Instance & Bound & IP_v & IP_b & IP_t & Miao_{av} & Miao_{at} & Miao_{bv} & Miao_{bt} & Heur_{av} & Heur_{at} & Heur_{bv} & Heur_{bt} & Heur_{bHL}\n")

        writer = csv.writer(output_file_boxplot)

        if write_header:
            if InstanceSetting == "TTP": 
                writer.writerow(["Algorithm", "LB", "Obj"])
            else:
                writer.writerow(["Algorithm", "InstanceClass", "LB", "Obj"])

        Results = {inst: {"Avg_Base": 0, "Min_Base": 0, "Avg_iPTS": 0, "Min_iPTS": 0, "Avg_M": 0, "Min_M": 0, "Avg_BM": 0, "Min_BM": 0, "Avg_all": 0, "Min_all": 0, "Min_GM": 0, "Avg_GM": 0} for inst in instances}

        Bounds = {inst: -1 for inst in instances}

        for Instance in Table.keys():
            Heuristic_Avg_value = -1
            Heuristic_Avg_time = -1
            MiaoAlgo_Avg_value = -1
            MiaoAlgo_Avg_time = -1
            MiaoAlgo_Best_value = -1
            MiaoAlgo_Best_time = -1
            MiaoAlgoBestSeed = -1
            Heuristic_Best_value = -1
            Heuristic_Best_time = -1
            Heuristic_best_HL = -1
            Initial_Avg_value = -1
            Initial_Best_value = -1
            InitialBestSeed = -1
            if not iTTP and len(Table[Instance]["Heuristic_Avg_value"]) > 0:
                print(f"length of {Instance} = {len(Table[Instance]['Heuristic_Avg_value'])}")
                Heuristic_Avg_value = round(sum(Table[Instance]["Heuristic_Avg_value"]) / len(Table[Instance]["Heuristic_Avg_value"]),2)
                Heuristic_Avg_time = round(sum(Table[Instance]["Heuristic_Avg_time"]) / len(Table[Instance]["Heuristic_Avg_time"]),2)
                Heuristic_Best_value = Table[Instance]["Heuristic_Avg_value"][0]
                Heuristic_Best_time = Table[Instance]["Heuristic_Avg_time"][0]
                Heuristic_best_HL = Table[Instance]["Heuristic_HL"][0]

                for i in range(1,len(Table[Instance]["Heuristic_Avg_value"])):
                    if Table[Instance]["Heuristic_Avg_value"][i] < Heuristic_Best_value:
                        Heuristic_Best_value = Table[Instance]["Heuristic_Avg_value"][i]
                        Heuristic_Best_time = Table[Instance]["Heuristic_Avg_time"][i]
                        Heuristic_best_HL = Table[Instance]["Heuristic_HL"][i]

                Results[Instance]["Avg_all"] = int(Heuristic_Avg_value)
                Results[Instance]["Min_all"] = int(Heuristic_Best_value) 
                
            if len(Table[Instance]["MiaoAlgo_Avg_value"]) > 0:
                MiaoAlgo_Avg_value = round(sum(Table[Instance]["MiaoAlgo_Avg_value"]) / len(Table[Instance]["MiaoAlgo_Avg_value"]))
                MiaoAlgo_Avg_time = round(sum(Table[Instance]["MiaoAlgo_time"]) / len(Table[Instance]["MiaoAlgo_time"]))
                MiaoAlgo_Best_value = Table[Instance]["MiaoAlgo_Avg_value"][0]
                MiaoAlgo_Best_time = Table[Instance]["MiaoAlgo_time"][0]
                MiaoAlgoBestSeed = Table[Instance]["MiaoAlgo_seed"][0]
    
                for i in range(1,len(Table[Instance]["MiaoAlgo_Avg_value"])):
                    if Table[Instance]["MiaoAlgo_Avg_value"][i] < MiaoAlgo_Best_value:
                        MiaoAlgo_Best_value = Table[Instance]["MiaoAlgo_Avg_value"][i]
                        MiaoAlgo_Best_time = Table[Instance]["MiaoAlgo_time"][i]
                        MiaoAlgoBestSeed = Table[Instance]["MiaoAlgo_seed"][i]

                Results[Instance]["Min_GM"] = int(MiaoAlgo_Best_value) 
                Results[Instance]["Avg_GM"] = int(MiaoAlgo_Avg_value) 

            if iTTP and len(Table[Instance]["Initial_Avg_value"]) > 0:
                Initial_Avg_value = round(sum(Table[Instance]["Initial_Avg_value"]) / len(Table[Instance]["Initial_Avg_value"]))
                Initial_Best_value = Table[Instance]["Initial_Avg_value"][0]
                InitialBestSeed = Table[Instance]["Initial_seed"][0]
    
                for i in range(1,len(Table[Instance]["Initial_Avg_value"])):
                    if Table[Instance]["MiaoAlgo_Avg_value"][i] < MiaoAlgo_Best_value:
                        Initial_Best_value = Table[Instance]["Initial_Avg_value"][i]
                        InitialBestSeed = Table[Instance]["Initial_seed"][i]

            # print(f'{{"{Instance}",{MiaoAlgoBestSeed}}},')
            
            line = Instance + " & "

            # assumes that the file "Bounds contains the best bound, including LP of TripModel for iTTP!!"

            if InstanceSetting == "TTP" or InstanceSetting == "Hockey":
                if InstanceSetting == "TTP":
                    FilePathBounds = os.path.join(os.path.join(os.path.join("Instances", "TTP"), "Bounds"), "BestBounds.txt")
                else:
                    FilePathBounds = os.path.join(os.path.join(os.path.join("Instances", "Hockey"), "Bounds"), "BestBounds.txt")
                if not os.path.exists(FilePathBounds):
                    raise FileNotFoundError(f"The file '{FilePathBounds}' does not exist.")
                    sys.exit(0)
                with open(FilePathBounds, 'r', newline="") as file:
                    reader = csv.reader(file)
                    for i, row in enumerate(reader):
                        if row[0] == Instance:
                            Bounds[Instance] = int(row[1])
                            break
            if Bounds[Instance] == -1:
                Bounds[Instance] = Table[Instance]["IP_bound"]
            elif Table[Instance]["IP_bound"] > Bounds[Instance]:
                Bounds[Instance] = Table[Instance]["IP_bound"]
            '''
            elif Table[Instance]["IP_TripModel_bound"] > bound:
                bound = Table[Instance]["IP_TripModel_bound"]
            '''
                
            line += str(Bounds[Instance]) + " & "

            if (Table[Instance]["IP_value"] <= MiaoAlgo_Best_value or MiaoAlgo_Best_value == -1) and (Table[Instance]["IP_value"] <= Heuristic_Best_value or Heuristic_Best_value == -1) and Table[Instance]["IP_value"] != -1 and (Table[Instance]["IP_value"] <= Table[Instance]["IP_TripModel_value"] or Table[Instance]["IP_TripModel_value"] == -1):
                # line += "\\cellcolor{green!25}" 
                if not iTTP or (iTTP and Table[Instance]["IP_value"] <= Table[Instance]["IP_TripModel_HAP_fixed_value"]):
                    line += "\\textbf{" + str(Table[Instance]["IP_value"]) + "}"
                else:
                    line += str(Table[Instance]["IP_value"])
            elif Table[Instance]["IP_value"] >= MiaoAlgo_Best_value and Table[Instance]["IP_value"] >= Heuristic_Best_value and Table[Instance]["IP_value"] >= Table[Instance]["IP_TripModel_value"]:
                # line += "\\cellcolor{red!25}" 
                if not iTTP or (iTTP and Table[Instance]["IP_value"] >= Table[Instance]["IP_TripModel_HAP_fixed_value"]):
                    line += "\\emph{" + str(Table[Instance]["IP_value"]) + "}"
                else:
                    line += str(Table[Instance]["IP_value"]) 
            else:
                line += str(Table[Instance]["IP_value"])  

            # if iTTP:
            if Table[Instance]["IP_value"] > 0:
                gap_ip = round(((Table[Instance]["IP_value"] - int(Bounds[Instance])) / Table[Instance]["IP_value"])*100,2)
            else:
                gap_ip = -1
            line += " & " + str(gap_ip) + " & "

            if iTTP:
                if Table[Instance]["IP_TripModel_value"] > 0:
                    gap_trip_model = round(((Table[Instance]["IP_TripModel_value"] - int(Bounds[Instance])) /  Table[Instance]["IP_TripModel_value"])*100,2)
                else:
                    gap_trip_model = -1
                if Table[Instance]["IP_TripModel_value"] <= MiaoAlgo_Best_value and Table[Instance]["IP_TripModel_value"] <= Table[Instance]["IP_value"] and Table[Instance]["IP_TripModel_value"] != -1 and (Table[Instance]["IP_TripModel_value"] <= Table[Instance]["IP_TripModel_HAP_fixed_value"]):
                    line += " \\textbf{" + str(Table[Instance]["IP_TripModel_value"]) + "}"
                elif Table[Instance]["IP_TripModel_value"] <= MiaoAlgo_Best_value and Table[Instance]["IP_TripModel_value"] <= Table[Instance]["IP_value"] and Table[Instance]["IP_TripModel_value"] <= Table[Instance]["IP_TripModel_HAP_fixed_value"] and Table[Instance]["IP_TripModel_value"] != -1:
                    line += " \\emph{" + str(Table[Instance]["IP_TripModel_value"]) + "}"
                else:
                    line += str(Table[Instance]["IP_TripModel_value"])
                line += " & " + str(gap_trip_model) + " & "

            if iTTP:
                gap_initial = round(((Initial_Best_value - int(Bounds[Instance])) / Initial_Best_value)*100,2)
                line += str(Initial_Best_value)  +" & " + str(gap_initial)  + " & "
            
            if (MiaoAlgo_Best_value <= Table[Instance]["IP_value"] or Table[Instance]["IP_value"] == -1) and (MiaoAlgo_Best_value <= Heuristic_Best_value or Heuristic_Best_value == -1) and MiaoAlgo_Best_value != -1 and (MiaoAlgo_Best_value <= Table[Instance]["IP_TripModel_value"] or Table[Instance]["IP_TripModel_value"] == -1):
                # line += "\\cellcolor{green!25}" 
                if not iTTP or (iTTP and MiaoAlgo_Best_value <= Table[Instance]["IP_TripModel_HAP_fixed_value"]):
                    line += "\\textbf{" + str(MiaoAlgo_Best_value) + "}"
                else:
                    line += str(MiaoAlgo_Best_value) 
            elif MiaoAlgo_Best_value >= Table[Instance]["IP_value"] and MiaoAlgo_Best_value >= Heuristic_Best_value and MiaoAlgo_Best_value >= Table[Instance]["IP_TripModel_value"]:
                # line += "\\cellcolor{red!25}" 
                if not iTTP or (iTTP and MiaoAlgo_Best_value >= Table[Instance]["IP_TripModel_HAP_fixed_value"]):
                    line += "\\emph{" + str(MiaoAlgo_Best_value) + "}"
                else:
                    line += str(MiaoAlgo_Best_value) 
            else:
                line += str(MiaoAlgo_Best_value) 

            gap_miao = round(((MiaoAlgo_Best_value - int(Bounds[Instance])) / MiaoAlgo_Best_value)*100,2)
            line += " & " + str(gap_miao)

            line += " & " + str(MiaoAlgo_Avg_value)

            # line += " \\\\ \n"

            if iTTP:
                gap_trip_model = round(((Table[Instance]["IP_TripModel_HAP_fixed_value"] - int(Bounds[Instance])) /  Table[Instance]["IP_TripModel_HAP_fixed_value"])*100,2)
                if Table[Instance]["IP_TripModel_HAP_fixed_value"] <= MiaoAlgo_Best_value and Table[Instance]["IP_TripModel_HAP_fixed_value"] <= Table[Instance]["IP_value"] and Table[Instance]["IP_TripModel_HAP_fixed_value"] != -1 and (Table[Instance]["IP_TripModel_value"] < 0 or Table[Instance]["IP_TripModel_HAP_fixed_value"] <= Table[Instance]["IP_TripModel_value"]):
                    line += " & \\textbf{" + str(Table[Instance]["IP_TripModel_HAP_fixed_value"]) + "}"
                elif Table[Instance]["IP_TripModel_HAP_fixed_value"] <= MiaoAlgo_Best_value and Table[Instance]["IP_TripModel_HAP_fixed_value"] <= Table[Instance]["IP_value"] and Table[Instance]["IP_TripModel_HAP_fixed_value"] <= Table[Instance]["IP_TripModel_value"]:
                    line += " & \\emph{" + str(Table[Instance]["IP_TripModel_HAP_fixed_value"]) + "}"
                else:
                    line += " & " + str(Table[Instance]["IP_TripModel_HAP_fixed_value"])
                line += " & " + str(gap_trip_model)
                line += " \\\\ \n"
            else:
            
                # line +=  " & " + str(Heuristic_Avg_value)  +" & " + str(Heuristic_Avg_time) + " & " 

                line += " & " + str(Heuristic_Avg_value) + " & "
                
                if (Heuristic_Best_value <= Table[Instance]["IP_value"] or Table[Instance]["IP_value"] == -1) and (Heuristic_Best_value <= MiaoAlgo_Best_value or MiaoAlgo_Best_value == -1) and Heuristic_Best_value != -1:
                    # line += "\\cellcolor{green!25}" + str(Heuristic_Best_value)  
                    line += "\\textbf{" + str(Heuristic_Best_value) + "}"
                elif Heuristic_Best_value >= Table[Instance]["IP_value"] and Heuristic_Best_value >= MiaoAlgo_Best_value:
                    # line += "\\cellcolor{red!25}" + str(Heuristic_Best_value)
                    line += "\\emph{" + str(Heuristic_Best_value) + "}"
                else:
                    line += str(Heuristic_Best_value)  
                
                line += " & " + str(Heuristic_Best_time) + " & " + str(Heuristic_best_HL) 

                gap_heuristic = round(((Heuristic_Best_value - int(Bounds[Instance])) / Heuristic_Best_value)*100,2)
                line += " & " + str(gap_heuristic) + " \\\\ \n"

                if InstanceSetting == "TTP":
                    for value in Table[Instance]["MiaoAlgo_Avg_value"]:
                        writer.writerow(["Greedy", Bounds[Instance], value])
                    writer.writerow(["IP", Bounds[Instance], Table[Instance]["IP_value"]])
                else:
                    for i, value in enumerate(Table[Instance]["Heuristic_Avg_value"]):
                        heur = "Heuristic_" + str(Table[Instance]["Heuristic_HL"][i])
                        writer.writerow([heur, InstanceSetting, Bounds[Instance], value])
                        if value < int(Bounds[Instance]):
                            print(Instance)
                            breakpoint()
                    for value in Table[Instance]["MiaoAlgo_Avg_value"]:
                        writer.writerow(["Greedy", InstanceSetting, Bounds[Instance], value])
                        if value < int(Bounds[Instance]):
                            print(Instance)
                            breakpoint()
                    writer.writerow(["IP", InstanceSetting, Bounds[Instance], Table[Instance]["IP_value"]])
                    if Table[Instance]["IP_value"] < int(Bounds[Instance]):
                        print(Instance)
                        breakpoint()


            output_file.write(line)
        
    print(f"done")
    breakpoint()

    # Individual neighborhoods:
    Path_base = os.path.join(os.path.join(PathRoot, "Heuristic"), "Base") 
    Path_iPTS = os.path.join(os.path.join(PathRoot, "Heuristic"), "iPTS_Random_PR") 
    Path_BM = os.path.join(os.path.join(PathRoot, "Heuristic"), "Random_BM_C") #
    Path_M = os.path.join(os.path.join(PathRoot, "Heuristic"), "Random_M_Random_PR") #
    Path_MinCost_BM = os.path.join(os.path.join(PathRoot, "Heuristic"), "MinCost_BM_C")
    Paths = {"Base": Path_base, "iPTS": Path_iPTS, "BM": Path_BM, "M": Path_M, "MinCost_BM": Path_MinCost_BM}
    Values = {inst: {"Base": [], "iPTS": [], "M": [], "BM": [], "MinCost_BM": []} for inst in instances}
    Gaps = {inst: {"Base": [], "iPTS": [], "M": [], "BM": [], "MinCost_BM": [], "all": [], "IP": [], "GM": []} for inst in instances}

    for key, Path in Paths.items():
        for file_index, filename in enumerate(os.listdir(Path)):
            if not filename.endswith(".txt"):
                continue
            FilePath = os.path.join(Path, filename)
            if not os.path.exists(FilePath):
                raise FileNotFoundError(f"The file '{FilePath}' does not exist.")
                sys.exit(0)
            with open(FilePath, 'r', newline="") as file:
                reader = csv.reader(file)
                max_time = 0
                for i, row in enumerate(reader):
                    if i == 0:
                        if InstanceSetting == "Miao":
                            inst = row[2]
                            setting = str(row[3])
                            nr_breaks = str(row[4])
                            Instance = inst + "_s" + setting + "_b" + nr_breaks
                        elif InstanceSetting == "Hockey":
                            Instance = row[2]
                        elif InstanceSetting == "TTP":
                            pattern = re.compile(r'(?:^|_)([A-Z]+[0-9]+_[0-9]+)(?:_|\.|$)')

                            match = pattern.search(filename)
                            if match:
                                Instance = match.group(1)
                                if Instance == "N16_4":
                                    Instance = "NL16_4"
                                elif Instance == "N16_8":
                                    Instance = "NL16_8"
                                elif Instance == "N16_12":
                                    Instance = "NL16_12"
                                # print(f'Extracted {Instance} from {filename}')
                            else:
                                print(f'No match for {filename} in path {FilePath}')
                                break

                    elif i > 0 and row[0] != "Final" and row[0] != "Time":
                        Values[Instance][key].append(int(row[1]))
                        break

    print(f'Table in text:')
    print(f"Instance & LB & IP & GM & Base & Min IPTS & Min M & Min BM+C & Min All \\\\")
    for inst in instances:
        # print(f"Instance = {inst}")
        # print(f"Length iPTS: {len(Values[inst]['iPTS'])}")
        if (len(Values[inst]['Base'])) > 0:
            Results[inst]["Avg_Base"] = sum(Values[inst]['Base']) / len(Values[inst]['Base'])
            Results[inst]["Min_Base"] = min(Values[inst]['Base']) 
        if (len(Values[inst]['iPTS'])) > 0:
            Results[inst]["Avg_iPTS"] = sum(Values[inst]['iPTS']) / len(Values[inst]['iPTS'])
            Results[inst]["Min_iPTS"] = min(Values[inst]['iPTS'])
        # print(f"Length M: {len(Values[inst]['M'])}")
        if (len(Values[inst]['M'])) > 0:
            Results[inst]["Avg_M"] = sum(Values[inst]['M']) / len(Values[inst]['M'])
            Results[inst]["Min_M"] = min(Values[inst]['M'])
        # print(f"Length BM: {len(Values[inst]['BM'])}")
        if (len(Values[inst]['BM'])) > 0:
            Results[inst]["Avg_BM"] = sum(Values[inst]['BM']) / len(Values[inst]['BM'])
            Results[inst]["Min_BM"] = min(Values[inst]['BM'])
        # print(f"Length MinCost_BM: {len(Values[inst]['MinCost_BM'])}")
        if (len(Values[inst]['MinCost_BM'])) > 0:
            Results[inst]["Avg_MinCost_BM"] = sum(Values[inst]['MinCost_BM']) / len(Values[inst]['MinCost_BM'])
            Results[inst]["Min_MinCost_BM"] = min(Values[inst]['MinCost_BM'])

        for key in ['Base','iPTS','M','BM','MinCost_BM']:
            for value in Values[inst][key]:
                Gaps[inst][key].append(round(((value-Bounds[inst])/value)*100,1))
        for value in Table[inst]["Heuristic_Avg_value"]:
            Gaps[inst]["all"].append(round(((value-Bounds[inst])/value)*100,1))


        # print(f"{inst} & {Bounds[inst]} & {Table[inst]['IP_value']} ({Gaps[inst]['IP']}) & {Results[inst]['Min_GM']} ({Gaps[inst]['GM']}) & {Results[inst]['Min_Base']} ({Gaps[inst]['Base']}) & {Results[inst]['Min_iPTS']} ({Gaps[inst]['iPTS']}) & {Results[inst]['Min_M'] } ({Gaps[inst]['M']}) & {Results[inst]['Min_BM']} ({Gaps[inst]['BM']}) & {Results[inst]['Min_all']} ({Gaps[inst]['all']}) \\\\")
        if InstanceSetting == "TTP":
            print(f"{inst} & {Bounds[inst]} & {Table[inst]['IP_TripModel_HAP_fixed_value']} & {Results[inst]['Min_Base']} & {Results[inst]['Min_iPTS']} & {Results[inst]['Min_M'] } & {Results[inst]['Min_BM']} & {Results[inst]['Min_all']} \\\\")
        else:
            print(f"{inst} & {Bounds[inst]} & {Table[inst]['IP_value']} & {Results[inst]['Min_GM']} & {Results[inst]['Min_Base']} & {Results[inst]['Min_iPTS']} & {Results[inst]['Min_M'] } & {Results[inst]['Min_BM']} & {Results[inst]['Min_all']} \\\\")
    breakpoint()

    # boxplots:
    AllValuesGaps = {"Base": [], "iPTS": [], "M": [], "BM": [], "all": []}
    for inst in instances:
        for key in AllValuesGaps.keys():
            print(f"size of Gaps[{inst}][{key}] = {len(Gaps[inst][key])}")
            AllValuesGaps[key] += Gaps[inst][key]

    # Friedmans test:
    print(f"{len(AllValuesGaps['Base'])}, {len(AllValuesGaps['iPTS'])}, {len(AllValuesGaps['M'])}, {len(AllValuesGaps['BM'])}, {len(AllValuesGaps['all'])}")
    stats.friedmanchisquare(AllValuesGaps["Base"], AllValuesGaps["iPTS"], AllValuesGaps["M"], AllValuesGaps["BM"], AllValuesGaps["all"])
    breakpoint()

    data = pd.DataFrame({
        "Method": ["Base"] * len(AllValuesGaps["Base"]) + ["iPTS"] * len(AllValuesGaps["iPTS"]) + ["M"] * len(AllValuesGaps["M"]) + ["BM"] * len(AllValuesGaps["BM"]) + ["all"] * len(AllValuesGaps["all"]),
        "Value": AllValuesGaps["Base"] + AllValuesGaps["iPTS"] + AllValuesGaps["M"] + AllValuesGaps["BM"] + AllValuesGaps["all"]
    })

    sns.boxplot(data=data, x="Method", y="Value")
    sns.swarmplot(data=data, x="Method", y="Value", color="black", alpha=0.8, size=2)

    # Ensure everything fits perfectly
    plt.tight_layout()

    if InstanceSetting == "TTP":
        plt.savefig("boxplot_TTP")
    elif InstanceSetting == "Miao":
        plt.savefig("boxplot_Football")
    else:
        plt.savefig("boxplot_Hockey")

    # Display the plot
    plt.show()

    breakpoint()

    print(f'Table in appendix:')
    print(f"Instance & Avg iPTS & Min IPTS & Avg M & Min M & Avg BM+C & Min BM+C & Avg All & Min All \\\\")
    for inst in instances:
        # print(f"Instance = {inst}")
        # print(f"Length iPTS: {len(Values[inst]['iPTS'])}")
        if (len(Values[inst]['iPTS'])) > 0:
            Results[inst]["Avg_iPTS"] = sum(Values[inst]['iPTS']) / len(Values[inst]['iPTS'])
            Results[inst]["Min_iPTS"] = min(Values[inst]['iPTS'])
        # print(f"Length M: {len(Values[inst]['M'])}")
        if (len(Values[inst]['M'])) > 0:
            Results[inst]["Avg_M"] = sum(Values[inst]['M']) / len(Values[inst]['M'])
            Results[inst]["Min_M"] = min(Values[inst]['M'])
        # print(f"Length BM: {len(Values[inst]['BM'])}")
        if (len(Values[inst]['BM'])) > 0:
            Results[inst]["Avg_BM"] = sum(Values[inst]['BM']) / len(Values[inst]['BM'])
            Results[inst]["Min_BM"] = min(Values[inst]['BM'])
        # print(f"Length MinCost_BM: {len(Values[inst]['MinCost_BM'])}")
        if (len(Values[inst]['MinCost_BM'])) > 0:
            Results[inst]["Avg_MinCost_BM"] = sum(Values[inst]['MinCost_BM']) / len(Values[inst]['MinCost_BM'])
            Results[inst]["Min_MinCost_BM"] = min(Values[inst]['MinCost_BM'])

        print(f"{inst} & {Results[inst]['Avg_iPTS']} & {Results[inst]['Min_iPTS']} & {Results[inst]['Avg_M']} & {Results[inst]['Min_M'] } & {Results[inst]['Avg_BM']} & {Results[inst]['Min_BM']} & {Results[inst]['Avg_all']} & {Results[inst]['Min_all']} \\\\")


def MergeBounds():
    OutputPath = os.path.join(os.path.join(os.path.join("Instances"), "TTP"), "Bounds.txt")
    with open(OutputPath, "w") as output_file:
        output_file.write(f'Instance,LB,UB,gap \n')
        for inst in NrRoundsTTP.keys():
            for r in NrRoundsTTP[inst]:
                FilePath = os.path.join(os.path.join(os.path.join(os.path.join("Instances"), "TTP"), "Bounds"), "DLB_I_" + inst + "_" + str(r) + ".txt")
                if not os.path.exists(FilePath):
                    print(f'The file {FilePath} does not exist yet!')
                    continue
                with open(FilePath, 'r', newline="") as file:
                    print(f'Open {FilePath}')
                    reader = csv.reader(file)
                    for i, row in enumerate(reader):
                        output_file.write(f'{inst}_{r},{row[1]},{row[2]},{round(float(row[3]),2)} \n')
    print(f'Merging bounds in file {OutputPath}')
                        

if __name__ == "__main__":
    setting = sys.argv[1]
    TTP = False
    FolderPath = "Instances"
    if setting == "TTP":
        TTP = True
        FolderPath = os.path.join(FolderPath, "TTP")
        print("Analyze results TTP")
    elif setting == "Miao":
        Miao = True
        FolderPath = os.path.join(FolderPath, "Miao")
    elif setting == "Hockey":
        FolderPath = os.path.join(FolderPath, "Hockey")
    else:
        print("Specify TTP or Miao or Hockey")
    FolderPath = os.path.join(FolderPath, "Results")

    if not os.path.exists(FolderPath):
        # Create the folder if it doesn't exist
        print(f"The folder {FolderPath} does not exists")

    ResultsInstances(setting, iTTP=False)


'''
import re

def RenameFiles(FolderPathHeuristicMinCost, FolderPathHeuristicNoMinCost):
    for directory in [FolderPathHeuristicMinCost, FolderPathHeuristicNoMinCost]:
        for file_index, filename in enumerate(os.listdir(directory)):
            for inst in ["CIRC40", "CON40", "GAL40", "INCR40", "LINE40", "N16"]:
                if inst in filename:
                    # match = re.match(f"({inst})_([\w_]+)_(\d+)\.txt", filename)
                    # if match:
                    # instance, middle, number = match.groups()
                    # new_name = f"{instance}_{number}_{middle}.txt"
                    old_path = os.path.join(directory, filename)
                    # new_path = os.path.join(directory, new_name)
                    # os.rename(old_path, new_path)
                    
                    # read all lines
                    with open(old_path, "r") as f:
                        lines = f.readlines()

                    # modify only the first line
                    if lines:
                        parts = lines[0].strip().split(",")
                        parts = parts[:2] + parts[4:] + parts[2:4]
                        lines[0] = ",".join(parts) + "\n"

                    # write everything back
                    with open(old_path, "w") as f:
                        f.writelines(lines)
'''
