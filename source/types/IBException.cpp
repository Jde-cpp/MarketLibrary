#include "IBException.h"
#include <jde/Log.h>
#include "../../../Framework/source/log/server/ServerSink.h"

#define var const auto
namespace Jde::Markets
{
	IBException::IBException( string message, int errorCode, long reqId, SL sl )noexcept:
		IException{ move(message), ELogLevel::Debug, (uint)errorCode, sl },
		RequestId{ reqId }
	{}

	IBException::~IBException()
	{
		Log();
		_level = ELogLevel::NoLog;
	}

	α IBException::Log()const noexcept->void
	{
		if( _level==ELogLevel::NoLog )
			return;
		std::ostringstream os;
		var message = format( "({})[{}] - {}", RequestId, (int)Code, what() );
		ASSERT( _stack.size() );
		var& sl = _stack.front();
		Logging::Default().log( spdlog::source_loc{FileName(sl.file_name()).c_str(),(int)sl.line(),sl.function_name()}, (spdlog::level::level_enum)_level, message );
		if( _level>=Logging::ServerLevel() )
			LogServer( Logging::Messages::ServerMessage{Logging::Message{_level, message, sl}} );
	}
}