#include "TwsConnectionSettings.h"
#include <jde/Log.h>
#define var const auto

namespace Jde::Markets
{
	α operator<<( std::ostream& os, const TwsConnectionSettings& settings )noexcept->std::ostream&
	{
		os << settings.Host <<  ":[";
		bool start=true;
		for( var port : settings.Ports )
		{
			if( start ) start=false; else os << ",";
			os << port;
		}
		os << "]";
		return os;
	}
}

α from_json( const nlohmann::json& j, Jde::Markets::TwsConnectionSettings& settings )noexcept->void
{
	if( j.find("options")!=j.end() )
		j.at("options").get_to( settings.Options );
	if( j.find("ports")!=j.end() )
	{
		settings.Ports.clear();
		//DBG( "{}", j.dump() );
		j.at("ports").get_to( settings.Ports );
	}
	if( j.find("host")!=j.end() )
		j.at("host").get_to( settings.Host );

	if( j.find("maxHistoricalDataRequest")!=j.end() )
		j.at("maxHistoricalDataRequest").get_to( settings.MaxHistoricalDataRequest );
}