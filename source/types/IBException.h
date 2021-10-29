#pragma once
#include <jde/Exception.h>
#include <jde/markets/Exports.h>

namespace Jde::Markets
{
	//#define IB_Exception(message,errorCode,reqId) IBException{ message, errorCode, reqId }
	struct JDE_MARKETS_EXPORT IBException : public IException
	{
		IBException( const IBException& ) = default;
		IBException( IBException&& ) = default;
		IBException( sv message, int errorCode, long reqId, SRCE )noexcept;

		template<class... Args> IBException( const source_location& sl, IBException&& inner, sv m, Args&&... args ):
			IException{ sl, move(inner), m, args... },
			ErrorCode{ inner.ErrorCode },
			RequestId{ inner.RequestId }
		{}

		template<class... Args> IBException( const source_location& sl, int errorCode, long reqId, sv value, Args&&... args )noexcept:
			IException{ sl, value, args... },
			ErrorCode{ errorCode },
			RequestId{ reqId }
		{}

		α Log()const noexcept->void override;

		const int ErrorCode;
		const long RequestId{0};
	};

}