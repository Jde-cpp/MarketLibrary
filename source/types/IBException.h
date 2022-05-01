#pragma once
#include <jde/Exception.h>
#include <jde/markets/Exports.h>

#define $ template<class... Args>
namespace Jde::Markets
{
	struct ΓM IBException : IException
	{
		//IBException( const IBException& ) = default;
		IBException( IBException&& from )ι:IException{move(from)}, RequestId{ from.RequestId }{}
		IBException( string message, int errorCode, long reqId, ELogLevel level=ELogLevel::Debug, SRCE )ι;
		~IBException();
		$ IBException( const source_location& sl, int errorCode, long reqId, sv value, Args&&... args )ι;
		Ω SP( string m, int c, long id, SRCE )ι->sp<IException>{ return std::dynamic_pointer_cast<IException>(ms<IBException>(move(m), c, id, ELogLevel::NoLog, sl)); }

		α Log()const ι->void override;
		using T=IBException;
		α Clone()ι->sp<IException> override{ return std::make_shared<T>(move(*this)); }
		α Move()ι->up<IException> override{ return mu<T>(move(*this)); }
		α Ptr()->std::exception_ptr override
		{ 
			auto p = Jde::make_exception_ptr( move(*this) ); 
			return p;
		}
		[[noreturn]] α Throw()->void override{ throw move(*this); }
		const long RequestId{0};
	};

	$ IBException::IBException( const source_location& sl, int errorCode, long reqId, sv m, Args&&... args )ι:
		IException{ sl, m, args... },
		//ErrorCode{ errorCode },
		RequestId{ reqId }
	{}
}
#undef $