#pragma once
#include <windows.h>
#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>
#include <wrl.h>
#include <string>

using namespace Microsoft::WRL;

class SimpleMediaStream;

class SimpleMediaSource : public RuntimeClass<RuntimeClassFlags<ClassicCom>, IMFMediaSource> {
public:
    SimpleMediaSource();
    virtual ~SimpleMediaSource();

    HRESULT Initialize(const std::wstring& videoPath);

    // IMFMediaEventGenerator
    STDMETHODIMP GetEvent(DWORD dwFlags, IMFMediaEvent** ppEvent) override;
    STDMETHODIMP BeginGetEvent(IMFAsyncCallback* pCallback, IUnknown* punkState) override;
    STDMETHODIMP EndGetEvent(IMFAsyncResult* pResult, IMFMediaEvent** ppEvent) override;
    STDMETHODIMP QueueEvent(MediaEventType met, REFGUID guidExtendedType, HRESULT hrStatus, const PROPVARIANT* pValue) override;

    // IMFMediaSource
    STDMETHODIMP GetCharacteristics(DWORD* pdwCharacteristics) override;
    STDMETHODIMP CreatePresentationDescriptor(IMFPresentationDescriptor** ppPresentationDescriptor) override;
    STDMETHODIMP Start(IMFPresentationDescriptor* pPresentationDescriptor, const GUID* pguidTimeFormat, const PROPVARIANT* pvarStartPosition) override;
    STDMETHODIMP Stop() override;
    STDMETHODIMP Pause() override;
    STDMETHODIMP Shutdown() override;

private:
    HRESULT CreatePresentationDescriptor();

    ComPtr<IMFMediaEventQueue> m_pEventQueue;
    ComPtr<SimpleMediaStream> m_pStream;
    ComPtr<IMFPresentationDescriptor> m_pPresentationDescriptor;
    std::wstring m_videoPath;
    bool m_isShutdown;
};
