#include <jde/markets/types/MyOrder.h>
#include "OrderEnums.h"
//#include "proto/requests.pb.h"


#define var const auto
namespace Jde
{
	α Markets::ToProto( const ::OrderState& state )noexcept->up<Markets::Proto::Results::OrderState>
	{
		auto p = mu<Proto::Results::OrderState>();
		p->set_status( state.status );
		constexpr sv NotSet = "1.7976931348623157E308";
		p->set_init_margin_before( state.initMarginBefore==NotSet ? "" : state.initMarginBefore );
		p->set_maint_margin_before( state.maintMarginBefore==NotSet ? "" : state.maintMarginBefore );
		p->set_equity_with_loan_before( state.equityWithLoanBefore==NotSet ? "" : state.equityWithLoanBefore );
		p->set_init_margin_change( state.initMarginChange==NotSet ? "" : state.initMarginChange );
		p->set_maint_margin_change( state.maintMarginChange==NotSet ? "" : state.maintMarginChange );
		p->set_equity_with_loan_change( state.equityWithLoanChange==NotSet ? "" : state.equityWithLoanChange );
		p->set_init_margin_after( state.initMarginAfter==NotSet ? "" : state.initMarginAfter );
		p->set_maint_margin_after( state.maintMarginAfter==NotSet ? "" : state.maintMarginAfter );
		p->set_equity_with_loan_after( state.equityWithLoanAfter==NotSet ? "" : state.equityWithLoanAfter );
		var max = std::numeric_limits<double>::max();
		p->set_commission( state.commission==max ? nan("") : state.commission );
		p->set_min_commission( state.minCommission==max ? nan("") : state.minCommission );
		p->set_max_commission( state.maxCommission==max ? nan("") : state.maxCommission );
		p->set_commission_currency( state.commissionCurrency );
		p->set_warning_text( state.warningText );
		p->set_completed_time( state.completedTime );
		p->set_completed_status( state.completedStatus );

		return p;
	}
}
namespace Jde::Markets
{
	MyOrder::MyOrder( ::OrderId id, const Proto::Order& proto )noexcept
	{
		orderId = id;
		clientId = proto.client_id();
		permId = proto.perm_id();
		IsBuy( proto.is_buy() );
		totalQuantity = ToDecimal( proto.quantity() );
		orderType = ToOrderTypeString( proto.type() );
		if( proto.limit() )
			lmtPrice = proto.limit();
		if( proto.aux() )
			auxPrice = proto.aux();
		TimeInForce( proto.time_in_force() );
		activeStartTime = ToDateString( proto.active_start_time() ); // for GTC orders
		activeStopTime = ToDateString( proto.active_stop_time() );
		ocaGroup = proto.oca_group(); // one cancels all group name
		ocaType = proto.oca_type(); // 1 = CANCEL_WITH_BLOCK, 2 = REDUCE_WITH_BLOCK, 3 = REDUCE_NON_BLOCK
		orderRef = proto.order_ref();      // order reference
		transmit = proto.transmit();      // if false, order will be created but not transmited
		parentId = proto.parent_id();      // Parent order Id, to associate Auto STP or TRAIL orders with the original order.
		blockOrder = proto.block_order();
		sweepToFill = proto.sweep_to_fill();
		displaySize = proto.display_size();
		triggerMethod = proto.trigger_method(); // 0=Default, 1=Double_Bid_Ask, 2=Last, 3=Double_Last, 4=Bid_Ask, 7=Last_or_Bid_Ask, 8=Mid-point
		outsideRth = proto.outside_rth();
		hidden = proto.hidden();
		goodAfterTime = ToDateString( proto.good_after_time() );    // Format: 20060505 08:00:00 {time zone}
		goodTillDate = ToDateString( proto.good_till_date() );     // Format: 20060505 08:00:00 {time zone}
		if( proto.rule_80a().length() )
			rule80A = proto.rule_80a(); // Individual = 'I', Agency = 'A', AgentOtherMember = 'W', IndividualPTIA = 'J', AgencyPTIA = 'U', AgentOtherMemberPTIA = 'M', IndividualPT = 'K', AgencyPT = 'Y', AgentOtherMemberPT = 'N'
		allOrNone = proto.all_or_none();
		if( proto.min_qty() )
			minQty = proto.min_qty();//UNSET_INTEGER;
		if( proto.percent_offset() )
			percentOffset = proto.percent_offset(); //UNSET_DOUBLE; // REL orders only
		overridePercentageConstraints = proto.override_percentage_constraints();
		if( proto.trail_stop_price() )
			trailStopPrice = proto.trail_stop_price(); //UNSET_DOUBLE; TRAILLIMIT orders only
		if( proto.trailing_percent() )
			trailingPercent = proto.trailing_percent();//UNSET_DOUBLE;

		faGroup = proto.fa_group();
		faProfile = proto.fa_profile();
		faMethod = proto.fa_method();
		faPercentage = proto.fa_percentage();

		auctionStrategy = proto.auction_strategy(); // AUCTION_MATCH, AUCTION_IMPROVEMENT, AUCTION_TRANSPARENT
		if( proto.starting_price() && !isnan(proto.starting_price()) ) startingPrice = proto.starting_price();
		if( proto.stock_ref_price() && !isnan(proto.stock_ref_price()) ) stockRefPrice = proto.stock_ref_price();
		if( proto.delta() && !isnan(proto.delta()) ) delta = proto.delta();
		if( proto.stock_range_lower() && !isnan(proto.stock_range_lower()) ) stockRangeLower = proto.stock_range_lower();
		if( proto.stock_range_upper() && !isnan(proto.stock_range_upper()) ) stockRangeUpper = proto.stock_range_upper();
		randomizeSize = proto.randomize_size();
		randomizePrice = proto.randomize_price();

		if( proto.volatility() && !isnan(proto.volatility()) ) volatility = proto.volatility();
		if( proto.volatility_type() )
		{
			WARN( "volatilityType={}"sv, proto.volatility_type() );
			volatilityType = proto.volatility_type();// 1=daily, 2=annual
		}
		deltaNeutralOrderType = proto.delta_neutral_order_type();
		if( proto.delta_neutral_aux_price() && !isnan(proto.delta_neutral_aux_price()) ) deltaNeutralAuxPrice = proto.delta_neutral_aux_price();
		deltaNeutralConId = proto.delta_neutral_con_id();

		whatIf = proto.what_if();
		account = proto.account();

		usePriceMgmtAlgo = proto.use_price_mngmnt_algrthm()==2 ? UsePriceMmgtAlgo::DONT_USE : proto.use_price_mngmnt_algrthm()==1 ? UsePriceMmgtAlgo::USE : UsePriceMmgtAlgo::DEFAULT;

	/*	go with defaults for now
		openClose = proto.open_close(); // institutional (ie non-cleared) only O=Open, C=Close
		origin = proto.origin();    // 0=Customer, 1=Firm
		shortSaleSlot = proto.short_sale_slot(); // 1 if you hold the shares, 2 if they will be delivered from elsewhere.  Only for Action="SSHORT
		designatedLocation = proto.designated_location(); // set when slot=2 only.
		exemptCode = proto.exempt_code();

		// SMART routing only
		discretionaryAmt = proto.discretionary_amt();
		eTradeOnly = proto.etrade_only();
		firmQuoteOnly = proto.firm_quote_only();
		nbboPriceCap = proto.nbbo_price_cap();
		optOutSmartRouting = proto.opt_out_smart_routing();
	*/
		//TODO rest of variables
	}
	α MyOrder::ToProto()const noexcept->up<Proto::Order>
	{
		auto p = make_unique<Proto::Order>();
		auto& proto = *p;
		proto.set_id( orderId );
		proto.set_client_id(clientId);
		proto.set_perm_id(permId);
		proto.set_is_buy( IsBuy() );
		proto.set_quantity( ToDouble(totalQuantity) );
		proto.set_type( ToOrderType(orderType) );
		proto.set_limit( lmtPrice==UNSET_DOUBLE ? 0 : lmtPrice );
		proto.set_aux( auxPrice==UNSET_DOUBLE ? 0 : auxPrice );
		proto.set_time_in_force( TimeInForce() );
		if( activeStartTime.size() )
			proto.set_active_start_time( static_cast<int32>(ParseDateTime(activeStartTime)) ); // for GTC orders
		if( activeStopTime.size() )
			proto.set_active_stop_time(static_cast<int32>(ParseDateTime(activeStopTime)) );
		proto.set_oca_group( ocaGroup ); // one cancels all group name
		proto.set_oca_type( ocaType ); // 1 = CANCEL_WITH_BLOCK, 2 = REDUCE_WITH_BLOCK, 3 = REDUCE_NON_BLOCK
		proto.set_order_ref( orderRef );      // order reference
		proto.set_transmit( transmit );      // if false, order will be created but not transmited
		proto.set_parent_id( parentId );      // Parent order Id, to associate Auto STP or TRAIL orders with the original order.
		proto.set_block_order( blockOrder );
		proto.set_sweep_to_fill( sweepToFill );
		proto.set_display_size( displaySize );
		proto.set_trigger_method( triggerMethod ); // 0=Default, 1=Double_Bid_Ask, 2=Last, 3=Double_Last, 4=Bid_Ask, 7=Last_or_Bid_Ask, 8=Mid-point
		proto.set_outside_rth( outsideRth );
		proto.set_hidden( hidden );
		if( goodAfterTime.size() )
			proto.set_good_after_time(static_cast<int32>(ParseDateTime(goodAfterTime)) );    // Format: 20060505 08:00:00 {time zone}
		if( goodTillDate.size() )
			proto.set_good_till_date(static_cast<int32>(ParseDateTime(goodTillDate)) );     // Format: 20060505 08:00:00 {time zone}
		proto.set_rule_80a( rule80A ); // Individual = 'I', Agency = 'A', AgentOtherMember = 'W', IndividualPTIA = 'J', AgencyPTIA = 'U', AgentOtherMemberPTIA = 'M', IndividualPT = 'K', AgencyPT = 'Y', AgentOtherMemberPT = 'N'
		proto.set_all_or_none( allOrNone );
		proto.set_min_qty( minQty==UNSET_INTEGER ? 0 : minQty );
		proto.set_percent_offset( percentOffset==UNSET_DOUBLE ? 0 : percentOffset );
		proto.set_override_percentage_constraints( overridePercentageConstraints );
		proto.set_trail_stop_price( trailStopPrice==UNSET_DOUBLE ? 0 : trailStopPrice );
		proto.set_trailing_percent( trailingPercent==UNSET_DOUBLE ? 0 : trailingPercent );
		proto.set_fa_group(faGroup);
		proto.set_fa_profile( faProfile );
		proto.set_fa_method( faMethod );
		proto.set_fa_percentage( faPercentage );

		proto.set_open_close( openClose ); // institutional (ie non-cleared) only O=Open, C=Close
		proto.set_origin( origin );    // 0=Customer, 1=Firm
		proto.set_short_sale_slot( shortSaleSlot ); // 1 if you hold the shares, 2 if they will be delivered from elsewhere.  Only for Action="SSHORT
		proto.set_designated_location( designatedLocation ); // set when slot=2 only.
		proto.set_exempt_code( exemptCode );

		// SMART routing only
		proto.set_discretionary_amt( discretionaryAmt );
		//proto.set_etrade_only( eTradeOnly );
		//proto.set_firm_quote_only( firmQuoteOnly );
		//proto.set_nbbo_price_cap( nbboPriceCap );
		proto.set_opt_out_smart_routing( optOutSmartRouting );

		proto.set_auction_strategy( auctionStrategy ); // AUCTION_MATCH, AUCTION_IMPROVEMENT, AUCTION_TRANSPARENT
		proto.set_starting_price( startingPrice==UNSET_DOUBLE ? nan("") : startingPrice );
		proto.set_stock_ref_price( stockRefPrice==UNSET_DOUBLE ? nan("") : stockRefPrice );
		proto.set_delta( delta==UNSET_DOUBLE ? nan("") : delta );
		proto.set_stock_range_lower( stockRangeLower==UNSET_DOUBLE ? nan("") : stockRangeLower );
		proto.set_stock_range_upper( stockRangeUpper==UNSET_DOUBLE ? nan("") : stockRangeUpper );
		proto.set_randomize_size( randomizeSize );
		proto.set_randomize_price( randomizePrice );

		proto.set_volatility( deltaNeutralAuxPrice==UNSET_DOUBLE ? nan("") : volatility );
		proto.set_volatility_type( volatilityType==UNSET_INTEGER ? 0 : volatilityType );// 1=daily, 2=annual
		proto.set_delta_neutral_order_type( deltaNeutralOrderType );
		proto.set_delta_neutral_aux_price( deltaNeutralAuxPrice==UNSET_DOUBLE ? nan("") : deltaNeutralAuxPrice );
		proto.set_delta_neutral_con_id( deltaNeutralConId );

		proto.set_what_if( whatIf );
		proto.set_account( account );

		proto.set_use_price_mngmnt_algrthm( usePriceMgmtAlgo==UsePriceMmgtAlgo::USE ? 1 : usePriceMgmtAlgo==UsePriceMmgtAlgo::DONT_USE ? 2 : 0 );

		return p;
	}
	α MyOrder::TimeInForce()const noexcept->Proto::ETimeInForce
	{
		return Str::ToEnum<Proto::ETimeInForce>( ETifStrings, tif ).value_or( Proto::ETimeInForce::DayTif );
	}

	α MyOrder::TimeInForce( Proto::ETimeInForce value )noexcept->void
	{
		tif = static_cast<uint>(value)<ETifStrings.size() ? ETifStrings[value] : "";
	}
	α MyOrder::OrderType()const noexcept->Proto::EOrderType
	{
		return Str::ToEnum<Proto::EOrderType>( EOrderTypeStrings, orderType ).value_or( Proto::EOrderType::Limit );
	}
	α MyOrder::OrderType( Proto::EOrderType value )noexcept->void
	{
		orderType = ToOrderTypeString( value );
	}
	α MyOrder::ParseDateTime( const string& date )noexcept->time_t
	{
		//20060505 08:00:00 {time zone}
		auto time = DateTime{ (uint16)stoi(date.substr(0,4)), (uint8)stoi(date.substr(4,2)), (uint8)stoi(date.substr(6,2)), (uint8)stoi(date.substr(9,2)), (uint8)stoi(date.substr(13,2)), (uint8)stoi(date.substr(15,2)) }.GetTimePoint();
		if( date.size()>17 )
		{
			var timezone = date.substr(17);
			if( timezone=="EST" || timezone=="EDT" )
				time+=Timezone::EasternTimezoneDifference(time);
			else if( timezone!="GMT" && timezone!="UTC" )
				ERR( "non-implemented timezone {}"sv, timezone );
		}
		return Clock::to_time_t( time );
	}
	α MyOrder::ToDateString( time_t date )noexcept->string
	{
		string result;
		if( date )
		{
			DateTime time{date};
			result = fmt::format( "{}{:0>2}{:0>2} {:0>2}:{:0>2}:{:0>2} GMT", time.Year(), time.Month(), time.Day(), time.Hour(), time.Minute(), time.Second() );
		}
		return result;
	}

	α MyOrder::Changes( const MyOrder& rhs, Fields fields )const noexcept->MyOrder::Fields
	{
		auto changes = Fields::None;
		if( LastUpdate!=rhs.LastUpdate ) changes|=Fields::LastUpdate;
		if( lmtPrice!=rhs.lmtPrice ) changes|=Fields::Limit;
		if( totalQuantity!=rhs.totalQuantity ) changes|=Fields::Quantity;
		if( action!=rhs.action ) changes|=Fields::Action;
		if( orderType!=rhs.orderType ) changes|=Fields::Type;
		if( auxPrice!=rhs.auxPrice ) changes|=Fields::Aux;
		if( transmit!=rhs.transmit ) changes|=Fields::Transmit;
		if( account!=rhs.account ) changes|=Fields::Account;
		return changes;
	}

	α OrderStatus::Changes( const OrderStatus& status )const noexcept->OrderStatus::Fields
	{
		auto changes = Fields::None;
		if( Status!=status.Status ) changes|=Fields::Status;
		if( Filled!=status.Filled ) changes|=Fields::Filled;
		if( Remaining!=status.Remaining ) changes|=Fields::Remaining;
		return changes;
	}

	α OrderStatus::ToProto()const noexcept->up<Proto::Results::OrderStatus>
	{
		auto p = make_unique<Proto::Results::OrderStatus>();
		p->set_order_id( Id );
		p->set_status( Status );
		p->set_filled( Filled );
		p->set_remaining( Remaining );
		p->set_average_fill_price( AverageFillPrice );
		p->set_perm_id( PermId );
		p->set_parent_id( ParentId );
		p->set_last_fill_price( LastFillPrice );
		p->set_why_held( WhyHeld );
		p->set_market_cap_price( MarketCapPrice );
		return p;
	}

	α OrderStateChanges( const ::OrderState& a, const ::OrderState& b )noexcept->OrderStateFields
	{
		auto changes = OrderStateFields::None;
		if( a.status!=b.status ) changes|=OrderStateFields::Status;
		if( a.completedTime!=b.completedTime ) changes|=OrderStateFields::CompletedTime;
		if( a.completedStatus!=b.completedStatus ) changes|=OrderStateFields::CompletedStatus;
		return changes;
	}


}