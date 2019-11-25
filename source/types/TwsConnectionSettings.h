#pragma once
#include "../Exports.h"

namespace Jde::Markets
{
	using nlohmann::json;
	struct TwsConnectionSettings
	{
		//int ClientId{1};
		string Host{"localhost"};
		string Options;
		//uint16 Port{7497};//{7497,7496,4001,4002}; //tws paper, tws, gateway, gateway paper
		vector<uint16> Ports={7497,4002};
		const bool CommunicationSink{false};
	};
	JDE_MARKETS_EXPORT std::ostream& operator<<( std::ostream& os, const TwsConnectionSettings& settings );
}
//JDE_MARKETS_EXPORT void to_json( nlohmann::json& j, const Jde::Markets::TwsConnectionSettings& server );
JDE_MARKETS_EXPORT void from_json( const nlohmann::json& j, Jde::Markets::TwsConnectionSettings& server );

inline void to_json( nlohmann::json& j, const Jde::Markets::TwsConnectionSettings& settings )
{
	j = nlohmann::json{};
	static Jde::Markets::TwsConnectionSettings defaultValues;
	//if( settings.ClientId!=defaultValues.ClientId )
	//	j["clientId"] = settings.ClientId;
	if( settings.Options!=defaultValues.Options )
		j["options"] = settings.Options;
	if( settings.Ports!=defaultValues.Ports )
		j["ports"] = settings.Ports;
	if( settings.Host!=defaultValues.Host )
		j["host"] = settings.Host;
}


