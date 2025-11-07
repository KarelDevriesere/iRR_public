# In this version we included the Xerces library as a static library so that the executables can be shipped more easily

APP = decompose
DEBUG =  -g -fstack-protector -DDEBUG  #-Wall -Wvla -Werror -D_GLIBCXX_DEBUG
RELEASE = -Ofast -frename-registers -funroll-loops -march=native 

# HPC
HOMEDIR = $(shell pwd)
#HOMEDIR = /data/gent/419/vsc41980/SCOR2018/Papers/MinCostTimeRelaxed/PATATAbstract/Code_Relaxed

ROBINX = $(HOMEDIR)/source/include

# CPLEX SUPPORT: Change the following paths as needed
CONCERTDIR = /opt/ibm/ILOG/CPLEX_Studio221/concert
CPLEXDIR = /opt/ibm/ILOG/CPLEX_Studio221/cplex
CPOPTDIR = /opt/ibm/ILOG/CPLEX_Studio221/cpoptimizer

# HPC paths
#CONCERTDIR = /apps/gent/CO7/haswell-ib/software/CPLEX/12.10-GCCcore-8.3.0-Python-3.7.4/concert
#CPLEXDIR = /apps/gent/CO7/haswell-ib/software/CPLEX/12.10-GCCcore-8.3.0-Python-3.7.4/cplex
#CPOPTDIR = /apps/gent/CO7/haswell-ib/software/CPLEX/12.10-GCCcore-8.3.0-Python-3.7.4/cpoptimizer

# TODO TODO TODO
# Do not forget 
# module load Boost


INCLUDECPLEX = -DIL_STD -I$(CONCERTDIR)/include -I$(CPLEXDIR)/include -I${CPOPTDIR}/include
# -lilocplex must come before -lcplex !!
LINKCPLEX = -L${CPOPTDIR}/lib/x86-64_linux/static_pic -lcp -L$(CPLEXDIR)/lib/x86-64_linux/static_pic -lilocplex -lcplex -L$(CONCERTDIR)/lib/x86-64_linux/static_pic -lconcert -lpthread
CFLAGS += ${INCLUDECPLEX} -DPERFECT_MATCHING_DOUBLE ${RELEASE}
# See  -fno-strict-aliasing --> avoid cplex warnings
CFLAGS +=  -fno-strict-aliasing
LDFLAGS += ${LINKCPLEX}

CC = g++-11
LD = g++-11

CFLAGS += -std=c++11 -I${ROBINX}
LDFLAGS += -lpthread -ldl -I${ROBINX}

# Use all threads to compile in parallel
MAKEFLAGS += -j 2

MODULES   := $(shell ls ./source/)
SRC_DIR   := $(addprefix source/,$(MODULES))
BUILD_DIR := $(addprefix build/,$(MODULES))

SRC       := $(foreach sdir,$(SRC_DIR),$(wildcard $(sdir)/*.cpp))
OBJ       := $(patsubst source/%.cpp,build/%.o,$(SRC))
INCLUDES  := $(addprefix -I,$(SRC_DIR))



vpath %.cpp $(SRC_DIR)

define make-goal
$1/%.o: %.cpp
	@echo "[CC] $<"
	$(CC) $(CFLAGS) $(RELEASE) -c $$< -o $$@
endef

.PHONY: all checkdirs clean

all: checkdirs build/$(APP)



CFLAGS += -DNDEBUG -DSAVECUT #-DVERBOSE


random.o: source/padbergRao/random.c source/padbergRao/random.h
	        $(CC) $(CFLAGS) -c -o build/padbergRao/random.o source/padbergRao/random.c

fprintfll.o: source/padbergRao/fprintfll.c source/padbergRao/fprintfll.h
	        $(CC) $(CFLAGS) -c -o build/padbergRao/fprintfll.o source/padbergRao/fprintfll.c

timer.o: source/padbergRao/timer.c source/padbergRao/timer.h
	        $(CC) $(CFLAGS) -c -o build/padbergRao/timer.o source/padbergRao/timer.c

tree.o: source/padbergRao/graph.h source/padbergRao/tree.c source/padbergRao/tree.h
	        $(CC) $(CFLAGS) -DHO -DNO_PR -c -o build/padbergRao/tree.o source/padbergRao/tree.c

compact.o: source/padbergRao/graph.h source/padbergRao/compact.c source/padbergRao/compact.h
	        $(CC) $(CFLAGS) -DHO -DNO_PR -c -o build/padbergRao/compact.o source/padbergRao/compact.c

#roundrobin.o: source/padbergRao/roundrobin.cpp source/padbergRao/graph.c source/padbergRao/graph.h
#	        $(CC) $(CFLAGS) -c -o roundrobin.o source/padbergRao/roundrobin.cpp

graph_HO_nopr.o: source/padbergRao/graph.c source/padbergRao/graph.h source/padbergRao/memassert.h source/padbergRao/fprintfll.h source/padbergRao/timer.h
	        $(CC) $(CFLAGS) -DHO -DNO_PR -c source/padbergRao/graph.c -o build/padbergRao/graph_HO_nopr.o

#gh.o: ho_nopr.o random.o timer.o compact.o tree.o graph_HO_nopr.o fprintfll.o roundrobin.o
#	        $(CC) $(CFLAGS) ho_nopr.o random.o timer.o compact.o tree.o graph_HO_nopr.o fprintfll.o roundrobin.o $(LOADLIBES) -c -o gh.o


ho_nopr.o: source/padbergRao/ho.c source/padbergRao/graph.h source/padbergRao/memassert.h source/padbergRao/fprintfll.h source/padbergRao/timer.h source/padbergRao/ho.h source/padbergRao/qs.h
	        $(CC) $(CFLAGS) $(LDFLAGS) -DNO_PR  -c source/padbergRao/ho.c -o build/padbergRao/ho_nopr.o


build/$(APP): $(OBJ) 
	@echo "[LD] $<"
	$(LD) $(ALLPADBis)  $^ $(LDFLAGS) -o $(APP)

checkdirs: $(BUILD_DIR)

$(BUILD_DIR):
	@mkdir -p $@

clean:
	@rm -rf $(BUILD_DIR)
	@rm $(APP)

$(foreach bdir,$(BUILD_DIR),$(eval $(call make-goal,$(bdir))))






