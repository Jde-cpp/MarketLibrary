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
		Logging::Default().log( spdlog::source_loc{FileName(_fileName).c_str(),(int)_line,_functionName.data()}, (spdlog::level::level_enum)_level, message );
		if( _level>=Logging::ServerLevel() )
			LogServer( Logging::Messages::Message{Logging::Message2{_level, message, _fileName, _functionName, _line}} );
	}
}