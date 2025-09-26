/*
 Copyright (c) 2010, The Barbarian Group
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

#include "cinder/CaptureImplDirectShow.h"
#include <dshow.h>
#include <dvdmedia.h>  // For VIDEO_STREAM_CONFIG_CAPS
#include <set>
#include <tuple>
#include <sstream>

// DirectShow sample grabber interface definitions (qedit.h not available)
static const GUID CLSID_SampleGrabber = { 0xC1F400A0, 0x3F08, 0x11d3, { 0x9F, 0x0B, 0x00, 0x60, 0x08, 0x03, 0x9E, 0x37 } };
static const GUID IID_ISampleGrabber = { 0x6B652FFF, 0x11FE, 0x4fce, { 0x92, 0xAD, 0x02, 0x66, 0xB5, 0xD7, 0xC7, 0x8F } };
static const GUID IID_ISampleGrabberCB = { 0x0579154A, 0x2B53, 0x4994, { 0xB0, 0xD0, 0xE7, 0x73, 0x14, 0x8E, 0xFF, 0x85 } };
static const GUID CLSID_NullRenderer = { 0xC1F400A4, 0x3F08, 0x11d3, { 0x9F, 0x0B, 0x00, 0x60, 0x08, 0x03, 0x9E, 0x37 } };

// Additional DirectShow GUIDs are provided by strmiids.lib

// DirectShow GUID constants that may be missing from some headers
#ifndef MEDIASUBTYPE_I420
static const GUID MEDIASUBTYPE_I420 = { 0x30323449, 0x0000, 0x0010, { 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71 } };
#endif

// ISampleGrabberCB interface definition
MIDL_INTERFACE("0579154A-2B53-4994-B0D0-E773148EFF85")
ISampleGrabberCB : public IUnknown
{
public:
    virtual HRESULT STDMETHODCALLTYPE SampleCB(double SampleTime, IMediaSample* pSample) = 0;
    virtual HRESULT STDMETHODCALLTYPE BufferCB(double SampleTime, BYTE* pBuffer, long BufferLen) = 0;
};

// ISampleGrabber interface definition
MIDL_INTERFACE("6B652FFF-11FE-4fce-92AD-0266B5D7C78F")
ISampleGrabber : public IUnknown
{
public:
    virtual HRESULT STDMETHODCALLTYPE SetOneShot(BOOL OneShot) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetMediaType(const AM_MEDIA_TYPE* pType) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetConnectedMediaType(AM_MEDIA_TYPE* pType) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetBufferSamples(BOOL BufferThem) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetCurrentBuffer(long* pBufferSize, long* pBuffer) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetCurrentSample(IMediaSample** ppSample) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetCallback(ISampleGrabberCB* pCallback, long WhichMethodToCallback) = 0;
};

#pragma comment(lib, "strmiids.lib")
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "oleaut32.lib")

using namespace std;

namespace cinder {

// Forward declarations for types that will be moved from DirectShowCapture
namespace {
    struct DeviceInfo {
        std::string friendlyName;
        std::string devicePath;
    };
    
    struct StreamFormat {
        int width;
        int height;
        GUID mediaType;
        long averageTimePerFrame;
        std::string description;
    };
    
    // Forward declarations for types
    struct DeviceContext;
    template<typename T> class ComPtr;
    
    // ComPtr helper template (moved from DirectShowCapture)
    template<typename T>
    class ComPtr {
    public:
        ComPtr() : ptr(nullptr) {}
        ComPtr(T* p) : ptr(p) { if (ptr) ptr->AddRef(); }
        ComPtr(const ComPtr& other) : ptr(other.ptr) { if (ptr) ptr->AddRef(); }
        ~ComPtr() { if (ptr) ptr->Release(); }
        
        ComPtr& operator=(T* p) {
            if (ptr != p) {
                if (ptr) ptr->Release();
                ptr = p;
                if (ptr) ptr->AddRef();
            }
            return *this;
        }
        
        ComPtr& operator=(const ComPtr& other) {
            return *this = other.ptr;
        }
        
        T* get() const { return ptr; }
        T** operator&() { return &ptr; }
        T* operator->() const { return ptr; }
        operator bool() const { return ptr != nullptr; }
        
        void reset(T* p = nullptr) {
            if (ptr != p) {
                if (ptr) ptr->Release();
                ptr = p;
            }
        }
        
    private:
        T* ptr;
    };
    
    // Static device cache management (moved from DirectShowCapture)
    std::vector<DeviceInfo> sDeviceCache;
    bool sDeviceCacheValid = false;
    
    // Helper functions (moved from DirectShowCapture static methods)
    void refreshDeviceCache() {
        if (sDeviceCacheValid) {
            return;
        }
        
        sDeviceCache.clear();
        
        // Initialize COM for this thread if needed
        HRESULT hrCom = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
        bool comInitialized = SUCCEEDED(hrCom);
        
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
            }
            
            moniker->Release();
            deviceIndex++;
        }
        
        // Clean up COM if we initialized it
        if (comInitialized) {
            CoUninitialize();
        }
        
        sDeviceCacheValid = true;
    }
    
    std::vector<DeviceInfo> enumerateDevices() {
        refreshDeviceCache();
        return sDeviceCache;
    }
    
    std::vector<std::string> getDeviceNames() {
        auto devices = enumerateDevices();
        std::vector<std::string> names;
        names.reserve(devices.size());
        
        for (const auto& device : devices) {
            names.push_back(device.friendlyName);
        }
        
        return names;
    }
    
    // Helper function for GUID to string conversion
    std::string guidToString(const GUID& guid) {
        if (IsEqualGUID(guid, MEDIASUBTYPE_RGB24))
            return "RGB24";
        if (IsEqualGUID(guid, MEDIASUBTYPE_RGB32))
            return "RGB32";
        if (IsEqualGUID(guid, MEDIASUBTYPE_YUY2))
            return "YUY2";
        if (IsEqualGUID(guid, MEDIASUBTYPE_UYVY))
            return "UYVY";
        if (IsEqualGUID(guid, MEDIASUBTYPE_IYUV))
            return "IYUV";
        if (IsEqualGUID(guid, MEDIASUBTYPE_YV12))
            return "YV12";
        if (IsEqualGUID(guid, MEDIASUBTYPE_NV12))
            return "NV12";
        return "Unknown";
    }
    
    // Convert pixel format to DirectShow media subtype
    GUID pixelFormatToMediaSubtype(Capture::Mode::PixelFormat format) {
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
    
    // Convert DirectShow media subtype to pixel format
    Capture::Mode::PixelFormat mediaSubtypeToPixelFormat(const GUID& subtype) {
        if (IsEqualGUID(subtype, MEDIASUBTYPE_RGB24)) {
            return Capture::Mode::PixelFormat::RGB24;
        }
        else if (IsEqualGUID(subtype, MEDIASUBTYPE_RGB32)) {
            return Capture::Mode::PixelFormat::ARGB32;
        }
        else if (IsEqualGUID(subtype, MEDIASUBTYPE_YUY2)) {
            return Capture::Mode::PixelFormat::YUY2;
        }
        else if (IsEqualGUID(subtype, MEDIASUBTYPE_UYVY)) {
            return Capture::Mode::PixelFormat::UYVY;
        }
        else if (IsEqualGUID(subtype, MEDIASUBTYPE_IYUV)) {
            return Capture::Mode::PixelFormat::YUV420P;
        }
        else if (IsEqualGUID(subtype, MEDIASUBTYPE_YV12)) {
            return Capture::Mode::PixelFormat::YV12;
        }
        else if (IsEqualGUID(subtype, MEDIASUBTYPE_NV12)) {
            return Capture::Mode::PixelFormat::NV12;
        }
        else {
            return Capture::Mode::PixelFormat::BGR24;
        }
    }
    
    // Create a DirectShow source filter for the specified device
    ComPtr<IBaseFilter> createSourceFilter(int deviceId) {
        auto devices = enumerateDevices();
        if (deviceId < 0 || deviceId >= static_cast<int>(devices.size())) {
            return ComPtr<IBaseFilter>();
        }
        
        ComPtr<ICreateDevEnum> deviceEnum;
        HRESULT hr = CoCreateInstance(CLSID_SystemDeviceEnum, nullptr, CLSCTX_INPROC, IID_ICreateDevEnum, reinterpret_cast<void**>(&deviceEnum));
        
        if (FAILED(hr)) {
            return ComPtr<IBaseFilter>();
        }
        
        IEnumMoniker* enumMonikerPtr = nullptr;
        hr = deviceEnum->CreateClassEnumerator(CLSID_VideoInputDeviceCategory, &enumMonikerPtr, 0);
        ComPtr<IEnumMoniker> enumMoniker(enumMonikerPtr);
        
        if (hr != S_OK) {
            return ComPtr<IBaseFilter>();
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
        
        return ComPtr<IBaseFilter>();
    }
    
    // Get available formats for a device
    std::vector<StreamFormat> getDeviceFormats(int deviceId) {
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
                if (mediaType->majortype == MEDIATYPE_Video && mediaType->formattype == FORMAT_VideoInfo && mediaType->cbFormat >= sizeof(VIDEOINFOHEADER)) {
                    
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
    
    // Get available modes for a device
    std::vector<Capture::Mode> getDeviceModes(int deviceId) {
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
                modes.push_back(Capture::Mode(format.width, format.height, frameTime, Capture::Mode::Codec::Uncompressed, pixelFormat, format.description));
            }
        }
        
        if (modes.empty()) {
            modes.push_back(Capture::Mode(640, 480, MediaTime(1.0 / 30.0), Capture::Mode::Codec::Uncompressed, Capture::Mode::PixelFormat::BGR24, "640x480 30fps BGR24 (fallback)"));
        }
        
        // Sort modes by area (width × height) in ascending order
        std::sort(modes.begin(), modes.end(), [](const Capture::Mode& a, const Capture::Mode& b) {
            int areaA = a.getWidth() * a.getHeight();
            int areaB = b.getWidth() * b.getHeight();
            return areaA < areaB;
        });
        
        return modes;
    }
    
    // Check if a device is connected
    bool isDeviceConnected(int deviceId) {
        auto devices = enumerateDevices();
        return deviceId >= 0 && deviceId < static_cast<int>(devices.size());
    }
    
    // COM initialization management class
    class ComInitializer {
    public:
        ComInitializer() : mInitialized(false) {
            if (sRefCount == 0) {
                HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
                if (SUCCEEDED(hr)) {
                    mInitialized = true;
                }
            }
            else {
                mInitialized = true;
            }
            sRefCount++;
        }
        
        ~ComInitializer() {
            sRefCount--;
            if (sRefCount == 0 && mInitialized) {
                CoUninitialize();
            }
        }
        
        bool isInitialized() const { return mInitialized; }
        
    private:
        bool mInitialized;
        static int sRefCount;
    };
    
    // Static member for ComInitializer
    int ComInitializer::sRefCount = 0;
    
    // Forward declaration - we'll use CaptureImplDirectShow directly
    // class CaptureDevice; // No longer needed
    
    // Internal device management structure
    struct DeviceContext {
        ComPtr<ICaptureGraphBuilder2> captureBuilder;
        ComPtr<IGraphBuilder> graphBuilder;
        ComPtr<IMediaControl> mediaControl;
        ComPtr<IBaseFilter> sourceFilter;
        ComPtr<IBaseFilter> grabberFilter;
        ComPtr<ISampleGrabber> sampleGrabber;
        ComPtr<IAMStreamConfig> streamConfig;
        
        int deviceId = -1;
        int width = 0;
        int height = 0;
        bool isSetup = false;
    };
}

// Sample grabber callback for frame capture (outside anonymous namespace to enable friend access)
class SampleGrabberCallback : public ISampleGrabberCB {
public:
    SampleGrabberCallback(CaptureImplDirectShow* parent);
    virtual ~SampleGrabberCallback();
    
    // IUnknown methods
    STDMETHODIMP QueryInterface(REFIID riid, void** ppv) override;
    STDMETHODIMP_(ULONG) AddRef() override;
    STDMETHODIMP_(ULONG) Release() override;
    
    // ISampleGrabberCB methods
    STDMETHODIMP SampleCB(double sampleTime, IMediaSample* sample) override;
    STDMETHODIMP BufferCB(double sampleTime, BYTE* buffer, long bufferLen) override;
    
    // Public access to parent for setup functions
    CaptureImplDirectShow* mParent;
    
private:
    LONG mRefCount;
};

// SampleGrabberCallback implementation
SampleGrabberCallback::SampleGrabberCallback(CaptureImplDirectShow* parent)
    : mParent(parent), mRefCount(1)
{
}

SampleGrabberCallback::~SampleGrabberCallback()
{
}

STDMETHODIMP SampleGrabberCallback::QueryInterface(REFIID riid, void** ppv)
{
    if (riid == IID_ISampleGrabberCB || riid == IID_IUnknown) {
        *ppv = static_cast<ISampleGrabberCB*>(this);
        AddRef();
        return S_OK;
    }
    return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG) SampleGrabberCallback::AddRef()
{
    return InterlockedIncrement(&mRefCount);
}

STDMETHODIMP_(ULONG) SampleGrabberCallback::Release()
{
    LONG count = InterlockedDecrement(&mRefCount);
    // Note: We don't delete this object here because it's managed by unique_ptr
    // in the parent CaptureImplDirectShow object. The parent will clean it up properly.
    return count;
}

STDMETHODIMP SampleGrabberCallback::SampleCB(double sampleTime, IMediaSample* sample)
{
    OutputDebugStringA("SampleCB called\n");
    
    if (!mParent || !sample) {
        OutputDebugStringA("SampleCB: No parent or sample\n");
        return S_OK;
    }
    
    try {
        std::lock_guard<std::mutex> lock(mParent->mFrameMutex);
        
        BYTE* ptrBuffer = nullptr;
        HRESULT hr = sample->GetPointer(&ptrBuffer);
        
        if (SUCCEEDED(hr) && ptrBuffer && mParent->mPixelBuffer) {
            long actualDataLength = sample->GetActualDataLength();
            
            // Calculate actual dimensions from data size (RGB24 = width * height * 3)
            int actualPixels = actualDataLength / 3;
            int actualWidth = mParent->mWidth;
            int actualHeight = mParent->mHeight;
            
            // Verify that our stored dimensions match the actual data
            int expectedSize = actualWidth * actualHeight * 3;
            if (expectedSize != actualDataLength) {
                // Dimensions are out of sync - try to calculate from common aspect ratios
                OutputDebugStringA("SampleCB: Dimension mismatch detected, recalculating\n");
                
                // Try common aspect ratios to find actual dimensions
                // Most likely it's one of the camera's supported modes
                if (actualDataLength == 2560 * 720 * 3) {
                    actualWidth = 2560; actualHeight = 720;
                } else if (actualDataLength == 3840 * 2160 * 3) {
                    actualWidth = 3840; actualHeight = 2160;
                } else if (actualDataLength == 1920 * 1080 * 3) {
                    actualWidth = 1920; actualHeight = 1080;
                } else if (actualDataLength == 1280 * 720 * 3) {
                    actualWidth = 1280; actualHeight = 720;
                } else if (actualDataLength == 640 * 480 * 3) {
                    actualWidth = 640; actualHeight = 480;
                } else {
                    // Unknown resolution - use stored values anyway
                    OutputDebugStringA("SampleCB: Unknown resolution, using stored dimensions\n");
                }
                
                // Update parent dimensions and reallocate buffer if needed
                if (actualWidth != mParent->mWidth || actualHeight != mParent->mHeight) {
                    OutputDebugStringA("SampleCB: Updating parent dimensions and reallocating buffer\n");
                    mParent->updateDimensions(actualWidth, actualHeight);
                }
            }
            
            char debugMsg[256];
            sprintf_s(debugMsg, "SampleCB: actualDataLength=%ld, using %dx%d (%d bytes), stored=%dx%d\n", 
                     actualDataLength, actualWidth, actualHeight, actualWidth * actualHeight * 3,
                     mParent->mWidth, mParent->mHeight);
            OutputDebugStringA(debugMsg);
            
            // Process RGB24 data with proper dimensions
            if (actualDataLength == actualWidth * actualHeight * 3) {
                // RGB24 format - copy with format fixes using correct dimensions
                OutputDebugStringA("SampleCB: Converting RGB24 with flip and channel swap\n");
                const uint8_t* src = static_cast<const uint8_t*>(ptrBuffer);
                uint8_t* dst = mParent->mPixelBuffer.get();
                
                // Copy with vertical flip only (no channel conversion needed)
                for (int y = 0; y < actualHeight; y++) {
                    int srcRow = (actualHeight - 1 - y); // Flip vertically
                    const uint8_t* srcLine = src + srcRow * (actualWidth * 3);
                    uint8_t* dstLine = dst + y * (actualWidth * 3);
                    
                    // Direct copy of the line (no channel swapping)
                    memcpy(dstLine, srcLine, actualWidth * 3);
                }
                mParent->mNewFrameAvailable = true;
                OutputDebugStringA("SampleCB: Frame converted successfully\n");
            }
            else {
                // Unknown format - copy what we can for debugging
                OutputDebugStringA("SampleCB: Unknown format, copying raw data\n");
                int maxCopySize = mParent->mWidth * mParent->mHeight * 3;
                int copySize = (actualDataLength < maxCopySize) ? actualDataLength : maxCopySize;
                memcpy(mParent->mPixelBuffer.get(), ptrBuffer, copySize);
                mParent->mNewFrameAvailable = true;
                OutputDebugStringA("SampleCB: Raw data copied\n");
            }
        }
    }
    catch (...) {
        // Ignore exceptions in callback
    }
    
    return S_OK;
}

STDMETHODIMP SampleGrabberCallback::BufferCB(double sampleTime, BYTE* buffer, long bufferLen)
{
    OutputDebugStringA("BufferCB called\n");
    
    if (!mParent || !buffer) {
        OutputDebugStringA("BufferCB: No parent or buffer\n");
        return S_OK;
    }
    
    try {
        std::lock_guard<std::mutex> lock(mParent->mFrameMutex);
        
        if (mParent->mPixelBuffer) {
            int expectedSize = mParent->mWidth * mParent->mHeight * 3; // RGB24 is width * height * 3
            
            char debugMsg[256];
            sprintf_s(debugMsg, "BufferCB: bufferLen=%ld, expectedSize=%d, width=%d, height=%d\n", 
                     bufferLen, expectedSize, mParent->mWidth, mParent->mHeight);
            OutputDebugStringA(debugMsg);
            
            // Handle DirectShow RGB24 data with proper stride and flipping
            if (bufferLen > 0) {
                // Calculate actual stride from buffer size
                int actualStride = bufferLen / mParent->mHeight;
                int expectedStride = mParent->mWidth * 3;
                
                char strideMsg[256];
                sprintf_s(strideMsg, "BufferCB: actualStride=%d, expectedStride=%d\n", 
                         actualStride, expectedStride);
                OutputDebugStringA(strideMsg);
                
                if (actualStride == expectedStride) {
                    // No padding, but might need to flip vertically (DirectShow is often bottom-up)
                    const uint8_t* src = static_cast<const uint8_t*>(buffer);
                    uint8_t* dst = mParent->mPixelBuffer.get();
                    
                    // Copy and flip vertically (DirectShow RGB is typically bottom-up)
                    for (int y = 0; y < mParent->mHeight; y++) {
                        int srcRow = (mParent->mHeight - 1 - y); // Flip vertically
                        const uint8_t* srcLine = src + srcRow * actualStride;
                        uint8_t* dstLine = dst + y * expectedStride;
                        
                        // Copy and convert RGB->BGR for Cinder
                        for (int x = 0; x < mParent->mWidth; x++) {
                            dstLine[x * 3 + 0] = srcLine[x * 3 + 2]; // B = R
                            dstLine[x * 3 + 1] = srcLine[x * 3 + 1]; // G = G
                            dstLine[x * 3 + 2] = srcLine[x * 3 + 0]; // R = B
                        }
                    }
                    OutputDebugStringA("BufferCB: Frame copied with vertical flip and RGB->BGR conversion\n");
                } else {
                    // Handle stride padding
                    const uint8_t* src = static_cast<const uint8_t*>(buffer);
                    uint8_t* dst = mParent->mPixelBuffer.get();
                    
                    for (int y = 0; y < mParent->mHeight; y++) {
                        int srcRow = (mParent->mHeight - 1 - y); // Flip vertically
                        const uint8_t* srcLine = src + srcRow * actualStride;
                        uint8_t* dstLine = dst + y * expectedStride;
                        
                        // Copy only the actual width, handle padding
                        for (int x = 0; x < mParent->mWidth; x++) {
                            dstLine[x * 3 + 0] = srcLine[x * 3 + 2]; // B = R
                            dstLine[x * 3 + 1] = srcLine[x * 3 + 1]; // G = G
                            dstLine[x * 3 + 2] = srcLine[x * 3 + 0]; // R = B
                        }
                    }
                    OutputDebugStringA("BufferCB: Frame copied with stride handling, vertical flip and RGB->BGR conversion\n");
                }
                
                mParent->mNewFrameAvailable = true;
            }
        }
    }
    catch (...) {
        OutputDebugStringA("BufferCB: Exception caught\n");
    }
    
    return S_OK;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
// SurfaceCache

class SurfaceCache {
  public:
	SurfaceCache( int32_t width, int32_t height, SurfaceChannelOrder sco, int numSurfaces )
		: mSurfaces( numSurfaces, nullptr ), mWidth( width ), mHeight( height ), mSCO( sco )
	{
		for( auto& surf : mSurfaces ) {
			surf = Surface8u::create( mWidth, mHeight, mSCO.hasAlpha(), mSCO );
		}
	}

	Surface8uRef getNewSurface()
	{
		// try to find a surface that isn't used by anyone else
		// TODO: this is racy, but the worst that can happen is that we over-allocate Surfaces in the cache
		auto it = std::find_if( mSurfaces.begin(), mSurfaces.end(), [](const Surface8uRef& s) { return s.use_count() == 1; } );

		// if no free surface is found, create a new one and add it to the cache set
		if( it == mSurfaces.end() ) {
			mSurfaces.push_back( Surface8u::create( mWidth, mHeight, mSCO.hasAlpha(), mSCO ) );
			it = mSurfaces.end() - 1;
		}

		return *it;
	}

  private:
	std::vector<Surface8uRef>			mSurfaces;
	int32_t								mWidth, mHeight;
	SurfaceChannelOrder					mSCO;
};


//////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CaptureImplDirectShow

bool CaptureImplDirectShow::Device::checkAvailable() const
{
	return ( mUniqueId >=0 ) && ( mUniqueId < (int)CaptureImplDirectShow::getDevices().size() ) && isDeviceConnected( mUniqueId );
}

bool CaptureImplDirectShow::Device::isConnected() const
{
	return isDeviceConnected( mUniqueId );
}

const vector<Capture::DeviceRef>& CaptureImplDirectShow::getDevices( bool forceRefresh )
{
	static bool firstCall = true;
	static std::vector<Capture::DeviceRef>	devices;

	if( firstCall || forceRefresh ) {
		auto deviceNames = getDeviceNames();
		devices.resize( deviceNames.size() );
		for ( int i = 0; i < (int)deviceNames.size(); ++i ) {
			devices[i] = std::make_shared<CaptureImplDirectShow::Device>( deviceNames[i], i );
		}

		firstCall = false;
	}
	return devices;
}

CaptureImplDirectShow::CaptureImplDirectShow( int32_t width, int32_t height, const Capture::DeviceRef device )
	: mWidth( width ), mHeight( height ), mCurrentFrame( Surface8u::create( width, height, false, SurfaceChannelOrder::BGR ) ), mDeviceID( 0 ), mNewFrameAvailable( false )
{
	mDevice = device;
	if( mDevice ) {
		mDeviceID = device->getUniqueId();
	}
	
	// Initialize new DirectShow members (use anonymous namespace classes)
	mComInit = new ComInitializer();
	mDeviceContext = new DeviceContext();
	mCallback = new SampleGrabberCallback(this);
	
	// Try our direct DirectShow setup
	if( ! setupDeviceDirect( mDeviceID, mWidth, mHeight ) ) {
		throw CaptureExcInitFail( "Failed to setup DirectShow video input device" );
	}
	
	// Allocate pixel buffer for the actual size
	int bufferSize = mWidth * mHeight * 3;
	mPixelBuffer = std::make_unique<unsigned char[]>(bufferSize);
	
	mIsCapturing = true;
	mSurfaceCache.reset( new SurfaceCache( mWidth, mHeight, SurfaceChannelOrder::BGR, 4 ) );
}


CaptureImplDirectShow::CaptureImplDirectShow( const Capture::DeviceRef& device, const Capture::Mode& mode )
	: mWidth( mode.getWidth() ), mHeight( mode.getHeight() ), mCurrentFrame( Surface8u::create( mode.getWidth(), mode.getHeight(), false, SurfaceChannelOrder::BGR ) ), mDeviceID( 0 ), mNewFrameAvailable( false )
{
	mDevice = device;
	if( mDevice ) {
		mDeviceID = device->getUniqueId();
	}
	
	// Initialize new DirectShow members (use anonymous namespace classes)
	mComInit = new ComInitializer();
	mDeviceContext = new DeviceContext();
	mCallback = new SampleGrabberCallback(this);

	// Try direct DirectShow setup first
	OutputDebugStringA("Trying direct DirectShow setup\n");
	if (!setupDeviceDirect(mDeviceID, mode.getWidth(), mode.getHeight())) {
		throw CaptureExcInitFail( "Failed to setup DirectShow video input device with specified mode" );
	}
	
	// Start capture
	DeviceContext* deviceContext = static_cast<DeviceContext*>(mDeviceContext);
	if (deviceContext->mediaControl) {
		HRESULT hr = deviceContext->mediaControl->Run();
		if (FAILED(hr)) {
			throw CaptureExcInitFail( "Failed to start DirectShow media control" );
		}
		// Give the graph a moment to start streaming
		Sleep(100);
	}
	
	// Keep constructor width/height
	mWidth = mode.getWidth();
	mHeight = mode.getHeight();
	
	// Allocate pixel buffer for the actual size
	int bufferSize = mWidth * mHeight * 3;
	mPixelBuffer = std::make_unique<unsigned char[]>(bufferSize);
	
	mIsCapturing = true;
	mSurfaceCache.reset( new SurfaceCache( mWidth, mHeight, SurfaceChannelOrder::BGR, 4 ) );
}

CaptureImplDirectShow::~CaptureImplDirectShow()
{
	stop();
	
	// Clean up DirectShow objects
	if( mCallback ) {
		delete static_cast<SampleGrabberCallback*>(mCallback);
		mCallback = nullptr;
	}
	if( mDeviceContext ) {
		delete static_cast<DeviceContext*>(mDeviceContext);
		mDeviceContext = nullptr;
	}
	if( mComInit ) {
		delete static_cast<ComInitializer*>(mComInit);
		mComInit = nullptr;
	}
}

void CaptureImplDirectShow::start()
{
	if( mIsCapturing ) return;

	// Start DirectShow capture
	DeviceContext* deviceContext = static_cast<DeviceContext*>(mDeviceContext);
	if (deviceContext && deviceContext->mediaControl) {
		HRESULT hr = deviceContext->mediaControl->Run();
		if (FAILED(hr)) {
			throw CaptureExcInitFail( "Failed to start DirectShow video capture" );
		}
	}
	mIsCapturing = true;
}

void CaptureImplDirectShow::stop()
{
	if( ! mIsCapturing ) return;

	// Stop DirectShow capture
	DeviceContext* deviceContext = static_cast<DeviceContext*>(mDeviceContext);
	if (deviceContext && deviceContext->mediaControl) {
		deviceContext->mediaControl->Stop();
	}
	mIsCapturing = false;
}

bool CaptureImplDirectShow::isCapturing()
{
	return mIsCapturing;
}

bool CaptureImplDirectShow::checkNewFrame() const
{
	std::lock_guard<std::mutex> lock(mFrameMutex);
	return mNewFrameAvailable;
}

Surface8uRef CaptureImplDirectShow::getSurface() const
{
	std::lock_guard<std::mutex> lock(mFrameMutex);
	if (mNewFrameAvailable && mPixelBuffer) {
		mCurrentFrame = mSurfaceCache->getNewSurface();
		// Direct copy - conversion already done in BufferCB
		int bufferSize = mWidth * mHeight * 3;
		memcpy(mCurrentFrame->getData(), mPixelBuffer.get(), bufferSize);
		mNewFrameAvailable = false;
	}
	return mCurrentFrame;
}

std::vector<Capture::Mode> CaptureImplDirectShow::Device::getModes() const
{
	return getDeviceModes(mUniqueId);
}

// Helper method to create a sample grabber filter
ComPtr<IBaseFilter> createGrabberFilter() {
	IBaseFilter* filter = nullptr;
	HRESULT hr = CoCreateInstance(CLSID_SampleGrabber, nullptr, CLSCTX_INPROC_SERVER,
								  IID_IBaseFilter, reinterpret_cast<void**>(&filter));
	if (FAILED(hr)) {
		return ComPtr<IBaseFilter>(nullptr);
	}
	return ComPtr<IBaseFilter>(filter);
}

// Helper method to create the capture graph
bool createCaptureGraph(DeviceContext* deviceContext) {
	HRESULT hr;
	
	// Create the capture graph builder
	hr = CoCreateInstance(CLSID_CaptureGraphBuilder2, nullptr, CLSCTX_INPROC_SERVER, 
						  IID_ICaptureGraphBuilder2, reinterpret_cast<void**>(&deviceContext->captureBuilder));
	if (FAILED(hr)) {
		return false;
	}
	
	// Create the filter graph
	hr = CoCreateInstance(CLSID_FilterGraph, nullptr, CLSCTX_INPROC_SERVER,
						  IID_IGraphBuilder, reinterpret_cast<void**>(&deviceContext->graphBuilder));
	if (FAILED(hr)) {
		return false;
	}
	
	// Connect the capture graph builder to the filter graph
	hr = deviceContext->captureBuilder->SetFiltergraph(deviceContext->graphBuilder.get());
	if (FAILED(hr)) {
		return false;
	}
	
	// Get the media control interface
	hr = deviceContext->graphBuilder->QueryInterface(IID_IMediaControl, reinterpret_cast<void**>(&deviceContext->mediaControl));
	if (FAILED(hr)) {
		return false;
	}
	
	return true;
}

// Helper method to connect the DirectShow filters
bool connectFilters(DeviceContext* deviceContext) {
	OutputDebugStringA("connectFilters: Starting filter connection\n");
	HRESULT hr;
	
	// Create the sample grabber filter
	deviceContext->grabberFilter = createGrabberFilter();
	if (!deviceContext->grabberFilter) {
		OutputDebugStringA("connectFilters: Failed to create grabber filter\n");
		return false;
	}
	OutputDebugStringA("connectFilters: Created grabber filter\n");
	
	// Add the sample grabber to the graph
	hr = deviceContext->graphBuilder->AddFilter(deviceContext->grabberFilter.get(), L"Sample Grabber");
	if (FAILED(hr)) {
		char debugMsg[128];
		sprintf_s(debugMsg, "connectFilters: AddFilter grabber failed with hr=0x%08X\n", hr);
		OutputDebugStringA(debugMsg);
		return false;
	}
	OutputDebugStringA("connectFilters: Added grabber filter to graph\n");
	
	// Get the ISampleGrabber interface
	hr = deviceContext->grabberFilter->QueryInterface(IID_ISampleGrabber, reinterpret_cast<void**>(&deviceContext->sampleGrabber));
	if (FAILED(hr)) {
		char debugMsg[128];
		sprintf_s(debugMsg, "connectFilters: QueryInterface ISampleGrabber failed with hr=0x%08X\n", hr);
		OutputDebugStringA(debugMsg);
		return false;
	}
	OutputDebugStringA("connectFilters: Got ISampleGrabber interface\n");
	
	// Configure the sample grabber to force RGB24 format (like videoInput did)
	// DirectShow will handle YUY2->RGB24 conversion automatically
	AM_MEDIA_TYPE mt;
	ZeroMemory(&mt, sizeof(mt));
	mt.majortype = MEDIATYPE_Video;
	mt.subtype = MEDIASUBTYPE_RGB24;
	hr = deviceContext->sampleGrabber->SetMediaType(&mt);
	if (FAILED(hr)) {
		char debugMsg[128];
		sprintf_s(debugMsg, "connectFilters: SetMediaType RGB24 failed with hr=0x%08X\n", hr);
		OutputDebugStringA(debugMsg);
		return false;
	}
	OutputDebugStringA("connectFilters: Forced RGB24 format (DirectShow will convert)\n");
	
	// Create a null renderer to prevent video display window
	IBaseFilter* nullRenderer = nullptr;
	hr = CoCreateInstance(CLSID_NullRenderer, nullptr, CLSCTX_INPROC_SERVER,
						  IID_IBaseFilter, reinterpret_cast<void**>(&nullRenderer));
	if (FAILED(hr)) {
		char debugMsg[128];
		sprintf_s(debugMsg, "connectFilters: Create null renderer failed with hr=0x%08X\n", hr);
		OutputDebugStringA(debugMsg);
		return false;
	}
	OutputDebugStringA("connectFilters: Created null renderer\n");
	
	// Add null renderer to graph
	hr = deviceContext->graphBuilder->AddFilter(nullRenderer, L"Null Renderer");
	if (FAILED(hr)) {
		char debugMsg[128];
		sprintf_s(debugMsg, "connectFilters: AddFilter null renderer failed with hr=0x%08X\n", hr);
		OutputDebugStringA(debugMsg);
		nullRenderer->Release();
		return false;
	}
	OutputDebugStringA("connectFilters: Added null renderer to graph\n");
	
	// Use RenderStream to connect: source -> sample grabber -> null renderer
	OutputDebugStringA("connectFilters: Starting RenderStream connection\n");
	hr = deviceContext->captureBuilder->RenderStream(&PIN_CATEGORY_CAPTURE, &MEDIATYPE_Video,
													deviceContext->sourceFilter.get(), 
													deviceContext->grabberFilter.get(), nullRenderer);
	nullRenderer->Release(); // Release our reference, graph holds its own
	if (FAILED(hr)) {
		char debugMsg[128];
		sprintf_s(debugMsg, "connectFilters: RenderStream failed with hr=0x%08X\n", hr);
		OutputDebugStringA(debugMsg);
		return false;
	}
	OutputDebugStringA("connectFilters: RenderStream succeeded\n");
	
	return true;
}

// Helper method to set up the callback
bool setupCallback(DeviceContext* deviceContext, ::cinder::SampleGrabberCallback* callback) {
	OutputDebugStringA("setupCallback: Starting callback setup\n");
	
	if (!deviceContext->sampleGrabber || !callback) {
		OutputDebugStringA("setupCallback: Missing sampleGrabber or callback\n");
		return false;
	}
	
	// Configure the sample grabber to use BufferCB callback mode (mode 0)
	HRESULT hr = deviceContext->sampleGrabber->SetCallback(static_cast<ISampleGrabberCB*>(callback), 0);
	if (FAILED(hr)) {
		char debugMsg[128];
		sprintf_s(debugMsg, "setupCallback: SetCallback failed with hr=0x%08X\n", hr);
		OutputDebugStringA(debugMsg);
		return false;
	}
	OutputDebugStringA("setupCallback: SetCallback succeeded (BufferCB mode)\n");
	
	// Set the sample grabber to not buffer samples (we'll handle them immediately)
	hr = deviceContext->sampleGrabber->SetBufferSamples(FALSE);
	if (FAILED(hr)) {
		char debugMsg[128];
		sprintf_s(debugMsg, "setupCallback: SetBufferSamples failed with hr=0x%08X\n", hr);
		OutputDebugStringA(debugMsg);
		return false;
	}
	OutputDebugStringA("setupCallback: SetBufferSamples succeeded\n");
	
	// Set to one shot mode (grab every frame)
	hr = deviceContext->sampleGrabber->SetOneShot(FALSE);
	if (FAILED(hr)) {
		char debugMsg[128];
		sprintf_s(debugMsg, "setupCallback: SetOneShot failed with hr=0x%08X\n", hr);
		OutputDebugStringA(debugMsg);
		return false;
	}
	OutputDebugStringA("setupCallback: SetOneShot succeeded\n");
	
	// Get the actual negotiated media type and extract actual dimensions
	AM_MEDIA_TYPE mt;
	hr = deviceContext->sampleGrabber->GetConnectedMediaType(&mt);
	if (SUCCEEDED(hr)) {
		char debugMsg[256];
		sprintf_s(debugMsg, "setupCallback: Connected format major=%08X sub=%08X\n", 
				 mt.majortype.Data1, mt.subtype.Data1);
		OutputDebugStringA(debugMsg);
		
		// Extract actual width and height from video info header
		if (mt.formattype == FORMAT_VideoInfo && mt.cbFormat >= sizeof(VIDEOINFOHEADER)) {
			VIDEOINFOHEADER* vih = reinterpret_cast<VIDEOINFOHEADER*>(mt.pbFormat);
			int actualWidth = vih->bmiHeader.biWidth;
			int actualHeight = abs(vih->bmiHeader.biHeight);
			bool isBottomUp = vih->bmiHeader.biHeight > 0; // Positive height = bottom-up
			
			char sizeMsg[512];
			sprintf_s(sizeMsg, "setupCallback: Actual negotiated size: %dx%d (requested %dx%d), stride=%ld, biHeight=%ld (%s)\n", 
					 actualWidth, actualHeight, deviceContext->width, deviceContext->height,
					 vih->bmiHeader.biSizeImage / actualHeight, vih->bmiHeader.biHeight,
					 isBottomUp ? "bottom-up" : "top-down");
			OutputDebugStringA(sizeMsg);
			
			// Update our stored dimensions to match what DirectShow actually negotiated
			deviceContext->width = actualWidth;
			deviceContext->height = actualHeight;
			
			// Update parent dimensions and allocate pixel buffer for actual size
			callback->mParent->updateDimensions(actualWidth, actualHeight);
			
			char bufferMsg[256];
			sprintf_s(bufferMsg, "setupCallback: Updated dimensions and allocated buffer for %dx%d RGB24\n", 
					 actualWidth, actualHeight);
			OutputDebugStringA(bufferMsg);
		}
		
		if (mt.cbFormat != 0) {
			CoTaskMemFree(mt.pbFormat);
		}
		if (mt.pUnk != nullptr) {
			mt.pUnk->Release();
		}
	} else {
		char debugMsg[128];
		sprintf_s(debugMsg, "setupCallback: GetConnectedMediaType failed with hr=0x%08X\n", hr);
		OutputDebugStringA(debugMsg);
	}
	
	return true;
}

// Direct DirectShow setup implementation
bool CaptureImplDirectShow::setupDeviceDirect(int deviceId, int width, int height)
{
	DeviceContext* deviceContext = static_cast<DeviceContext*>(mDeviceContext);
	
	// Create the capture graph
	if (!createCaptureGraph(deviceContext)) {
		return false;
	}
	
	// Create source filter
	deviceContext->sourceFilter = createSourceFilter(deviceId);
	if (!deviceContext->sourceFilter) {
		return false;
	}
	
	// Add source filter to graph
	HRESULT hr = deviceContext->graphBuilder->AddFilter(deviceContext->sourceFilter.get(), L"Video Capture Source");
	if (FAILED(hr)) {
		return false;
	}
	
	// Connect the filters (sample grabber, null renderer, etc.)
	if (!connectFilters(deviceContext)) {
		return false;
	}
	
	// Set up our callback
	::cinder::SampleGrabberCallback* callback = static_cast<::cinder::SampleGrabberCallback*>(mCallback);
	if (!setupCallback(deviceContext, callback)) {
		return false;
	}
	
	// Store device info
	deviceContext->deviceId = deviceId;
	deviceContext->isSetup = true;
	
	// Note: width and height will be updated in setupCallback based on actual negotiated size
	// We'll allocate the pixel buffer after we know the actual dimensions
	
	return true;
}

// Method to update dimensions and reallocate pixel buffer
void CaptureImplDirectShow::updateDimensions(int width, int height) {
	mWidth = width;
	mHeight = height;
	
	// Reallocate pixel buffer for new size
	int bufferSize = width * height * 3; // RGB24
	mPixelBuffer = std::make_unique<unsigned char[]>(bufferSize);
	
	// Update the surface cache as well
	mSurfaceCache.reset(new SurfaceCache(width, height, SurfaceChannelOrder::BGR, 4));
}

} //namespace