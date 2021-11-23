#include "StatAwait.h"
#include <bar.h>
#include <jde/markets/types/Contract.h>
#include "../../../Framework/source/Cache.h"
#include "../client/TwsClientCo.h"
#include "../types/Bar.h"

#define var const auto
namespace Jde::Markets::HistoricalDataCache
{
	static const LogTag& _logLevel{ Logging::TagLevel("mrk.hist") };

	StatAwait::StatAwait( ContractPtr_ pContract, double days, Day start, Day end )noexcept:IAwaitable{"ReqStats"}, ContractPtr{pContract}, Days{days}, Start{start}, End{end}, Count{ DayCount(TradingDay{Start, ContractPtr->Exchange}, End) }
	{
		LOG( "StatAwait" );
	}
	StatAwait::~StatAwait(){ LOG("~StatAwait"); }

	α StatAwait::await_ready()noexcept->bool
	{
		double fullDaysDouble;
		var minutesInDay = DayLengthMinutes( ContractPtr->Exchange );
		double partialDays = std::modf( Days, &fullDaysDouble );
		FullDays = Math::URound<Day>( fullDaysDouble );
		Minutes = Math::URound<uint16>( partialDays*minutesInDay );
		if( Minutes==minutesInDay )
		{
			Minutes=0;
			++FullDays;
		}
	#pragma region Tests
		try
		{
			THROW_IF( FullDays==0 && Minutes<1, "days '{}' should be at least 1 minute.", Days );
			THROW_IF( Days>3*365, "days {} should be less than 3 years {}", Days, 3*365 );
			THROW_IF( End>Chrono::ToDays(Clock::now()), "end should be < now" );
			THROW_IF( Count<Days, "days should be less than end-start." );
			THROW_IF( End<Start, "end '{}' should be greater than start '{}'.", End, Start );
	#pragma endregion
			CacheId = Minutes==0 ? "" : format( "ReqStdDev.{}.{}.{}.{}", ContractPtr->Symbol, FullDays, Start, End );

			if( var pValue = Cache::Get<StatCount>(CacheId); !CacheId.empty() && pValue )
				_result = pValue;
		}
		catch( IException& e )
		{
			_result = e.Clone();
		}
		return _result.index() || get<sp<StatCount>>( _result );
	}
	α StatAwait::await_suspend( HCoroutine h )noexcept->void
	{
		base::await_suspend( h );
		_h = move(h);
		auto f = [this]()->Task2//mutable
		{
			try
			{
				var pAllBars = ( co_await Tws::HistoricalData( ContractPtr, End, Count, Minutes==0 ? EBarSize::Day : EBarSize::Minute, Proto::Requests::Display::Trades, true) ).Get<vector<::Bar>>();
				THROW_IF( pAllBars->size()==0, "No history" );
				map<Day,vector<::Bar>> bars;
				for_each( pAllBars->begin(), pAllBars->end(), [&bars](var bar)
				{
					bars.try_emplace( Chrono::ToDays(ConvertIBDate(bar.time)) ).first->second.push_back( bar );
				} );

				vector<double> returns;
				auto pEnd = bars.rbegin();
				//typedef decltype(pEnd) RIterator;
				var diff = FullDays<6 ? TradingDay{pEnd->first, ContractPtr->Exchange}-(FullDays==0 ? 0 : FullDays-1) : pEnd->first-( FullDays-1 );
				auto pForward = bars.lower_bound( diff );
				decltype(pEnd) pStart{ pForward==bars.end() ? pForward : std::next(pForward) };
				for( ; pStart!=bars.rend() && pEnd!=bars.rend(); ++pStart, ++pEnd )
				{
					var index = Minutes==0 ? 0/*pStart->second.size()-1*/ : Minutes<=pStart->second.size() ? pStart->second.size()-Minutes : std::numeric_limits<uint>::max();
					var pStartBar = pStart->second.size() && index!=std::numeric_limits<uint>::max() ? &pStart->second[index] : nullptr;
					var pEndBar = pEnd->second.size() ? &pEnd->second.back() : nullptr;
					if( pStartBar && pEndBar )
					{
						returns.push_back( 1+(pEndBar->close-pStartBar->open)/pStartBar->open );
					}
				}
				auto pValue = make_shared<StatCount>( Math::Statistics(returns), returns.size() );
				if( !CacheId.empty() )
					Cache::Set<StatCount>( CacheId, pValue );
				_pPromise->get_return_object().SetResult(  pValue );
			}
			catch( IException& e )
			{
				_pPromise->get_return_object().SetResult( e.Clone() );
			}
			_h.resume();
		};
		f();
	}
	α StatAwait::await_resume()noexcept->TaskResult
	{
		return _pPromise ? _pPromise->get_return_object().GetResult() : _result.index() ? TaskResult{ std::get<sp<IException>>(_result) } : TaskResult{ std::get<sp<StatCount>>(_result) };
	}
}