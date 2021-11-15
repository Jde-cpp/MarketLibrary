#pragma once
#include <jde/markets/Exports.h>
#pragma warning( disable : 4715 )
#include <nlohmann/json.hpp>
#pragma warning( default : 4715 )
namespace Jde::Markets
{
	using nlohmann::json;
	struct TwsConnectionSettings
	{
		string Host{"localhost"};
		string Options;
		vector<uint16> Ports={7497,4002};
		const bool CommunicationSink{false};
		uint MaxHistoricalDataRequest{ std::numeric_limits<uint>::max() };
	};
	ΓM std::ostream& operator<<( std::ostream& os, const TwsConnectionSettings& settings );
}
ΓM void from_json( const nlohmann::json& j, Jde::Markets::TwsConnectionSettings& server );

inline void to_json( nlohmann::json& j, const Jde::Markets::TwsConnectionSettings& settings )
{
	j = nlohmann::json{};
	static Jde::Markets::TwsConnectionSettings defaultValues;
	if( settings.Options!=defaultValues.Options )
		j["options"] = settings.Options;
	if( settings.Ports!=defaultValues.Ports )
		j["ports"] = settings.Ports;
	if( settings.Host!=defaultValues.Host )
		j["host"] = settings.Host;
	if( settings.MaxHistoricalDataRequest!=defaultValues.MaxHistoricalDataRequest )
		j["maxHistoricalDataRequest"] = settings.MaxHistoricalDataRequest;
}


