CCACHE_EXISTS := $(shell ccache -V)
ifdef CCACHE_EXISTS
    CC := ccache $(CC)
    CXX := ccache clang++
endif

DEBUG ?= 1
CXXFLAGS_BASE = -c -g -pthread -fPIC -std=c++20 -stdlib=libc++ -Wall $(INCLUDE)
BIN_DIR_BASE:=.obj/
OUT_DIR_BASE := .

ifeq ($(DEBUG), 1)
	OUT_DIR=$(OUT_DIR_BASE)debug
	CXXFLAGS= $(CXXFLAGS_BASE)-O0 -I$(OUT_DIR)
	BIN_DIR=$(BIN_DIR_BASE)debug
else
	OUT_DIR=$(OUT_DIR_BASE)release
	CXXFLAGS= $(CXXFLAGS_BASE) -DNDEBUG -O3 -I$(OUT_DIR)
	BIN_DIR=$(BIN_DIR_BASE)release
endif

OBJECTS = $(OUT_DIR)/EMutex.o $(OUT_DIR)/EClient.o $(OUT_DIR)/EReader.o $(OUT_DIR)/ESocket.o $(OUT_DIR)/EOrderDecoder.o $(OUT_DIR)/EDecoder.o $(OUT_DIR)/EMessage.o $(OUT_DIR)/EClientSocket.o $(OUT_DIR)/TimeCondition.o $(OUT_DIR)/OrderCondition.o $(OUT_DIR)/PriceCondition.o $(OUT_DIR)/SoftDollarTier.o $(OUT_DIR)/DefaultEWrapper.o $(OUT_DIR)/EReaderOSSignal.o $(OUT_DIR)/MarginCondition.o $(OUT_DIR)/VolumeCondition.o $(OUT_DIR)/ContractCondition.o $(OUT_DIR)/OperatorCondition.o $(OUT_DIR)/executioncondition.o $(OUT_DIR)/PercentChangeCondition.o
EXEC = $(BIN_DIR)/libIb.so
all: $(EXEC)
	$(NOECHO) $(NOOP)

$(EXEC): $(OUT_DIR)/pc.h.gch $(OBJECTS)
	$(CXX) -pthread -L$(BIN_DIR) -shared -Wl,-z,origin -Wl,-rpath='$$ORIGIN' $(OBJECTS) -o$(EXEC)

$(OUT_DIR)/pc.h.gch: StdAfx.h
	$(CXX) $(CXXFLAGS) StdAfx.h -o $(OUT_DIR)/pc.h.gch

$(OUT_DIR)/%.o: %.cpp %.h $(OUT_DIR)/pc.h.gch
	$(CXX) $(CXXFLAGS) $(INCLUDE) ./$< -o$@ -c -DStdAfx=pc

clean:
	#rm stdafx.h.gch
	rm -rf -d $(OUT_DIR)/*.*
	rm -rf -d $(EXEC)

