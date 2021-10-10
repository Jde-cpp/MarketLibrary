#include "IBException.h"
#include <jde/Log.h>
#include "../../../Framework/source/log/server/ServerSink.h"

#define var const auto
namespace Jde::Markets
{
	IBException::IBException( sv message, int errorCode, long reqId, sv function, sv file, long line )noexcept:
		Exception( ELogLevel::Debug, message, function, file, static_cast<uint>(line) ),
		ErrorCode( errorCode ),
		RequestId( reqId )
	{}

	α IBException::Log( sv additionalInformation, optional<ELogLevel> pLevel )const noexcept->void
	{
		std::ostringstream os;
		if( additionalInformation.size() )
			os << "[" << additionalInformation << "] ";
		var message = format( "({})[{}] - {}{}", RequestId, ErrorCode, additionalInformation, what() );
		var level = pLevel.value_or( ELogLevel::Trace );
		Logging::Default().log( spdlog::source_loc{FileName(_fileName).c_str(),_line,_functionName.data()}, (spdlog::level::level_enum)level, message );
		if( level>=Logging::ServerLevel() )
			LogServer( Logging::Messages::Message{Logging::Message2{level, message, _fileName, _functionName, _line}} );
	}
}