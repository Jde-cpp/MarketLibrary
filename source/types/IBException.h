#pragma once
#include <jde/Exception.h>
#include <jde/markets/Exports.h>

namespace Jde::Markets
{
	struct ΓM IBException : public IException
	{
		IBException( const IBException& ) = default;
		IBException( IBException&& ) = default;
		IBException( sv message, int errorCode, long reqId, SRCE )noexcept;
		~IBException();
		template<class... Args> IBException( const source_location& sl, int errorCode, long reqId, sv value, Args&&... args )noexcept;
		Ω SP( sv m, int c, long id )noexcept->sp<IException>{ return std::dynamic_pointer_cast<IException>(std::make_shared<IBException>(m, c, id, source_location{})); }
		α Clone()noexcept->sp<IException> override{ return std::make_shared<IBException>(move(*this)); }
		α Log()const noexcept->void override;
		α Ptr()->std::exception_ptr override{ return std::make_exception_ptr(*this); }
		[[noreturn]] α Throw()->void override{ throw *this; }

		const int ErrorCode;
		const long RequestId{0};
	};

	template<class... Args> IBException::IBException( const source_location& sl, int errorCode, long reqId, sv value, Args&&... args )noexcept:
		IException{ sl, value, args... },
		ErrorCode{ errorCode },
		RequestId{ reqId }
	{}
}