HPC   ?= 0
DAVID ?= 1

# TODO TODO Add a release and debug mode
CXX      = g++
CXXFLAGS = -Wall -g -std=c++17
LDFLAGS  =
INCLUDES =

# Default build mode: release
# To compile debug: make MODE=DEBUG
MODE ?= release
ifeq ($(MODE),release)
    PRINT ?= 0
    CXXFLAGS += -O3 -DNDEBUG -DPRINT=$(PRINT)
else ifeq ($(MODE),debug)
    PRINT ?= 1
    CXXFLAGS += -O0 -g -DDEBUG -DPRINT=$(PRINT)
endif

# TODO Karel define PATHSEP as compileflag in cmake
-DPATHSEP="\"/\""

LDFLAGS += -lgurobi_c++ 

ifeq ($(DAVID),1)
    GUROBI        = /opt/gurobi952/linux64
    INCLUDES     += -I$(GUROBI)/include
    LDFLAGS      += -lgurobi95 -L$(GUROBI)/lib -lm 
    CXXFLAGS     += -DPATHSEP="\"/\""
else ifeq ($(HPC),1)
    LDFLAGS      += -lgurobi120 -lm 
    CXXFLAGS     += -DPATHSEP="\"/\""
endif



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

