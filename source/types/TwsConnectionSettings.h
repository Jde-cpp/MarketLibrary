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
		bool CommunicationSink{false};
		uint8 MaxHistoricalDataRequest{ std::numeric_limits<uint8>::max() };
	};
	ΓM α operator<<( std::ostream& os, const TwsConnectionSettings& settings )noexcept->std::ostream&;
}
ΓM α from_json( const nlohmann::json& j, Jde::Markets::TwsConnectionSettings& server )noexcept->void;

Ξ to_json( nlohmann::json& j, const Jde::Markets::TwsConnectionSettings& settings )noexcept
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