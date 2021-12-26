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
		StatAwait( ContractPtr_ pContract, double days, Day start, Day end )noexcept;
		~StatAwait();
		α await_ready()noexcept->bool override;
		α await_suspend( HCoroutine h )noexcept->void override;
		α await_resume()noexcept->Task::TResult override;
	private:
		ContractPtr_ ContractPtr; double Days; Day Start, End, FullDays; uint16 Minutes; string CacheId; Day Count;
		std::variant<sp<StatCount>,sp<IException>> _result;
		HCoroutine _h;
	};
}
namespace Jde::Markets
{
	Ξ ReqStats( ContractPtr_ pContract, double days, Day start, Day end=CurrentTradingDay() )noexcept{ return HistoricalDataCache::StatAwait{ pContract, days, start, end }; }
}
