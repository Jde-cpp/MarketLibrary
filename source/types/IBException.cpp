#include "IBException.h"
#include <jde/Log.h>
#include "../../../Framework/source/log/server/ServerSink.h"

#define var const auto
namespace Jde::Markets
{
	IBException::IBException( sv message, int errorCode, long reqId, const source_location& sl )noexcept:
		IException{ ELogLevel::Debug, message, sl },
		ErrorCode{ errorCode },
		RequestId{ reqId }
	{
		Log();
	}

	α IBException::Log()const noexcept->void
	{
		std::ostringstream os;
		var message = format( "({})[{}] - {}", RequestId, ErrorCode, what() );
		Logging::Default().log( spdlog::source_loc{FileName(_sl.file_name()).c_str(),(int)_sl.line(),_sl.function_name()}, (spdlog::level::level_enum)_level, message );
		if( _level>=Logging::ServerLevel() )
			LogServer( Logging::Messages::ServerMessage{Logging::Message{_level, message, _sl}} );
	}
}