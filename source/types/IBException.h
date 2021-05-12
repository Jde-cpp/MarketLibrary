#pragma once
#include <jde/Exception.h>
#include <jde/markets/Exports.h>

using std::string;
namespace Jde::Markets
{
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