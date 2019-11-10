CCACHE_EXISTS := $(shell ccache -V)
ifdef CCACHE_EXISTS
    CC := ccache $(CC)
    CXX := ccache g++-8
endif
DEBUG ?= 1
CXXFLAGS_BASE = -c -g -pthread -fPIC -std=c++17 -Wall -DJDE_EXPORT_MARKETS -DBOOST_SYSTEM_NO_DEPRECATED -Wno-unknown-pragmas -Wno-ignored-attributes
#-Wno-unknown-pragmas -Wno-ignored-attributes -Wno-int-in-bool-context

BIN_DIR_BASE:=../../bin/
OUT_DIR_BASE := .obj/
#LIBS = -lprotobuf -lboost_system

ifeq ($(DEBUG), 1)
	OUT_DIR=$(OUT_DIR_BASE)debug
	CXXFLAGS= $(CXXFLAGS_BASE) -O0 -I$(OUT_DIR)
	BIN_DIR=$(BIN_DIR_BASE)debug
else
	OUT_DIR=$(OUT_DIR_BASE)release
	CXXFLAGS= $(CXXFLAGS_BASE) -march=native -DNDEBUG -O3 -I$(OUT_DIR)
	BIN_DIR=$(BIN_DIR_BASE)release
endif

OUTPUT_FILE = libMarkets.so
OUTPUT = $(BIN_DIR)/$(OUTPUT_FILE)

all: $(OUTPUT)
	$(NOECHO) $(NOOP)

OBJECTS = $(OUT_DIR)/TwsProcessor.o $(OUT_DIR)/TwsClient.o $(OUT_DIR)/WrapperLog.o\
	$(OUT_DIR)/Bar.o $(OUT_DIR)/Contract.o $(OUT_DIR)/Exchanges.o $(OUT_DIR)/IBException.o $(OUT_DIR)/TwsConnectionSettings.o\
	$(OUT_DIR)/ib.pb.o $(OUT_DIR)/requests.pb.o $(OUT_DIR)/results.pb.o

#$(OUT_DIR)/QuoteProc.o $(OUT_DIR)/TwsClientSync.o $(OUT_DIR)/WrapperSync.o  $(OUT_DIR)/TradeSim.o  $(OUT_DIR)/TradeUtilities.o
#$(OUT_DIR)/AITypes.o $(OUT_DIR)/Bar.o  $(OUT_DIR)/Order.o $(OUT_DIR)/Transactions.o 
#$(OUT_DIR)/BarData.o $(OUT_DIR)/ContractData.o $(OUT_DIR)/QuoteData.o $(OUT_DIR)/OptionData.o $(OUT_DIR)/AIData.o  $(OUT_DIR)/SettingsData.o\
#$(OUT_DIR)/IBEnums.o 
#$(OUT_DIR)/ReceivedMessages.o $(OUT_DIR)/Requests.o\
#$(OUT_DIR)/dts.pb.o 
#$(OUT_DIR)/MinuteBar.pb.o $(OUT_DIR)/OptionBar.pb.o $(OUT_DIR)/OptionOI.pb.o
LDFLAGS = -pthread -shared -Wl,-z,origin -Wl,-rpath='$$ORIGIN'

$(OUTPUT): $(OUT_DIR)/stdafx.h.gch $(OBJECTS)
	$(CXX) -L$(BIN_DIR) $(LDFLAGS) $(OBJECTS) -o$(OUTPUT) $(LIBS)

$(OUT_DIR)/stdafx.h.gch: pc.h
	$(CXX) $(CXXFLAGS) pc.h -o$(OUT_DIR)/stdafx.h.gch -I/home/duffyj/code/libraries/spdlog/include -I/home/duffyj/code/libraries/tws-api/source/cppclient/client -I/home/duffyj/code/libraries/json/include -I/home/duffyj/code/libraries/boostorg/boost_1_68_0 -I../../Framework/source
	#-I/home/duffyj/code/libraries/eigen 

$(OUT_DIR)/%.o: ./%.cpp ./%.h pc.h
	$(CXX) $(CXXFLAGS) $(INCLUDE) ./$< -o$@ -c -I/home/duffyj/code/libraries/tws-api/source/cppclient/client
#-I/home/duffyj/code/libraries/eigen -I/home/duffyj/code/libraries/json/include

$(OUT_DIR)/%.o: ./types/%.cpp types/%.h pc.h
	$(CXX) $(CXXFLAGS) $(INCLUDE) ./$< -o$@ -c -I/home/duffyj/code/libraries/tws-api/source/cppclient/client
$(OUT_DIR)/%.o: ./types/messages/%.cpp types/messages/%.h pc.h
	$(CXX) $(CXXFLAGS) $(INCLUDE) ./$< -o$@ -c 
$(OUT_DIR)/%.o: ./types/proto/%.cc types/proto/%.h pc.h
	$(CXX) $(CXXFLAGS) $(INCLUDE) ./$< -o$@ -c -DJDE_MARKETS_EXPORT= -I/home/duffyj/code/libraries/protobuf/src
$(OUT_DIR)/%.o: ./data/%.cpp data/%.h pc.h
	$(CXX) $(CXXFLAGS) $(INCLUDE) ./$< -o$@ -c -c -I/home/duffyj/code/libraries/eigen

clean:
	rm -rf -d $(OUT_DIR)/*.*
	rm -rf -d $(OUTPUT)
