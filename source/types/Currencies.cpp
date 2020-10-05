#include "Currencies.h"
#include "proto/ib.pb.h"

#define var const auto
namespace Jde
{
	string_view Markets::ToString( Proto::Currencies x )noexcept
	{
		var found = x>=0 && x<CurrencyStrings.size();
		if( !found )
			DBG( "could not find currency value='{}'"sv, (uint)x );
		return found ? CurrencyStrings[x] : "";
	}
	Markets::Proto::Currencies Markets::ToCurrency( string_view x )noexcept
	{
		auto found = x.length()>0;
		std::array<std::string_view,53>::const_iterator p;
		if( found )
		{
			p = std::find( CurrencyStrings.begin(), CurrencyStrings.end(), x );
			found = p!=CurrencyStrings.end();
			if( !found )
				DBG( "could not find currency '{}'"sv, x );
		}
		return found ? static_cast<Proto::Currencies>( std::distance(CurrencyStrings.begin(), p) ) : Proto::Currencies::NoCurrency;
	}
}