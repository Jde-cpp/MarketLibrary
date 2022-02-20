#include "./OptionData.h"
#include <jde/io/File.h>
#include <jde/markets/types/Contract.h>
#include <jde/markets/types/proto/results.pb.h>

#include "../../../Framework/source/db/Database.h"
#include "../../../Framework/source/Cache.h"
#include "../../../XZ/source/XZ.h"
#include "../client/TwsClientCo.h"
#include "../data/BarData.h"
#include "../types/Exchanges.h"
#include "../types/IBException.h"

#define var const auto

namespace Jde::Markets
{
	Option::Option( const ContractDetails& ib ):
		Id{ ib.contract.conId },
		IsCall{ ib.contract.right=="C" },
		Strike{ ib.contract.strike },
		UnderlyingId{ ib.underConId },
		ContractPtr{ make_shared<Contract>(ib.contract) }
	{
		var& date = ib.contract.lastTradeDateOrContractMonth;
		if( date.size()>0 )
		{
			var year = static_cast<uint16>(stoul(date.substr(0,4)) );
			var month = static_cast<uint8>( stoul(date.substr(4,2)) );
			var day = static_cast<uint8>( stoul(date.substr(6,2)) );
			ExpirationDay = Chrono::ToDays( DateTime(year, month, day).GetTimePoint() );
		}
	}
	Option::Option( Day ExpirationDay, Amount strike, bool isCall, ContractPK underlyingId ):
		ExpirationDay{ ExpirationDay },
		IsCall{ isCall },
		Strike{ strike },
		UnderlyingId{ underlyingId }
	{}

	α Option::operator<( const Option& op2 )const noexcept->bool
	{
		bool less =
			  UnderlyingId!=op2.UnderlyingId ? UnderlyingId<op2.UnderlyingId
			: ExpirationDay!=op2.ExpirationDay ? ExpirationDay<op2.ExpirationDay
			: Strike!=op2.Strike ? Strike<op2.Strike
			: IsCall<op2.IsCall;
		return less;
	}

	α OptionData::SyncContracts( ContractPtr_ pContract, const vector<ContractDetails>& details )noexcept(false)->OptionSetPtr
	{
		auto pExisting = Load( pContract->Id );
		for( var& detail : details )
		{
			auto pNewOption = make_shared<const Option>( detail );
			if( pExisting->emplace(pNewOption).second )
				Insert( *pNewOption );
		}
		return pExisting;
	}

	α OptionData::Insert( const Option& value )noexcept(false)->void
	{
		var sql = "insert into sec_option_contracts( id, expiration_date, flags, strike, under_contract_id ) values( ?, ?, ?, ?, ? )";
		DB::DataSource().Execute( sql, {(uint)value.Id, (uint)value.ExpirationDay, (uint)value.IsCall ? 1 : 2, (double)value.Strike, (uint)value.UnderlyingId} );
	}

	α OptionData::Load( ContractPK underlyingId, Day earliestDay )noexcept(false)->OptionSetPtr
	{
		var sql = "select id, expiration_date, flags, strike from sec_option_contracts where under_contract_id=? and expiration_date>=?";
		auto pResults = make_shared<flat_set<OptionPtr,SPCompare<const Option>>>();
		auto result = [&pResults, underlyingId]( const DB::IRow& row )
		{
			OptionPtr pOption = make_shared<const Option>();
			auto& option = const_cast<Option&>( *pOption );
			option.UnderlyingId = underlyingId;
			uint8 flags;
			row >> option.Id >> option.ExpirationDay >> flags >> option.Strike;
			option.IsCall = (flags&1)==1;
			option.ContractPtr = make_shared<Contract>( option.Id );

			pResults->emplace( pOption );
		};
		DB::DataSource().Select( sql, result, {(uint)underlyingId, (uint)earliestDay} );
		return pResults;
	}

	α Load( const Contract& contract, uint16 year, uint8 month, uint8 day )noexcept(false)->sp<Proto::UnderlyingOIValues>
	{
		var file = OptionData::OptionFile( contract, year, month, day );
		sp<Proto::UnderlyingOIValues> pUnderlying;
		if( fs::exists(file) )
		{
			var cacheId = file.string();
			if( Cache::Has(cacheId) )
				pUnderlying = dynamic_pointer_cast<Proto::UnderlyingOIValues>( Cache::Get<google::protobuf::Message>(cacheId) );
			else
			{
				var pBytes = Jde::IO::Zip::XZ::Read( file ); THROW_IFX( !pBytes, IOException(file, "has 0 bytes.") );
				google::protobuf::io::CodedInputStream input( (const uint8*)pBytes->data(), (int)pBytes->size() );
				pUnderlying = make_shared<Proto::UnderlyingOIValues>();
				pUnderlying->ParseFromCodedStream( &input );
				Cache::Set<google::protobuf::Message>( cacheId, pUnderlying );
			}
		}
		return pUnderlying;
	}

	α OptionData::LoadDiff( const Contract& underlying, const vector<sp<::ContractDetails>>& ibOptions, Proto::Results::OptionValues& results )noexcept(false)->Day
	{
		flat_map<ContractPK, tuple<Contract,const Proto::OptionOIDay*>> options;
		for( var p : ibOptions )
			options.emplace( p->contract.conId, make_tuple(Contract{*p},nullptr) );

	//load tws details for day.
		var currentTradingDay = IsOpen() ? CurrentTradingDay() : PreviousTradingDay(); // ;
		var to = PreviousTradingDay( currentTradingDay );
		var from = PreviousTradingDay( to );
		const DateTime toDate{ Chrono::FromDays(to) };
		const DateTime fromDate{ Chrono::FromDays(from) };
		var pFromValues = Load( underlying, fromDate.Year(), fromDate.Month(), fromDate.Day() );//sp<Proto::UnderlyingOIValues>
		var fromSize = pFromValues ? pFromValues->contracts_size() : 0;
		for( auto i=0; i<fromSize; ++i )
		{
			var pMutableOptionDays = pFromValues->mutable_contracts( i );
			var pOptionIdValues = options.find( pMutableOptionDays->id() );
			if( pOptionIdValues!=options.end() )//maybe only have calls
				pOptionIdValues->second = make_tuple( get<0>(pOptionIdValues->second), pMutableOptionDays );
		}

		var pToValues = Load( underlying, toDate.Year(), toDate.Month(), toDate.Day() );
		if( !pToValues )
			return 0;
		var toSize = pToValues->contracts_size();
		flat_map<uint16,Proto::Results::OptionDay*> values;
		flat_set<ContractPK> matchedIds;
		auto setValue = [&]( const Contract& option, const Proto::OptionOIDay& toDay, const Proto::OptionOIDay* pFromDay )
		{
			var daysSinceEpoch = option.Expiration;
			auto pDayValue = values.find( daysSinceEpoch );
			if( pDayValue==values.end() )
			{
				pDayValue = values.emplace( daysSinceEpoch, results.add_option_days() ).first;
				pDayValue->second->set_expiration_days( daysSinceEpoch );
				pDayValue->second->set_is_call( option.Right==SecurityRight::Call );
			}
			auto pDay = pDayValue->second;
			auto pValue = pDay->add_values();
			pValue->set_id( toDay.id() );
			pValue->set_strike( (float)option.Strike );
			pValue->set_ask( toDay.ask() ); pValue->set_bid( toDay.bid() ); pValue->set_last( toDay.last() );
			pValue->set_open_interest( toDay.open_interest() );
			const int toOI = toDay.open_interest();
			pValue->set_oi_change( toOI - (pFromDay ? pFromDay->open_interest() : 0) );
			pValue->set_previous_price( pFromDay ? pFromDay->last()<pFromDay->bid() || pFromDay->last()>pFromDay->ask() ? (pFromDay->bid()+pFromDay->ask())/2 : pFromDay->last() : 0 );
			pValue->set_volume( toDay.volume() );
		};
		var today = DateTime::Today();
		for( auto i=0; i<toSize; ++i )
		{
			var& toDay = pToValues->contracts( i );
			var pOptionIdValues = options.find( toDay.id() );
			if( pOptionIdValues==options.end() )
				continue;
			matchedIds.emplace( toDay.id() );
			var& option = get<0>( pOptionIdValues->second );
			if( Chrono::FromDays(option.Expiration)+23h<today )
				continue;

			var pFromDay = get<1>( pOptionIdValues->second );
			setValue( option, toDay, pFromDay );
		}
		return to;
	}

	α OptionData::LoadDiff( const Contract& contract, bool isCall, Day from, Day to, bool includeExpired, bool noFromDayOk )noexcept(false)->Proto::Results::OptionValues*
	{
		var pOptions = Load( contract.Id );
		flat_map<ContractPK, tuple<OptionPtr,const Proto::OptionOIDay*>> options;
		for( var& pOption : *pOptions )
			options.emplace( pOption->Id, make_tuple(pOption,nullptr) );

		const DateTime fromDate( Chrono::FromDays(from) );
		var pFromValues = Load( contract, fromDate.Year(), fromDate.Month(), fromDate.Day() );//sp<Proto::UnderlyingOIValues>
		if( !pFromValues && !noFromDayOk )
			return nullptr;
		if( pFromValues )
		{
			for( auto i=0; i<pFromValues->contracts_size(); ++i )
			{
				var pMutableOptionDays = pFromValues->mutable_contracts( i );
				var pOptionIdValues = options.find( pMutableOptionDays->id() );
				if( pOptionIdValues==options.end() )
					WARN( "Could not find option id='{}'."sv, pMutableOptionDays->id() );
				else
					pOptionIdValues->second = make_tuple( get<0>(pOptionIdValues->second),pMutableOptionDays );
			}
		}
		const DateTime toDate( Chrono::FromDays(to) );
		var pToValues = Load( contract, toDate.Year(), toDate.Month(), toDate.Day() );
		if( !pToValues )
			return nullptr;
		var toSize = pToValues->contracts_size();
		auto pResults = new Proto::Results::OptionValues();
		var today = DateTime::Today();
		flat_map<uint16,Proto::Results::OptionDay*> values;
		flat_set<ContractPK> matchedIds;
		auto setValue = [&]( OptionPtr pOption, const Proto::OptionOIDay& toDay, const Proto::OptionOIDay* pFromDay, bool expired )
		{
			uint16 daysSinceEpoch = pOption->ExpirationDay;
			auto pDayValue = values.find( daysSinceEpoch );
			if( pDayValue==values.end() )
			{
				pDayValue = values.emplace( daysSinceEpoch, pResults->add_option_days() ).first;
				pDayValue->second->set_expiration_days( daysSinceEpoch );
				pDayValue->second->set_is_call( isCall );
			}
			auto pDay = pDayValue->second;
			auto pValue = pDay->add_values();
			pValue->set_id( toDay.id() );
			pValue->set_strike( (float)pOption->Strike );
			pValue->set_ask( toDay.ask() ); pValue->set_bid( toDay.bid() ); pValue->set_last( toDay.last() );
			pValue->set_open_interest( expired ? 0 : toDay.open_interest() );
			const int toOI = toDay.open_interest();
			pValue->set_oi_change( (expired ? -toOI : toOI) - (pFromDay ? pFromDay->open_interest() : 0) );
			pValue->set_previous_price( pFromDay ? pFromDay->last()<pFromDay->bid() || pFromDay->last()>pFromDay->ask() ? (pFromDay->bid()+pFromDay->ask())/2 : pFromDay->last() : 0 );
		};
		for( auto i=0; i<toSize; ++i )
		{
			var& toDay = pToValues->contracts( i );
			var pOptionIdValues = options.find( toDay.id() );
			if( pOptionIdValues==options.end() )
				THROW( "Could not find option id='{}'.", toDay.id() );
			matchedIds.emplace( toDay.id() );
			var pOption = get<0>( pOptionIdValues->second );
			if( pOption->IsCall!=isCall )
				continue;
			if( !includeExpired && Chrono::FromDays(pOption->ExpirationDay)+23h<today )
				continue;

			var pFromDay = get<1>( pOptionIdValues->second );
			setValue( pOption, toDay, pFromDay, false );
		}
		if( includeExpired )
		{
			for( var& [optionId, pOptionDay] : options )
 			{
				var pFromDay = get<1>( pOptionDay );
				var pOption = get<0>( pOptionDay );
				if( pFromDay && pOption->IsCall==isCall && matchedIds.find(optionId)==matchedIds.end() )
					setValue( pOption, *pFromDay, nullptr, true );
			}
		}
		return pResults;
	}

	α Load2( sp<::ContractDetails> pUnderlying, DayIndex expiration, SecurityRight right, double startStrike, double endStrike, Proto::Results::OptionValues* pResults )->AsyncAwait
	{
		return AsyncAwait( [=]( HCoroutine h )->Task
		{
			var& symbol = pUnderlying->contract.symbol;
			var ibContract = ToIb( symbol, expiration, right, startStrike && startStrike==endStrike ? startStrike : 0 );
			auto pContracts = ( co_await Tws::ContractDetails(ibContract) ).UP<vector<sp<::ContractDetails>>>(); THROW_IF( pContracts->size()<1, "'{}' - '{}' {} has {} contracts", symbol, DateTime{Chrono::FromDays(expiration)}.DateDisplay(), ToString(right), pContracts->size() );
			if( !ibContract.strike && (startStrike!=0 || endStrike!=0) )//remove contracts outside bounds?
			{
				auto pContracts2 = mu<vector<sp<ContractDetails>>>();
				for( var& details : *pContracts )
				{
					if( (startStrike==0 || startStrike<=details->contract.strike) && (endStrike==0 || endStrike>=details->contract.strike) )
						pContracts2->push_back( details );
				}
				pContracts = move( pContracts2 );
			}
			Contract c{*pUnderlying};
			var valueDay = OptionData::LoadDiff( c, *pContracts, *pResults );
			if( !valueDay )
			{
				flat_map<double,ContractPK> sorted;
				for( var p : *pContracts )
					sorted.emplace( p->contract.strike, p->contract.conId );

				auto pExpiration = pResults->add_option_days();
				pExpiration->set_is_call( right==SecurityRight::Call );
				pExpiration->set_expiration_days( expiration );
				for( var& [strike,contractId] : sorted )
				{
					auto pValue = pExpiration->add_values();
					pValue->set_id( contractId );
					pValue->set_strike( static_cast<float>(strike) );
				}
			}
			if( valueDay )
				h.promise().get_return_object().SetResult( mu<Day>(valueDay) );
			h.resume();
		});
	}
	α OptionData::Load( sp<::ContractDetails> pDetail, Day startExp, Day endExp, SecurityRight right, double startStrike, double endStrike, Proto::Results::OptionValues* pResults )->AsyncAwait
	{
		return AsyncAwait( [=]( HCoroutine h )->Task
		{
			if( startExp==endExp )
			{
				var pDay = ( co_await Load2(pDetail, startExp, right, startStrike, endStrike, pResults) ).UP<Day>();
				pResults->set_day( pDay ? *pDay : 0 );
			}
			else
			{
				var pOptionParams = ( co_await Tws::SecDefOptParams(pDetail->contract.conId,true) ).SP<Proto::Results::ExchangeContracts>();
				var end = endExp ? endExp : std::numeric_limits<DayIndex>::max();
				for( var expiration : pOptionParams->expirations() )
				{
					if( expiration<startExp || expiration>end )
						continue;
					try
					{
						var pDay = ( co_await Load2(pDetail, expiration, right, startStrike, endStrike, pResults) ).UP<Day>();
						pResults->set_day( *pDay );//sets day multiple times...
					}
					catch( IBException& e )
					{
						if( e.Code!=200 )//No security definition has been found for the request
							throw move(e);
					}
				}
			}
			h.resume();
		});
	}

	α OptionDir( const Contract& contract )->fs::path
	{
		const string exchangeString{ ToString(contract.PrimaryExchange) };
		return BarData::Path()/Str::ToLower(exchangeString)/Str::ToUpper(contract.Symbol)/"options";
	}

	α OptionData::OptionFile( const Contract& contract, uint16 year, uint8 month, uint8 day )noexcept->fs::path
	{
		return OptionDir(contract)/(IO::FileUtilities::DateFileName(year,month,day)+".dat.xz");
	}

	α OptionData::LoadFiles( const Contract& contract )noexcept->flat_map<Day,sp<Proto::UnderlyingOIValues>>
	{
		var pFiles = IO::FileUtilities::GetDirectory( OptionDir(contract) );
		flat_map<Day,sp<Proto::UnderlyingOIValues>> values;
		for( auto pFile = pFiles->rbegin(); pFile!=pFiles->rend(); ++pFile )
		{
			var file = pFile->path().filename().stem().string();
			if( !pFile->is_regular_file() || file[4]!='-' || file[7]!='-' )
				continue;
			var year = stoi( file.substr(0, 4) );
			var month = stoi( file.substr(5, 2) );
			var day = stoi( file.substr(8, 2) );
			var daysSinceEpoch = Chrono::ToDays( DateTime(year,(uint8)month,(uint8)day).GetTimePoint() );
			values.emplace( daysSinceEpoch, Load(contract, year, month, day) );
		}
		return values;
	}
}