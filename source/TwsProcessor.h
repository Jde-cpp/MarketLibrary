#pragma once
#include <jde/markets/Exports.h>
class  EClientSocket;
struct EReaderSignal;
class EReader;

namespace Jde::Threading{ struct InterruptibleThread;}
namespace Jde::Markets
{
	struct TwsClient;
	struct ΓM TwsProcessor final
	{
		~TwsProcessor();
		static void CreateInstance( sp<TwsClient> pTwsClient, sp<EReaderSignal> pReaderSignal )noexcept;
		static bool IsConnected()noexcept{ return _pInstance && _pInstance->_isConnected; }
		static void Stop()noexcept;
	private:
		TwsProcessor( sp<TwsClient> pTwsClient, sp<EReaderSignal> pReaderSignal )noexcept;
		void ProcessMessages( sp<TwsClient> pTwsClient, sp<EReaderSignal> pReaderSignal )noexcept;

		sp<Threading::InterruptibleThread> _pThread;
		static sp<TwsProcessor> _pInstance;
		std::atomic<bool> _isConnected{false};
	};
}