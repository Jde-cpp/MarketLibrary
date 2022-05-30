#include <Decimal.h>
#include <jde/markets/types/Tick.h>
#include <jde/Exports.h>
#pragma warning( disable : 4244 )
#include <jde/markets/types/proto/results.pb.h>
#include <jde/blockly/BlocklyLibrary.h>
#pragma warning( default : 4244 )
#include <jde/Str.h>
#include "../../../Framework/source/collections/Vector.h"

#define var const auto
namespace Jde::Markets
{
	α Tick::SetString( ETickType type, const string& value )noexcept->bool
	{
		bool set{ true };
		switch( type )
		{
		case ETickType::BidExchange: BidExchange = value; break;
		case ETickType::AskExchange: AskExchange = value; break;
		case ETickType::LastExchange: LastExchange = value; break;
		case ETickType::LastTimestamp: LastTimestamp = To<time_t>( value ); break;
		case ETickType::RT_VOLUME: RT_VOLUME = value; break;
		case ETickType::FUNDAMENTAL_RATIOS: RatioString = value; break;
		case ETickType::IB_DIVIDENDS: DividendString = value; break;
		default:
			WARN( "Ticktype {} value {} is not a string."sv, Str::FromEnum(TickTypeStrings, type), value );
			set = false;
		}
		if( set )
			_setFields.set( type );
		return set;
	}
	α Tick::SetInt( ETickType type, _int value )noexcept->bool
	{
		bool set{ true };
		switch( type )
		{
		case ETickType::OPEN_INTEREST: OPEN_INTEREST = value; break;
		case ETickType::OPTION_CALL_OPEN_INTEREST: OPTION_CALL_OPEN_INTEREST = value; break;
		case ETickType::OPTION_PUT_OPEN_INTEREST: OPTION_PUT_OPEN_INTEREST = value; break;
		case ETickType::OPTION_CALL_VOLUME: OPTION_CALL_VOLUME = value; break;
		case ETickType::OPTION_PUT_VOLUME: OPTION_PUT_VOLUME = value; break;
		case ETickType::AUCTION_VOLUME: AUCTION_VOLUME = value; break;
		case ETickType::LastTimestamp: LastTimestamp = value; break;
		case ETickType::RT_VOLUME: RT_VOLUME = std::to_string( value ); break;
		case ETickType::TRADE_COUNT: TRADE_COUNT = value; break;
		case ETickType::VOLUME_RATE: VOLUME_RATE = value; break;
		case ETickType::SHORT_TERM_VOLUME_3_MIN: SHORT_TERM_VOLUME_3_MIN = value; break;
		case ETickType::SHORT_TERM_VOLUME_5_MIN: SHORT_TERM_VOLUME_5_MIN = value; break;
		case ETickType::SHORT_TERM_VOLUME_10_MIN: SHORT_TERM_VOLUME_10_MIN = value; break;
		case ETickType::DELAYED_BID_SIZE: DELAYED_BID_SIZE = value; break;
		case ETickType::DELAYED_ASK_SIZE: DELAYED_ASK_SIZE = value; break;
		case ETickType::DELAYED_LAST_SIZE: DELAYED_LAST_SIZE = value; break;
		case ETickType::DELAYED_VOLUME: DELAYED_VOLUME = value; break;
		case ETickType::RT_TRD_VOLUME: RT_TRD_VOLUME = value; break;
		case ETickType::LAST_REG_TIME: LAST_REG_TIME = value; break;
		case ETickType::FUTURES_OPEN_INTEREST: FUTURES_OPEN_INTEREST = value; break;
		case ETickType::AVG_OPT_VOLUME: AVG_OPT_VOLUME = value; break;
		case ETickType::DELAYED_LAST_TIMESTAMP: DELAYED_LAST_TIMESTAMP = value; break;
		case ETickType::NOT_SET: NOT_SET = (int)value; break;
		default:
			WARN( "Ticktype {} value {} is not a int."sv, Str::FromEnum(TickTypeStrings, type), value );
			set = false;
		}
		if( set )
			_setFields.set( type );
		return set;
	}
	α Tick::SetPrices( ::Decimal bidSize, double bid, ::Decimal askSize, double ask )noexcept->void
	{
		BidSize = bidSize; _setFields.set( ETickType::BidSize );
		Bid = bid; _setFields.set( ETickType::BidPrice );
		AskSize = askSize; _setFields.set( ETickType::AskSize );
		Ask = ask; _setFields.set( ETickType::AskPrice );
	}
	α Tick::SetPrice( ETickType type, double value/*, const TickAttrib& attribs*/ )noexcept->bool
	{
		bool set{ true };
		switch( type )
		{
		case ETickType::BidPrice: Bid = value; break;
		case ETickType::AskPrice: Ask = value; break;
		case ETickType::LastPrice: LastPrice = value; break;
		case ETickType::High: High = value; break;
		case ETickType::Low: Low = value; break;
		case ETickType::MarkPrice: MarkPrice = value; break;
		case ETickType::OpenTick: OpenTick = value; break;
		case ETickType::ClosePrice: ClosePrice = value; break;
		case ETickType::OptionHistoricalVol: OptionHistoricalVol = value; break;
		case ETickType::OptionImpliedVol: OptionImpliedVol = value; break;
		case ETickType::SHORTABLE: SHORTABLE = value; break;
		case ETickType::Low13Week: Low13Week = value; break;
		case ETickType::High13Week: High13Week = value; break;
		case ETickType::Low26Week: Low26Week = value; break;
		case ETickType::High26Week: High26Week = value; break;
		case ETickType::Low52Week: Low52Week = value; break;
		case ETickType::High52Week: High52Week = value; break;
		case ETickType::Halted: Halted = value; break;
		default:
			WARN( "Ticktype {} value {} is not a price."sv, Str::FromEnum(TickTypeStrings, type), value );
			set = SetDouble( type, value );
		}
		if( set )
			_setFields.set( type );
		return set;
	}
	α Tick::SetDecimal( ETickType type, Decimal value )noexcept->bool
	{
		bool set{ true };
		if( type==ETickType::BidSize ) BidSize	= value;
		else if( type==ETickType::AskSize )  AskSize = value;
		else if( type==ETickType::LastSize )  LastSize = value;
		else if( type==ETickType::Volume )  Volume = value;
		else if( type==ETickType::AverageVolume_ ) AverageVolume = value;
		else if( type==ETickType::ShortableShares ) ShortableShares = value;
		else set = false;
		if( set )
			_setFields.set( type );
		ASSERT( set );
		return set;
	}
	α Tick::SetDouble( ETickType type, double value )noexcept->bool
	{
		bool set{ true };
		switch( type )
		{
		case ETickType::OPTION_BID_EXCH: OPTION_BID_EXCH = value; break;
		case ETickType::OPTION_ASK_EXCH: OPTION_ASK_EXCH = value; break;
		case ETickType::INDEX_FUTURE_PREMIUM: INDEX_FUTURE_PREMIUM = value; break;
		case ETickType::AUCTION_PRICE: AUCTION_PRICE = value; break;
		case ETickType::AUCTION_IMBALANCE: AUCTION_IMBALANCE = value; break;
		case ETickType::MarkPrice: MarkPrice = value; break;
		case ETickType::BID_EFP_COMPUTATION: BID_EFP_COMPUTATION = value; break;
		case ETickType::ASK_EFP_COMPUTATION: ASK_EFP_COMPUTATION = value; break;
		case ETickType::LAST_EFP_COMPUTATION: LAST_EFP_COMPUTATION = value; break;
		case ETickType::OPEN_EFP_COMPUTATION: OPEN_EFP_COMPUTATION = value; break;
		case ETickType::HIGH_EFP_COMPUTATION: HIGH_EFP_COMPUTATION = value; break;
		case ETickType::LOW_EFP_COMPUTATION: LOW_EFP_COMPUTATION = value; break;
		case ETickType::CLOSE_EFP_COMPUTATION: CLOSE_EFP_COMPUTATION = value; break;
		case ETickType::BID_YIELD: BID_YIELD = value; break;
		case ETickType::ASK_YIELD: ASK_YIELD = value; break;
		case ETickType::LAST_YIELD: LAST_YIELD = value; break;
		case ETickType::CUST_OPTION_COMPUTATION: CUST_OPTION_COMPUTATION = value; break;
		case ETickType::TRADE_RATE: TRADE_RATE = value; break;
		case ETickType::LAST_RTH_TRADE: LAST_RTH_TRADE = value; break;
		case ETickType::RT_HISTORICAL_VOL: RT_HISTORICAL_VOL = value; break;
		case ETickType::BOND_FACTOR_MULTIPLIER: BOND_FACTOR_MULTIPLIER = value; break;
		case ETickType::REGULATORY_IMBALANCE: REGULATORY_IMBALANCE = value; break;
		case ETickType::DELAYED_BID: DELAYED_BID = value; break;
		case ETickType::DELAYED_ASK: DELAYED_ASK = value; break;
		case ETickType::DELAYED_LAST: DELAYED_LAST = value; break;
		case ETickType::DELAYED_HIGH: DELAYED_HIGH = value; break;
		case ETickType::DELAYED_LOW: DELAYED_LOW = value; break;
		case ETickType::DELAYED_CLOSE: DELAYED_CLOSE = value; break;
		case ETickType::DELAYED_OPEN: DELAYED_OPEN = value; break;
		case ETickType::CREDITMAN_MARK_PRICE: CREDITMAN_MARK_PRICE = value; break;
		case ETickType::CREDITMAN_SLOW_MARK_PRICE: CREDITMAN_SLOW_MARK_PRICE = value; break;
		case ETickType::DELAYED_BID_OPTION_COMPUTATION: DELAYED_BID_OPTION_COMPUTATION = value; break;
		case ETickType::DELAYED_ASK_OPTION_COMPUTATION: DELAYED_ASK_OPTION_COMPUTATION = value; break;
		case ETickType::DELAYED_LAST_OPTION_COMPUTATION: DELAYED_LAST_OPTION_COMPUTATION = value; break;
		case ETickType::DELAYED_MODEL_OPTION_COMPUTATION: DELAYED_MODEL_OPTION_COMPUTATION = value; break;
		default:
			WARN( "Ticktype '{}' value '{}' is not a double."sv, Str::FromEnum(TickTypeStrings, type), value );
			set = false;
		}
		if( set )
			_setFields.set( type );
		return set;
	}

	α Tick::SetOptionComputation( ETickType type, OptionComputation&& v )noexcept->bool
	{
		bool set{ true };
		switch( type )
		{
		case ETickType::BID_OPTION_COMPUTATION: BID_OPTION_COMPUTATION = move( v ); break;
		case ETickType::ASK_OPTION_COMPUTATION: ASK_OPTION_COMPUTATION = move( v ); break;
		case ETickType::LAST_OPTION_COMPUTATION: LAST_OPTION_COMPUTATION = move( v ); break;
		case ETickType::MODEL_OPTION: MODEL_OPTION = move( v ); break;
		default:
			WARN( "Ticktype '{}' value '{}' is not a option comp."sv, Str::FromEnum(TickTypeStrings, type) );
			set = false;
		}
		if( set )
			_setFields.set( type );
		return set;
	}
	using Proto::Results::TickPrice;
	using Proto::Results::TickSize;
	using Proto::Results::TickGeneric;
	using Proto::Results::TickString;
	α Tick::ToProto()Ι->up<Jde::Markets::Proto::Tick>
	{
		auto y = mu<Jde::Markets::Proto::Tick>();
		y->set_ask( Ask );
		y->set_ask_size( ToDouble(AskSize) );
		y->set_bid( Bid );
		y->set_bid_size( ToDouble(BidSize) );
		y->set_high( High );
		y->set_last_price( LastPrice );
		y->set_last_size( ToDouble(LastSize) );
		y->set_low( Low );
		y->set_volume( ToDouble(Volume) );

		return y;
	}

	Proto::Results::MessageUnion Tick::ToProto( ETickType type )Ι
	{
		Proto::Results::MessageUnion msg;
		auto price = [type, &msg, id=ContractId](double v)mutable{ auto p = mu<TickPrice>(); p->set_request_id( id ); p->set_tick_type( type ); p->set_price( v ); /*p->set_allocated_attributes( pAttributes );*/ msg.set_allocated_tick_price(p.release()); };
		auto size  = [type, &msg, id=ContractId](Decimal v)mutable{ auto p = mu<TickSize>(); p->set_request_id( id ); p->set_tick_type( type ); p->set_size( ToDouble(v) ); msg.set_allocated_tick_size(p.release()); };
		auto dble  = [type, &msg, id=ContractId](double v)mutable{ auto p = mu<TickGeneric>(); p->set_request_id( id ); p->set_tick_type( type ); p->set_value( v ); msg.set_allocated_tick_generic(p.release()); };
		auto stng  = [type, &msg, id=ContractId](str v)mutable{ auto p = mu<TickString>(); p->set_request_id( id ); p->set_tick_type( type ); p->set_value( v ); msg.set_allocated_tick_string(p.release()); };
		auto option  = [type, &msg, id=ContractId](const OptionComputation& v)mutable{ auto p = v.ToProto( id, type ); msg.set_allocated_option_calculation(p.release()); };
		up<google::protobuf::Message> p;
		switch( type )
		{
		case ETickType::BidPrice: price( Bid ); break;
		case ETickType::AskPrice: price( Ask ); break;
		case ETickType::LastPrice: price( LastPrice ); break;

		case ETickType::BidSize: size( BidSize ); break;
		case ETickType::AskSize: size( AskSize ); break;
		case ETickType::Volume: size( Volume ); break;

		case ETickType::BidExchange: stng( BidExchange ); break;
		case ETickType::AskExchange: stng( AskExchange ); break;
		case ETickType::LastExchange: stng( LastExchange ); break;

		case ETickType::AverageVolume_: size( AverageVolume ); break;
		case ETickType::OPEN_INTEREST: size( (int)OPEN_INTEREST ); break;
		case ETickType::OPTION_CALL_OPEN_INTEREST: size( (int)OPTION_CALL_OPEN_INTEREST ); break;
		case ETickType::OPTION_PUT_OPEN_INTEREST: size( (int)OPTION_PUT_OPEN_INTEREST ); break;
		case ETickType::OPTION_CALL_VOLUME: size( (int)OPTION_CALL_VOLUME ); break;
		case ETickType::OPTION_PUT_VOLUME: size( (int)OPTION_PUT_VOLUME ); break;
		case ETickType::AUCTION_VOLUME: size( (int)AUCTION_VOLUME ); break;
		case ETickType::LastTimestamp: size( (int)LastTimestamp ); break;
		case ETickType::SHORTABLE: price( SHORTABLE ); break;
		case ETickType::FUNDAMENTAL_RATIOS: stng( RatioString ); break;
		case ETickType::RT_VOLUME: stng( RT_VOLUME ); break;
		case ETickType::Halted: price( Halted ); break;
		case ETickType::TRADE_COUNT: size( (int)TRADE_COUNT ); break;
		case ETickType::VOLUME_RATE: size( (int)VOLUME_RATE ); break;
		case ETickType::SHORT_TERM_VOLUME_3_MIN: size( (int)SHORT_TERM_VOLUME_3_MIN ); break;
		case ETickType::SHORT_TERM_VOLUME_5_MIN: size( (int)SHORT_TERM_VOLUME_5_MIN ); break;
		case ETickType::SHORT_TERM_VOLUME_10_MIN: size( (int)SHORT_TERM_VOLUME_10_MIN ); break;
		case ETickType::DELAYED_BID_SIZE: size( (int)DELAYED_BID_SIZE ); break;
		case ETickType::DELAYED_ASK_SIZE: size( (int)DELAYED_ASK_SIZE ); break;
		case ETickType::DELAYED_LAST_SIZE: size( (int)DELAYED_LAST_SIZE ); break;
		case ETickType::DELAYED_VOLUME: size( (int)DELAYED_VOLUME ); break;
		case ETickType::RT_TRD_VOLUME: size( (int)RT_TRD_VOLUME ); break;
		case ETickType::LAST_REG_TIME: size( (int)LAST_REG_TIME ); break;
		case ETickType::FUTURES_OPEN_INTEREST: size( (int)FUTURES_OPEN_INTEREST ); break;
		case ETickType::AVG_OPT_VOLUME: size( (int)AVG_OPT_VOLUME ); break;
		case ETickType::DELAYED_LAST_TIMESTAMP: size( (int)DELAYED_LAST_TIMESTAMP ); break;
		case ETickType::ShortableShares: size( ShortableShares ); break;
		case ETickType::NOT_SET: size( NOT_SET ); break;

		case ETickType::LastSize: size( LastSize ); break;
		case ETickType::High: price( High ); break;
		case ETickType::Low: price( Low ); break;
		case ETickType::ClosePrice: price( ClosePrice ); break;
		case ETickType::OpenTick: price( OpenTick ); break;
		case ETickType::Low13Week: price( Low13Week ); break;
		case ETickType::High13Week: price( High13Week ); break;
		case ETickType::Low26Week: price( Low26Week ); break;
		case ETickType::High26Week: price( High26Week ); break;
		case ETickType::Low52Week: price( Low52Week ); break;
		case ETickType::High52Week: price( High52Week ); break;
		case ETickType::OptionHistoricalVol: dble( OptionHistoricalVol ); break;
		case ETickType::OptionImpliedVol: dble( OptionImpliedVol ); break;
		case ETickType::OPTION_BID_EXCH: dble( OPTION_BID_EXCH ); break;
		case ETickType::OPTION_ASK_EXCH: dble( OPTION_ASK_EXCH ); break;
		case ETickType::INDEX_FUTURE_PREMIUM: dble( INDEX_FUTURE_PREMIUM ); break;
		case ETickType::AUCTION_PRICE: dble( AUCTION_PRICE ); break;
		case ETickType::AUCTION_IMBALANCE: dble( AUCTION_IMBALANCE ); break;
		case ETickType::MarkPrice: price( MarkPrice ); break;
		case ETickType::BID_EFP_COMPUTATION: dble( BID_EFP_COMPUTATION ); break;
		case ETickType::ASK_EFP_COMPUTATION: dble( ASK_EFP_COMPUTATION ); break;
		case ETickType::LAST_EFP_COMPUTATION: dble( LAST_EFP_COMPUTATION ); break;
		case ETickType::OPEN_EFP_COMPUTATION: dble( OPEN_EFP_COMPUTATION ); break;
		case ETickType::HIGH_EFP_COMPUTATION: dble( HIGH_EFP_COMPUTATION ); break;
		case ETickType::LOW_EFP_COMPUTATION: dble( LOW_EFP_COMPUTATION ); break;
		case ETickType::CLOSE_EFP_COMPUTATION: dble( CLOSE_EFP_COMPUTATION ); break;
		case ETickType::BID_YIELD: dble( BID_YIELD ); break;
		case ETickType::ASK_YIELD: dble( ASK_YIELD ); break;
		case ETickType::LAST_YIELD: dble( LAST_YIELD ); break;
		case ETickType::CUST_OPTION_COMPUTATION: dble( CUST_OPTION_COMPUTATION ); break;
		case ETickType::TRADE_RATE: dble( TRADE_RATE ); break;
		case ETickType::LAST_RTH_TRADE: dble( LAST_RTH_TRADE ); break;
		case ETickType::RT_HISTORICAL_VOL: dble( RT_HISTORICAL_VOL ); break;
		case ETickType::IB_DIVIDENDS: stng( DividendString ); break;
		case ETickType::BOND_FACTOR_MULTIPLIER: dble( BOND_FACTOR_MULTIPLIER ); break;
		case ETickType::REGULATORY_IMBALANCE: dble( REGULATORY_IMBALANCE ); break;
		case ETickType::DELAYED_BID: dble( DELAYED_BID ); break;
		case ETickType::DELAYED_ASK: dble( DELAYED_ASK ); break;
		case ETickType::DELAYED_LAST: dble( DELAYED_LAST ); break;
		case ETickType::DELAYED_HIGH: dble( DELAYED_HIGH ); break;
		case ETickType::DELAYED_LOW: dble( DELAYED_LOW ); break;
		case ETickType::DELAYED_CLOSE: dble( DELAYED_CLOSE ); break;
		case ETickType::DELAYED_OPEN: dble( DELAYED_OPEN ); break;
		case ETickType::CREDITMAN_MARK_PRICE: dble( CREDITMAN_MARK_PRICE ); break;
		case ETickType::CREDITMAN_SLOW_MARK_PRICE: dble( CREDITMAN_SLOW_MARK_PRICE ); break;
		case ETickType::DELAYED_BID_OPTION_COMPUTATION: dble( DELAYED_BID_OPTION_COMPUTATION ); break;
		case ETickType::DELAYED_ASK_OPTION_COMPUTATION: dble( DELAYED_ASK_OPTION_COMPUTATION ); break;
		case ETickType::DELAYED_LAST_OPTION_COMPUTATION: dble( DELAYED_LAST_OPTION_COMPUTATION ); break;
		case ETickType::DELAYED_MODEL_OPTION_COMPUTATION: dble( DELAYED_MODEL_OPTION_COMPUTATION ); break;
		case ETickType::NewsTick:
			ERR( "ETickType::NewsTick ToProto not implemented"sv );
			break;
		case ETickType::BID_OPTION_COMPUTATION: option( BID_OPTION_COMPUTATION ); break;
		case ETickType::ASK_OPTION_COMPUTATION: option( ASK_OPTION_COMPUTATION ); break;
		case ETickType::LAST_OPTION_COMPUTATION: option( LAST_OPTION_COMPUTATION ); break;
		case ETickType::MODEL_OPTION: option( MODEL_OPTION ); break;
		default:
			WARN( "Ticktype '{}' is not defined for proto."sv, Str::FromEnum(TickTypeStrings, type) );
		}
		return msg;
	}
	α Tick::AddProto( ETickType type, vector<Proto::Results::MessageUnion>& messages )Ι->void
	{
		if( type==ETickType::NewsTick && NewsPtr )
		{
			shared_lock l{ NewsPtr->Mutex };
			for( auto p = NewsPtr->begin(l); p!=NewsPtr->end(l); ++p )
			{
				Proto::Results::MessageUnion msg;
				msg.set_allocated_tick_news( NewsPtr->begin(l)->ToProto(ContractId).release() );
				messages.push_back( move(msg) );
			}
		}
		else
			messages.push_back( ToProto(type) );
	}
	α Tick::AllSet( Markets::Tick::Fields fields )Ι->bool
	{ 
		bool allSet = true;
		for( uint i=0; i<fields.size() && fields.any() && allSet; ++i )
		{
			if( !fields[i] )
				continue;
			allSet = Tick::IsSet( (ETickType)i );
			fields.flip( i );
		}
		return allSet;
	}

	Tick::TVariant Tick::Variant( ETickType type )Ι
	{
		TVariant result{nullptr};
		if( !_setFields[type] )
			return result;

		switch( type )
		{
		case ETickType::BidPrice: result = Bid; break;
		case ETickType::AskPrice: result = Ask; break;
		case ETickType::LastPrice: result = LastPrice; break;

		case ETickType::BidSize: result = BidSize; break;
		case ETickType::AskSize: result = AskSize; break;
		case ETickType::Volume: result = Volume; break;
		case ETickType::AverageVolume_: result = AverageVolume; break;
		case ETickType::OPEN_INTEREST: result = OPEN_INTEREST; break;
		case ETickType::OPTION_CALL_OPEN_INTEREST: result = OPTION_CALL_OPEN_INTEREST; break;
		case ETickType::OPTION_PUT_OPEN_INTEREST: result = OPTION_PUT_OPEN_INTEREST; break;
		case ETickType::OPTION_CALL_VOLUME: result = OPTION_CALL_VOLUME; break;
		case ETickType::OPTION_PUT_VOLUME: result = OPTION_PUT_VOLUME; break;
		case ETickType::AUCTION_VOLUME: result = AUCTION_VOLUME; break;
		case ETickType::LastTimestamp: result = LastTimestamp; break;
		case ETickType::SHORTABLE: result = SHORTABLE; break;
		case ETickType::FUNDAMENTAL_RATIOS: result = RatioString; break;
		case ETickType::RT_VOLUME: result = RT_VOLUME; break;
		case ETickType::Halted: result = Halted; break;
		case ETickType::TRADE_COUNT: result = TRADE_COUNT; break;
		case ETickType::VOLUME_RATE: result = VOLUME_RATE; break;
		case ETickType::SHORT_TERM_VOLUME_3_MIN: result = SHORT_TERM_VOLUME_3_MIN; break;
		case ETickType::SHORT_TERM_VOLUME_5_MIN: result = SHORT_TERM_VOLUME_5_MIN; break;
		case ETickType::SHORT_TERM_VOLUME_10_MIN: result = SHORT_TERM_VOLUME_10_MIN; break;
		case ETickType::DELAYED_BID_SIZE: result = DELAYED_BID_SIZE; break;
		case ETickType::DELAYED_ASK_SIZE: result = DELAYED_ASK_SIZE; break;
		case ETickType::DELAYED_LAST_SIZE: result = DELAYED_LAST_SIZE; break;
		case ETickType::DELAYED_VOLUME: result = DELAYED_VOLUME; break;
		case ETickType::RT_TRD_VOLUME: result = RT_TRD_VOLUME; break;
		case ETickType::LAST_REG_TIME: result = LAST_REG_TIME; break;
		case ETickType::FUTURES_OPEN_INTEREST: result = FUTURES_OPEN_INTEREST; break;
		case ETickType::AVG_OPT_VOLUME: result = AVG_OPT_VOLUME; break;
		case ETickType::DELAYED_LAST_TIMESTAMP: result = DELAYED_LAST_TIMESTAMP; break;
		case ETickType::ShortableShares: result = ShortableShares; break;
		case ETickType::NOT_SET: result = NOT_SET; break;

		case ETickType::LastSize: result = LastSize; break;
		case ETickType::High: result = High; break;
		case ETickType::Low: result = Low; break;
		case ETickType::ClosePrice: result = ClosePrice; break;
		case ETickType::OpenTick: result = OpenTick; break;
		case ETickType::Low13Week: result = Low13Week; break;
		case ETickType::High13Week: result = High13Week; break;
		case ETickType::Low26Week: result = Low26Week; break;
		case ETickType::High26Week: result = High26Week; break;
		case ETickType::Low52Week: result = Low52Week; break;
		case ETickType::High52Week: result = High52Week; break;
		case ETickType::OptionHistoricalVol: result = OptionHistoricalVol; break;
		case ETickType::OptionImpliedVol: result = OptionImpliedVol; break;
		case ETickType::OPTION_BID_EXCH: result = OPTION_BID_EXCH; break;
		case ETickType::OPTION_ASK_EXCH: result = OPTION_ASK_EXCH; break;
		case ETickType::INDEX_FUTURE_PREMIUM: result = INDEX_FUTURE_PREMIUM; break;
		case ETickType::AUCTION_PRICE: result = AUCTION_PRICE; break;
		case ETickType::AUCTION_IMBALANCE: result = AUCTION_IMBALANCE; break;
		case ETickType::MarkPrice: result = MarkPrice; break;
		case ETickType::BID_EFP_COMPUTATION: result = BID_EFP_COMPUTATION; break;
		case ETickType::ASK_EFP_COMPUTATION: result = ASK_EFP_COMPUTATION; break;
		case ETickType::LAST_EFP_COMPUTATION: result = LAST_EFP_COMPUTATION; break;
		case ETickType::OPEN_EFP_COMPUTATION: result = OPEN_EFP_COMPUTATION; break;
		case ETickType::HIGH_EFP_COMPUTATION: result = HIGH_EFP_COMPUTATION; break;
		case ETickType::LOW_EFP_COMPUTATION: result = LOW_EFP_COMPUTATION; break;
		case ETickType::CLOSE_EFP_COMPUTATION: result = CLOSE_EFP_COMPUTATION; break;
		case ETickType::BID_YIELD: result = BID_YIELD; break;
		case ETickType::ASK_YIELD: result = ASK_YIELD; break;
		case ETickType::LAST_YIELD: result = LAST_YIELD; break;
		case ETickType::CUST_OPTION_COMPUTATION: result = CUST_OPTION_COMPUTATION; break;
		case ETickType::TRADE_RATE: result = TRADE_RATE; break;
		case ETickType::LAST_RTH_TRADE: result = LAST_RTH_TRADE; break;
		case ETickType::RT_HISTORICAL_VOL: result = RT_HISTORICAL_VOL; break;
		case ETickType::IB_DIVIDENDS: result = DividendString; break;
		case ETickType::BOND_FACTOR_MULTIPLIER: result = BOND_FACTOR_MULTIPLIER; break;
		case ETickType::REGULATORY_IMBALANCE: result = REGULATORY_IMBALANCE; break;
		case ETickType::DELAYED_BID: result = DELAYED_BID; break;
		case ETickType::DELAYED_ASK: result = DELAYED_ASK; break;
		case ETickType::DELAYED_LAST: result = DELAYED_LAST; break;
		case ETickType::DELAYED_HIGH: result = DELAYED_HIGH; break;
		case ETickType::DELAYED_LOW: result = DELAYED_LOW; break;
		case ETickType::DELAYED_CLOSE: result = DELAYED_CLOSE; break;
		case ETickType::DELAYED_OPEN: result = DELAYED_OPEN; break;
		case ETickType::CREDITMAN_MARK_PRICE: result = CREDITMAN_MARK_PRICE; break;
		case ETickType::CREDITMAN_SLOW_MARK_PRICE: result = CREDITMAN_SLOW_MARK_PRICE; break;
		case ETickType::DELAYED_BID_OPTION_COMPUTATION: result = DELAYED_BID_OPTION_COMPUTATION; break;
		case ETickType::DELAYED_ASK_OPTION_COMPUTATION: result = DELAYED_ASK_OPTION_COMPUTATION; break;
		case ETickType::DELAYED_LAST_OPTION_COMPUTATION: result = DELAYED_LAST_OPTION_COMPUTATION; break;
		case ETickType::DELAYED_MODEL_OPTION_COMPUTATION: result = DELAYED_MODEL_OPTION_COMPUTATION; break;

		case ETickType::BidExchange: result = BidExchange; break;
		case ETickType::AskExchange: result = AskExchange; break;
		case ETickType::NewsTick: result = NewsPtr; break;
		case ETickType::LastExchange: result = LastExchange; break;
		case ETickType::BID_OPTION_COMPUTATION: result = BID_OPTION_COMPUTATION; break;
		case ETickType::ASK_OPTION_COMPUTATION: result = ASK_OPTION_COMPUTATION; break;
		case ETickType::LAST_OPTION_COMPUTATION: result = LAST_OPTION_COMPUTATION; break;
		case ETickType::MODEL_OPTION: result = MODEL_OPTION; break;

		default:
			WARN( "Ticktype {} is not defined for Variant."sv, Str::FromEnum(TickTypeStrings, type) );
		}
		return result;
	}
	α Tick::FieldEqual( const Tick& other, ETickType tick )Ι->bool
	{
		return Variant( tick )==other.Variant( tick );
	}

	α Tick::HasRatios()Ι->bool
	{
		bool fundamentals = _setFields[ETickType::FUNDAMENTAL_RATIOS];
		bool dividends = _setFields[ETickType::IB_DIVIDENDS];
//		DBG( "fundamentals={} && dividends={}", fundamentals, dividends );
		return fundamentals && dividends;
	}

	α Tick::Ratios()Ι->std::map<string,double>
	{
		std::map<string,double> values;
		{
			var dividendSplit = Str::Split( DividendString, ';' );
			for( var& subValue : dividendSplit )
			{
				var dividendValues = Str::Split( subValue );
				if( subValue==",,," )
				{
					values.emplace( "DIV_PAST_YEAR", 0.0 );
					values.emplace( "DIV_NEXT_YEAR", 0.0 );
					values.emplace( "DIV_NEXT_DAY", 0.0 );
					values.emplace( "DIV_NEXT", 0.0 );
				}
				else if( dividendValues.size()!=4 )
					DBG( "Could not convert '{}' to dividends."sv, subValue );
				else
				{
					values.emplace( "DIV_PAST_YEAR", To<double>(dividendValues[0]) );
					values.emplace( "DIV_NEXT_YEAR", To<double>(dividendValues[1]) );
					var& dateString = dividendValues[2];
					if( dateString.size()==8 )
					{
						const DateTime date( To<uint16>(dateString.substr(0,4)), To<uint8>(dateString.substr(4,2)), To<uint8>(dateString.substr(6,2)) );
						values.emplace( "DIV_NEXT_DAY", Chrono::ToDays(date.GetTimePoint()) );
					}
					else
						DBG( "Could not read next dividend day '{}'."sv, dateString );
					values.emplace( "DIV_NEXT", To<double>(dividendValues[3]) );
				}
			}
		}
		{
			var split = Str::Split( RatioString, ';' );
			for( var& subValue : split )
			{
				if( subValue.size()==0 )
					continue;
				var pair = Str::Split( subValue, '=' );
				if( pair.size()!=2 || pair[0]=="CURRENCY" )
					continue;
				try
				{
					values.emplace( pair[0], To<double>(pair[1]) );
				}
				catch( std::invalid_argument& )
				{
					DBG( "Could not convert [{}]='{}' to double."sv, pair[0], pair[1] );
				}
			}
		}
		return values;
	}
	α Tick::AddNews( News&& news )noexcept->void
	{
		if( !NewsPtr )
			NewsPtr = make_shared<Vector<News>>();
		NewsPtr->push_back( move(news) );
	}

	Tick::Fields Tick::PriceFields()noexcept
	{
		Fields fields;
		fields.set( ETickType::BidSize );
		fields.set( ETickType::BidPrice );
		fields.set( ETickType::AskPrice );
		fields.set( ETickType::AskSize );
		fields.set( ETickType::LastPrice );
		fields.set( ETickType::LastSize );
		fields.set( ETickType::High );
		fields.set( ETickType::Low );
		fields.set( ETickType::Volume );
		fields.set( ETickType::ClosePrice );
		fields.set( ETickType::OpenTick );
		return fields;
	}
	up<Proto::Results::TickNews> News::ToProto( ContractPK contractId )Ι
	{
		auto p = mu<Proto::Results::TickNews>();
		p->set_id( contractId );
		p->set_time( static_cast<uint32>(TimeStamp) );
		p->set_provider_code( ProviderCode );
		p->set_article_id( ArticleId );
		p->set_headline( Headline );
		p->set_extra_data( ExtraData );

		return p;
	}
	up<Proto::Results::OptionCalculation> OptionComputation::ToProto( ContractPK contractId, ETickType tickType )Ι
	{
		auto p = mu<Proto::Results::OptionCalculation>();
		p->set_request_id( contractId );
		p->set_tick_type( (ETickType)tickType );
		p->set_price_based( !ReturnBased );
		p->set_implied_volatility( ImpliedVol );
		p->set_delta( Delta );
		p->set_option_price( OptPrice );
		p->set_pv_dividend( PVDividend );
		p->set_gamma( Gamma );
		p->set_vega( Vega );
		p->set_theta( Theta );
		p->set_underlying_price( UndPrice );

		return p;
	}
}