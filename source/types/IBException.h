#pragma once
#include "../../../Framework/source/Exception.h"
#include "../Exports.h"

using std::string;
namespace Jde::Markets
{
	struct JDE_MARKETS_EXPORT IBException : public Exception
	{
		IBException( const IBException& ) = default;
		IBException( IBException&& ) = default;
		IBException( string_view message, int errorCode, long reqId, string_view function, string_view file, long line )noexcept;
		IBException( string_view message, int errorCode, long reqId=-1 )noexcept:
			Exception( message ),
			ErrorCode( errorCode ),
			RequestId( reqId )
		{}

		void Log( std::string_view pszAdditionalInformation="", ELogLevel level=ELogLevel::Trace )const noexcept override;

		const int ErrorCode;
		const long RequestId{0};
	};

}