cmake_minimum_required(VERSION 3.16)
project( Tests.MarketLibrary VERSION 1.0.0.0 )
set( CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/../../Framework/cmake )
include(SetUpConfigurations)

configure_file(CMakeLists.txt.in googletest-download/CMakeLists.txt)
execute_process(COMMAND ${CMAKE_COMMAND} -G "${CMAKE_GENERATOR}" .
  RESULT_VARIABLE result
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/googletest-download )
if(result)
  message(FATAL_ERROR "CMake step for googletest failed: ${result}")
endif()
execute_process(COMMAND ${CMAKE_COMMAND} --build .
  RESULT_VARIABLE result
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/googletest-download )
if(result)
  message(FATAL_ERROR "Build step for googletest failed: ${result}")
endif()

# Prevent overriding the parent project's compiler/linker settings on Windows
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

# Add googletest directly to our build. This defines the gtest and gtest_main targets.
add_subdirectory(${CMAKE_CURRENT_BINARY_DIR}/googletest-src
                 ${CMAKE_CURRENT_BINARY_DIR}/googletest-build
                 EXCLUDE_FROM_ALL)


link_directories( ${CMAKE_LIBRARY_OUTPUT_DIRECTORY} )
add_executable(Tests.MarketLibrary main.cpp HistoricalDataCacheTest.cpp NewsTest.cpp)
include_directories( "$ENV{REPO_DIR}/tws-api/source/cppclient/client" )
#set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -stdlib=libc++")
#target_link_libraries(Tests.MarketLibrary c++abi)
target_link_libraries(Tests.MarketLibrary Jde.Markets)
target_link_libraries(Tests.MarketLibrary Jde)
target_link_libraries(Tests.MarketLibrary Jde.XZ)
target_link_libraries(Tests.MarketLibrary lzma)
target_link_libraries(Tests.MarketLibrary Ib)
target_link_libraries(Tests.MarketLibrary protobuf)
target_link_libraries(Tests.MarketLibrary gtest_main)
target_link_libraries(Tests.MarketLibrary stdc++)
target_link_libraries(Tests.MarketLibrary stdc++fs)


#target_include_directories(Tests.MarketLibrary PUBLIC ${REPO_DIR}/llvm-project/build/lib)
add_test(NAME HistoricalDataCacheTest COMMAND MethodBarDoesAbc)
target_precompile_headers(Tests.MarketLibrary PRIVATE
	<boost/uuid/uuid.hpp>
	<boost/uuid/uuid_generators.hpp>
	<boost/uuid/uuid_io.hpp>
	<boost/crc.hpp>
	<boost/system/error_code.hpp>
	<spdlog/spdlog.h>
	<spdlog/sinks/basic_file_sink.h>
	<spdlog/fmt/ostr.h>
	<nlohmann/json.hpp>

	"../../Framework/source/TypeDefs.h"
	"../../Framework/source/DateTime.h"
	"../../MarketLibrary/source/TypeDefs.h"
)