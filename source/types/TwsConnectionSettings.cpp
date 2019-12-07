#include "TwsConnectionSettings.h"
#define var const auto
namespace Jde::Markets
{
	std::ostream& operator<<( std::ostream& os, const TwsConnectionSettings& settings )
	{
		os << settings.Host <<  ":[";
		bool start=true;
		for( var port : settings.Ports )
		{
			if( start ) start=false; else os << ",";
			os << port; //<< "?ClientId=" << settings.ClientId << "&options=" << settings.Options;
		}
		os << "]";
		return os;
	}
}

void from_json( const nlohmann::json& j, Jde::Markets::TwsConnectionSettings& settings )
{
	//if( j.find("clientId")!=j.end() )
	//	j.at("clientId").get_to( settings.ClientId );
//	DBG("{}", "HI");
	//for( nlohmann::json::const_iterator it = j.begin(); it != j.end(); ++it )
	//	DBG( "\"{}\":\"{}\"", it.key(), it.value() );
	
 	if( j.find("options")!=j.end() )
		j.at("options").get_to( settings.Options );
	if( j.find("ports")!=j.end() )
	{
		settings.Ports.clear();
		j.at("ports").get_to( settings.Ports );
	}
	if( j.find("host")!=j.end() )
		j.at("host").get_to( settings.Host );
}

// void to_json( nlohmann::json& j, const Jde::Markets::TwsConnectionSettings& settings )
// {
// 	j = nlohmann::json{};
// 	static Jde::Markets::TwsConnectionSettings defaultValues;
// 	if( settings.ClientId!=defaultValues.ClientId )
// 		j["clientId"] = settings.ClientId;
// 	if( settings.Options!=defaultValues.Options )
// 		j["options"] = settings.Options;
// 	if( settings.Port!=defaultValues.Port )
// 		j["port"] = settings.Port;
// 	if( settings.Host!=defaultValues.Host )
// 		j["host"] = settings.Host;
// }

// void from_json( const nlohmann::json& j, Jde::Markets::TwsConnectionSettings& settings )
// {
// 	if( j.find("clientId")!=j.end() )
// 		j.at("clientId").get_to( settings.ClientId );
// 	if( j.find("options")!=j.end() )
// 		j.at("options").get_to( settings.Options );
// 	if( j.find("port")!=j.end() )
// 		j.at("port").get_to( settings.Port );
// 	if( j.find("host")!=j.end() )
// 		j.at("host").get_to( settings.Host );
// }
