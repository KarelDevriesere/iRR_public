import sys
import os
import csv
import pandas as pd

NrRoundsCM = {36: 8, 100: 16, 250: 30}
ListLengths = [1,10,50,100,500,5000] # list lengths


def MakePlot(Paths):
    row_table = {"IP": -1, "MinCost": {l: -1 for l in ListLengths}, "NoMinCost": {l: -1 for l in ListLengths}}
    for path in Paths:
        if not os.path.exists(path):
            raise FileNotFoundError(f"The file '{path}' does not exist.")
            sys.exit(0)
        with open(path, 'r', newline="") as file:
            reader = csv.reader(file)
            for i, row in enumerate(reader):
                if i == 0:
                    method = row[1]
                    seed = row[0]
                    if method == "Heuristic":
                        if int(row[2]) == 0:
                            MinCost = "NoMinCost"
                        else:
                            MinCost = "MinCost"
                        ListLength = int(row[3])
                elif row[0] == "Final":
                    value = int(float(row[1]))
                    if method == "IP":
                        row_table[method] = value
                    else:
                        row_table[MinCost][ListLength] = value
                    break
    return row_table
                 

if __name__ == "__main__":
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
        print("Analyze results TTP")
    else:
        print("Specify CM or TTP")
    FolderPath = os.path.join(FolderPath, "Results")

    if not os.path.exists(FolderPath):
        # Create the folder if it doesn't exist
        print(f"The folder {FolderPath} does not exists")
    
    FolderPathIP = os.path.join(FolderPath, "IP")
    FolderPathHeuristicMinCost = os.path.join(os.path.join(FolderPath, "Heuristic"), "MinCost")
    FolderPathHeuristicNoMinCost = os.path.join(os.path.join(FolderPath, "Heuristic"), "NoMinCost")

    if CM:
        OutputPath = os.path.join(os.path.join("Results", "CM"), "BestValues.txt")
        print(f"Save results in {OutputPath}")
        with open(OutputPath, "w") as output_file:
            table = []
            columns = ["Instance", "IP"] + ["MinCost_" + "L" + str(l) for l in ListLengths] + ["NoMinCost_" + "L" + str(l) for l in ListLengths]
            for c, col in enumerate(columns):
                output_file.write(col)
                if c < len(columns):
                    output_file.write(",")
            output_file.write("\n")
            for NrTeams in [36, 100, 250]:
                for k in [1,5,10]:
                    for i in range(5):
                        Instance = str(NrTeams) + "_" + str(NrRoundsCM[NrTeams]) + "_k" + str(k) + "_" + str(i)
                        # print(Instance)
                        # Now, search through all the files and see if Instance is in the name of the file
                        Paths = []
                        for directory in [FolderPathIP, FolderPathHeuristicMinCost, FolderPathHeuristicNoMinCost]:
                            for file_index, filename in enumerate(os.listdir(directory)):
                                if Instance in filename:
                                    Paths.append(os.path.join(directory,filename))

                        if len(Paths) == 0:
                            # print(f"No results for instance {Instance}")
                            pass
                        else:
                            # print(f"Make plot of instance {Instance}")
                            row_dict = MakePlot(Paths)
                            row = [Instance, row_dict["IP"]] + [row_dict["MinCost"][l] for l in ListLengths] + [row_dict["NoMinCost"][l] for l in ListLengths]
                            table.append(row)
                            for v, val in enumerate(row):
                                output_file.write(str(val))
                                if v < len(row):
                                    output_file.write(",")
                            output_file.write("\n")
            df = pd.DataFrame(table, columns=columns)
            print(df)

    elif TTP:
        pass
