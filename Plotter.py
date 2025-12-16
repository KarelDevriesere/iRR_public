import sys
import os
import csv
import pandas as pd
import re

import matplotlib.pyplot as plt
import numpy as np

# HPC: module load SciPy-bundle/2023.11-gfbf-2023b

NrRoundsCM = {36: 8, 100: 16, 250: 30}
RoundSet40 = [10,20,30]
RoundSet32 = [8,16,24]
RoundSet24 = [6,12,18]
RoundSet16 = [4,8,12]
NrRoundsTTP = {"BRA24": RoundSet24, "CIRC40": RoundSet40, "CON40": RoundSet40, "GAL40": RoundSet40, "INCR40": RoundSet40, "LINE40": RoundSet40, "NL16": RoundSet16, "NFL32": RoundSet32}
ListLengths = [1,5,10,50,100,500,1000,5000,10000,50000,100000] # list lengths
# ListLengths = [500,1000,5000,10000]

TIME_LIMIT = 600

def ResultsInstances(InstanceSetting):
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

    Table = {inst: {"IP_value": -1, "IP_bound": -1, "IP_time": -1, "MiaoAlgo_Avg_value": [], "MiaoAlgo_Avg_time": [], "MiaoAlgo_seed": [], "Heuristic_Avg_value": [], "Heuristic_Avg_time": [], "Heuristic_HL": []} for inst in instances}
    TableMatchings = {inst: {"NrSuccesfullMatchings": [], "NrInfeasibleMatchings": []} for inst in instances}
    PathIP = os.path.join(PathRoot, "IP")
    PathHeuristic = os.path.join(PathRoot, "Heuristic") # HeuristicStartingFromMiao
    PathMiaoAlgo = os.path.join(PathRoot, "MiaoAlgo")
    Paths = [PathIP, PathMiaoAlgo, PathHeuristic]
    for Path in Paths:
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
                        if method == "IP":
                            Table[Instance]["IP_value"] = int(row[1])
                            Table[Instance]["IP_bound"] = row[2]
                            if abs(int(row[1]) - int(row[2])) > 1:
                                Table[Instance]["IP_time"] = 7200
                            else:
                                Table[Instance]["IP_time"] = max_time
                        elif method == "MiaoAlgo":
                            Table[Instance]["MiaoAlgo_Avg_value"].append(int(row[1]))
                            Table[Instance]["MiaoAlgo_Avg_time"].append(int(row[2]))
                            Table[Instance]["MiaoAlgo_seed"].append(seed)
                        elif method == "Heuristic": # HeuristicStartingFromMiao
                            if row[0] == "Final":
                                Table[Instance]["Heuristic_Avg_value"].append(float(row[1]))
                                Table[Instance]["Heuristic_Avg_time"].append(float(row[2]))
                            else:
                                Table[Instance]["Heuristic_Avg_value"].append(float(row[1]))
                                Table[Instance]["Heuristic_Avg_time"].append(float(row[0]))
                        else:
                            print(f'Unknown method = {method}')
                            breakpoint()
                        break


    for inst in TableMatchings.keys():
        if len(TableMatchings[inst]["NrSuccesfullMatchings"]) > 0:
            avg_succes = round(sum(TableMatchings[inst]["NrSuccesfullMatchings"]) / len(TableMatchings[inst]["NrSuccesfullMatchings"]),2)
            avg_fail = round(sum(TableMatchings[inst]["NrInfeasibleMatchings"]) / len(TableMatchings[inst]["NrInfeasibleMatchings"]),2)
            print('{} & {} & {} \\\\ \n'.format(inst, avg_succes, avg_fail))
    
    if InstanceSetting == "Miao":
        OutputPath = os.path.join(os.path.join("Results", "Miao"), "Analysis.txt")   
    elif InstanceSetting == "TTP":
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
            output_file.write("Instance & Bound & IP_v & IP_b & IP_t & Miao_{av} & Miao_{at} & Miao_{bv} & Miao_{bt} & Heur_{av} & Heur_{at} & Heur_{bv} & Heur_{bt} & Heur_{bHL}\n")

        writer = csv.writer(output_file_boxplot)

        if write_header:
            if InstanceSetting == "TTP": 
                writer.writerow(["Algorithm", "LB", "Obj"])
            else:
                writer.writerow(["Algorithm", "InstanceClass", "LB", "Obj"])

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
            if len(Table[Instance]["Heuristic_Avg_value"]) > 0:
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
                
            if len(Table[Instance]["MiaoAlgo_Avg_value"]) > 0:
                MiaoAlgo_Avg_value = round(sum(Table[Instance]["MiaoAlgo_Avg_value"]) / len(Table[Instance]["MiaoAlgo_Avg_value"]),2)
                MiaoAlgo_Avg_time = round(sum(Table[Instance]["MiaoAlgo_Avg_time"]) / len(Table[Instance]["MiaoAlgo_Avg_time"]),2)
                MiaoAlgo_Best_value = Table[Instance]["MiaoAlgo_Avg_value"][0]
                MiaoAlgo_Best_time = Table[Instance]["MiaoAlgo_Avg_time"][0]
                MiaoAlgoBestSeed = Table[Instance]["MiaoAlgo_seed"][0]
    
                for i in range(1,len(Table[Instance]["MiaoAlgo_Avg_value"])):
                    if Table[Instance]["MiaoAlgo_Avg_value"][i] < MiaoAlgo_Best_value:
                        MiaoAlgo_Best_value = Table[Instance]["MiaoAlgo_Avg_value"][i]
                        MiaoAlgo_Best_time = Table[Instance]["MiaoAlgo_Avg_time"][i]
                        MiaoAlgoBestSeed = Table[Instance]["MiaoAlgo_seed"][i]

            # print(f'{{"{Instance}",{MiaoAlgoBestSeed}}},')
            
            line = Instance + " & "

            bound = -1
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
                            bound = row[1]
                            break
                line += str(bound) + " & "

            if (Table[Instance]["IP_value"] <= MiaoAlgo_Best_value or MiaoAlgo_Best_value == -1) and (Table[Instance]["IP_value"] <= Heuristic_Best_value or Heuristic_Best_value == -1) and Table[Instance]["IP_value"] != -1:
                line += "\\cellcolor{green!25}" + str(Table[Instance]["IP_value"]) 
            elif Table[Instance]["IP_value"] >= MiaoAlgo_Best_value and Table[Instance]["IP_value"] >= Heuristic_Best_value:
                line += "\\cellcolor{red!25}" + str(Table[Instance]["IP_value"])
            else:
                line += str(Table[Instance]["IP_value"]) 
            
            line += " & " + str(Table[Instance]["IP_bound"]) + " & " + str(Table[Instance]["IP_time"]) + " & " + str(MiaoAlgo_Avg_value)  +" & " + str(MiaoAlgo_Avg_time)  + " & " 

            if bound == -1:
                bound = Table[Instance]["IP_bound"]
            
            if (MiaoAlgo_Best_value <= Table[Instance]["IP_value"] or Table[Instance]["IP_value"] == -1) and (MiaoAlgo_Best_value <= Heuristic_Best_value or Heuristic_Best_value == -1) and MiaoAlgo_Best_value != -1:
                line += "\\cellcolor{green!25}" + str(MiaoAlgo_Best_value)  
            elif MiaoAlgo_Best_value >= Table[Instance]["IP_value"] and MiaoAlgo_Best_value >= Heuristic_Best_value:
                line += "\\cellcolor{red!25}" + str(MiaoAlgo_Best_value)
            else:
                line += str(MiaoAlgo_Best_value) 

            line += " & " + str(MiaoAlgo_Best_time) + " & " + str(Heuristic_Avg_value)  +" & " + str(Heuristic_Avg_time) + " & " 
            
            if (Heuristic_Best_value <= Table[Instance]["IP_value"] or Table[Instance]["IP_value"] == -1) and (Heuristic_Best_value <= MiaoAlgo_Best_value or MiaoAlgo_Best_value == -1) and Heuristic_Best_value != -1:
                line += "\\cellcolor{green!25}" + str(Heuristic_Best_value)  
            elif Heuristic_Best_value >= Table[Instance]["IP_value"] and Heuristic_Best_value >= MiaoAlgo_Best_value:
                line += "\\cellcolor{red!25}" + str(Heuristic_Best_value)
            else:
                line += str(Heuristic_Best_value)  
            
            line += " & " + str(Heuristic_Best_time) + " & " + str(Heuristic_best_HL) + " \\\\ \n"

            if InstanceSetting == "TTP":
                for value in Table[Instance]["MiaoAlgo_Avg_value"]:
                    writer.writerow(["Greedy", bound, value])
                writer.writerow(["IP", bound, Table[Instance]["IP_value"]])
            else:
                for i, value in enumerate(Table[Instance]["Heuristic_Avg_value"]):
                    heur = "Heuristic_" + str(Table[Instance]["Heuristic_HL"][i])
                    writer.writerow([heur, InstanceSetting, bound, value])
                    if value < int(bound):
                        print(Instance)
                        breakpoint()
                for value in Table[Instance]["MiaoAlgo_Avg_value"]:
                    writer.writerow(["Greedy", InstanceSetting, bound, value])
                    if value < int(bound):
                        print(Instance)
                        breakpoint()
                writer.writerow(["IP", InstanceSetting, bound, Table[Instance]["IP_value"]])
                if Table[Instance]["IP_value"] < int(bound):
                    print(Instance)
                    breakpoint()


            output_file.write(line)
        
    print(f"done")


def ResultsPercHAPs():
    Path = os.path.join(os.path.join(os.path.join("Instances", "TTP"), "Results"), "MiaoAlgo")
    Instances = []
    for key in NrRoundsTTP.keys():
        for round_ in NrRoundsTTP[key]:
            Instances.append("I_"+key+"_"+str(round_))
    Percentages = [10, 30, 50, 70, 90]
    Seeds = [0,11,42,154,396,588,1217,2486,5003,10000]
    Values = {inst: {perc: [] for perc in Percentages} for inst in Instances}
    Times = {inst: {perc: [] for perc in Percentages} for inst in Instances}
    BestValues = {inst: {perc: -1 for perc in Percentages} for inst in Instances}
    BestTimes = {inst: {perc: -1 for perc in Percentages} for inst in Instances}
    for file_index, filename in enumerate(os.listdir(Path)):
        if not filename.endswith(".txt") or "PercHAPs" not in filename:
            continue
        FilePath = os.path.join(Path, filename)
        if not os.path.exists(FilePath):
            raise FileNotFoundError(f"The file '{FilePath}' does not exist.")
            sys.exit(0)
        with open(FilePath, 'r', newline="") as file:
            reader = csv.reader(file)
            for i, row in enumerate(reader):
                if i == 0:
                    seed = str(row[0])
                    method = row[1]
                    instance = row[2]
                    HL = row[3] # does not matter
                    Perc = int(row[4]) # percentage of the haps used
                    if instance not in Instances or Perc not in Percentages:
                        break
                elif row[0] == "Final":
                    Values[instance][Perc].append(int(row[1]))
                    Times[instance][Perc].append(int(row[2]))
                    break

    print(f'Instance & bv10 & t10 & bv30 & t30 & bv50 & t50 & bv70 & t70 & bv90 & t90 \\\\ \n')
    for instance in Instances:
        for Perc in Percentages:
            if len(Values[instance][Perc]) > 0:
                best_value = Values[instance][Perc][0]
                best_time = Times[instance][Perc][0]
                for i in range(1,len(Values[instance][Perc])):
                    if Values[instance][Perc][i] < best_value:
                        best_value = Values[instance][Perc][i]
                        best_time = Times[instance][Perc][i]
                BestValues[instance][Perc] = best_value
                BestTimes[instance][Perc] = best_time
        print(f'{instance} & {BestValues[instance][10]} & {BestTimes[instance][10]} & {BestValues[instance][30]} & {BestTimes[instance][30]} & {BestValues[instance][50]} & {BestTimes[instance][50]} & {BestValues[instance][70]} & {BestTimes[instance][70]} & {BestValues[instance][90]} & {BestTimes[instance][90]} \\\\')


def MergeBounds():
    OutputPath = os.path.join(os.path.join(os.path.join("Instances"), "TTP"), "Bounds.txt")
    with open(OutputPath, "w") as output_file:
        output_file.write(f'Instance,LB,UB,gap \n')
        for inst in NrRoundsTTP.keys():
            for r in NrRoundsTTP[inst]:
                FilePath = os.path.join(os.path.join(os.path.join("Instances"), "TTP"), "Bound_" + inst + "_" + str(r) + ".txt")
                if not os.path.exists(FilePath):
                    print(f'The file {FilePath} does not exist yet!')
                    continue
                with open(FilePath, 'r', newline="") as file:
                    print(f'Open {FilePath}')
                    reader = csv.reader(file)
                    for i, row in enumerate(reader):
                        output_file.write(f'{row[0]}_{row[4]},{row[1]},{row[2]},{round(float(row[3]),2)} \n')
    print(f'Merging bounds in file {OutputPath}')


def TableListLengthst(FolderPathHeuristic):
    OutputPath = os.path.join(os.path.join(os.path.join("Results"), "TTP"), "ListLengths.txt")
    OutputPath_InstGapL = os.path.join(os.path.join(os.path.join("Results"), "TTP"), "InstGapL.txt")
    OutputPath_InstTimeL = os.path.join(os.path.join(os.path.join("Results"), "TTP"), "InstTimeL.txt")
    Output = {inst: {r: {l: {"Time": 0, "Value": 0} for l in ListLengths} for r in NrRoundsTTP[inst]} for inst in NrRoundsTTP.keys()}
    Output_InstGapL = {inst: {r: {l: 0 for l in ListLengths} for r in NrRoundsTTP[inst]} for inst in NrRoundsTTP.keys()}
    Output_InstTimeL = {inst: {r: {l: 0 for l in ListLengths} for r in NrRoundsTTP[inst]} for inst in NrRoundsTTP.keys()}
    with open(OutputPath, "w") as output_file, open(OutputPath_InstGapL, "w") as output_file_InstGapL, open(OutputPath_InstTimeL , "w") as output_file_InstTimeL :
        for i, l in enumerate(ListLengths):
            if i < len(ListLengths)-1:
                output_file.write(f'L{l}-Value,L{l}-Time,')
            else:
                output_file.write(f'L{l}-Value,L{l}-Time \n')
        output_file_InstGapL.write("Instance,Gap,L\n")
        output_file_InstTimeL.write("Instance,Time,L\n")
        for inst in NrRoundsTTP.keys():
            for r in NrRoundsTTP[inst]:
                BestList = ListLengths[0]
                BestValue = 2147483647
                for l in ListLengths:
                    FilePath = os.path.join(os.path.join(os.path.join(os.path.join(os.path.join("Instances"), "TTP"), "Results"), "Heuristic"), inst + "_" + str(r) + "_s0_HL" + str(l) + ".txt")
                    if not os.path.exists(FilePath):
                        print(f'The file {FilePath} does not exist yet!')
                        continue
                    with open(FilePath, 'r', newline="") as file:
                        # print(f'Open {FilePath}')
                        reader = csv.reader(file)
                        for i, row in enumerate(reader):
                            if row[0] == "Final":
                                Output[inst][r][l]["Value"] = row[1]
                                Output[inst][r][l]["Time"] = row[2]
                                if int(row[1]) < BestValue :
                                    BestList = l
                                    BestValue = int(row[1])
                                FilePathBound = os.path.join(os.path.join(os.path.join(os.path.join("Instances"), "TTP"), "Bounds"), "Bound_" + inst + "_" + str(r) + ".txt")
                                with open(FilePathBound, 'r', newline="") as file2:
                                    # print(f'Open {FilePathBound}')
                                    reader2 = csv.reader(file2)
                                    for i, row2 in enumerate(reader2):
                                        Bound = int(row2[1])
                                Output_InstGapL[inst][r][l] = str(round((int(row[1])-Bound)/int(row[1]),2))
                                Output_InstTimeL[inst][r][l] = row[2]
                                break
                print(f'{inst}_{r}, {BestList}')
        for inst in NrRoundsTTP.keys():
            for r in NrRoundsTTP[inst]:
                output_file.write(f'{inst}_{r},')
                output_file.write(f'{inst}_{r},')
                for i, l in enumerate(ListLengths):
                    if i < len(ListLengths)-1:
                        output_file.write(str(Output[inst][r][l]["Value"])+","+str(Output[inst][r][l]["Time"])+",")
                    else:
                        output_file.write(str(Output[inst][r][l]["Value"])+","+str(Output[inst][r][l]["Time"])+"\n")
                    output_file_InstGapL.write(f"{inst}_{r},{Output_InstGapL[inst][r][l]},{l} \n")
                    output_file_InstTimeL.write(f"{inst}_{r},{Output_InstTimeL[inst][r][l]},{l} \n")


def MakeLinePlotInstTimeL():
    InputPath_InstGapL = os.path.join(os.path.join(os.path.join("Results"), "TTP"), "InstTimeL.txt")

    df = pd.read_csv(InputPath_InstGapL, sep=",")

    # Ensure L is numeric and sort to get correct line order
    df["L"] = pd.to_numeric(df["L"])
    df = df.sort_values(["Instance", "L"])

    fig, ax = plt.subplots(figsize=(20, 10))  # fig is the Figure, ax is the Axes

    ax.set_xscale("log")
    ax.set_xticks([l for l in ListLengths])  
    from matplotlib.ticker import ScalarFormatter
    ax.xaxis.set_major_formatter(ScalarFormatter())

    for instance, group in df.groupby("Instance"):
        ax.plot(group["L"], group["Time"], label=instance)

    ax.set_title("Line plot")
    ax.set_xlabel("Length")
    ax.set_ylabel("Time")
    ax.legend(loc="center left", bbox_to_anchor=(1, 0.5))
    fig.savefig(os.path.join("Figures","TimeListLengths.png"))
    plt.show()

        
def MakeLinePlotInstGapL():
    InputPath_InstGapL = os.path.join(os.path.join(os.path.join("Results"), "TTP"), "InstGapL.txt")

    df = pd.read_csv(InputPath_InstGapL, sep=",")

    # Ensure L is numeric and sort to get correct line order
    df["L"] = pd.to_numeric(df["L"])
    df = df.sort_values(["Instance", "L"])

    fig, ax = plt.subplots(figsize=(20, 10))  # fig is the Figure, ax is the Axes

    ax.set_xscale("log")
    ax.set_xticks([l for l in ListLengths])  
    from matplotlib.ticker import ScalarFormatter
    ax.xaxis.set_major_formatter(ScalarFormatter())

    for instance, group in df.groupby("Instance"):
        ax.plot(group["L"], group["Gap"], label=instance)

    ax.set_title("Line plot")
    ax.set_xlabel("Length")
    ax.set_ylabel("Gap")
    ax.legend(loc="center left", bbox_to_anchor=(1, 0.5))
    fig.savefig(os.path.join("Figures","GapListLengths.png"))
    plt.show()



def BoxPlotsAblation(FolderPath):
    FolderPathIP = os.path.join(FolderPath, "IP")
    FolderPathBase = os.path.join(FolderPath, "Base")
    FolderPathM = os.path.join(os.path.join(FolderPath, "Heuristic"), "M_uniform")
    FolderPathBM = os.path.join(os.path.join(FolderPath, "Heuristic"), "BM_uniform")
    FolderPathPTS = os.path.join(os.path.join(FolderPath, "Heuristic"), "iPTS_uniform")
    FolderPathAll = os.path.join(FolderPath, "Heuristic")
    FolderPaths = [FolderPathIP, FolderPathBase, FolderPathM, FolderPathBM, FolderPathPTS, FolderPathAll]

    FilePathBounds = os.path.join(os.path.join("Instances", "TTP"), "DVBKDB.txt")
    print(f'Open {FilePathBounds}')
    Bounds = {inst: {r: 0 for r in NrRoundsTTP[inst]} for inst in NrRoundsTTP.keys()}
    with open(FilePathBounds, 'r', newline="") as file:
        reader = csv.reader(file)
        for i, row in enumerate(reader):
            inst = row[0]
            if inst == "SUP12":
                continue
            lb = int(row[1])
            r = int(row[2])
            Bounds[inst][r] = lb
            # print(f'LB of {inst} with {r} rounds  = {lb}')

    # boxplots 
    data = {"IP": [], "Base": [], "M": [], "BM": [], "iPTS": [], "All": []}

    for inst in NrRoundsTTP.keys():
        for r in NrRoundsTTP[inst]:
            for algo, FolderPath in zip(data.keys(), FolderPaths):
                if algo == "IP":
                    File = inst + "_" + str(r) + ".txt"
                else:
                    File = inst + "_" + str(r) + "_s0_HL500" + ".txt"
                FilePath = os.path.join(FolderPath, File)
                if not os.path.exists(FilePath):
                    print(f'The file {FilePath} does not exist yet!')
                    continue
                with open(FilePath, 'r', newline="") as file:
                    reader = csv.reader(file)
                    for i, row in enumerate(reader):
                        if row[0] == "Final":
                            BestSolution = int(row[1])
                            gap = (BestSolution-Bounds[inst][r])/BestSolution
                            if gap > 0.5 and algo != "IP":
                                print(f'Instance {inst}_{r} has a gap of {round(gap,2)} (lb = {Bounds[inst][r]}), (ub = {BestSolution})')
                            data[algo].append(gap)
                            break

    BoxPlots = [data["IP"], data["Base"], data["M"], data["BM"], data["iPTS"], data["All"]]
    fig, ax = plt.subplots(figsize=(10, 7))
    ax.boxplot(BoxPlots, labels=data.keys())
    OutputPath = os.path.join("Figures", "Ablation_test1.png")
    print(f"Save results in {OutputPath}")
    fig.savefig(OutputPath) 
    plt.show()


def MakeTable(Paths):
    row_table = {"IP": -1, "Heuristic": {l: -1 for l in ListLengths}, "BaseAlgo": {l: -1 for l in ListLengths}}
    for path in Paths:
        if not os.path.exists(path):
            raise FileNotFoundError(f"The file '{path}' does not exist.")
            sys.exit(0)
        with open(path, 'r', newline="") as file:
            reader = csv.reader(file)
            for i, row in enumerate(reader):
                if i == 0:
                    method = row[1]
                    print(f'{method}')
                    seed = row[0]
                    if method != "IP":
                        MaxIt = int(row[2])
                        TimeLimit = int(row[3])
                        ListLength = int(row[4])
                        ConstrViolationCost = int(row[5])
                elif row[0] == "Final":
                    value = int(float(row[1]))
                    if method == "IP":
                        row_table[method] = value
                    else:
                        row_table[method][ListLength] = value
                    break
    return row_table


def InstancesCM():
    Instances = []
    for NrTeams in [36, 100, 250]:
        for k in [1,5,10]:
            for i in range(5):
                Instance = str(NrTeams) + "_" + str(NrRoundsCM[NrTeams]) + "_k" + str(k) + "_" + str(i)
                Instances.append(Instance)
    return Instances


def InstancesTTP():
    Instances = []
    for i in ["BRA24", "CIRC40", "CON40", "GAL40", "INCR40", "LINE40", "N16", "NFL32"]:
        for NrRounds in NrRoundsTTP[i]:
            Instance = i + "_" + str(NrRounds)
            Instances.append(Instance)
    return Instances


def WriteOutput(CM,TTP,FolderPathIP, FolderPathHeuristic, FolderPathBase,OutputPath):
    with open(OutputPath, "w") as output_file:
        table = []
        columns = ["Instance", "IP"] + ["Heuristic_" + "L" + str(l) for l in ListLengths] + ["Base_" + "L" + str(l) for l in ListLengths]
        for c, col in enumerate(columns):
            output_file.write(col)
            if c < len(columns):
                output_file.write(",")
        output_file.write("\n")
        if CM:
            Instances = InstancesCM()
        else:
            Instances = InstancesTTP()

        for Instance in Instances:
            # print(Instance)
            # Now, search through all the files and see if Instance is in the name of the file
            Paths = []
            for directory in [FolderPathIP, FolderPathHeuristic, FolderPathBase]:
                for file_index, filename in enumerate(os.listdir(directory)):
                    if Instance in filename and filename.endswith(".txt"):
                        Paths.append(os.path.join(directory,filename))

            if len(Paths) == 0:
                # print(f"No results for instance {Instance}")
                pass
            else:
                # print(f"Make plot of instance {Instance}")
                row_dict = MakeTable(Paths)
                row = [Instance, row_dict["IP"]] + [row_dict["Heuristic"][l] for l in ListLengths] + [row_dict["BaseAlgo"][l] for l in ListLengths]
                table.append(row)
                for v, val in enumerate(row):
                    output_file.write(str(val))
                    if v < len(row):
                        output_file.write(",")
                output_file.write("\n")
        df = pd.DataFrame(table, columns=columns)
        print(df)


def Analyze(CM,TTP,FolderPathIP, FolderPathHeuristic, FolderPathBase):
    if CM:
        OutputPath = os.path.join(os.path.join("Results", "CM"), "BestValues.txt")
    else:
        OutputPath = os.path.join(os.path.join("Results", "TTP"), "BestValues.txt")
    print(f"Save results in {OutputPath}")
    WriteOutput(CM,TTP,FolderPathIP, FolderPathHeuristic, FolderPathBase,OutputPath)


def MakePlotTimeListLength(FolderPathHeuristic,CM,TTP):
    # loop over all instances, store values per list length!!
    if CM:
        OutputPath = os.path.join(os.path.join("Results", "CM"), "PlotLengthTime.png")
    else:
        OutputPath = os.path.join(os.path.join("Results", "TTP"), "PlotLengthTime.png")

    data = {l: [] for l in ListLengths}


    for directory in [FolderPathHeuristic]:
        for file_index, filename in enumerate(os.listdir(directory)):
            if not filename.endswith(".txt"):
                continue
            path = os.path.join(directory, filename)
            if not os.path.exists(path):
                raise FileNotFoundError(f"The file '{path}' does not exist.")
                sys.exit(0)
            with open(path, 'r', newline="") as file:
                print(path)
                reader = csv.reader(file)
                prev_time = 0
                for i, row in enumerate(reader):
                    if i == 0:
                        Length = int(row[3])
                    if i > 1:
                        time = row[0]
                        value = int(row[1])
                        if time == "Final":
                            time = prev_time+30
                            data[Length].append(time)
                            break
                        else:
                            prev_time = int(time)
                            data[Length].append(int(time))
                            
    ListBoxplot = [data[l] for l in ListLengths]
    fig, ax = plt.subplots()  # fig is the Figure, ax is the Axes

    ax.set_yscale("log")
    ax.set_yticks([1, 10, 30, 60, 120, 180, 300, 600, 1200, 2400, 3600, 7200])  
    from matplotlib.ticker import ScalarFormatter
    ax.yaxis.set_major_formatter(ScalarFormatter())

    ax.boxplot(ListBoxplot, labels=ListLengths)
    ax.set_title("Boxplot")
    ax.set_xlabel("Length")
    ax.set_ylabel("Time")
    print(f"Save results in {OutputPath}")
    # fig.savefig(OutputPath)  # saves as PNG
    plt.show()
                        

if __name__ == "__main__":
    '''
    TableListLengthst("")
    breakpoint()
    MakeLinePlotInstGapL()
    MakeLinePlotInstTimeL()
    breakpoint()
    '''
    ResultsPercHAPs()
    breakpoint()
    setting = sys.argv[1]
    CM = False
    TTP = False
    FolderPath = "Instances"
    if setting == "CM":
        CM = True
        print("Analyze results cost minimization")
        for subfolder in ["CostMinimization", "Karel", "0_100"]:
            FolderPath = os.path.join(FolderPath, subfolder)
    elif setting == "TTP":
        TTP = True
        FolderPath = os.path.join(FolderPath, "TTP")
        print("Analyze results TTP")
    elif setting == "Miao":
        Miao = True
        FolderPath = os.path.join(FolderPath, "Miao")
    elif setting == "Hockey":
        FolderPath = os.path.join(FolderPath, "Hockey")
    else:
        print("Specify CM or TTP or Miao or Hockey")
    FolderPath = os.path.join(FolderPath, "Results")

    if not os.path.exists(FolderPath):
        # Create the folder if it doesn't exist
        print(f"The folder {FolderPath} does not exists")

    ResultsInstances(setting)
    breakpoint()
    
    FolderPathIP = os.path.join(FolderPath, "IP")
    FolderPathHeuristic = os.path.join(FolderPath, "Heuristic")
    FolderPathHeuristic = os.path.join(os.path.join(FolderPath, "Heuristic"), "NoMinCost") # TODo
    FolderPathHeuristicNoMinCost = FolderPathHeuristic
    FolderPathHeuristicMinCost = os.path.join(os.path.join(FolderPath, "Heuristic"), "MinCost")
    FolderPathBase = os.path.join(FolderPath, "Base")

    TableListLengthst(FolderPathHeuristic)
    # BoxPlotsAblation(FolderPath)
    # MakePlotTimeListLength(FolderPathHeuristic,CM,TTP)
    # Analyze(CM,TTP,FolderPathIP, FolderPathHeuristic, FolderPathBase)


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
