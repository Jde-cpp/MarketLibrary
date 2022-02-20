#include "Currencies.h"
#pragma warning( disable : 4244 )
#include <jde/markets/types/proto/ib.pb.h>
#pragma warning( default : 4244 )
#include <jde/Log.h>

#define var const auto
namespace Jde
{
	sv Markets::ToString( Proto::Currencies x )noexcept
	{
		var found = x>=0 && x<CurrencyStrings.size();
		if( !found )
			DBG( "could not find currency value='{}'"sv, (uint)x );
		return found ? CurrencyStrings[x] : "";
	}
	Markets::Proto::Currencies Markets::ToCurrency( sv x )noexcept
	{
		auto found = x.length()>0;
		std::array<sv,53>::const_iterator p;
		if( found )
		{
			p = std::find( CurrencyStrings.begin(), CurrencyStrings.end(), x );
			found = p!=CurrencyStrings.end();
			if( !found )
				WARN( "could not find currency '{}'"sv, x );
		}
		return found ? static_cast<Proto::Currencies>( std::distance(CurrencyStrings.begin(), p) ) : Proto::Currencies::NoCurrency;
	}
}