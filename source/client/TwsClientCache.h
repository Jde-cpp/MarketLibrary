#pragma once
#include <jde/markets/Exports.h>
#include "TwsClientCo.h"



namespace Jde::Markets
{

	struct WrapperCache;
	struct TwsConnectionSettings;
	struct Contract;
	struct ΓM TwsClientCache : public Tws
	{
		TwsClientCache( const TwsConnectionSettings& settings, sp<WrapperCache> wrapper, sp<EReaderSignal>& pReaderSignal, uint clientId )noexcept(false);
		α Wrapper()noexcept->sp<WrapperCache>;
		Ω ToContract( sv symbol, Day dayIndex, Proto::SecurityRight isCall, double strike=0 )noexcept->::Contract;
		α ReqContractDetails( TickerId reqId, const ::Contract& contract )noexcept->void;
	};
}