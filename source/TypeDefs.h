#pragma once

//#include "../../ai/dts/LightGbm/TypeDefs.h"
#include "../../Framework/source/math/Decimal.h"
#include <boost/container/flat_map.hpp>

namespace Jde::Markets
{
	using boost::container::flat_map;
	using boost::container::flat_set;
	using fmt::format;
	typedef int32 AccountPK;
	typedef long ContractPK;
	typedef int32 DecisionTreePK;
	typedef uint8 MetricPK;

	typedef long ReqId;//
	typedef unsigned long long OrderId;
	typedef uint16 MinuteIndex;//TODO change to int

	typedef Decimal2 Amount;
	typedef double PositionAmount;
	inline PositionAmount ToPosition( double value ){ return value; }
}
