#pragma once
#include <windows.h>
#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>
#include <wrl.h>
#include <string>

using namespace Microsoft::WRL;

class SimpleMediaStream : public RuntimeClass<RuntimeClassFlags<ClassicCom>, IMFMediaStream> {
public:
    SimpleMediaStream();
    virtual ~SimpleMediaStream();

    HRESULT Initialize(const std::wstring& videoPath);

    // IMFMediaEventGenerator
    STDMETHODIMP GetEvent(DWORD dwFlags, IMFMediaEvent** ppEvent) override;
    STDMETHODIMP BeginGetEvent(IMFAsyncCallback* pCallback, IUnknown* punkState) override;
    STDMETHODIMP EndGetEvent(IMFAsyncResult* pResult, IMFMediaEvent** ppEvent) override;
    STDMETHODIMP QueueEvent(MediaEventType met, REFGUID guidExtendedType, HRESULT hrStatus, const PROPVARIANT* pValue) override;

    // IMFMediaStream
    STDMETHODIMP GetMediaSource(IMFMediaSource** ppMediaSource) override;
    STDMETHODIMP GetStreamDescriptor(IMFStreamDescriptor** ppStreamDescriptor) override;
    STDMETHODIMP RequestSample(IUnknown* pToken) override;

    // Control methods
    HRESULT Start();
    HRESULT Stop();
    void SetParent(IMFMediaSource* pParent);

private:
    HRESULT DeliverSample();
    HRESULT SetupSourceReader(const std::wstring& path);

    ComPtr<IMFMediaEventQueue> m_pEventQueue;
    ComPtr<IMFMediaSource> m_pParent;
    ComPtr<IMFSourceReader> m_pReader;
    ComPtr<IMFStreamDescriptor> m_pStreamDescriptor;
    
    CRITICAL_SECTION m_critSec;
    bool m_isStarted;
    UINT64 m_frameTimestamp;
};
