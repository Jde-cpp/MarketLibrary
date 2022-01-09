#export CXX=g++-8
cmake_minimum_required(VERSION 3.16)
project(Jde.Markets)
set( CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/../../Framework/cmake )
include(SetUpConfigurations)
add_definitions( -DJDE_EXPORT_MARKETS )
#Custom######################################################
include_directories( "$ENV{REPO_DIR}/tws-api/source/cppclient/client" )
include_directories( "$ENV{REPO_DIR}/IntelRDFPMathLib20U2/LIBRARY/src" )
include_directories( "${PROJECT_SOURCE_DIR}/../../Public" )
#############################################################
add_library( Jde.Markets SHARED TwsProcessor.cpp OrderManager.cpp TickManager.cpp
	client/TwsClient.cpp client/TwsClientCo.cpp client/TwsClientSync.cpp
	client/awaitables/TwsAwaitable.cpp client/awaitables/HistoricalDataAwaitable.cpp
	wrapper/WrapperLog.cpp wrapper/WrapperCo.cpp wrapper/WrapperSync.cpp
	data/Accounts.cpp data/BarData.cpp data/HistoricalDataCache.cpp data/OptionData.cpp data/StatAwait.cpp
	types/Bar.cpp types/Contract.cpp types/Currencies.cpp types/Exchanges.cpp types/IBException.cpp types/MyOrder.cpp types/Tick.cpp types/TwsConnectionSettings.cpp
	types/proto/ib.pb.cc types/proto/bar.pb.cc types/proto/edgar.pb.cc types/proto/OptionOI.pb.cc types/proto/requests.pb.cc types/proto/results.pb.cc types/proto/watch.pb.cc )
#############################################################
target_precompile_headers(Jde.Markets
  PRIVATE
  		<Order.h>
		<OrderState.h>
		<CommonDefs.h>
		<Contract.h>
		<Execution.h>
		<jde/markets/Exports.h>
		<jde/markets/TypeDefs.h>
		"../../Framework/source/DateTime.h"
)
