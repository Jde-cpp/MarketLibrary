#include "StatAwait.h"
#include <bar.h>
#include <jde/markets/types/Contract.h>
#include <jde/markets/types/proto/ib.pb.h>
#include "../../../Framework/source/Cache.h"
#include "../../../Framework/source/db/Database.h"
#include "../client/TwsClientCo.h"
#include "../types/Bar.h"

#define var const auto
namespace Jde::Markets::HistoricalDataCache
{
	using namespace Chrono;
	static const LogTag& _logLevel{ Logging::TagLevel("mrk.hist") };

	StatAwait::StatAwait( ContractPtr_ pContract, double days, Day start, Day end )ι:IAwait{"ReqStats"}, ContractPtr{pContract}, Days{days}, Start{start}, End{end}, Count{ DayCount(TradingDay{Start, ContractPtr->Exchange}, End) }
	{
		LOG( "StatAwait" );
	}
	StatAwait::~StatAwait(){ LOG("~StatAwait"); }

	α StatAwait::await_ready()ι->bool
	{
		double fullDaysDouble;
		var minutesInDay = DayLengthMinutes( ContractPtr->Exchange );
		double partialDays = std::modf( Days, &fullDaysDouble );
		FullDays = Round<Day>( fullDaysDouble );
		Minutes = Round<uint16>( partialDays*minutesInDay );
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
			THROW_IF( End>ToDays(Clock::now()), "end should be < now" );
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
	α StatAwait::await_suspend( HCoroutine h )ι->void
	{
		base::await_suspend( h );
		_h = move(h);
		auto f = [this]()->Task
		{
			try
			{
				var pAllBars = ( co_await Tws::HistoricalData( ContractPtr, End, Count, Minutes==0 ? EBarSize::Day : EBarSize::Minute, Proto::Requests::Display::Trades, true) ).SP<vector<::Bar>>();
				THROW_IF( pAllBars->size()==0, "No history" );
				map<Day,vector<::Bar>> bars;
				for_each( pAllBars->begin(), pAllBars->end(), [&bars](var bar)
				{
					bars.try_emplace( ToDays(ConvertIBDate(bar.time)) ).first->second.push_back( bar );
				} );

				vector<double> returns;
				auto pEnd = bars.rbegin();
				var diff = FullDays<6 ? TradingDay{pEnd->first, ContractPtr->Exchange}-(FullDays==0 ? 0 : FullDays-1) : pEnd->first-( FullDays-1 );
				auto pForward = bars.lower_bound( diff );
				decltype(pEnd) pStart{ pForward==bars.end() ? pForward : std::next(pForward) };
				for( ; pStart!=bars.rend() && pEnd!=bars.rend(); ++pStart, ++pEnd )
				{
					var index = Minutes==0 ? 0 : Minutes<=pStart->second.size() ? pStart->second.size()-Minutes : std::numeric_limits<uint>::max();
					var pStartBar = pStart->second.size() && index!=std::numeric_limits<uint>::max() ? &pStart->second[index] : nullptr;
					var pEndBar = pEnd->second.size() ? &pEnd->second.back() : nullptr;
					if( pStartBar && pEndBar )
					{
						returns.push_back( 1+(pEndBar->close-pStartBar->open)/pStartBar->open );
					}
				}
				auto pValue = ms<StatCount>( Math::Statistics(returns), returns.size() );
				if( !CacheId.empty() )
					Cache::Set<StatCount>( CacheId, pValue );
				_pPromise->get_return_object().SetResult(  pValue );
			}
			catch( IException& e )
			{
				_pPromise->get_return_object().SetResult( move(e) );
			}
			_h.resume();
		};
		f();
	}
	α StatAwait::await_resume()ι->AwaitResult
	{
		return _pPromise ? move(_pPromise->get_return_object().Result()) : _result.index() ? AwaitResult{ std::get<sp<IException>>(_result) } : AwaitResult{ std::get<sp<StatCount>>(_result) };
	}

	α AthAwait::await_suspend( HCoroutine h_ )ι->void
	{
		base::await_suspend( h_ );
		//_h = move( h );
		auto f = [this, h2_=move(h_)]()->Task
		{
			var contractId{ _contractId }; var dayCount{ _dayCount }; auto pPromise{ _pPromise }; auto h{ move(h2_) }; var headTimestamp{ _headTimestamp };//gets lost in coroutines HCoroutine h{ h_ };
			try
			{
				auto c = ms<Contract>( *((co_await Tws::ContractDetail(contractId)).SP<::ContractDetails>()) );
				var prev{ PreviousTradingDay(*c->TradingHoursPtr) };
				var end = dayCount ? prev : NextTradingDay( PreviousTradingDay(*c->TradingHoursPtr)-365 );
				var day100{ dayCount && dayCount>100 ? end-100 : 0 };
				var start = dayCount ? prev-dayCount : std::min( ToDays(headTimestamp), end-3 );
				DBG( "({})start={}, headTimestamp={} {}-{}", c->Symbol, start, DateDisplay(headTimestamp), DateDisplay(start), DateDisplay(end) );
				var pBars = ( co_await Tws::HistoricalData(c, prev, end-start, EBarSize::Day, Proto::Requests::Display::Trades, true) ).SP<vector<::Bar>>(); THROW_IF( pBars->size()==0, "No history" );
				vector<double> last100; last100.reserve( 100 );
				auto y = mu<Result>();
				for( var& bar : *pBars )
				{
					var day = ToDays( ConvertIBDate(bar.time) );
					if( bar.high>y->High ){ y->High = bar.high; y->HighDay = day; }
					if( bar.low<y->Low ){ y->Low = bar.low; y->LowDay = day; }
					if( day100 && day>day100 ) last100.push_back( bar.close );
				}

				y->Average =  last100.size()==0 ? 0 : std::reduce( last100.begin(), last100.end() )/(double)last100.size();
				( co_await *DB::ExecuteProcCo("mrk_statistic_update( ?, ?, ?, ?, ?, ?, ?, ? )", {(uint32_t)contractId, dayCount, prev, y->Low, y->LowDay, y->High, y->HighDay, y->Average}) ).CheckError();

				pPromise->get_return_object().SetResult( move(y) );
			}
			catch( IException& e )
			{
				pPromise->get_return_object().SetResult( move(e) );
			}
			h.resume();
		};
		f();
	}
	α AthAwait::await_resume()ι->AwaitResult
	{
		return move( _pPromise->get_return_object().Result() );
	}
}