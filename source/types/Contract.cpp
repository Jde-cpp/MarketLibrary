#include "Contract.h"
#include "proto/results.pb.h"

#define var const auto

namespace Jde::Markets
{
	using std::ostream;
/*	
	Contract::Contract( IO::IncomingMessage& message, bool havePrimaryExchange )noexcept(false):
		Id{message.ReadInt32()},
		Symbol{ message.ReadString() },
		SecType{ message.ReadString() },
		LastTradeDateOrContractMonth{ message.ReadString() },
		Strike{ message.ReadDouble() },
		Right{ message.ReadString() },
		Multiplier{message.ReadString()},
		Exchange{message.ReadString()},
		PrimaryExchange{ havePrimaryExchange ? ToExchange(message.ReadString()) : Exchanges::None },
		Currency{message.ReadString()},
		LocalSymbol{message.ReadString()},
		TradingClass{message.ReadString()}
	{}
*/
	Contract::Contract( const ibapi::Contract& other ):
		Id{ other.conId },
		Symbol{ other.symbol },
		SecType{ other.secType },
		LastTradeDateOrContractMonth{ other.lastTradeDateOrContractMonth },
		Strike{ other.strike },
		Right{ other.right },
		Multiplier{other.multiplier},
		Exchange{other.exchange},
		PrimaryExchange{ ToExchange(other.primaryExchange) },
		Currency{other.currency},
		LocalSymbol{other.localSymbol},
		TradingClass{ other.tradingClass }
	{
		//GetDefaultLogger()->trace( "here" );
	}
	sp<ibapi::Contract> Contract::ToTws()const
	{
		auto pIB = ms<ibapi::Contract>();
		auto& other = *pIB;
		other.conId = Id;
		other.symbol = Symbol;
		other.secType = SecType;
		other.lastTradeDateOrContractMonth = LastTradeDateOrContractMonth;
		other.strike = Strike;
		other.right = Right;
		other.multiplier = Multiplier;
		other.exchange = Exchange;
		
		other.primaryExchange = to_string( PrimaryExchange );
		other.currency = Currency;
		other.localSymbol = LocalSymbol;
		other.tradingClass = TradingClass;

		return pIB;
	}
	Contract::Contract( const Proto::Contract& contract ):
		Id{ (ContractPK)contract.id() },
		Symbol{  contract.symbol() },
		SecType{ contract.sectype() },
		LastTradeDateOrContractMonth{ contract.lasttradedateorcontractmonth() },
		Strike{ contract.strike() },
		Right{ contract.right() },
		Multiplier{ contract.multiplier() },
		Exchange{ contract.exchange() },
		PrimaryExchange{ ToExchange(contract.primaryexchange()) },
		Currency{ contract.currency() },
		LocalSymbol{ contract.localsymbol() },
		TradingClass{ contract.tradingclass() },
		IncludeExpired{ contract.includeexpired() },
		SecIdType{ contract.secidtype() },
		SecId{ contract.secid() },
		ComboLegsDescrip{ contract.combolegsdescrip() },
		Name{ contract.name() },
		Flags{ contract.flags() }
	{
		var legs = contract.combolegs_size();
		if( legs )
		{
			ComboLegsPtr = make_shared<vector<ComboLegPtr_>>(); ComboLegsPtr->reserve( legs );
			for( auto i=0; i<legs; ++i )
				ComboLegsPtr->push_back( make_shared<ComboLeg>(contract.combolegs(i)) );
		}
		if( contract.has_deltaneutral() )
			DeltaNeutral = DeltaNeutralContract{ contract.deltaneutral() };
	}
	sp<Proto::Contract> Contract::ToProto( bool stupidPointer )const noexcept
	{
		auto pProto = stupidPointer ? shared_ptr<Proto::Contract>( new Proto::Contract(), [](Proto::Contract*){} ) : make_shared<Proto::Contract>();
		pProto->set_id( Id );
		pProto->set_symbol( Symbol );
		pProto->set_sectype( SecType );
		pProto->set_lasttradedateorcontractmonth( LastTradeDateOrContractMonth );
		pProto->set_strike( Strike );
		pProto->set_right( Right );
		pProto->set_multiplier( Multiplier );
		pProto->set_exchange( Exchange );
		pProto->set_primaryexchange( to_string(PrimaryExchange) );
		pProto->set_currency( Currency );
		pProto->set_localsymbol( LocalSymbol );
		pProto->set_tradingclass( TradingClass );
		pProto->set_includeexpired( IncludeExpired );
		pProto->set_secidtype( SecIdType );		// CUSIP;SEDOL;ISIN;RIC
		pProto->set_secid( SecId );
		pProto->set_combolegsdescrip( ComboLegsDescrip ); // received in open order 14 and up for all combos
		if( ComboLegsPtr )
		{
			for( var& pComboLeg : *ComboLegsPtr )
				pComboLeg->SetProto( pProto->add_combolegs() );
		}
		sp<Proto::DeltaNeutralContract> pDeltaNeutral = DeltaNeutral.ToProto(true);
		pProto->set_allocated_deltaneutral( pDeltaNeutral.get() );

		pProto->set_name( Name );
		pProto->set_flags( (uint32)Flags );

		return pProto;
	}
	Contract::Contract( ContractPK id, string_view currency, string_view localSymbol, string_view multiplier, string_view name, Exchanges exchange, string_view symbol, string_view tradingClass, TimePoint issueDate ):
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
/*	Contract( const Contract& contract )
	{
		*this = contract;
	}
	operator=( const Contract& contract );
*/
	
	Contract::Contract(ContractPK id, string_view symbol ):
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
			TRACE( "{} - ${} resulted in 1 shares...", Symbol, price );//TODO only log once...
		}
		if( shareCount>89 && shareCount<100 )
			shareCount = 100;
		return shareCount;
	}
	PositionAmount Contract::RoundShares( PositionAmount amount, PositionAmount roundAmount )const noexcept
	{
		return PositionAmount( roundAmount==1 ? llround( amount ) : static_cast<_int>( amount/100 )*100 );
	}
	sp<DateTime> Contract::ExpirationTime()const noexcept
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
	}
	Amount Contract::RoundDownToMinTick( Amount price )const noexcept//TODO find min tick.
	{
		return Amount( std::round( (static_cast<double>(price)-.000005)*100.0 )/100.0 );//.00005 rounds down .005
	}
	std::ostream& Contract::to_stream( std::ostream& os, bool includePrimaryExchange )const
	{
		os << Id << Symbol << SecType << LastTradeDateOrContractMonth << Strike << Right << Multiplier << Exchange;
		if( includePrimaryExchange )
			os  << to_string(PrimaryExchange);
		os << Currency << LocalSymbol << TradingClass;
		return os;
	}
	ostream& operator<<( ostream& os, const Contract& contract )
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
	ComboLeg::ComboLeg( const Proto::ComboLeg& proto ):
		ConId{ (ContractPK)proto.conid() },
		Ratio{ proto.ratio() },
		Action{ proto.action() },
		Exchange {proto.exchange() },
		OpenClose{ proto.openclose() },
		ShortSaleSlot{ proto.shortsaleslot() },
		DesignatedLocation{ proto.designatedlocation() },
		ExemptCode{ proto.exemptcode() }
	{}

	void ComboLeg::SetProto( Proto::ComboLeg* pProto )const noexcept
	{
		pProto->set_conid( ConId );
		pProto->set_ratio( Ratio );
		pProto->set_action( Action );
		pProto->set_exchange( Exchange );
		pProto->set_openclose( OpenClose );
		pProto->set_shortsaleslot( ShortSaleSlot );
		pProto->set_designatedlocation( DesignatedLocation );
		pProto->set_exemptcode( ExemptCode );
	}
	std::ostream& ComboLeg::to_stream( std::ostream& os, bool isOrder )const
	{
		os << ConId << Ratio << Action << Exchange;
		if( isOrder ) 
			os << OpenClose << ShortSaleSlot << DesignatedLocation << ExemptCode;
		return os;
	}
	std::ostream& operator<<( std::ostream& os, const ComboLeg& comboLeg )
	{
		return comboLeg.to_stream( os, false );
	}	
	namespace Contracts
	{
		const Contract Spy{ 756733, "USD", "SPY", "0", "SPDR S&P 500 ETF TRUST", Exchanges::Arca, "SPY", "SPY", DateTime(2004,1,23,14,30).GetTimePoint() };
		const Contract SH{ 236687911, "USD", "SH", "0", "PROSHARES SHORT S&P500", Exchanges::Arca, "SH", "SH" };
		const Contract Qqq{ 320227571, "USD", "QQQ", "0", "POWERSHARES QQQ TRUST SERIES", Exchanges::Arca, "QQQ", "QQQ" };
		const Contract Psq{ 43661924, "USD", "PSQ", "0", "PROSHARES SHORT QQQ", Exchanges::Arca, "PSQ", "PSQ" };
		const Contract Tsla{ 76792991, "USD", "TSLA", "0", "TESLA INC", Exchanges::Nasdaq, "TSLA", "TSLA" };
	}
#pragma region SecurityRight
	SecurityRight ToSecurityRight( string_view inputName )noexcept
	{
		CIString name = string(inputName).c_str();
		auto securityRight = SecurityRight::None;
		if( name=="call" || name=="C" )
			securityRight = SecurityRight::Call;
		else if( name=="put" || name=="P" )
			securityRight = SecurityRight::Put;
		else
			GetDefaultLogger()->warn( fmt::format("Could not parse security right {}", inputName) );

		return securityRight;
	}
#pragma endregion
#pragma region SecurityType
	SecurityType ToSecurityType( string_view inputName )noexcept
	{
		CIString name = string(inputName).c_str();
		SecurityType type = SecurityType::None;
		if( name=="OPT" )
			type = SecurityType::Option;
		else if( name=="STK" )
			type = SecurityType::Stock;
		else if( name=="IND" )
			type = SecurityType::Index;
		else if( name=="WAR" )
			type = SecurityType::Warrant;
		else
			GetDefaultLogger()->warn( fmt::format("Could not parse security type {}", inputName) );
		
		return type;
	}
#pragma endregion
	ContractPtr_ Find( const map<ContractPK, ContractPtr_>& contracts, string_view symbol )
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

	Proto::Results::ContractDetails* ToProto( const ibapi::ContractDetails& details )noexcept
	{
		var pResult = new Proto::Results::ContractDetails();
		pResult->set_allocated_contract( Contract(details.contract).ToProto(true).get() );
		pResult->set_marketname( details.marketName );
		pResult->set_mintick( details.minTick );
		pResult->set_ordertypes( details.orderTypes );
		pResult->set_validexchanges( details.validExchanges );
		pResult->set_pricemagnifier( details.priceMagnifier );
		pResult->set_underconid( details.underConId );
		pResult->set_longname( details.longName );
		pResult->set_contractmonth( details.contractMonth );
		pResult->set_industry( details.industry );
		pResult->set_category( details.category );
		pResult->set_subcategory( details.subcategory );
		pResult->set_timezoneid( details.timeZoneId );
		auto parseDate= []( const string& value )
		{
			auto year  = stoi( value.substr(0,4) );
			auto month = stoi( value.substr(4,2) );
			auto day =   stoi( value.substr(6,2) );
			auto hour =  stoi( value.substr(9,2) );
			auto minute= stoi( value.substr(11,2) );
			return DateTime( year,month,day,hour,minute ).GetTimePoint();
		};
		var days = StringUtilities::Split( details.tradingHours, ';' );
		for( auto day : days )
		{
			var startEnd = StringUtilities::Split( details.tradingHours, '-' );
			if( startEnd.size()!=2 || startEnd[0].size()!=13 || startEnd[1].size()!=13 )
				continue;
			
		}

		pResult->set_tradinghours( details.tradingHours );
		pResult->set_liquidhours( details.liquidHours );
		pResult->set_evrule( details.evRule );
		pResult->set_evmultiplier( details.evMultiplier );
		pResult->set_mdsizemultiplier( details.mdSizeMultiplier );
		pResult->set_agggroup( details.aggGroup );
		pResult->set_undersymbol( details.underSymbol );
		pResult->set_undersectype( details.underSecType );
		pResult->set_marketruleids( details.marketRuleIds );
		pResult->set_realexpirationdate( details.realExpirationDate );
		pResult->set_lasttradetime( details.lastTradeTime );
		if( details.secIdList )
		{
			for( var& pTagValue : *details.secIdList )
			{
				var pProto = pResult->add_secidlist(); pProto->set_tag( pTagValue->tag ); pProto->set_value( pTagValue->value );
			}
		}
		pResult->set_cusip( details.cusip );
		pResult->set_ratings( details.ratings );
		pResult->set_descappend( details.descAppend );
		pResult->set_bondtype( details.bondType );
		pResult->set_coupontype( details.couponType );
		pResult->set_callable( details.callable );
		pResult->set_putable( details.putable );
		pResult->set_coupon( details.coupon );
		pResult->set_convertible( details.convertible );
		pResult->set_maturity( details.maturity );
		pResult->set_issuedate( details.issueDate );
		pResult->set_nextoptiondate( details.nextOptionDate );
		pResult->set_nextoptiontype( details.nextOptionType );
		pResult->set_nextoptionpartial( details.nextOptionPartial );
		pResult->set_notes( details.notes );
		return pResult;
	}

}