#include "proto/ib.pb.h"

namespace Jde::Markets
{
	namespace Proto
	{
		//namespace Requests{  }
		namespace Results{ class OrderState; }
		namespace IB{ enum ETimeInForce:int; class Order; }
	}

	struct MyOrder : public ibapi::Order
	{
		MyOrder( ibapi::OrderId orderId, const Proto::Order& order );
		MyOrder( const ibapi::Order& order ):ibapi::Order{order}{}
		bool IsBuy()const noexcept{ return action=="BUY"; } void IsBuy( bool value)noexcept{ action = value ? "BUY" : "SELL"; }
		Proto::ETimeInForce TimeInForce()const noexcept; void TimeInForce( Proto::ETimeInForce value )noexcept;
		Proto::EOrderType OrderType()const noexcept; void OrderType( Proto::EOrderType value )noexcept;
		sp<Proto::Order> ToProto( bool stupidPointer=false )const noexcept;
		static time_t ParseDateTime( const string& date )noexcept;
		static string ToDateString( time_t date )noexcept;
		static Proto::Results::OrderState* ToAllocatedProto( const ibapi::OrderState& state )noexcept;
	};
}