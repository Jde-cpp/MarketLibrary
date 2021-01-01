
#ifdef _MSC_VER
	#include <WinSock2.h>
	#define TWSAPIDLLEXP __declspec( dllimport )
//	#include <SDKDDKVer.h>
#else
	//#define IB_POSIX 1
	#define TWSAPIDLLEXP
#endif
//#ifndef __INTELLISENSE__

#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/fmt/ostr.h>
//#endif

/*
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

#pragma warning( disable : 4245)
#include <boost/crc.hpp>
#pragma warning( default : 4245)
#include <boost/system/error_code.hpp>
*/
//#include <nlohmann/json.hpp>
//#ifdef _MSC_VER
//	#undef assert
//	#include <platformspecific.h>
//	#ifdef NDEBUG
//		#define assert(expression) ((void)0)
//	#else
//		#define assert(expression) (void)( (!!(expression)) || (_wassert(_CRT_WIDE(#expression), _CRT_WIDE(__FILE__), (unsigned)(__LINE__)), 0) )
//	#endif
//#endif
/*
#include <EClientSocket.h>
#include <OrderState.h>
#include <Order.h>
*/
#include <Contract.h>
#include <Execution.h>
/*
#include <EClient.h>
#include <EWrapper.h>
#include <CommissionReport.h>

*/
#include "../../Framework/source/TypeDefs.h"
/*
#include "../../Framework/source/JdeAssert.h"
//#include "Collections.h"
#include "../../Framework/source/application/Application.h"
#include "../../Framework/source/threading/InterruptibleThread.h"
*/
#include "../../Framework/source/DateTime.h"
//#include "../../Framework/source/Stopwatch.h"

#include "../../Framework/source/StringUtilities.h"
/*
#include "../../Framework/source/threading/Thread.h"
#include "../../Framework/source/threading/InterruptibleThread.h"
//#include "../../io/zip/xz/XZ.h"
//#include "db/Database.h"
//#include "math/EMatrix.h"
#include "Exports.h" //for ib.pb.h below
#pragma warning( disable : 4244 )
#include "types/proto/ib.pb.h"
//#include "types/proto/dts.pb.h"
#pragma warning( default : 4244 )

*/