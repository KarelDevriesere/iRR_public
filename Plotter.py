import sys
import os
import csv
import pandas as pd

import matplotlib.pyplot as plt
import numpy as np

# HPC: module load SciPy-bundle/2023.11-gfbf-2023b

NrRoundsCM = {36: 8, 100: 16, 250: 30}
RoundSet40 = [10,20,30]
RoundSet32 = [8,16,24]
RoundSet24 = [6,12,18]
RoundSet16 = [4,8,12]
NrRoundsTTP = {"BRA24": RoundSet24, "CIRC40": RoundSet40, "CON40": RoundSet40, "GAL40": RoundSet40, "INCR40": RoundSet40, "LINE40": RoundSet40, "N16": RoundSet16, "NFL32": RoundSet32}
ListLengths = [1,5,10,50,100,500,1000,5000,10000,50000,100000] # list lengths


def ResultsMiaoInstances():
    I = ["i01", "i02", "i03", "i04", "i05", "i06"]
    S = ["0","1","2"]
    B = ["3", "100"]
    instances = []
    for i in I:
        for s in S:
            for b in B:
                instances.append(i+"_s"+s+"_b"+b)
    Table = {inst: {"IP_value": -1, "IP_bound": -1, "IP_time": -1, "MiaoAlgo_Avg_value": [], "MiaoAlgo_Avg_time": [], "Heuristic_Avg_value": [], "Heuristic_Avg_time": [], "Heuristic_HL": []} for inst in instances}
    PathRoot = os.path.join(os.path.join(os.path.join("Instances"), "Miao"), "Results")
    PathIP = os.path.join(PathRoot, "IP")
    PathHeuristic = os.path.join(PathRoot, "Heuristic")
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
                        inst = row[2]
                        setting = str(row[3])
                        nr_breaks = str(row[4])
                        Instance = inst + "_s" + setting + "_b" + nr_breaks
                        if method == "Heuristic":
                            HL = str(row[5])
                            Table[Instance]["Heuristic_HL"].append(HL)
                    elif i > 1 and row[0] != "Final":
                        if int(row[0]) > max_time:
                            max_time = int(row[0])
                    elif row[0] == "Final":
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
                        elif method == "Heuristic":
                            Table[Instance]["Heuristic_Avg_value"].append(float(row[1]))
                            Table[Instance]["Heuristic_Avg_time"].append(float(row[2]))
                        else:
                            print(f'Method = {method}')
                            breakpoint()
                        break

    OutputPath = os.path.join(os.path.join("Results", "Miao"), "Analysis.txt")   
    with open(OutputPath, 'w') as output_file:
        output_file.write("Instance & IP_v & IP_b & IP_t & Miao_{av} & Miao_{at} & Miao_{bv} & Miao_{bt} & Heur_{av} & Heur_{at} & Heur_{bv} & Heur_{bt} & Heur_{bHL}\n")
        for Instance in Table.keys():
            if len(Table[Instance]["Heuristic_Avg_value"]) == 0 or len(Table[Instance]["MiaoAlgo_Avg_value"]) == 0:
                continue
            Heuristic_Avg_value = round(sum(Table[Instance]["Heuristic_Avg_value"]) / len(Table[Instance]["Heuristic_Avg_value"]),2)
            Heuristic_Avg_time = round(sum(Table[Instance]["Heuristic_Avg_time"]) / len(Table[Instance]["Heuristic_Avg_time"]),2)
            MiaoAlgo_Avg_value = round(sum(Table[Instance]["MiaoAlgo_Avg_value"]) / len(Table[Instance]["MiaoAlgo_Avg_value"]),2)
            MiaoAlgo_Avg_time = round(sum(Table[Instance]["MiaoAlgo_Avg_time"]) / len(Table[Instance]["MiaoAlgo_Avg_time"]),2)
            MiaoAlgo_Best_value = Table[Instance]["MiaoAlgo_Avg_value"][0]
            MiaoAlgo_Best_time = Table[Instance]["MiaoAlgo_Avg_time"][0]
            Heuristic_Best_value = Table[Instance]["Heuristic_Avg_value"][0]
            Heuristic_Best_time = Table[Instance]["Heuristic_Avg_time"][0]
            Heuristic_best_HL = Table[Instance]["Heuristic_HL"][0]
            for i in [1,2,3,4]:
                if Table[Instance]["Heuristic_Avg_value"][i] < Heuristic_Best_value:
                    Heuristic_Best_value = Table[Instance]["Heuristic_Avg_value"][i]
                    Heuristic_Best_time = Table[Instance]["Heuristic_Avg_time"][i]
                    Heuristic_best_HL = Table[Instance]["Heuristic_HL"][i]
                if Table[Instance]["MiaoAlgo_Avg_value"][i] < MiaoAlgo_Best_value:
                    MiaoAlgo_Best_value = Table[Instance]["MiaoAlgo_Avg_value"][i]
                    MiaoAlgo_Best_time = Table[Instance]["MiaoAlgo_Avg_time"][i]
            
            line = Instance + " & "
            if Table[Instance]["IP_value"] <= MiaoAlgo_Best_value and Table[Instance]["IP_value"] <= Heuristic_Best_value:
                line += "\\cellcolor{green!25}" + str(Table[Instance]["IP_value"]) 
            elif Table[Instance]["IP_value"] >= MiaoAlgo_Best_value and Table[Instance]["IP_value"] >= Heuristic_Best_value:
                line += "\\cellcolor{red!25}" + str(Table[Instance]["IP_value"])
            else:
                line += str(Table[Instance]["IP_value"]) 
            
            line += " & " + str(Table[Instance]["IP_bound"]) + " & " + str(Table[Instance]["IP_time"]) + " & " + str(MiaoAlgo_Avg_value)  +" & " + str(MiaoAlgo_Avg_time)  + " & " 
            
            if MiaoAlgo_Best_value <= Table[Instance]["IP_value"] and MiaoAlgo_Best_value <= Heuristic_Best_value:
                line += "\\cellcolor{green!25}" + str(MiaoAlgo_Best_value)  
            elif MiaoAlgo_Best_value >= Table[Instance]["IP_value"] and MiaoAlgo_Best_value >= Heuristic_Best_value:
                line += "\\cellcolor{red!25}" + str(MiaoAlgo_Best_value)
            else:
                line += str(MiaoAlgo_Best_value) 

            line += " & " + str(MiaoAlgo_Best_time) + " & " + str(Heuristic_Avg_value)  +" & " + str(Heuristic_Avg_time) + " & " 
            
            if Heuristic_Best_value <= Table[Instance]["IP_value"] and Heuristic_Best_value <= MiaoAlgo_Best_value:
                line += "\\cellcolor{green!25}" + str(Heuristic_Best_value)  
            elif Heuristic_Best_value >= Table[Instance]["IP_value"] and Heuristic_Best_value >= MiaoAlgo_Best_value:
                line += "\\cellcolor{red!25}" + str(Heuristic_Best_value)
            else:
                line += str(Heuristic_Best_value)  
            
            line += " & " + str(Heuristic_Best_time) + " & " + str(Heuristic_best_HL) + " \\\\ \n"


            output_file.write(line)
        
    print(f"done")
                        


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
    ResultsMiaoInstances()
    breakpoint()
    CM = False
    TTP = False
    FolderPath = "Instances"
    if sys.argv[1] == "CM":
        CM = True
        print("Analyze results cost minimization")
        for subfolder in ["CostMinimization", "Karel", "0_100"]:
            FolderPath = os.path.join(FolderPath, subfolder)
    elif sys.argv[1] == "TTP":
        TTP = True
        FolderPath = os.path.join(FolderPath, "TTP")
        print("Analyze results TTP")
    elif sys.argv[1] == "Miao":
        Miao = True
        FolderPath = os.path.join(FolderPath, "Miao")
    else:
        print("Specify CM or TTP")
    FolderPath = os.path.join(FolderPath, "Results")

    if not os.path.exists(FolderPath):
        # Create the folder if it doesn't exist
        print(f"The folder {FolderPath} does not exists")
    
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
