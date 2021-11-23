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
		Ω CreateInstance( sp<TwsClient> pTwsClient, sp<EReaderSignal> pReaderSignal )noexcept->void;
		Ω IsConnected()noexcept->bool{ return _pInstance && _pInstance->_isConnected; }
		Ω Stop()noexcept->void;
	private:
		TwsProcessor( sp<TwsClient> pTwsClient, sp<EReaderSignal> pReaderSignal )noexcept;
		α ProcessMessages( sp<TwsClient> pTwsClient, sp<EReaderSignal> pReaderSignal )noexcept->void;

		sp<Threading::InterruptibleThread> _pThread;
		static sp<TwsProcessor> _pInstance;
		std::atomic<bool> _isConnected{false};
	};
}