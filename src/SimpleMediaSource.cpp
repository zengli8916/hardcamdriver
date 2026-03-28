#include "SimpleMediaSource.h"
#include "SimpleMediaStream.h"
#include <mfapi.h>
#include <mferror.h>

SimpleMediaSource::SimpleMediaSource() : m_isShutdown(false) {
    MFCreateEventQueue(&m_pEventQueue);
}

SimpleMediaSource::~SimpleMediaSource() {
    Shutdown();
}

HRESULT SimpleMediaSource::Initialize(const std::wstring& videoPath) {
    m_videoPath = videoPath;
    m_pStream = Make<SimpleMediaStream>();
    HRESULT hr = m_pStream->Initialize(videoPath);
    if (FAILED(hr)) return hr;

    m_pStream->SetParent(this);
    return CreatePresentationDescriptor();
}

HRESULT SimpleMediaSource::CreatePresentationDescriptor() {
    ComPtr<IMFStreamDescriptor> pSD;
    HRESULT hr = m_pStream->GetStreamDescriptor(&pSD);
    if (FAILED(hr)) return hr;

    IMFStreamDescriptor* pSDPtr = pSD.Get();
    hr = MFCreatePresentationDescriptor(1, &pSDPtr, &m_pPresentationDescriptor);
    if (FAILED(hr)) return hr;

    return m_pPresentationDescriptor->SelectStream(0);
}

// --- IMFMediaEventGenerator Methods (Standard Implementation) ---
STDMETHODIMP SimpleMediaSource::GetEvent(DWORD dwFlags, IMFMediaEvent** ppEvent) { return m_pEventQueue->GetEvent(dwFlags, ppEvent); }
STDMETHODIMP SimpleMediaSource::BeginGetEvent(IMFAsyncCallback* pCallback, IUnknown* punkState) { return m_pEventQueue->BeginGetEvent(pCallback, punkState); }
STDMETHODIMP SimpleMediaSource::EndGetEvent(IMFAsyncResult* pResult, IMFMediaEvent** ppEvent) { return m_pEventQueue->EndGetEvent(pResult, ppEvent); }
STDMETHODIMP SimpleMediaSource::QueueEvent(MediaEventType met, REFGUID guidExtendedType, HRESULT hrStatus, const PROPVARIANT* pValue) { return m_pEventQueue->QueueEventParamVar(met, guidExtendedType, hrStatus, pValue); }

// --- IMFMediaSource Methods ---
STDMETHODIMP SimpleMediaSource::GetCharacteristics(DWORD* pdwCharacteristics) {
    *pdwCharacteristics = MFMEDIASOURCE_IS_LIVE; // Fake it as a live camera
    return S_OK;
}

STDMETHODIMP SimpleMediaSource::CreatePresentationDescriptor(IMFPresentationDescriptor** ppPresentationDescriptor) {
    if (m_isShutdown) return MF_E_SHUTDOWN;
    return m_pPresentationDescriptor.CopyTo(ppPresentationDescriptor);
}

STDMETHODIMP SimpleMediaSource::Start(IMFPresentationDescriptor* pPresentationDescriptor, const GUID* pguidTimeFormat, const PROPVARIANT* pvarStartPosition) {
    if (m_isShutdown) return MF_E_SHUTDOWN;
    HRESULT hr = m_pStream->Start();
    if (SUCCEEDED(hr)) {
        QueueEvent(MESourceStarted, GUID_NULL, S_OK, pvarStartPosition);
    }
    return hr;
}

STDMETHODIMP SimpleMediaSource::Stop() {
    m_pStream->Stop();
    QueueEvent(MESourceStopped, GUID_NULL, S_OK, nullptr);
    return S_OK;
}

STDMETHODIMP SimpleMediaSource::Pause() { return E_NOTIMPL; }
STDMETHODIMP SimpleMediaSource::Shutdown() {
    m_isShutdown = true;
    if (m_pEventQueue) m_pEventQueue->Shutdown();
    if (m_pStream) m_pStream->Stop();
    return S_OK;
}
