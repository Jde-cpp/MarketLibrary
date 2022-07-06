#include "IBException.h"
#include <jde/Log.h>
#include "../../../Framework/source/log/server/ServerSink.h"

#define var const auto
namespace Jde::Markets
{
	IBException::IBException( string m, int code, long reqId, ELogLevel l, SL sl )ι:
		IException{ move(m), l, (uint)code, sl },
		RequestId{ reqId }
	{}

	IBException::~IBException()
	{
		Log();
	}

	α IBException::Log()Ι->void
	{
		if( Level()==ELogLevel::NoLog || Level()==ELogLevel::None )
			return;
		var message = IBMessage(); ASSERT( _stack.size() );
		var& sl = _stack.front();
		Logging::Default().log( spdlog::source_loc{FileName(sl.file_name()).c_str(),(int)sl.line(),sl.function_name()}, (spdlog::level::level_enum)Level(), message );
		if( Logging::ServerLevel()<=Level() )
			LogServer( Logging::Messages::ServerMessage{Logging::Message{Level(), message, sl}} );
	}
}