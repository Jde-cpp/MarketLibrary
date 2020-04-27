#include "./OptionData.h"
#include "../types/Contract.h"
#include "../../../XZ/source/XZ.h"
#include "../types/proto/results.pb.h"
#include "../../../Framework/source/io/File.h"
#include "../../../Framework/source/db/Database.h"
#define var const auto

namespace Jde::Markets
{
	Option::Option( const ibapi::ContractDetails& ib ):
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
			ExpirationDate = DateTime( year, month, day ).GetTimePoint();
		}
	}
	Option::Option( TimePoint expirationDate, Amount strike, bool isCall ):
		ExpirationDate{ expirationDate },
		IsCall{ isCall },
		Strike{ strike }
	{}

	bool Option::operator<( const Option& op2 )const noexcept
	{
		return Id!=op2.Id ? Id<op2.Id
			: UnderlyingId!=op2.UnderlyingId ? UnderlyingId<op2.UnderlyingId
			: ExpirationDate!=op2.ExpirationDate ? ExpirationDate<op2.ExpirationDate
			: Strike!=op2.Strike ? Strike<op2.Strike
			: IsCall<op2.IsCall;
	}

	OptionSetPtr OptionData::SyncContracts( ContractPtr_ pContract, const list<ibapi::ContractDetails>& details )noexcept(false)
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

	void OptionData::Insert( const Option& value )noexcept(false)
	{
		var sql = "insert into sec_option_contracts( id, expiration_date, flags, strike, under_contract_id ) values( ?, ?, ?, ?, ? )";
		var days = std::chrono::duration_cast<std::chrono::hours>( value.ExpirationDate-DateTime(1970,1,1).GetTimePoint() ).count()/24;
		DB::DataSource()->Execute( sql, {value.Id, (uint)days, (uint)value.IsCall ? 1 : 2, (double)value.Strike, value.UnderlyingId} );
	}

	OptionSetPtr OptionData::Load( ContractPK underlyingId )noexcept(false)
	{
		var sql = "select id, expiration_date, flags, strike from sec_option_contracts where under_contract_id=?";
		auto pResults = make_shared<set<OptionPtr,SPCompare<const Option>>>();
		auto result = [&pResults, underlyingId]( const DB::IRow& row )
		{
			OptionPtr pOption = make_shared<const Option>();
			auto& option = const_cast<Option&>( *pOption );
			option.UnderlyingId = underlyingId;
			uint16 dayCount; uint8 flags;
			row >> option.Id >> dayCount >> flags >> option.Strike;
			option.ExpirationDate = DateTime(1970,1,1).GetTimePoint()+std::chrono::hours(24*dayCount);
			option.IsCall = (flags&1)==1;
			option.ContractPtr = make_shared<Contract>( option.Id );

			pResults->emplace( pOption );
		};
		DB::DataSource()->Select( sql, result, {underlyingId} );
		return pResults;
	}

	sp<Proto::UnderlyingOIValues> Load( const Contract& contract, uint16 year, uint8 month, uint8 day )noexcept(false)
	{
		var file = OptionData::OptionFile( contract, year, month, day );
		sp<Proto::UnderlyingOIValues> pUnderlying;
		if( fs::exists(file) )
		{
			var pBytes = Jde::IO::Zip::XZ::Read( file );
			google::protobuf::io::CodedInputStream input( (const uint8*)pBytes->data(), (int)pBytes->size() );
			pUnderlying = make_shared<Proto::UnderlyingOIValues>();
			pUnderlying->ParseFromCodedStream( &input );

			//uint size;
			//input.ReadVarint64( &size );
			//pCache->ParseFromCodedStream( &input );
			//Jde::IO::IStreamBuffer buffer{ pBytes->data(), pBytes->size() };
			//istream is{&buffer};
		}
		return pUnderlying;
	}

	Proto::Results::OptionValues* OptionData::LoadDiff( const Contract& contract, bool isCall, DayIndex from, DayIndex to, bool includeExpired )noexcept(false)
	{
		var pOptions = Load( contract.Id );
		map<ContractPK, tuple<OptionPtr,const Proto::OptionOIDay*>> options;
		for( var& pOption : *pOptions )
			options.emplace( pOption->Id, make_tuple(pOption,nullptr) );

		const DateTime fromDate( Chrono::FromDays(from) );
		//var pContract = Data::ContractData::Load( contract.Id );
		var pFromValues = Load( contract, fromDate.Year(), fromDate.Month(), fromDate.Day() );//sp<Proto::UnderlyingOIValues>
		if( !pFromValues )
			return nullptr;
		var size = pFromValues->contracts_size();
		for( auto i=0; i<size; ++i )
		{
			var pMutableOptionDays = pFromValues->mutable_contracts( i );
			var pOptionIdValues = options.find( pMutableOptionDays->id() );
			if( pOptionIdValues==options.end() )
				WARN( "Could not find option id='{}'."sv, pMutableOptionDays->id() );
			else
				pOptionIdValues->second = make_tuple( get<0>(pOptionIdValues->second),pMutableOptionDays );
		}

		const DateTime toDate( Chrono::FromDays(to) );
		var pToValues = Load( contract, toDate.Year(), toDate.Month(), toDate.Day() );
		if( !pToValues )
			return nullptr;
		var toSize = pToValues->contracts_size();
		auto pResults = new Proto::Results::OptionValues();
		var today = DateTime::Today();
		map<uint16,Proto::Results::OptionDay*> values;
		set<ContractPK> matchedIds;
		auto setValue = [&]( OptionPtr pOption, const Proto::OptionOIDay& toDay, const Proto::OptionOIDay* pFromDay, bool expired )
		{
			uint16 daysSinceEpoch = pOption->DaysSinceEpoch();
			auto pDayValue = values.find( daysSinceEpoch );
			if( pDayValue==values.end() )
			{
				//DBG( "Adding - {}({})", DateTime(DateTime::FromDays(daysSinceEpoch)).ToIsoString(), daysSinceEpoch );
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
			//pValue->set_delta( toDay.delta() );
			//pValue->set_delta_pr evious(pFromDay ? pFromDay->delta() : toDay.delta() );
		};
		for( auto i=0; i<toSize; ++i )
		{
			var& toDay = pToValues->contracts( i );
			var pOptionIdValues = options.find( toDay.id() );
			if( pOptionIdValues==options.end() )
				THROW( ArgumentException("Could not find option id='{}'.", toDay.id()) );
			matchedIds.emplace( toDay.id() );
			var pOption = get<0>( pOptionIdValues->second );
			if( pOption->IsCall!=isCall )
				continue;
			if( !includeExpired && pOption->ExpirationDate+23h<today )
				continue;

			var pFromDay = get<1>( pOptionIdValues->second );
			setValue( pOption, toDay, pFromDay, false );
		}
		if( includeExpired )
		{
			for( var [optionId, pOptionDay] : options )
 			{
				var pFromDay = get<1>( pOptionDay );
				var pOption = get<0>( pOptionDay );
				if( pFromDay && pOption->IsCall==isCall && matchedIds.find(optionId)==matchedIds.end() )
					setValue( pOption, *pFromDay, nullptr, true );
			}
		}
		return pResults;
	}

	fs::path OptionDir( const Contract& contract )
	{
		var exchangeString = to_string( contract.PrimaryExchange );
		return RootMinuteBar()/StringUtilities::ToLower(exchangeString)/StringUtilities::ToUpper(contract.Symbol)/"options";
	}

	fs::path OptionData::OptionFile( const Contract& contract, uint16 year, uint8 month, uint8 day )noexcept
	{
		return OptionDir(contract)/(IO::FileUtilities::DateFileName(year,month,day)+".dat.xz");
	}

	map<DayIndex,sp<Proto::UnderlyingOIValues>> OptionData::LoadFiles( const Contract& contract )noexcept
	{
		var pFiles = IO::FileUtilities::GetDirectory( OptionDir(contract) );
		map<DayIndex,sp<Proto::UnderlyingOIValues>> values;
		for( auto pFile = pFiles->rbegin(); pFile!=pFiles->rend(); ++pFile )
		{
			var file = pFile->path().filename().stem().string();
			if( !pFile->is_regular_file() || file[4]!='-' || file[7]!='-' )
				continue;
			var year = stoi( file.substr(0, 4) );
			var month = stoi( file.substr(5, 2) );
			var day = stoi( file.substr(8, 2) );
			var daysSinceEpoch = Chrono::DaysSinceEpoch( DateTime(year,(uint8)month,(uint8)day).GetTimePoint() );
			values.emplace( daysSinceEpoch, Load(contract, year, month, day) );
		}
		return values;
	}
}