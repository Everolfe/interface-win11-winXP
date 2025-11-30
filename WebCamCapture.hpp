#pragma once

//#include <SFML/Graphics.hpp>
//#include <string>
//#include <vector>
//#include <iostream>
#ifdef _WIN32
//#include <utility>
//#include <windows.h>
//#include <dshow.h>
//#include <comdef.h>
//#include <atlbase.h>
//#pragma comment(lib, "strmiids")
//#pragma comment(lib, "ole32")
//#pragma comment(lib, "oleaut32")


#include "Source.hpp"

interface ISampleGrabberCB : public IUnknown { // интерфейс дл€ получени€ кадра

    virtual STDMETHODIMP SampleCB(double SampleTime, IMediaSample* pSample) = 0; // получени€ кадра через указатель

    virtual STDMETHODIMP BufferCB(double SampleTime, BYTE* pBuffer, long BufferLen) = 0; // получение кадра через буфер
};

interface ISampleGrabber : public IUnknown { // интерфейс дл€ настройки захвата кадра

    virtual HRESULT STDMETHODCALLTYPE SetOneShot(BOOL OneShot) = 0; // однократный захват кадра

    virtual HRESULT STDMETHODCALLTYPE SetMediaType(const AM_MEDIA_TYPE* pType) = 0; // установка формата видео

    virtual HRESULT STDMETHODCALLTYPE GetConnectedMediaType(AM_MEDIA_TYPE* pType) = 0; // получение текущего формата
    
    virtual HRESULT STDMETHODCALLTYPE SetBufferSamples(BOOL BufferThem) = 0; // буферизаци€ кадров
    
    virtual HRESULT STDMETHODCALLTYPE GetCurrentBuffer(long* pBufferSize, long* pBuffer) = 0; // доступ к буферу кадра
    
    virtual HRESULT STDMETHODCALLTYPE GetCurrentSample(IMediaSample** ppSample) = 0;  // получение кадра
    
    virtual HRESULT STDMETHODCALLTYPE SetCallback(ISampleGrabberCB* pCallback, long WhichMethodToCallback) = 0; // установка получени€ кадров
};

static const IID IID_ISampleGrabber = { 0x6B652FFF, 0x11FE, 0x4fce, {0x92, 0xAD, 0x02, 0x66, 0xB5, 0xD7, 0xC7, 0x8F} }; // ID дл€ COM объекта

static const CLSID CLSID_SampleGrabber = { 0xC1F400A0, 0x3F08, 0x11d3, {0x9F, 0x0B, 0x00, 0x60, 0x08, 0x03, 0x9E, 0x37} }; // ID дл€ создани€ экземпл€ра

static const CLSID CLSID_NullRenderer = { 0xC1F400A4, 0x3F08, 0x11d3, {0x9F, 0x0B, 0x00, 0x60, 0x08, 0x03, 0x9E, 0x37} }; // ID дл€ отбрасывани€ данных
#endif

struct CameraInfo {
    std::wstring name;
    std::wstring path;
    int index;
    std::vector<std::pair<int, int>> resolutions; 
    std::vector<double> fpsOptions; 
    int minBrightness;
    int maxBrightness;
    int minContrast;
    int maxContrast;
};

class WebcamCapture {
private:
#ifdef _WIN32
    IGraphBuilder* pGraph = nullptr; // граф фильтров
    ICaptureGraphBuilder2* pBuild = nullptr; // построение графа захвата
    IMediaControl* pControl = nullptr; // управление воспроизведением видео
    IBaseFilter* pCaptureFilter = nullptr; // фильтры камеры
    ISampleGrabber* pGrabber = nullptr; // захват кадров 
    IBaseFilter* pNullRenderer = nullptr; 

    int width = 640;
    int height = 480;

    std::vector<unsigned char> buffer;
    bool isInitialized = false;

    class SampleGrabberCallback : public ISampleGrabberCB { // класс дл€ захвата кадров
    private:
        std::vector<unsigned char>& bufferRef;
        int width, height;

    public:
        SampleGrabberCallback(std::vector<unsigned char>& buf, int w, int h)
            : bufferRef(buf), width(w), height(h) {
        }

        //не используютс€
        STDMETHODIMP_(ULONG) AddRef() { return 1; }
        STDMETHODIMP_(ULONG) Release() { return 2; }
        STDMETHODIMP QueryInterface(REFIID, void**) { return E_NOINTERFACE; }

        STDMETHODIMP SampleCB(double, IMediaSample*) { return E_NOTIMPL; }



        STDMETHODIMP BufferCB(double, BYTE* pBuffer, long BufferLen) {
            if (BufferLen == width * height * 3) {
                bufferRef.assign(pBuffer, pBuffer + BufferLen);
            }
            return S_OK;
        }
    };

    SampleGrabberCallback* pCallback = nullptr;
#endif

public:
    ~WebcamCapture();
    std::vector<CameraInfo> listCameras();
    bool initialize(int cameraIndex = 0);
    void cleanup();
    bool getFrame(sf::Image& image);
    int getWidth() const;
    int getHeight() const;

    void getCameraProperties(CameraInfo& cameraInfo);
    void getResolutionsAndFPS(IBaseFilter* pFilter, CameraInfo& cameraInfo);
    void getBrightnessAndContrastRange(IBaseFilter* pFilter, CameraInfo& cameraInfo);
    IPin* getPin(IBaseFilter* pFilter, PIN_DIRECTION dir);
};