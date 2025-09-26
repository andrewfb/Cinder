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

#pragma once

#include "cinder/Cinder.h"
#include "cinder/Capture.h"
#include <memory>
#include <vector>
#include <functional>
#include <mutex>
#include <windows.h>
#include <dshow.h>

// Forward declarations for DirectShow interfaces
struct ISampleGrabber;
struct ISampleGrabberCB;

namespace cinder {

class CI_API DirectShowCapture {
public:
    // Modern C++17 RAII wrappers for COM objects
    template<typename T>
    struct ComDeleter {
        void operator()(T* ptr) const {
            if (ptr) ptr->Release();
        }
    };
    
    template<typename T>
    using ComPtr = std::unique_ptr<T, ComDeleter<T>>;
    
    // Device enumeration and capability structures
    struct DeviceInfo {
        std::string friendlyName;
        std::string devicePath;
        std::vector<Capture::Mode> supportedModes;
    };
    
    struct StreamFormat {
        int width;
        int height;
        GUID mediaType;
        long averageTimePerFrame;  // 100-nanosecond units
        std::string description;
    };

    // Construction and lifecycle
    DirectShowCapture();
    ~DirectShowCapture();
    
    // Device enumeration
    static std::vector<DeviceInfo> enumerateDevices();
    static std::vector<std::string> getDeviceNames();
    static int getDeviceCount();
    
    // Device capabilities
    std::vector<Capture::Mode> getDeviceModes(int deviceId);
    std::vector<StreamFormat> getDeviceFormats(int deviceId);
    std::vector<std::pair<int, int>> getDeviceResolutions(int deviceId);
    
    // Setup and configuration
    bool setupDevice(int deviceId);
    bool setupDevice(int deviceId, int width, int height);
    bool setupDevice(int deviceId, int width, int height, const GUID& mediaType);
    bool setupDevice(int deviceId, const Capture::Mode& mode);
    
    // Capture control
    bool start();
    bool stop();
    bool isCapturing() const { return mIsCapturing; }
    
    // Frame access
    bool isFrameNew() const;
    bool getPixels(unsigned char* buffer, bool flipRedBlue = true, bool flipImage = false);
    unsigned char* getPixels(bool flipRedBlue = true, bool flipImage = false);
    
    // Properties
    int getWidth() const { return mWidth; }
    int getHeight() const { return mHeight; }
    int getSize() const { return mWidth * mHeight * 3; }  // Assuming BGR24
    
    // Device state
    bool isDeviceConnected(int deviceId) const;
    std::string getDeviceName(int deviceId) const;
    
    // Settings and controls
    bool showSettingsWindow();
    bool setVideoProperty(long property, long value, long flags = 0);
    bool getVideoProperty(long property, long& min, long& max, long& step, long& current, long& flags, long& defaultValue);

private:
    // COM initialization management
    class ComInitializer {
    public:
        ComInitializer();
        ~ComInitializer();
        bool isInitialized() const { return mInitialized; }
    private:
        bool mInitialized;
        static int sRefCount;
    };
    
    // Internal device management
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
    
    // Sample grabber callback for frame capture
    class SampleGrabberCallback;
    
    // Helper methods
    bool createCaptureGraph();
    bool connectFilters();
    bool configureSampleGrabber();
    ComPtr<IBaseFilter> createSourceFilter(int deviceId);
    ComPtr<IBaseFilter> createGrabberFilter();
    bool setStreamFormat(int width, int height, const GUID& mediaType);
    bool findClosestFormat(int& width, int& height, GUID& mediaType);
    
    // Format conversion helpers
    static GUID pixelFormatToMediaSubtype(Capture::Mode::PixelFormat format);
    static Capture::Mode::PixelFormat mediaSubtypeToPixelFormat(const GUID& subtype);
    static std::string guidToString(const GUID& guid);
    
    // Static device cache management
    static std::vector<DeviceInfo> sDeviceCache;
    static bool sDeviceCacheValid;
    static void refreshDeviceCache();
    
    // Instance members
    ComInitializer mComInit;
    DeviceContext mDevice;
    
    std::unique_ptr<SampleGrabberCallback> mCallback;
    std::unique_ptr<unsigned char[]> mPixelBuffer;
    
    int mWidth = 0;
    int mHeight = 0;
    bool mIsCapturing = false;
    bool mNewFrameAvailable = false;
    
    // Actual camera format info
    GUID mActualFormat = GUID_NULL;
    int mActualWidth = 0;
    int mActualHeight = 0;
    
    mutable std::mutex mFrameMutex;
    
    // Critical section for thread safety
    CRITICAL_SECTION mCriticalSection;
    bool mCriticalSectionInitialized = false;
};

} // namespace cinder