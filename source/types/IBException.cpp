#include "IBException.h"


namespace Jde::Markets
{
	IBException::IBException( string_view message, int errorCode, long reqId, string_view function, string_view file, uint line )noexcept:
		Exception( ELogLevel::Debug, message, function, file, line ),
		ErrorCode( errorCode ),
		RequestId( reqId )
	{}

	void IBException::Log( string_view pszAdditionalInformation, ELogLevel level )const noexcept
	{
		string additionalInformation = pszAdditionalInformation.size() ? format("[{}]", pszAdditionalInformation)  : "";

		GetDefaultLogger()->log( (spdlog::level::level_enum)level, "{{{}}}[{}] {}{} - ({}){}({})", RequestId, ErrorCode, additionalInformation, what(), _functionName, _fileName, _line );
	}
}