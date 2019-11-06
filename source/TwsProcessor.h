#pragma once
#include "Exports.h"
class  EClientSocket;
struct EReaderSignal;
class EReader;
namespace Jde
{
	namespace Threading{ struct InterruptibleThread;}
namespace Markets
{
	struct JDE_MARKETS_EXPORT TwsProcessor final
	{
		~TwsProcessor();
		static void CreateInstance( sp<EClientSocket> pTwsClient, sp<EReaderSignal> pReaderSignal )noexcept;
		static bool IsConnected()noexcept{ return _pInstance && _pInstance->_isConnected; }
		static void Stop()noexcept;
	private:
		TwsProcessor( sp<EClientSocket> pTwsClient, sp<EReaderSignal> pReaderSignal )noexcept;
		void ProcessMessages( sp<EClientSocket> pTwsClient, sp<EReaderSignal> pReaderSignal )noexcept;

		sp<Threading::InterruptibleThread> _pThread;
		static sp<TwsProcessor> _pInstance;
		std::atomic<bool> _isConnected{false};
	};
}}