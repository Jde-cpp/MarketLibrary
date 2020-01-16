#export CXX=g++-8
cmake_minimum_required(VERSION 3.16)
project(Jde.Markets)
set( CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/../../Framework/cmake )
include(SetUpConfigurations)
#Custom######################################################
include_directories( "/home/duffyj/code/libraries/tws-api/source/cppclient/client" )
#############################################################
add_library( Jde.Markets SHARED TwsProcessor.cpp TwsClient.cpp WrapperLog.cpp
	types/Bar.cpp types/Contract.cpp types/Exchanges.cpp types/IBException.cpp types/MyOrder.cpp types/TwsConnectionSettings.cpp
	types/proto/ib.pb.cc types/proto/requests.pb.cc types/proto/results.pb.cc )
#############################################################
target_precompile_headers(Jde.Markets
  PRIVATE
	<boost/uuid/uuid.hpp>
	<boost/uuid/uuid_generators.hpp>
	<boost/uuid/uuid_io.hpp>
	<boost/crc.hpp>
	<boost/system/error_code.hpp>
	<spdlog/spdlog.h>
	<spdlog/sinks/basic_file_sink.h>
	<spdlog/fmt/ostr.h>
	<nlohmann/json.hpp>
	<EClientSocket.h>
	<Contract.h>
	<OrderState.h>
	<Order.h>
	<Execution.h>
	<EClient.h>
	<EWrapper.h>
	<CommissionReport.h>
	"../../Framework/source/TypeDefs.h"
	"../../Framework/source/JdeAssert.h"
	"../../Framework/source/application/Application.h"
	"../../Framework/source/threading/InterruptibleThread.h"
	"../../Framework/source/DateTime.h"
	"../../Framework/source/Stopwatch.h"
	"../../Framework/source/StringUtilities.h"
	"../../Framework/source/threading/Thread.h"
	"../../Framework/source/threading/InterruptibleThread.h"
	"Exports.h"
)
