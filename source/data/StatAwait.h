#include "../../../Framework/source/coroutine/Awaitable.h"
#include "../../../Framework/source/math/MathUtilities.h"
#include "../types/Exchanges.h"

namespace Jde::Markets
{
	struct Contract; using ContractPtr_=sp<const Contract>;
	struct StatCount : public Math::StatResult<double>{ StatCount( Math::StatResult<double> x, uint y ):Math::StatResult<double>{x}, Count{y}{} const uint Count;};
}
namespace Jde::Markets::HistoricalDataCache
{
	struct ΓM StatAwait final : IAwait
	{
		using base=IAwait;
		StatAwait( ContractPtr_ pContract, double days, Day start, Day end )ι;
		~StatAwait();
		α await_ready()ι->bool override;
		α await_suspend( HCoroutine h )ι->void override;
		α await_resume()ι->Task::TResult override;
	private:
		ContractPtr_ ContractPtr; double Days; Day Start, End, FullDays; uint16 Minutes; string CacheId; Day Count;
		std::variant<sp<StatCount>,sp<IException>> _result;
		HCoroutine _h;
	};

	struct ΓM AthAwait final : IAwait
	{
		struct Result{ DayIndex LowDay{}; double Low{std::numeric_limits<double>::max()}; DayIndex HighDay{}; double High{}; double Average{}; };
		using base=IAwait;
		AthAwait( ContractPK id, TimePoint headTimestamp, DayIndex dayCount )ι:_contractId{ id }, _dayCount{ dayCount }, _headTimestamp{ headTimestamp }{};
		~AthAwait(){}
		α await_suspend( HCoroutine h )ι->void override;
		α await_resume()ι->Task::TResult override;
	private:
		//double _ath{}; DayIndex _day{};
		up<Proto::Results::ContractStats> _pResults;
		ContractPK _contractId;
		DayIndex _dayCount;
		TimePoint _headTimestamp;
		HCoroutine _h;
	};
}
namespace Jde::Markets
{
	Ξ ReqStats( ContractPtr_ pContract, double days, Day start, Day end=CurrentTradingDay() )ι{ return HistoricalDataCache::StatAwait{ pContract, days, start, end }; }
	Ξ ReqAth( ContractPK id, TimePoint headTimestamp, DayIndex dayCount=0 )ι{ return HistoricalDataCache::AthAwait{ id, headTimestamp, dayCount }; }
}