#pragma once
#include <jde/Exception.h>
#include <jde/markets/Exports.h>

namespace Jde::Markets
{
	#define IB_Exception(message,errorCode,reqId) IBException{ message, errorCode, reqId, __func__, __FILE__, __LINE__ }
	struct JDE_MARKETS_EXPORT IBException : public Exception
	{
		IBException( const IBException& ) = default;
		IBException( IBException&& ) = default;
		IBException( sv message, int errorCode, long reqId, sv function, sv file, long line )noexcept;
		IBException( sv message, int errorCode, long reqId=-1 )noexcept:
			Exception( message ),
			ErrorCode( errorCode ),
			RequestId( reqId )
		{}

		void Log( sv pszAdditionalInformation="", ELogLevel level=ELogLevel::Trace )const noexcept override;

		const int ErrorCode;
		const long RequestId{0};
	};

}