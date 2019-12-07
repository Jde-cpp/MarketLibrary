#include "IBException.h"


namespace Jde::Markets
{
	void IBException::Log( string_view pszAdditionalInformation, ELogLevel level )const noexcept
	{
		string additionalInformation = pszAdditionalInformation.size() ? fmt::format("[{}]", pszAdditionalInformation)  : "";
		
		GetDefaultLogger()->log( (spdlog::level::level_enum)level, "{{{}}}[{}] {}{} - ({}){}({})", RequestId, ErrorCode, additionalInformation, what(), _functionName, _fileName, _line );
	}
}