HPC   ?= 1
DAVID ?= 0

# TODO TODO Add a release and debug mode
CXX      = g++
CXXFLAGS = -O3 -Wall -g -std=c++17
LDFLAGS  =
INCLUDES =

# TODO Karel define PATHSEP as compileflag in cmake
-DPATHSEP="\"/\""

ifeq ($(DAVID),1)
    GUROBI        = /opt/gurobi952/linux64
    INCLUDES     += -I$(GUROBI)/include
    LDFLAGS      += -L$(GUROBI)/lib -lgurobi95 -lm 
    CXXFLAGS     += -DPATHSEP="\"/\""
else ifeq ($(HPC),1)
    LDFLAGS      += -lgurobi120 -lm 
    CXXFLAGS     += -DPATHSEP="\"/\""
endif

LDFLAGS += -lgurobi_c++ 

# Sources / objects / target
SRCS = $(wildcard *.cpp)
OBJS = $(SRCS:.cpp=.o)
TARGET = irr

# Default rule
all: $(TARGET)

# Link step
$(TARGET): $(OBJS)
	$(CXX) $(OBJS) -o $@ $(LDFLAGS)

# Compile step (pattern rule)
%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

# Cleanup
clean:
	rm -f $(OBJS) $(TARGET)

