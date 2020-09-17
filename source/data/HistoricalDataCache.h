#pragma once
#include "../TypeDefs.h"
#include "../Exports.h"
#include "../types/proto/requests.pb.h"

namespace Jde::Markets
{
	struct Contract;
namespace HistoricalDataCache
{
	JDE_MARKETS_EXPORT MapPtr<DayIndex,VectorPtr<sp<::Bar>>> ReqHistoricalData( const Contract& contract, DayIndex current, uint dayCount, Proto::Requests::BarSize barSize, Proto::Requests::Display display, bool useRth )noexcept;
	JDE_MARKETS_EXPORT void Push( const Contract& contract, Proto::Requests::Display display, Proto::Requests::BarSize barSize, bool useRth, const vector<::Bar>& bars, DayIndex end, DayIndex subDayCount )noexcept;
}}