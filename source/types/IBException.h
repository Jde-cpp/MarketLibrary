#pragma once

using std::string;
namespace Jde::Markets
{
	struct JDE_MARKETS_EXPORT IBException : public Exception
	{
		IBException()=default;
		IBException(const IBException&) = default;
		IBException(IBException&&) = default;
		IBException( string_view message, int errorCode, long reqId=-1 ):
			Exception( message ),
			ErrorCode( errorCode ),
			//Request( request ),
			RequestId( reqId )
		{}

		void Log( std::string_view pszAdditionalInformation="", ELogLevel level=ELogLevel::Trace )const noexcept override;

		const int ErrorCode;
		//const void* Request;
		const long RequestId;
	};

}