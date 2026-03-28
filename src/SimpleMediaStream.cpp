#include "SimpleMediaStream.h"
#include <mfapi.h>
#include <mfidl.h>
#include <mferror.h>

// If mferror.h doesn't define it in some SDK versions, define it here.
#ifndef MF_E_STREAMSINK_STOPPED
#define MF_E_STREAMSINK_STOPPED ((HRESULT)0xC00D4E02L)
#endif

SimpleMediaStream::SimpleMediaStream() : m_isStarted(false), m_frameTimestamp(0) {
    InitializeCriticalSection(&m_critSec);
    MFCreateEventQueue(&m_pEventQueue);
}

SimpleMediaStream::~SimpleMediaStream() {
    Stop();
    DeleteCriticalSection(&m_critSec);
}

HRESULT SimpleMediaStream::Initialize(const std::wstring& videoPath) {
    return SetupSourceReader(videoPath);
}

HRESULT SimpleMediaStream::SetupSourceReader(const std::wstring& path) {
    ComPtr<IMFAttributes> pAttributes;
    MFCreateAttributes(&pAttributes, 1);
    pAttributes->SetUINT32(MF_SOURCE_READER_ENABLE_VIDEO_PROCESSING, 1);

    HRESULT hr = MFCreateSourceReaderFromURL(path.c_str(), pAttributes.Get(), &m_pReader);
    if (FAILED(hr)) return hr;

    // Force output to NV12 which is widely supported
    ComPtr<IMFMediaType> pType;
    MFCreateMediaType(&pType);
    pType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);
    pType->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_NV12);
    hr = m_pReader->SetCurrentMediaType(MF_SOURCE_READER_FIRST_VIDEO_STREAM, NULL, pType.Get());
    if (FAILED(hr)) return hr;

    // Create Stream Descriptor
    hr = m_pReader->GetCurrentMediaType(MF_SOURCE_READER_FIRST_VIDEO_STREAM, &pType);
    if (FAILED(hr)) return hr;
    
    IMFMediaType* pTypePtr = pType.Get();
    hr = MFCreateStreamDescriptor(1, 1, &pTypePtr, &m_pStreamDescriptor);
    return hr;
}

// --- IMFMediaEventGenerator Methods ---
STDMETHODIMP SimpleMediaStream::GetEvent(DWORD dwFlags, IMFMediaEvent** ppEvent) { return m_pEventQueue->GetEvent(dwFlags, ppEvent); }
STDMETHODIMP SimpleMediaStream::BeginGetEvent(IMFAsyncCallback* pCallback, IUnknown* punkState) { return m_pEventQueue->BeginGetEvent(pCallback, punkState); }
STDMETHODIMP SimpleMediaStream::EndGetEvent(IMFAsyncResult* pResult, IMFMediaEvent** ppEvent) { return m_pEventQueue->EndGetEvent(pResult, ppEvent); }
STDMETHODIMP SimpleMediaStream::QueueEvent(MediaEventType met, REFGUID guidExtendedType, HRESULT hrStatus, const PROPVARIANT* pValue) { return m_pEventQueue->QueueEventParamVar(met, guidExtendedType, hrStatus, pValue); }

// --- IMFMediaStream Methods ---
STDMETHODIMP SimpleMediaStream::GetMediaSource(IMFMediaSource** ppMediaSource) { return m_pParent.CopyTo(ppMediaSource); }
STDMETHODIMP SimpleMediaStream::GetStreamDescriptor(IMFStreamDescriptor** ppStreamDescriptor) { return m_pStreamDescriptor.CopyTo(ppStreamDescriptor); }

STDMETHODIMP SimpleMediaStream::RequestSample(IUnknown* pToken) {
    EnterCriticalSection(&m_critSec);
    if (!m_isStarted) {
        LeaveCriticalSection(&m_critSec);
        return MF_E_STREAMSINK_STOPPED;
    }
    HRESULT hr = DeliverSample();
    LeaveCriticalSection(&m_critSec);
    return hr;
}

HRESULT SimpleMediaStream::DeliverSample() {
    DWORD flags = 0;
    ComPtr<IMFSample> pSample;
    HRESULT hr = m_pReader->ReadSample(MF_SOURCE_READER_FIRST_VIDEO_STREAM, 0, nullptr, &flags, nullptr, &pSample);
    
    // If EOF, loop back
    if (flags & MF_SOURCE_READERF_ENDOFSTREAM) {
        PROPVARIANT var;
        var.vt = VT_I8;
        var.hVal.QuadPart = 0;
        m_pReader->SetCurrentPosition(GUID_NULL, var);
        return DeliverSample();
    }

    if (SUCCEEDED(hr) && pSample) {
        pSample->SetSampleTime(m_frameTimestamp);
        m_frameTimestamp += 333333; // ~30 FPS (1 tick = 100ns)
        QueueEvent(MEMediaSample, GUID_NULL, S_OK, (PROPVARIANT*)nullptr);
        m_pEventQueue->QueueEventParamUnk(MEMediaSample, GUID_NULL, S_OK, pSample.Get());
    }
    return hr;
}

HRESULT SimpleMediaStream::Start() {
    m_isStarted = true;
    QueueEvent(MEStreamStarted, GUID_NULL, S_OK, nullptr);
    return S_OK;
}

HRESULT SimpleMediaStream::Stop() {
    m_isStarted = false;
    QueueEvent(MEStreamStopped, GUID_NULL, S_OK, nullptr);
    return S_OK;
}

void SimpleMediaStream::SetParent(IMFMediaSource* pParent) {
    m_pParent = pParent;
}
