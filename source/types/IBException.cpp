#include "IBException.h"
#include <jde/Log.h>
#include "../../../Framework/source/log/server/ServerSink.h"

#define var const auto
namespace Jde::Markets
{
	IBException::IBException( string m, int code, long reqId, ELogLevel l, SL sl )noexcept:
		IException{ move(m), l, (uint)code, sl },
		RequestId{ reqId }
	{}

	IBException::~IBException()
	{
		Log();
	}

	α IBException::Log()const noexcept->void
	{
		if( Level()==ELogLevel::NoLog || Level()==ELogLevel::None )
			return;
		std::ostringstream os;
		var message = format( "({})[{}] - {}", RequestId, (int)Code, what() );
		ASSERT( _stack.size() );
		var& sl = _stack.front();
		Logging::Default().log( spdlog::source_loc{FileName(sl.file_name()).c_str(),(int)sl.line(),sl.function_name()}, (spdlog::level::level_enum)Level(), message );
		if( Logging::ServerLevel()<=Level() )
			LogServer( Logging::Messages::ServerMessage{Logging::Message{Level(), message, sl}} );
	}
}