/*
 Copyright (c) 2025, The Cinder Project
 All rights reserved.

 Redistribution and use in source and binary forms, with or without modification, are permitted provided that
 the following conditions are met:

    * Redistributions of source code must retain the above copyright notice, this list of conditions and
	the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and
	the following disclaimer in the documentation and/or other materials provided with the distribution.

 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
 TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 POSSIBILITY OF SUCH DAMAGE.
*/

#include "cinder/msw/DirectShowCapture.h"
#include "cinder/Exception.h"
#include "cinder/MediaTime.h"
#include <dvdmedia.h>
#include <algorithm>
#include <sstream>
#include <mutex>
#include <set>
#include <tuple>
#include <string>

// DirectShow sample grabber interface definitions (qedit.h not available)
static const GUID CLSID_SampleGrabber = { 0xC1F400A0, 0x3F08, 0x11d3, { 0x9F, 0x0B, 0x00, 0x60, 0x08, 0x03, 0x9E, 0x37 } };
static const GUID IID_ISampleGrabber = { 0x6B652FFF, 0x11FE, 0x4fce, { 0x92, 0xAD, 0x02, 0x66, 0xB5, 0xD7, 0xC7, 0x8F } };
static const GUID IID_ISampleGrabberCB = { 0x0579154A, 0x2B53, 0x4994, { 0xB0, 0xD0, 0xE7, 0x73, 0x14, 0x8E, 0xFF, 0x85 } };
static const GUID CLSID_NullRenderer = { 0xC1F400A4, 0x3F08, 0x11d3, { 0x9F, 0x0B, 0x00, 0x60, 0x08, 0x03, 0x9E, 0x37 } };

// Missing DirectShow GUID constants
#ifndef MEDIASUBTYPE_I420
static const GUID MEDIASUBTYPE_I420 = { 0x30323449, 0x0000, 0x0010, { 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71 } };
#endif

// ISampleGrabberCB interface definition
MIDL_INTERFACE("0579154A-2B53-4994-B0D0-E773148EFF85")
ISampleGrabberCB : public IUnknown
{
public:
    virtual HRESULT STDMETHODCALLTYPE SampleCB(double SampleTime, IMediaSample *pSample) = 0;
    virtual HRESULT STDMETHODCALLTYPE BufferCB(double SampleTime, BYTE *pBuffer, long BufferLen) = 0;
};

// ISampleGrabber interface definition  
MIDL_INTERFACE("6B652FFF-11FE-4fce-92AD-0266B5D7C78F")
ISampleGrabber : public IUnknown
{
public:
    virtual HRESULT STDMETHODCALLTYPE SetOneShot(BOOL OneShot) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetMediaType(const AM_MEDIA_TYPE *pType) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetConnectedMediaType(AM_MEDIA_TYPE *pType) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetBufferSamples(BOOL BufferThem) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetCurrentBuffer(long *pBufferSize, long *pBuffer) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetCurrentSample(IMediaSample **ppSample) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetCallback(ISampleGrabberCB *pCallback, long WhichMethodToCallback) = 0;
};

#pragma comment(lib, "strmiids.lib")
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "oleaut32.lib")

namespace cinder {

// Static member initialization
std::vector<DirectShowCapture::DeviceInfo> DirectShowCapture::sDeviceCache;
bool DirectShowCapture::sDeviceCacheValid = false;
int DirectShowCapture::ComInitializer::sRefCount = 0;

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ComInitializer implementation

DirectShowCapture::ComInitializer::ComInitializer() : mInitialized(false) {
    if (sRefCount == 0) {
        HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
        if (SUCCEEDED(hr)) {
            mInitialized = true;
        }
    } else {
        mInitialized = true;
    }
    sRefCount++;
}

DirectShowCapture::ComInitializer::~ComInitializer() {
    sRefCount--;
    if (sRefCount == 0 && mInitialized) {
        CoUninitialize();
    }
}


//////////////////////////////////////////////////////////////////////////////////////////////////////////////
// SampleGrabberCallback implementation

class DirectShowCapture::SampleGrabberCallback : public ISampleGrabberCB {
public:
    SampleGrabberCallback(DirectShowCapture* parent);
    virtual ~SampleGrabberCallback();
    
    // IUnknown methods
    STDMETHODIMP QueryInterface(REFIID riid, void** ppv) override;
    STDMETHODIMP_(ULONG) AddRef() override;
    STDMETHODIMP_(ULONG) Release() override;
    
    // ISampleGrabberCB methods
    STDMETHODIMP SampleCB(double sampleTime, IMediaSample* sample) override;
    STDMETHODIMP BufferCB(double sampleTime, BYTE* buffer, long bufferLen) override;
    
private:
    DirectShowCapture* mParent;
    LONG mRefCount;
};

DirectShowCapture::SampleGrabberCallback::SampleGrabberCallback(DirectShowCapture* parent)
    : mParent(parent), mRefCount(1) {
}

DirectShowCapture::SampleGrabberCallback::~SampleGrabberCallback() {
}

STDMETHODIMP DirectShowCapture::SampleGrabberCallback::QueryInterface(REFIID riid, void** ppv) {
    if (riid == IID_ISampleGrabberCB || riid == IID_IUnknown) {
        *ppv = static_cast<ISampleGrabberCB*>(this);
        AddRef();
        return S_OK;
    }
    return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG) DirectShowCapture::SampleGrabberCallback::AddRef() {
    return InterlockedIncrement(&mRefCount);
}

STDMETHODIMP_(ULONG) DirectShowCapture::SampleGrabberCallback::Release() {
    LONG count = InterlockedDecrement(&mRefCount);
    // Note: We don't delete this object here because it's managed by unique_ptr
    // in the parent DirectShowCapture object. The parent will clean it up properly.
    return count;
}

STDMETHODIMP DirectShowCapture::SampleGrabberCallback::SampleCB(double sampleTime, IMediaSample* sample) {
    return S_OK;
}

STDMETHODIMP DirectShowCapture::SampleGrabberCallback::BufferCB(double sampleTime, BYTE* buffer, long bufferLen) {
    if (!mParent || !buffer || bufferLen <= 0) {
        return S_OK;
    }
    
    try {
        std::lock_guard<std::mutex> lock(mParent->mFrameMutex);
        
        int expectedSize = mParent->getSize();
        OutputDebugStringA(("BufferCB called: bufferLen=" + std::to_string(bufferLen) + ", expected=" + std::to_string(expectedSize) + "\n").c_str());
        
        if (mParent->mPixelBuffer) {
            // DirectShow has already converted to RGB24 format
            int copySize = std::min(static_cast<int>(bufferLen), expectedSize);
            memcpy(mParent->mPixelBuffer.get(), buffer, copySize);
            
            if (bufferLen < expectedSize) {
                memset(mParent->mPixelBuffer.get() + bufferLen, 0, expectedSize - bufferLen);
                OutputDebugStringA("BufferCB: Padding with zeros\n");
            }
            
            mParent->mNewFrameAvailable = true;
            OutputDebugStringA("BufferCB: RGB24 data copied successfully\n");
        } else {
            OutputDebugStringA("BufferCB: No pixel buffer allocated\n");
        }
    } catch (...) {
        // Ignore any exceptions to prevent crashes in DirectShow callback
        OutputDebugStringA("BufferCB: Exception caught, ignoring\n");
    }
    
    return S_OK;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
// DirectShowCapture implementation

DirectShowCapture::DirectShowCapture() {
    OutputDebugStringA("DirectShowCapture constructor starting\n");
    
    if (!mComInit.isInitialized()) {
        OutputDebugStringA("DirectShowCapture: COM initialization failed\n");
        throw Exception("Failed to initialize COM");
    }
    
    InitializeCriticalSection(&mCriticalSection);
    mCriticalSectionInitialized = true;
    OutputDebugStringA("DirectShowCapture constructor completed\n");
}

DirectShowCapture::~DirectShowCapture() {
    stop();
    
    if (mCriticalSectionInitialized) {
        DeleteCriticalSection(&mCriticalSection);
    }
}

std::vector<DirectShowCapture::DeviceInfo> DirectShowCapture::enumerateDevices() {
    refreshDeviceCache();
    return sDeviceCache;
}

std::vector<std::string> DirectShowCapture::getDeviceNames() {
    auto devices = enumerateDevices();
    std::vector<std::string> names;
    names.reserve(devices.size());
    
    for (const auto& device : devices) {
        names.push_back(device.friendlyName);
    }
    
    return names;
}

int DirectShowCapture::getDeviceCount() {
    return static_cast<int>(enumerateDevices().size());
}

void DirectShowCapture::refreshDeviceCache() {
    if (sDeviceCacheValid) {
        return;
    }
    
    sDeviceCache.clear();
    OutputDebugStringA("DirectShow: Starting device enumeration\n");
    
    // Initialize COM for this thread if needed
    HRESULT hrCom = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    bool comInitialized = SUCCEEDED(hrCom);
    if (comInitialized) {
        OutputDebugStringA("DirectShow: COM initialized successfully\n");
    } else {
        OutputDebugStringA("DirectShow: COM initialization failed or already initialized\n");
    }
    
    ComPtr<ICreateDevEnum> deviceEnum;
    HRESULT hr = CoCreateInstance(CLSID_SystemDeviceEnum, nullptr, CLSCTX_INPROC,
                                  IID_ICreateDevEnum, reinterpret_cast<void**>(&deviceEnum));
    
    if (FAILED(hr)) {
        return;
    }
    
    IEnumMoniker* enumMonikerPtr = nullptr;
    hr = deviceEnum->CreateClassEnumerator(CLSID_VideoInputDeviceCategory, &enumMonikerPtr, 0);
    ComPtr<IEnumMoniker> enumMoniker(enumMonikerPtr);
    
    if (hr != S_OK) {
        return;
    }
    
    IMoniker* moniker = nullptr;
    ULONG fetched = 0;
    int deviceIndex = 0;
    
    while (enumMoniker->Next(1, &moniker, &fetched) == S_OK) {
        ComPtr<IPropertyBag> propertyBag;
        hr = moniker->BindToStorage(0, 0, IID_IPropertyBag, reinterpret_cast<void**>(&propertyBag));
        
        if (SUCCEEDED(hr)) {
            DeviceInfo deviceInfo;
            
            VARIANT variant;
            VariantInit(&variant);
            
            hr = propertyBag->Read(L"FriendlyName", &variant, 0);
            if (SUCCEEDED(hr)) {
                if (variant.vt == VT_BSTR) {
                    int len = WideCharToMultiByte(CP_UTF8, 0, variant.bstrVal, -1, nullptr, 0, nullptr, nullptr);
                    if (len > 0) {
                        deviceInfo.friendlyName.resize(len - 1);
                        WideCharToMultiByte(CP_UTF8, 0, variant.bstrVal, -1, &deviceInfo.friendlyName[0], len, nullptr, nullptr);
                    }
                }
                VariantClear(&variant);
            }
            
            hr = propertyBag->Read(L"DevicePath", &variant, 0);
            if (SUCCEEDED(hr)) {
                if (variant.vt == VT_BSTR) {
                    int len = WideCharToMultiByte(CP_UTF8, 0, variant.bstrVal, -1, nullptr, 0, nullptr, nullptr);
                    if (len > 0) {
                        deviceInfo.devicePath.resize(len - 1);
                        WideCharToMultiByte(CP_UTF8, 0, variant.bstrVal, -1, &deviceInfo.devicePath[0], len, nullptr, nullptr);
                    }
                }
                VariantClear(&variant);
            }
            
            sDeviceCache.push_back(std::move(deviceInfo));
            OutputDebugStringA(("DirectShow: Found device: " + deviceInfo.friendlyName + "\n").c_str());
        }
        
        moniker->Release();
        deviceIndex++;
    }
    
    OutputDebugStringA(("DirectShow: Device enumeration complete. Found " + std::to_string(sDeviceCache.size()) + " devices\n").c_str());
    
    // Clean up COM if we initialized it
    if (comInitialized) {
        CoUninitialize();
    }
    
    sDeviceCacheValid = true;
}

std::vector<Capture::Mode> DirectShowCapture::getDeviceModes(int deviceId) {
    auto devices = enumerateDevices();
    if (deviceId < 0 || deviceId >= static_cast<int>(devices.size())) {
        return {};
    }
    
    std::vector<Capture::Mode> modes;
    auto formats = getDeviceFormats(deviceId);
    
    std::set<std::tuple<int, int, Capture::Mode::PixelFormat>> uniqueModes;
    
    for (const auto& format : formats) {
        Capture::Mode::PixelFormat pixelFormat = mediaSubtypeToPixelFormat(format.mediaType);
        
        auto key = std::make_tuple(format.width, format.height, pixelFormat);
        if (uniqueModes.find(key) == uniqueModes.end()) {
            uniqueModes.insert(key);
            
            double frameRate = 30.0;
            if (format.averageTimePerFrame > 0) {
                frameRate = 10000000.0 / format.averageTimePerFrame;
            }
            
            MediaTime frameTime(1.0 / frameRate);
            modes.push_back(Capture::Mode(
                format.width,
                format.height,
                frameTime,
                Capture::Mode::Codec::Uncompressed,
                pixelFormat,
                format.description
            ));
        }
    }
    
    if (modes.empty()) {
        modes.push_back(Capture::Mode(
            640, 480,
            MediaTime(1.0 / 30.0),
            Capture::Mode::Codec::Uncompressed,
            Capture::Mode::PixelFormat::BGR24,
            "640x480 30fps BGR24 (fallback)"
        ));
    }
    
    return modes;
}

std::vector<DirectShowCapture::StreamFormat> DirectShowCapture::getDeviceFormats(int deviceId) {
    std::vector<StreamFormat> formats;
    
    auto sourceFilter = createSourceFilter(deviceId);
    if (!sourceFilter) {
        return formats;
    }
    
    ComPtr<IPin> outputPin;
    IEnumPins* enumPinsPtr = nullptr;
    HRESULT hr = sourceFilter->EnumPins(&enumPinsPtr);
    ComPtr<IEnumPins> enumPins(enumPinsPtr);
    
    if (FAILED(hr)) {
        return formats;
    }
    
    IPin* pin = nullptr;
    while (enumPins->Next(1, &pin, nullptr) == S_OK) {
        PIN_DIRECTION direction;
        hr = pin->QueryDirection(&direction);
        
        if (SUCCEEDED(hr) && direction == PINDIR_OUTPUT) {
            outputPin.reset(pin);
            break;
        }
        
        pin->Release();
    }
    
    if (!outputPin) {
        return formats;
    }
    
    ComPtr<IAMStreamConfig> streamConfig;
    hr = outputPin->QueryInterface(IID_IAMStreamConfig, reinterpret_cast<void**>(&streamConfig));
    
    if (FAILED(hr)) {
        return formats;
    }
    
    int count = 0, size = 0;
    hr = streamConfig->GetNumberOfCapabilities(&count, &size);
    
    if (FAILED(hr) || size != sizeof(VIDEO_STREAM_CONFIG_CAPS)) {
        return formats;
    }
    
    for (int i = 0; i < count; i++) {
        AM_MEDIA_TYPE* mediaType = nullptr;
        VIDEO_STREAM_CONFIG_CAPS caps;
        
        hr = streamConfig->GetStreamCaps(i, &mediaType, reinterpret_cast<BYTE*>(&caps));
        
        if (SUCCEEDED(hr) && mediaType) {
            if (mediaType->majortype == MEDIATYPE_Video && 
                mediaType->formattype == FORMAT_VideoInfo && 
                mediaType->cbFormat >= sizeof(VIDEOINFOHEADER)) {
                
                VIDEOINFOHEADER* vih = reinterpret_cast<VIDEOINFOHEADER*>(mediaType->pbFormat);
                
                StreamFormat format;
                format.width = vih->bmiHeader.biWidth;
                format.height = abs(vih->bmiHeader.biHeight);
                format.mediaType = mediaType->subtype;
                format.averageTimePerFrame = static_cast<long>(vih->AvgTimePerFrame);
                
                std::ostringstream desc;
                desc << format.width << "x" << format.height;
                if (format.averageTimePerFrame > 0) {
                    double fps = 10000000.0 / format.averageTimePerFrame;
                    desc << " " << static_cast<int>(fps + 0.5) << "fps";
                }
                desc << " " << guidToString(format.mediaType);
                format.description = desc.str();
                
                formats.push_back(format);
            }
            
            if (mediaType->cbFormat != 0) {
                CoTaskMemFree(mediaType->pbFormat);
            }
            if (mediaType->pUnk != nullptr) {
                mediaType->pUnk->Release();
            }
            CoTaskMemFree(mediaType);
        }
    }
    
    return formats;
}

std::vector<std::pair<int, int>> DirectShowCapture::getDeviceResolutions(int deviceId) {
    std::vector<std::pair<int, int>> resolutions;
    auto formats = getDeviceFormats(deviceId);
    
    std::set<std::pair<int, int>> uniqueResolutions;
    for (const auto& format : formats) {
        uniqueResolutions.insert({format.width, format.height});
    }
    
    resolutions.assign(uniqueResolutions.begin(), uniqueResolutions.end());
    return resolutions;
}

DirectShowCapture::ComPtr<IBaseFilter> DirectShowCapture::createSourceFilter(int deviceId) {
    auto devices = enumerateDevices();
    if (deviceId < 0 || deviceId >= static_cast<int>(devices.size())) {
        return nullptr;
    }
    
    ComPtr<ICreateDevEnum> deviceEnum;
    HRESULT hr = CoCreateInstance(CLSID_SystemDeviceEnum, nullptr, CLSCTX_INPROC,
                                  IID_ICreateDevEnum, reinterpret_cast<void**>(&deviceEnum));
    
    if (FAILED(hr)) {
        return nullptr;
    }
    
    IEnumMoniker* enumMonikerPtr = nullptr;
    hr = deviceEnum->CreateClassEnumerator(CLSID_VideoInputDeviceCategory, &enumMonikerPtr, 0);
    ComPtr<IEnumMoniker> enumMoniker(enumMonikerPtr);
    
    if (hr != S_OK) {
        return nullptr;
    }
    
    IMoniker* moniker = nullptr;
    ULONG fetched = 0;
    int currentIndex = 0;
    
    while (enumMoniker->Next(1, &moniker, &fetched) == S_OK && currentIndex <= deviceId) {
        if (currentIndex == deviceId) {
            ComPtr<IBaseFilter> sourceFilter;
            hr = moniker->BindToObject(0, 0, IID_IBaseFilter, reinterpret_cast<void**>(&sourceFilter));
            moniker->Release();
            
            if (SUCCEEDED(hr)) {
                return sourceFilter;
            }
            break;
        }
        
        moniker->Release();
        currentIndex++;
    }
    
    return nullptr;
}

GUID DirectShowCapture::pixelFormatToMediaSubtype(Capture::Mode::PixelFormat format) {
    switch (format) {
        case Capture::Mode::PixelFormat::RGB24:
            return MEDIASUBTYPE_RGB24;
        case Capture::Mode::PixelFormat::BGR24:
            return MEDIASUBTYPE_RGB24;
        case Capture::Mode::PixelFormat::ARGB32:
            return MEDIASUBTYPE_RGB32;
        case Capture::Mode::PixelFormat::BGRA32:
            return MEDIASUBTYPE_RGB32;
        case Capture::Mode::PixelFormat::YUV420P:
            return MEDIASUBTYPE_IYUV;
        case Capture::Mode::PixelFormat::NV12:
            return MEDIASUBTYPE_NV12;
        case Capture::Mode::PixelFormat::YUY2:
            return MEDIASUBTYPE_YUY2;
        case Capture::Mode::PixelFormat::UYVY:
            return MEDIASUBTYPE_UYVY;
        case Capture::Mode::PixelFormat::I420:
            return MEDIASUBTYPE_I420;
        case Capture::Mode::PixelFormat::YV12:
            return MEDIASUBTYPE_YV12;
        default:
            return MEDIASUBTYPE_RGB24;
    }
}

Capture::Mode::PixelFormat DirectShowCapture::mediaSubtypeToPixelFormat(const GUID& subtype) {
    if (IsEqualGUID(subtype, MEDIASUBTYPE_RGB24)) {
        return Capture::Mode::PixelFormat::RGB24;
    } else if (IsEqualGUID(subtype, MEDIASUBTYPE_RGB32)) {
        return Capture::Mode::PixelFormat::ARGB32;
    } else if (IsEqualGUID(subtype, MEDIASUBTYPE_YUY2)) {
        return Capture::Mode::PixelFormat::YUY2;
    } else if (IsEqualGUID(subtype, MEDIASUBTYPE_UYVY)) {
        return Capture::Mode::PixelFormat::UYVY;
    } else if (IsEqualGUID(subtype, MEDIASUBTYPE_IYUV)) {
        return Capture::Mode::PixelFormat::YUV420P;
    } else if (IsEqualGUID(subtype, MEDIASUBTYPE_YV12)) {
        return Capture::Mode::PixelFormat::YV12;
    } else if (IsEqualGUID(subtype, MEDIASUBTYPE_NV12)) {
        return Capture::Mode::PixelFormat::NV12;
    } else {
        return Capture::Mode::PixelFormat::BGR24;
    }
}

std::string DirectShowCapture::guidToString(const GUID& guid) {
    if (IsEqualGUID(guid, MEDIASUBTYPE_RGB24)) return "RGB24";
    if (IsEqualGUID(guid, MEDIASUBTYPE_RGB32)) return "RGB32";
    if (IsEqualGUID(guid, MEDIASUBTYPE_YUY2)) return "YUY2";
    if (IsEqualGUID(guid, MEDIASUBTYPE_UYVY)) return "UYVY";
    if (IsEqualGUID(guid, MEDIASUBTYPE_IYUV)) return "IYUV";
    if (IsEqualGUID(guid, MEDIASUBTYPE_YV12)) return "YV12";
    if (IsEqualGUID(guid, MEDIASUBTYPE_NV12)) return "NV12";
    return "Unknown";
}

bool DirectShowCapture::setupDevice(int deviceId) {
    return setupDevice(deviceId, 640, 480);
}

bool DirectShowCapture::setupDevice(int deviceId, int width, int height) {
    return setupDevice(deviceId, width, height, MEDIASUBTYPE_RGB24);
}

bool DirectShowCapture::setupDevice(int deviceId, int width, int height, const GUID& mediaType) {
    stop();
    
    mDevice.deviceId = deviceId;
    mWidth = width;
    mHeight = height;
    
    if (!createCaptureGraph()) {
        return false;
    }
    
    if (!connectFilters()) {
        return false;
    }
    
    if (!setStreamFormat(width, height, mediaType)) {
        GUID fallbackType = mediaType;
        int fallbackWidth = width;
        int fallbackHeight = height;
        
        if (!findClosestFormat(fallbackWidth, fallbackHeight, fallbackType)) {
            return false;
        }
        
        if (!setStreamFormat(fallbackWidth, fallbackHeight, fallbackType)) {
            return false;
        }
        
        mWidth = fallbackWidth;
        mHeight = fallbackHeight;
    }
    
    if (!configureSampleGrabber()) {
        return false;
    }
    
    mPixelBuffer = std::make_unique<unsigned char[]>(getSize());
    mDevice.isSetup = true;
    
    return true;
}

bool DirectShowCapture::setupDevice(int deviceId, const Capture::Mode& mode) {
    GUID mediaType = pixelFormatToMediaSubtype(mode.getPixelFormat());
    return setupDevice(deviceId, mode.getWidth(), mode.getHeight(), mediaType);
}

bool DirectShowCapture::createCaptureGraph() {
    HRESULT hr = CoCreateInstance(CLSID_CaptureGraphBuilder2, nullptr, CLSCTX_INPROC,
                                  IID_ICaptureGraphBuilder2, reinterpret_cast<void**>(&mDevice.captureBuilder));
    
    if (FAILED(hr)) {
        return false;
    }
    
    hr = CoCreateInstance(CLSID_FilterGraph, nullptr, CLSCTX_INPROC,
                          IID_IGraphBuilder, reinterpret_cast<void**>(&mDevice.graphBuilder));
    
    if (FAILED(hr)) {
        return false;
    }
    
    hr = mDevice.captureBuilder->SetFiltergraph(mDevice.graphBuilder.get());
    
    if (FAILED(hr)) {
        return false;
    }
    
    hr = mDevice.graphBuilder->QueryInterface(IID_IMediaControl, reinterpret_cast<void**>(&mDevice.mediaControl));
    
    return SUCCEEDED(hr);
}

bool DirectShowCapture::connectFilters() {
    mDevice.sourceFilter = createSourceFilter(mDevice.deviceId);
    if (!mDevice.sourceFilter) {
        return false;
    }
    
    HRESULT hr = mDevice.graphBuilder->AddFilter(mDevice.sourceFilter.get(), L"Video Capture Source");
    if (FAILED(hr)) {
        return false;
    }
    
    mDevice.grabberFilter = createGrabberFilter();
    if (!mDevice.grabberFilter) {
        return false;
    }
    
    hr = mDevice.graphBuilder->AddFilter(mDevice.grabberFilter.get(), L"Sample Grabber");
    if (FAILED(hr)) {
        return false;
    }
    
    hr = mDevice.grabberFilter->QueryInterface(IID_ISampleGrabber, reinterpret_cast<void**>(&mDevice.sampleGrabber));
    if (FAILED(hr)) {
        return false;
    }
    
    // Configure SampleGrabber to convert to RGB24 BEFORE connecting filters
    AM_MEDIA_TYPE mediaType;
    ZeroMemory(&mediaType, sizeof(mediaType));
    mediaType.majortype = MEDIATYPE_Video;
    mediaType.subtype = MEDIASUBTYPE_RGB24;
    mediaType.formattype = GUID_NULL;
    
    hr = mDevice.sampleGrabber->SetMediaType(&mediaType);
    if (FAILED(hr)) {
        OutputDebugStringA("DirectShow: Failed to set RGB24 media type on SampleGrabber\n");
        return false;
    }
    OutputDebugStringA("DirectShow: Set SampleGrabber to request RGB24 format\n");
    
    ComPtr<IPin> sourcePin;
    IEnumPins* enumPinsPtr = nullptr;
    hr = mDevice.sourceFilter->EnumPins(&enumPinsPtr);
    ComPtr<IEnumPins> enumPins(enumPinsPtr);
    
    if (FAILED(hr)) {
        return false;
    }
    
    IPin* pin = nullptr;
    while (enumPins->Next(1, &pin, nullptr) == S_OK) {
        PIN_DIRECTION direction;
        hr = pin->QueryDirection(&direction);
        
        if (SUCCEEDED(hr) && direction == PINDIR_OUTPUT) {
            sourcePin.reset(pin);
            break;
        }
        
        pin->Release();
    }
    
    if (!sourcePin) {
        return false;
    }
    
    hr = sourcePin->QueryInterface(IID_IAMStreamConfig, reinterpret_cast<void**>(&mDevice.streamConfig));
    if (FAILED(hr)) {
        return false;
    }
    
    // Now connect the source filter to the sample grabber
    ComPtr<IPin> grabberInputPin;
    IEnumPins* grabberEnumPinsPtr = nullptr;
    hr = mDevice.grabberFilter->EnumPins(&grabberEnumPinsPtr);
    ComPtr<IEnumPins> grabberEnumPins(grabberEnumPinsPtr);
    
    if (FAILED(hr)) {
        return false;
    }
    
    IPin* grabberPin = nullptr;
    while (grabberEnumPins->Next(1, &grabberPin, nullptr) == S_OK) {
        PIN_DIRECTION direction;
        hr = grabberPin->QueryDirection(&direction);
        
        if (SUCCEEDED(hr) && direction == PINDIR_INPUT) {
            grabberInputPin.reset(grabberPin);
            break;
        }
        
        grabberPin->Release();
    }
    
    if (!grabberInputPin) {
        return false;
    }
    
    // Connect source output to grabber input
    hr = mDevice.graphBuilder->Connect(sourcePin.get(), grabberInputPin.get());
    if (FAILED(hr)) {
        return false;
    }
    
    // Add null renderer for the grabber output
    ComPtr<IBaseFilter> nullRenderer;
    hr = CoCreateInstance(CLSID_NullRenderer, nullptr, CLSCTX_INPROC,
                          IID_IBaseFilter, reinterpret_cast<void**>(&nullRenderer));
    if (FAILED(hr)) {
        return false;
    }
    
    hr = mDevice.graphBuilder->AddFilter(nullRenderer.get(), L"Null Renderer");
    if (FAILED(hr)) {
        return false;
    }
    
    // Find grabber output pin and null renderer input pin, then connect them
    ComPtr<IPin> grabberOutputPin;
    IEnumPins* grabberOutEnumPinsPtr = nullptr;
    hr = mDevice.grabberFilter->EnumPins(&grabberOutEnumPinsPtr);
    ComPtr<IEnumPins> grabberOutEnumPins(grabberOutEnumPinsPtr);
    
    if (SUCCEEDED(hr)) {
        IPin* outPin = nullptr;
        while (grabberOutEnumPins->Next(1, &outPin, nullptr) == S_OK) {
            PIN_DIRECTION direction;
            hr = outPin->QueryDirection(&direction);
            
            if (SUCCEEDED(hr) && direction == PINDIR_OUTPUT) {
                grabberOutputPin.reset(outPin);
                break;
            }
            
            outPin->Release();
        }
    }
    
    ComPtr<IPin> rendererInputPin;
    IEnumPins* rendererEnumPinsPtr = nullptr;
    hr = nullRenderer->EnumPins(&rendererEnumPinsPtr);
    ComPtr<IEnumPins> rendererEnumPins(rendererEnumPinsPtr);
    
    if (SUCCEEDED(hr)) {
        IPin* inPin = nullptr;
        while (rendererEnumPins->Next(1, &inPin, nullptr) == S_OK) {
            PIN_DIRECTION direction;
            hr = inPin->QueryDirection(&direction);
            
            if (SUCCEEDED(hr) && direction == PINDIR_INPUT) {
                rendererInputPin.reset(inPin);
                break;
            }
            
            inPin->Release();
        }
    }
    
    if (grabberOutputPin && rendererInputPin) {
        hr = mDevice.graphBuilder->Connect(grabberOutputPin.get(), rendererInputPin.get());
    }
    
    return SUCCEEDED(hr);
}

DirectShowCapture::ComPtr<IBaseFilter> DirectShowCapture::createGrabberFilter() {
    ComPtr<IBaseFilter> grabberFilter;
    
    HRESULT hr = CoCreateInstance(CLSID_SampleGrabber, nullptr, CLSCTX_INPROC,
                                  IID_IBaseFilter, reinterpret_cast<void**>(&grabberFilter));
    
    return SUCCEEDED(hr) ? std::move(grabberFilter) : nullptr;
}

bool DirectShowCapture::setStreamFormat(int width, int height, const GUID& mediaType) {
    OutputDebugStringA(("DirectShow: setStreamFormat requesting " + std::to_string(width) + "x" + std::to_string(height) + " format=" + guidToString(mediaType) + "\n").c_str());
    
    if (!mDevice.streamConfig) {
        OutputDebugStringA("DirectShow: setStreamFormat failed - no stream config\n");
        return false;
    }
    
    AM_MEDIA_TYPE* currentMediaType = nullptr;
    HRESULT hr = mDevice.streamConfig->GetFormat(&currentMediaType);
    
    if (FAILED(hr)) {
        OutputDebugStringA(("DirectShow: setStreamFormat GetFormat failed, hr=0x" + std::to_string(hr) + "\n").c_str());
        return false;
    }
    
    bool formatSetSuccessfully = false;
    
    if (currentMediaType->formattype == FORMAT_VideoInfo &&
        currentMediaType->cbFormat >= sizeof(VIDEOINFOHEADER)) {
        
        VIDEOINFOHEADER* vih = reinterpret_cast<VIDEOINFOHEADER*>(currentMediaType->pbFormat);
        
        // Log original format
        OutputDebugStringA(("DirectShow: Original format " + std::to_string(vih->bmiHeader.biWidth) + "x" + std::to_string(abs(vih->bmiHeader.biHeight)) + " subtype=" + guidToString(currentMediaType->subtype) + "\n").c_str());
        
        vih->bmiHeader.biWidth = width;
        vih->bmiHeader.biHeight = height;
        currentMediaType->subtype = mediaType;
        
        hr = mDevice.streamConfig->SetFormat(currentMediaType);
        if (SUCCEEDED(hr)) {
            OutputDebugStringA("DirectShow: SetFormat succeeded\n");
            formatSetSuccessfully = true;
        } else {
            OutputDebugStringA(("DirectShow: SetFormat failed, hr=0x" + std::to_string(hr) + "\n").c_str());
        }
    } else {
        OutputDebugStringA("DirectShow: setStreamFormat - invalid format type or size\n");
    }
    
    if (currentMediaType->cbFormat != 0) {
        CoTaskMemFree(currentMediaType->pbFormat);
    }
    if (currentMediaType->pUnk != nullptr) {
        currentMediaType->pUnk->Release();
    }
    CoTaskMemFree(currentMediaType);
    
    return formatSetSuccessfully;
}

bool DirectShowCapture::findClosestFormat(int& width, int& height, GUID& mediaType) {
    auto formats = getDeviceFormats(mDevice.deviceId);
    
    if (formats.empty()) {
        return false;
    }
    
    auto bestFormat = std::min_element(formats.begin(), formats.end(),
        [width, height](const StreamFormat& a, const StreamFormat& b) {
            int diffA = abs(a.width - width) + abs(a.height - height);
            int diffB = abs(b.width - width) + abs(b.height - height);
            return diffA < diffB;
        });
    
    width = bestFormat->width;
    height = bestFormat->height;
    mediaType = bestFormat->mediaType;
    
    return true;
}

bool DirectShowCapture::configureSampleGrabber() {
    if (!mDevice.sampleGrabber) {
        return false;
    }
    
    // Check what format the SampleGrabber is actually delivering after graph connection
    AM_MEDIA_TYPE connectedType;
    HRESULT hr = mDevice.sampleGrabber->GetConnectedMediaType(&connectedType);
    if (SUCCEEDED(hr)) {
        OutputDebugStringA(("DirectShow: SampleGrabber final output format: " + guidToString(connectedType.subtype) + "\n").c_str());
        
        if (connectedType.pbFormat && connectedType.formattype == FORMAT_VideoInfo) {
            VIDEOINFOHEADER* pVih = (VIDEOINFOHEADER*)connectedType.pbFormat;
            mActualWidth = pVih->bmiHeader.biWidth;
            mActualHeight = abs(pVih->bmiHeader.biHeight);
            
            // Update our dimensions to match what DirectShow is actually providing
            mWidth = mActualWidth;
            mHeight = mActualHeight;
            
            OutputDebugStringA(("DirectShow: SampleGrabber output resolution: " + std::to_string(mActualWidth) + "x" + std::to_string(mActualHeight) + "\n").c_str());
            OutputDebugStringA(("DirectShow: Buffer size expected: " + std::to_string(pVih->bmiHeader.biSizeImage) + " bytes\n").c_str());
        }
        
        mActualFormat = connectedType.subtype;
        
        // Free the format block
        if (connectedType.pbFormat) CoTaskMemFree(connectedType.pbFormat);
    }
    
    hr = mDevice.sampleGrabber->SetOneShot(FALSE);
    if (FAILED(hr)) {
        return false;
    }
    
    hr = mDevice.sampleGrabber->SetBufferSamples(TRUE);
    if (FAILED(hr)) {
        return false;
    }
    
    mCallback = std::make_unique<SampleGrabberCallback>(this);
    hr = mDevice.sampleGrabber->SetCallback(mCallback.get(), 1);
    
    return SUCCEEDED(hr);
}

bool DirectShowCapture::start() {
    OutputDebugStringA("DirectShow: Starting capture\n");
    if (!mDevice.isSetup || mIsCapturing) {
        OutputDebugStringA("DirectShow: Cannot start - device not setup or already capturing\n");
        return false;
    }
    
    // Verify graph state before starting
    if (mDevice.mediaControl) {
        OAFilterState state;
        HRESULT stateHr = mDevice.mediaControl->GetState(1000, &state);
        if (SUCCEEDED(stateHr)) {
            OutputDebugStringA(("DirectShow: Current graph state before Run(): " + std::to_string(state) + " (0=Stopped, 1=Paused, 2=Running)\n").c_str());
        }
    }
    
    HRESULT hr = mDevice.mediaControl->Run();
    if (SUCCEEDED(hr)) {
        mIsCapturing = true;
        mNewFrameAvailable = false;
        OutputDebugStringA("DirectShow: Capture started successfully\n");
        
        // Check final graph state
        OAFilterState finalState;
        HRESULT finalStateHr = mDevice.mediaControl->GetState(2000, &finalState);
        if (SUCCEEDED(finalStateHr)) {
            OutputDebugStringA(("DirectShow: Final graph state after Run(): " + std::to_string(finalState) + "\n").c_str());
        }
        
        return true;
    }
    
    OutputDebugStringA(("DirectShow: Failed to start capture, HRESULT: 0x" + std::to_string(hr) + "\n").c_str());
    return false;
}

bool DirectShowCapture::stop() {
    if (!mIsCapturing || !mDevice.mediaControl) {
        mIsCapturing = false;
        return true;
    }
    
    HRESULT hr = mDevice.mediaControl->Stop();
    mIsCapturing = false;
    mNewFrameAvailable = false;
    
    // Disconnect callback before releasing COM objects
    if (mDevice.sampleGrabber) {
        mDevice.sampleGrabber->SetCallback(nullptr, 0);
    }
    
    // Clear callback before resetting COM objects
    mCallback.reset();
    
    mDevice = DeviceContext{};
    mPixelBuffer.reset();
    
    return SUCCEEDED(hr);
}

bool DirectShowCapture::isFrameNew() const {
    std::lock_guard<std::mutex> lock(mFrameMutex);
    return mNewFrameAvailable;
}

bool DirectShowCapture::getPixels(unsigned char* buffer, bool flipRedBlue, bool flipImage) {
    if (!buffer || !mPixelBuffer) {
        OutputDebugStringA("DirectShow: getPixels failed - no buffer or pixel buffer\n");
        return false;
    }
    
    std::lock_guard<std::mutex> lock(mFrameMutex);
    
    if (!mNewFrameAvailable) {
        return false;
    }
    
    int size = getSize();
    memcpy(buffer, mPixelBuffer.get(), size);
    mNewFrameAvailable = false;
    OutputDebugStringA(("DirectShow: Copied frame data, size=" + std::to_string(size) + " bytes\n").c_str());
    
    if (flipRedBlue) {
        for (int i = 0; i < size; i += 3) {
            std::swap(buffer[i], buffer[i + 2]);
        }
    }
    
    if (flipImage) {
        int rowSize = mWidth * 3;
        std::unique_ptr<unsigned char[]> tempRow = std::make_unique<unsigned char[]>(rowSize);
        
        for (int y = 0; y < mHeight / 2; y++) {
            unsigned char* topRow = buffer + y * rowSize;
            unsigned char* bottomRow = buffer + (mHeight - 1 - y) * rowSize;
            
            memcpy(tempRow.get(), topRow, rowSize);
            memcpy(topRow, bottomRow, rowSize);
            memcpy(bottomRow, tempRow.get(), rowSize);
        }
    }
    
    return true;
}

unsigned char* DirectShowCapture::getPixels(bool flipRedBlue, bool flipImage) {
    static std::unique_ptr<unsigned char[]> staticBuffer;
    
    int size = getSize();
    if (!staticBuffer) {
        staticBuffer = std::make_unique<unsigned char[]>(size);
    }
    
    if (getPixels(staticBuffer.get(), flipRedBlue, flipImage)) {
        return staticBuffer.get();
    }
    
    return nullptr;
}

bool DirectShowCapture::isDeviceConnected(int deviceId) const {
    auto devices = enumerateDevices();
    return deviceId >= 0 && deviceId < static_cast<int>(devices.size());
}

std::string DirectShowCapture::getDeviceName(int deviceId) const {
    auto devices = enumerateDevices();
    if (deviceId >= 0 && deviceId < static_cast<int>(devices.size())) {
        return devices[deviceId].friendlyName;
    }
    return "";
}

bool DirectShowCapture::showSettingsWindow() {
    return false;
}

bool DirectShowCapture::setVideoProperty(long property, long value, long flags) {
    return false;
}

bool DirectShowCapture::getVideoProperty(long property, long& min, long& max, long& step, long& current, long& flags, long& defaultValue) {
    return false;
}


} // namespace cinder