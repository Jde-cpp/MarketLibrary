#pragma once
#include <jde/Exception.h>
#include <jde/markets/Exports.h>

#define $ template<class... Args>
namespace Jde::Markets
{
	struct ΓM IBException : IException
	{
		//IBException( const IBException& ) = default;
		IBException( IBException&& from ):IException{move(from)}, RequestId{ from.RequestId }{}
		IBException( string message, int errorCode, long reqId, SRCE )noexcept;
		~IBException();
		$ IBException( const source_location& sl, int errorCode, long reqId, sv value, Args&&... args )noexcept;
		Ω SP( string m, int c, long id )noexcept->sp<IException>{ return std::dynamic_pointer_cast<IException>(ms<IBException>(move(m), c, id, source_location{})); }

		α Log()const noexcept->void override;
		using T=IBException;
		α Clone()noexcept->sp<IException> override{ return ms<T>(move(*this)); }\
		α Move()noexcept->up<IException> override{ return mu<T>(move(*this)); }\
		α Ptr()->std::exception_ptr override
		{ 
			auto p = Jde::make_exception_ptr( move(*this) ); 
			return p;
		}\
		[[noreturn]] α Throw()->void override{ throw move(*this); }
		const long RequestId{0};
	};

	$ IBException::IBException( const source_location& sl, int errorCode, long reqId, sv m, Args&&... args )noexcept:
		IException{ sl, m, args... },
		//ErrorCode{ errorCode },
		RequestId{ reqId }
	{}
}
#undef $