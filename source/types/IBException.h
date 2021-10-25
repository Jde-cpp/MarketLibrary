#pragma once
#include <jde/Exception.h>
#include <jde/markets/Exports.h>

namespace Jde::Markets
{
	#define IB_Exception(message,errorCode,reqId) IBException{ message, errorCode, reqId }
	struct JDE_MARKETS_EXPORT IBException : public IException
	{
		IBException( const IBException& ) = default;
		IBException( IBException&& ) = default;
		IBException( sv message, int errorCode, long reqId, SRCE )noexcept;

		void Log()const noexcept override;

		const int ErrorCode;
		const long RequestId{0};
	};

}