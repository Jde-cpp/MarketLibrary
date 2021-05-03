#include "IBException.h"


namespace Jde::Markets
{
	IBException::IBException( sv message, int errorCode, long reqId, sv function, sv file, long line )noexcept:
		Exception( ELogLevel::Debug, message, function, file, static_cast<uint>(line) ),
		ErrorCode( errorCode ),
		RequestId( reqId )
	{}

	void IBException::Log( sv pszAdditionalInformation, ELogLevel level )const noexcept
	{
		string additionalInformation = pszAdditionalInformation.size() ? format("[{}]", pszAdditionalInformation)  : "";

		GetDefaultLogger()->log( (spdlog::level::level_enum)level, "{{{}}}[{}] {}{} - ({}){}({})", RequestId, ErrorCode, additionalInformation, what(), _functionName, _fileName, _line );
	}
}