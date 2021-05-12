#include <jde/markets/types/Contract.h>
#include "Currencies.h"
#pragma warning( disable : 4244 )
#include <jde/markets/types/proto/results.pb.h>
#pragma warning( default : 4244 )
#include "../types/Exchanges.h"
#include "../../../Framework/source/Cache.h"

#define var const auto

namespace Jde::Markets
{
	using std::ostream;
	Contract::Contract( const ::Contract& other )noexcept:
		Id{ other.conId },
		Symbol{ other.symbol },
		SecType{ ToSecurityType(other.secType) },
		Expiration{ ToDay(other.lastTradeDateOrContractMonth) },
		Strike{ other.strike },
		Right{ ToSecurityRight(other.right) },
		Multiplier{ other.multiplier.size() ? (uint)stoi(other.multiplier) : 0 },
		Exchange{ ToExchange(other.exchange) },
		PrimaryExchange{ ToExchange(other.primaryExchange) },
		Currency{ ToCurrency(other.currency) },
		LocalSymbol{other.localSymbol},
		TradingClass{ other.tradingClass }
	{}

	DayIndex Contract::ToDay( str str )noexcept
	{
		DayIndex value = 0;
		if( str.size()==8 )
		{
			var year  = stoi( str.substr(0,4) ); var month = stoi( str.substr(4,2) ); var day = stoi( str.substr(6,2) );
			value = Chrono::DaysSinceEpoch( DateTime{(uint16)year, (uint8)month, (uint8)day} );
		}
		return value;
	}
	sp<::Contract> Contract::ToTws()const noexcept
	{
		auto pIB = ms<::Contract>();
		auto& other = *pIB;
		other.conId = Id;
		other.symbol = Symbol;
		other.secType = ToString(SecType);
		if( Expiration )
		{
			const DateTime expiration{ Chrono::FromDays( Expiration ) };
			other.lastTradeDateOrContractMonth = format( "{}{:0>2}{:0>2}", expiration.Year(), expiration.Month(), expiration.Day() );
		}
		other.strike = Strike;
		other.right = ToString( Right );
		if( Multiplier )
			other.multiplier = std::to_string( Multiplier );
		other.exchange = ToString(Exchange);
		if( PrimaryExchange!=Exchanges::Smart )
			other.primaryExchange = ToString( PrimaryExchange );
		other.currency = ToString( Currency );
		other.localSymbol = LocalSymbol;
		other.tradingClass = TradingClass;

		return pIB;
	}
	Contract::Contract( const Proto::Contract& contract )noexcept:
		Id{ (ContractPK)contract.id() },
		Symbol{  contract.symbol() },
		SecType{ contract.security_type()==SecurityType::Unknown ? SecurityType::Stock : contract.security_type() },
		Expiration{ contract.expiration() },
		Strike{ contract.strike() },
		Right{ contract.right() },
		Multiplier{ contract.multiplier() },
		Exchange{ contract.exchange() },
		PrimaryExchange{ contract.primary_exchange() },
		Currency{ contract.currency() },
		LocalSymbol{ contract.local_symbol() },
		TradingClass{ contract.trading_class() },
		IncludeExpired{ contract.include_expired() },
		SecIdType{ contract.sec_id_type() },
		SecId{ contract.sec_id() },
		ComboLegsDescrip{ contract.combo_legs_description() },
		Name{ contract.name() },
		Flags{ contract.flags() }
	{
		var legs = contract.combo_legs_size();
		if( legs )
		{
			ComboLegsPtr = make_shared<vector<ComboLegPtr_>>(); ComboLegsPtr->reserve( legs );
			for( auto i=0; i<legs; ++i )
				ComboLegsPtr->push_back( make_shared<ComboLeg>(contract.combo_legs(i)) );
		}
		if( contract.has_delta_neutral() )
			DeltaNeutral = DeltaNeutralContract{ contract.delta_neutral() };
	}
	sp<Proto::Contract> Contract::ToProto( bool stupidPointer )const noexcept
	{
		auto pProto = stupidPointer ? shared_ptr<Proto::Contract>( new Proto::Contract(), [](Proto::Contract*){} ) : make_shared<Proto::Contract>();
		pProto->set_id( Id );
		pProto->set_symbol( Symbol );
		pProto->set_security_type( SecType );
		pProto->set_expiration( Expiration );
		pProto->set_strike( Strike );
		pProto->set_right( Right );
		pProto->set_multiplier( Multiplier );
		pProto->set_exchange( Exchange );
		pProto->set_primary_exchange( PrimaryExchange );
		pProto->set_currency( Currency );
		pProto->set_local_symbol( LocalSymbol );
		pProto->set_trading_class( TradingClass );
		pProto->set_include_expired( IncludeExpired );
		pProto->set_sec_id_type( SecIdType );		// CUSIP;SEDOL;ISIN;RIC
		pProto->set_sec_id( SecId );
		pProto->set_combo_legs_description( ComboLegsDescrip ); // received in open order 14 and up for all combos
		if( ComboLegsPtr )
		{
			for( var& pComboLeg : *ComboLegsPtr )
				pComboLeg->SetProto( pProto->add_combo_legs() );
		}
		sp<Proto::DeltaNeutralContract> pDeltaNeutral = DeltaNeutral.ToProto(true);
		pProto->set_allocated_delta_neutral( pDeltaNeutral.get() );

		pProto->set_name( Name );
		pProto->set_flags( (uint32)Flags );

		return pProto;
	}
	Contract::Contract( ContractPK id, Proto::Currencies currency, sv localSymbol, uint32 multiplier, sv name, Exchanges exchange, sv symbol, sv tradingClass, TimePoint issueDate )noexcept:
		Id{id},
		Symbol{symbol},
		Multiplier{multiplier},
		PrimaryExchange{exchange},
		Currency{currency},
		LocalSymbol{localSymbol},
		TradingClass{tradingClass},
		Name{name},
		IssueDate{issueDate}
	{}
	sp<vector<Proto::Results::ContractHours>> ParseTradingHours( sv timeZoneId, str hours )noexcept;
	Contract::Contract( const ::ContractDetails& details )noexcept:
		Contract{ details.contract }
	{
		TradingHoursPtr = ParseTradingHours( details.timeZoneId, details.tradingHours );
		LiquidHoursPtr =  ParseTradingHours( details.timeZoneId, details.liquidHours );
	}
/*	Contract( const Contract& contract )
	{
		*this = contract;
	}
	operator=( const Contract& contract );
*/

	Contract::Contract(ContractPK id, sv symbol )noexcept:
		Id(id),
		Symbol(symbol)
	{}

	Contract::~Contract()
	{}

	ContractPK Contract::ShortContractId()const noexcept
	{
		ContractPK shortContractId{0};
		if( Id==Contracts::Spy.Id )
			shortContractId = Contracts::SH.Id;
		else if( Id==Contracts::Qqq.Id )
			shortContractId = Contracts::Psq.Id;
		return shortContractId;
	}

	PositionAmount Contract::LongShareCount( Amount price )const noexcept
	{
		return ShortShareCount( price );
	}
	PositionAmount Contract::ShortShareCount( Amount price )const noexcept
	{
		var count = price==0 ? 0 : std::min( 1000.0, 10000.0/price );
		auto shareCount = RoundShares( count, count>100 ? 100 : 1 );
		if( shareCount==0 && price!=0 )
		{
			shareCount = 1;
			TRACE( "{} - ${} resulted in 1 shares..."sv, Symbol, price );//TODO only log once...
		}
		if( shareCount>89 && shareCount<100 )
			shareCount = 100;
		return shareCount;
	}
	PositionAmount Contract::RoundShares( PositionAmount amount, PositionAmount roundAmount )const noexcept
	{
		return PositionAmount( roundAmount==1 ? llround( amount ) : static_cast<_int>( amount/100 )*100 );
	}
/*	sp<DateTime> Contract::ExpirationTime()const noexcept
	{
		auto result = sp<DateTime>( nullptr );
		if( LastTradeDateOrContractMonth.size()>0 )
		{
			var year = static_cast<uint16>(stoul( LastTradeDateOrContractMonth.substr(0,4)) );
			var month = static_cast<uint8>( stoul( LastTradeDateOrContractMonth.substr(4,6)) );
			var day = static_cast<uint8>(stoul( LastTradeDateOrContractMonth.substr(6,8) ));
			result = make_shared<DateTime>( year, month, day );
		}
		return result;
	}*/
	Amount Contract::RoundDownToMinTick( Amount price )const noexcept//TODO find min tick.
	{
		return Amount( std::round( (static_cast<double>(price)-.000005)*100.0 )/100.0 );//.00005 rounds down .005
	}

	string Contract::Display()const noexcept
	{
		return SecType==SecurityType::Option
			? format("{} - {}@{}", Symbol, Chrono::DateDisplay(Expiration), Strike )
			: Symbol;
	}

	std::ostream& Contract::to_stream( std::ostream& os, bool includePrimaryExchange )const noexcept
	{
		os << Id << Symbol << SecType << Expiration << Strike << ToString(Right) << Multiplier << Exchange;
		if( includePrimaryExchange )
			os  << ToString(PrimaryExchange);
		os << Currency << LocalSymbol << TradingClass;
		return os;
	}
	ostream& operator<<( ostream& os, const Contract& contract )noexcept
	{
		return contract.to_stream( os );
	}

/*	DeltaNeutralContract::DeltaNeutralContract( IO::IncomingMessage& message ):
		Id{ message.ReadInt32() },
		Delta{ message.ReadDouble() },
		Price{ message.ReadDouble() }
	{}
*/
	DeltaNeutralContract::DeltaNeutralContract( const Proto::DeltaNeutralContract& proto )noexcept:
		Id{ proto.id() },
		Delta{ proto.delta() },
		Price{ proto.price() }
	{}

	sp<Proto::DeltaNeutralContract> DeltaNeutralContract::ToProto( bool stupidPointer )const noexcept
	{
		auto pProto = stupidPointer ? shared_ptr<Proto::DeltaNeutralContract>( new Proto::DeltaNeutralContract(), [](Proto::DeltaNeutralContract*){} ) : make_shared<Proto::DeltaNeutralContract>();
		pProto->set_id( Id );
		pProto->set_delta( Delta );
		pProto->set_price( Price );
		return pProto;
	}
/*	ComboLeg::ComboLeg( IO::IncomingMessage& message, bool isOrder ):
		ConId{ message.ReadInt32() },
		Ratio{ message.ReadInt32() },
		Action{ message.ReadString() },
		Exchange{ message.ReadString() },
		OpenClose{ isOrder ? message.ReadInt32() : 0 },
		ShortSaleSlot{ isOrder ? message.ReadInt32() : 0 },
		DesignatedLocation{ isOrder ? message.ReadString() : "" },
		ExemptCode{ isOrder ? message.ReadInt32() : 0 }
	{}
	*/
	ComboLeg::ComboLeg( const Proto::ComboLeg& proto )noexcept:
		ConId{ (ContractPK)proto.contract_id() },
		Ratio{ proto.ratio() },
		Action{ proto.action() },
		Exchange {proto.exchange() },
		OpenClose{ proto.open_close() },
		ShortSaleSlot{ proto.short_sales_lot() },
		DesignatedLocation{ proto.designated_location() },
		ExemptCode{ proto.exempt_code() }
	{}

	void ComboLeg::SetProto( Proto::ComboLeg* pProto )const noexcept
	{
		pProto->set_contract_id( ConId );
		pProto->set_ratio( Ratio );
		pProto->set_action( Action );
		pProto->set_exchange( Exchange );
		pProto->set_open_close( OpenClose );
		pProto->set_short_sales_lot( ShortSaleSlot );
		pProto->set_designated_location( DesignatedLocation );
		pProto->set_exempt_code( ExemptCode );
	}
	std::ostream& ComboLeg::to_stream( std::ostream& os, bool isOrder )const noexcept
	{
		os << ConId << Ratio << Action << Exchange;
		if( isOrder )
			os << OpenClose << ShortSaleSlot << DesignatedLocation << ExemptCode;
		return os;
	}
	std::ostream& operator<<( std::ostream& os, const ComboLeg& comboLeg )noexcept
	{
		return comboLeg.to_stream( os, false );
	}
	namespace Contracts
	{
		const Contract Spy{ 756733, Proto::Currencies::UsDollar, "SPY", 0, "SPDR S&P 500 ETF TRUST", Exchanges::Arca, "SPY", "SPY", DateTime(2004,1,23,14,30).GetTimePoint() };
		const Contract SH{ 236687911, Proto::Currencies::UsDollar, "SH", 0, "PROSHARES SHORT S&P500", Exchanges::Arca, "SH", "SH" };
		const Contract Qqq{ 320227571, Proto::Currencies::UsDollar, "QQQ", 0, "POWERSHARES QQQ TRUST SERIES", Exchanges::Arca, "QQQ", "QQQ" };
		const Contract Psq{ 43661924, Proto::Currencies::UsDollar, "PSQ", 0, "PROSHARES SHORT QQQ", Exchanges::Arca, "PSQ", "PSQ" };
		const Contract Tsla{ 76792991, Proto::Currencies::UsDollar, "TSLA", 0, "TESLA INC", Exchanges::Nasdaq, "TSLA", "TSLA" };
		const Contract Aig{ 61319701, Proto::Currencies::UsDollar, "AIG", 0, "AMERICAN INTERNATIONAL GROUP", Exchanges::Nyse, "AIG", "AIG", DateTime(2004,1,23,14,30,00).GetTimePoint() };
		const Contract Xom{ 13977, Proto::Currencies::UsDollar, "XOM", 0, "EXXON MOBIL", Exchanges::Nyse, "XOM", "XOM", DateTime(2004,1,23,14,30,00).GetTimePoint() };
	}
#pragma region SecurityRight
	SecurityRight ToSecurityRight( sv inputName )noexcept
	{
		CIString name{inputName};
		auto securityRight = SecurityRight::None;
		if( name=="call"sv || name=="C"sv )
			securityRight = SecurityRight::Call;
		else if( name=="put"sv || name=="P"sv )
			securityRight = SecurityRight::Put;
		else if( name.size() && name!="?"sv && name!="0"sv )
			WARN( "Could not parse security right '{}'."sv, inputName );

		return securityRight;
	}
	sv ToString( SecurityRight right )noexcept
	{
		sv result = ""sv;
		if( right==SecurityRight::Call )
			result = "CALL"sv;
		else if( right==SecurityRight::Put )
			result = "PUT"sv;

		return result;
	}
#pragma endregion
#pragma region SecurityType
	SecurityType ToSecurityType( sv inputName )noexcept
	{
		CIString name{ inputName };
		SecurityType type = SecurityType::Unknown;
		if( name=="OPT"sv )
			type = SecurityType::Option;
		else if( name=="STK"sv )
			type = SecurityType::Stock;
		else if( name=="IND"sv )
			type = SecurityType::Index;
		else if( name=="WAR"sv )
			type = SecurityType::Warrant;
		else
			WARN( "Could not parse security type {}"sv, inputName );

		return type;
	}
	constexpr std::array<sv,12> SecurityTypes = {"None","STK","MutualFund","Etf","Future","Commodity","Bag","Cash","Fop","IND","OPT","WAR"};
	sv ToString( SecurityType type )noexcept
	{
		return SecurityTypes[ type<(int)SecurityTypes.size() ? type : 0];
	}
#pragma endregion
	ContractPtr_ Find( const map<ContractPK, ContractPtr_>& contracts, sv symbol )noexcept
	{
		ContractPtr_ pContract;
		for( var& pIdContract : contracts )
		{
			if( pIdContract.second->Symbol == symbol)
			{
				pContract = pIdContract.second;
				break;
			}
		}
		return pContract;
	}

	sp<vector<Proto::Results::ContractHours>> ParseTradingHours( sv timeZoneId, str hours )noexcept
	{
//		return make_shared<vector<Proto::Results::ContractHours>>();
		var cacheId = format( "TradingHours.{}.{}", timeZoneId, hours );
		if( auto pValue = Cache::Get<vector<Proto::Results::ContractHours>>(cacheId); pValue )
			return pValue;
		auto parseTimeframe = [timeZoneId]( str timeFrame )noexcept(false)
		{
			auto parseDateTime = [timeZoneId]( str value )noexcept(false)->time_t //20200131:0930, 20200104:CLOSED
			{
				var dateTime = Str::Split( value, ':' );
				var& date = dateTime[0];
				if( date.size()!=8 || dateTime.size()!=2 )
					THROW( Exception("Could not parse parseDateTime '{}'.", value) );
				var year  = stoi( date.substr(0,4) ); var month = stoi( date.substr(4,2) ); var day = stoi( date.substr(6,2) );
				var& time = dateTime[1]; uint8 hour=0; uint8 minute=0;
				if( time!="CLOSED" )
				{
					if( time.size()!=4 )
						THROW( Exception("Could not parse time part of parseDateTime '{}'.", value) );
					hour = stoi( time.substr(0,2) );
					minute = stoi( time.substr(2,2) );
				}
				var localTime = DateTime( year, month, day, hour, minute ).GetTimePoint();
				return Clock::to_time_t( localTime-Timezone::TryGetGmtOffset(timeZoneId, localTime) );
			};
			var startEnd = Str::Split( timeFrame, '-' );//20200131:0930-20200131:1600 //20200104:CLOSED
			var start = parseDateTime( startEnd[0] );
			if( startEnd.size()>2 )
				THROW( Exception("Could not parse parseTimeframe '{}'.", timeFrame) );
			var end = startEnd.size()==2 ? parseDateTime( startEnd[1] ) : 0;

			Proto::Results::ContractHours hours; hours.set_start( (int)start ); hours.set_end( (int)end );
			return hours;
		};
		var tradingHours = Str::Split( hours, ';' );
		auto pHours = make_shared<vector<Proto::Results::ContractHours>>();
		for( auto day : tradingHours )
			Try( [&](){ pHours->push_back(parseTimeframe(day)); } );
		Cache::Set( cacheId, pHours );
		return pHours;
	}


	void ToProto( const ::ContractDetails& details, Proto::Results::ContractDetail& proto )noexcept
	{
		proto.set_allocated_contract( Contract(details.contract).ToProto(true).get() );
		proto.set_market_name( details.marketName );
		proto.set_min_tick( details.minTick );
		proto.set_order_types( details.orderTypes );
		proto.set_valid_exchanges( details.validExchanges );
		proto.set_price_magnifier( details.priceMagnifier );
		proto.set_under_con_id( details.underConId );
		proto.set_long_name( details.longName );
		proto.set_contract_month( details.contractMonth );
		proto.set_industry( details.industry );
		proto.set_category( details.category );
		proto.set_subcategory( details.subcategory );
		proto.set_time_zone_id( details.timeZoneId );
		for( var& hours : *ParseTradingHours(details.timeZoneId, details.tradingHours) )
			*proto.add_trading_hours() = hours;
		for( var& hours : *ParseTradingHours(details.timeZoneId, details.liquidHours) )
			*proto.add_liquid_hours() = hours;

		proto.set_ev_rule( details.evRule );
		proto.set_ev_multiplier( details.evMultiplier );
		proto.set_md_size_multiplier( details.mdSizeMultiplier );
		proto.set_agg_group( details.aggGroup );
		proto.set_under_symbol( details.underSymbol );
		proto.set_under_sec_type( details.underSecType );
		proto.set_market_rule_ids( details.marketRuleIds );
		proto.set_real_expiration_date( details.realExpirationDate );
		proto.set_last_trade_time( details.lastTradeTime );
		proto.set_stock_type( details.stockType );
		if( details.secIdList )
		{
			for( var& pTagValue : *details.secIdList )
			{
				var pProto = proto.add_sec_id_list(); pProto->set_tag( pTagValue->tag ); pProto->set_value( pTagValue->value );
			}
		}
		proto.set_cusip( details.cusip );
		proto.set_ratings( details.ratings );
		proto.set_desc_append( details.descAppend );
		proto.set_bond_type( details.bondType );
		proto.set_coupon_type( details.couponType );
		proto.set_callable( details.callable );
		proto.set_putable( details.putable );
		proto.set_coupon( details.coupon );
		proto.set_convertible( details.convertible );
		proto.set_maturity( details.maturity );
		proto.set_issuedate( details.issueDate );
		proto.set_next_option_date( details.nextOptionDate );
		proto.set_next_option_type( details.nextOptionType );
		proto.set_next_option_partial( details.nextOptionPartial );
		proto.set_notes( details.notes );
	}
}