
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

#pragma warning( disable : 4245) 
#include <boost/crc.hpp> 
#pragma warning( default : 4245) 
#include <boost/system/error_code.hpp>
#ifndef __INTELLISENSE__
	#include <spdlog/spdlog.h>
	#include <spdlog/sinks/basic_file_sink.h>
	#include <spdlog/fmt/ostr.h>
#endif

// #include <Eigen/Dense>
// #include <Eigen/Sparse>
// #include <Eigen/SVD>

#ifdef _MSC_VER
	#define TWSAPIDLLEXP __declspec( dllimport )
	#include <SDKDDKVer.h>
#else
	//#define IB_POSIX 1
	#define TWSAPIDLLEXP 
#endif
#include <nlohmann/json.hpp>

#include <EClientSocket.h>
#include <Contract.h>
#include <OrderState.h>
#include <Order.h>
#include <Execution.h>
#include <EClient.h>
#include <EWrapper.h>
#include <CommissionReport.h>


#include "../../Framework/source/TypeDefs.h"
#include "JdeAssert.h"
//#include "Collections.h"
#include "application/Application.h"
#include "threading/InterruptibleThread.h"

#include "DateTime.h"
#include "Stopwatch.h"
#include "StringUtilities.h"
#include "threading/Thread.h"
#include "threading/InterruptibleThread.h"
//#include "io/zip/xz/XZ.h"
//#include "db/Database.h"
//#include "math/EMatrix.h"
#include "Exports.h" //for ib.pb.h below
#pragma warning( disable : 4244 )
#include "types/proto/ib.pb.h"
//#include "types/proto/dts.pb.h"
#pragma warning( default : 4244 )

