all:
	g++ *.cpp -g -I/opt/gurobi952/linux64//include/ -L/opt/gurobi952/linux64//lib -lgurobi_c++  -lgurobi95 -lm 
