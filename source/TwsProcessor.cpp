#include "TwsProcessor.h"
#include <EClientSocket.h>
#include <EReaderSignal.h>
#include <EReader.h>
#include "client/TwsClient.h"

namespace Jde::Markets
{
	sp<TwsProcessor> TwsProcessor::_pInstance{nullptr};
	void TwsProcessor::CreateInstance( sp<TwsClient> pTwsClient, sp<EReaderSignal> pReaderSignal )noexcept
	{
		DBG0( "TwsProcessor::CreateInstance"sv );
		Stop();
		_pInstance = sp<TwsProcessor>{ new TwsProcessor{pTwsClient, pReaderSignal} };
	}

	TwsProcessor::TwsProcessor( sp<TwsClient> pTwsClient, sp<EReaderSignal> pReaderSignal )noexcept:
		_pThread{ make_unique<Threading::InterruptibleThread>( "TwsProcessor", [&,p=pTwsClient, pReaderSignal](){ProcessMessages(p, pReaderSignal);} )  }
	{}

	TwsProcessor::~TwsProcessor()
	{
		if( _pThread )
		{
			_pThread->Interrupt();
			_pThread->Join();
		}
	}

	void TwsProcessor::Stop()noexcept
	{
		DBG( "TwsProcessor::Stop _pInstance={}"sv, _pInstance!=nullptr );
		if( _pInstance )
		{
			DBG0( "TwsProcessor::Stop - AddThread"sv );
			IApplication::AddThread( _pInstance->_pThread );
			_pInstance->_pThread->Interrupt();
			if( TwsClient::HasInstance() )
				TwsClient::Instance().reqCurrentTime();
		}
		DBG0( "Leaving TwsProcessor::Stop"sv );
	}
	void TwsProcessor::ProcessMessages( sp<TwsClient> pTwsClient, sp<EReaderSignal> pReaderSignal )noexcept
	{
		Threading::SetThreadDscrptn( "IBMessageProcessor" );
		EReader reader( pTwsClient.get(), pReaderSignal.get() );
		reader.start();
		_isConnected = true;
		DBG( "Enter TwsProcessor::ProcessMessages IsConnected = {}, Threading::GetThreadInterruptFlag().IsSet={}"sv, (bool)pTwsClient->isConnected(), Threading::GetThreadInterruptFlag().IsSet() );
		while( pTwsClient->isConnected() && !Threading::GetThreadInterruptFlag().IsSet() )
		{
			pReaderSignal->waitForSignal();
			reader.processMsgs();
			pTwsClient->CheckTimeouts();
		}
		DBG( "pTwsClient->isConnected={}, Threading::GetThreadInterruptFlag().IsSet={}"sv, pTwsClient->isConnected(), Threading::GetThreadInterruptFlag().IsSet() );
		_isConnected = false;
		DBG( "Leaving TwsProcessor::ProcessMessages IsConnected = {}"sv, (bool)_isConnected );
	}
}
