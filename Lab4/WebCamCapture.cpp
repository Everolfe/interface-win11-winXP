#include "WebcamCapture.hpp"

WebcamCapture::~WebcamCapture() {
    cleanup();
}


std::vector<CameraInfo> WebcamCapture::listCameras() {
    std::vector<CameraInfo> cameras;

#ifdef _WIN32
    CoInitialize(nullptr);

    ICreateDevEnum* pDevEnum = nullptr; // интерфейс для перечисления устройств
    IEnumMoniker* pEnum = nullptr; // перечисления указателя устройств

    // Создание системного перечисления устройств
    HRESULT hr = CoCreateInstance(CLSID_SystemDeviceEnum, nullptr,
        CLSCTX_INPROC_SERVER, IID_ICreateDevEnum, (void**)&pDevEnum);

    if (SUCCEEDED(hr)) {
        // Создание перечисления для видеоустройств
        hr = pDevEnum->CreateClassEnumerator(CLSID_VideoInputDeviceCategory, &pEnum, 0);

        if (hr == S_OK) {
            IMoniker* pMoniker = nullptr; // указатель на устройсво
            int index = 0;

            while (pEnum->Next(1, &pMoniker, nullptr) == S_OK) {
                IPropertyBag* pPropBag; // свойства устройства
                //получение свойство устройства
                hr = pMoniker->BindToStorage(0, 0, IID_IPropertyBag, (void**)&pPropBag);

                if (SUCCEEDED(hr)) {
                    
                    VARIANT varName; // для хранения данных устройства
                    VariantInit(&varName);

                    hr = pPropBag->Read(L"FriendlyName", &varName, 0);
                    if (SUCCEEDED(hr)) {
                        CameraInfo info;
                        info.name = varName.bstrVal;
                        info.index = index++;

                        VARIANT varPath;
                        VariantInit(&varPath);
                        hr = pPropBag->Read(L"DevicePath", &varPath, 0);
                        if (SUCCEEDED(hr)) {
                            info.path = varPath.bstrVal;
                            VariantClear(&varPath);
                        }

                        // Получаем дополнительные свойства камеры
                        getCameraProperties(info);

                        cameras.push_back(info);
                        VariantClear(&varName);
                    }
                    pPropBag->Release(); 
                }
                pMoniker->Release(); 
            }
            pEnum->Release();
        }
        pDevEnum->Release();
    }

    CoUninitialize();
#endif
    return cameras;
}

#ifdef _WIN32
void FreeMediaType(AM_MEDIA_TYPE& mt) {
    if (mt.cbFormat != 0) { // если есть данные формата
        CoTaskMemFree((PVOID)mt.pbFormat);
        mt.cbFormat = 0;
        mt.pbFormat = NULL;
    }
    if (mt.pUnk != NULL) { // если есть связанный COM-объект
        mt.pUnk->Release();
        mt.pUnk = NULL;
    }
}
void WebcamCapture::getCameraProperties(CameraInfo& cameraInfo) {
    IBaseFilter* pFilter = nullptr; // фильтр камеры
    IMoniker* pMoniker = nullptr;
    ICreateDevEnum* pDevEnum = nullptr;
    IEnumMoniker* pEnum = nullptr;

    HRESULT hr = CoCreateInstance(CLSID_SystemDeviceEnum, nullptr,
        CLSCTX_INPROC_SERVER, IID_ICreateDevEnum, (void**)&pDevEnum);

    if (SUCCEEDED(hr)) {
        hr = pDevEnum->CreateClassEnumerator(CLSID_VideoInputDeviceCategory, &pEnum, 0);

        if (SUCCEEDED(hr)) {
            int currentIndex = 0;
            while (pEnum->Next(1, &pMoniker, nullptr) == S_OK) {
                if (currentIndex == cameraInfo.index) {
                    // Преобразование указателя в COM-объект фильтра камеры
                    hr = pMoniker->BindToObject(0, 0, IID_IBaseFilter, (void**)&pFilter);
                    if (SUCCEEDED(hr)) {
                        // Получаем разрешения и FPS
                        getResolutionsAndFPS(pFilter, cameraInfo);

                        // Получаем диапазоны яркости и контраста
                        getBrightnessAndContrastRange(pFilter, cameraInfo);

                        pFilter->Release();
                    }
                    pMoniker->Release();
                    break;
                }
                currentIndex++;
                pMoniker->Release();
            }
            pEnum->Release();
        }
        pDevEnum->Release();
    }
}

void WebcamCapture::getResolutionsAndFPS(IBaseFilter* pFilter, CameraInfo& cameraInfo) {
    IAMStreamConfig* pStreamConfig = nullptr; // конфигурация потока
    IPin* pPin = getPin(pFilter, PINDIR_OUTPUT);

    if (pPin && SUCCEEDED(pPin->QueryInterface(IID_IAMStreamConfig, (void**)&pStreamConfig))) {
        int iCount = 0, iSize = 0;
        // Получаем количество поддерживаемых форматов видео
        if (SUCCEEDED(pStreamConfig->GetNumberOfCapabilities(&iCount, &iSize))) {
            if (iSize == sizeof(VIDEO_STREAM_CONFIG_CAPS)) {
                for (int iFormat = 0; iFormat < iCount; iFormat++) {
                    AM_MEDIA_TYPE* pmt = nullptr; // структура медиа-типа
                    VIDEO_STREAM_CONFIG_CAPS caps;

                    if (SUCCEEDED(pStreamConfig->GetStreamCaps(iFormat, &pmt, (BYTE*)&caps))) {
                        if (pmt->formattype == FORMAT_VideoInfo) {
                            VIDEOINFOHEADER* pvi = (VIDEOINFOHEADER*)pmt->pbFormat; // информация о видео

                            // Получаем разрешение
                            int width = pvi->bmiHeader.biWidth;
                            int height = abs(pvi->bmiHeader.biHeight);

                            // Получаем FPS
                            if (pvi->AvgTimePerFrame > 0) {
                                double fps = 10000000.0 / pvi->AvgTimePerFrame;
                                cameraInfo.fpsOptions.push_back(fps);
                            }

                            cameraInfo.resolutions.push_back(std::make_pair(width, height));
                        }
                        FreeMediaType(*pmt);
                        CoTaskMemFree(pmt);
                    }
                }
            }
        }
        pStreamConfig->Release();
    }

    if (pPin) pPin->Release();
}

void WebcamCapture::getBrightnessAndContrastRange(IBaseFilter* pFilter, CameraInfo& cameraInfo) {
    IAMVideoProcAmp* pProcAmp = nullptr; // интерфейс для управления настройками видеообработки

    if (SUCCEEDED(pFilter->QueryInterface(IID_IAMVideoProcAmp, (void**)&pProcAmp))) {
        long minVal, maxVal, step, defaultVal, flags;

        // Получаем диапазон яркости
        if (SUCCEEDED(pProcAmp->GetRange(VideoProcAmp_Brightness, &minVal, &maxVal, &step, &defaultVal, &flags))) {
            cameraInfo.minBrightness = minVal;
            cameraInfo.maxBrightness = maxVal;
        }

        // Получаем диапазон контраста
        if (SUCCEEDED(pProcAmp->GetRange(VideoProcAmp_Contrast, &minVal, &maxVal, &step, &defaultVal, &flags))) {
            cameraInfo.minContrast = minVal;
            cameraInfo.maxContrast = maxVal;
        }

        pProcAmp->Release();
    }
}

IPin* WebcamCapture::getPin(IBaseFilter* pFilter, PIN_DIRECTION dir) {
    IEnumPins* pEnumPins = nullptr;
    IPin* pPin = nullptr;

    if (SUCCEEDED(pFilter->EnumPins(&pEnumPins))) {
        while (pEnumPins->Next(1, &pPin, nullptr) == S_OK) {
            PIN_DIRECTION pinDir;
            pPin->QueryDirection(&pinDir);

            if (pinDir == dir) {
                pEnumPins->Release();
                return pPin;
            }
            pPin->Release();
        }
        pEnumPins->Release();
    }

    return nullptr;
}

#endif

bool WebcamCapture::initialize(int cameraIndex) {
#ifdef _WIN32
    CoInitialize(nullptr);

    //создание графа фильтров
    HRESULT hr = CoCreateInstance(CLSID_FilterGraph, nullptr,
        CLSCTX_INPROC_SERVER, IID_IGraphBuilder, (void**)&pGraph);

    if (FAILED(hr)) return false;

    //создание графа захвата
    hr = CoCreateInstance(CLSID_CaptureGraphBuilder2, nullptr,
        CLSCTX_INPROC_SERVER, IID_ICaptureGraphBuilder2, (void**)&pBuild);

    if (FAILED(hr)) return false;

    pBuild->SetFiltergraph(pGraph);

    // Поиск камеры
    ICreateDevEnum* pDevEnum = nullptr;
    IEnumMoniker* pEnum = nullptr;

    hr = CoCreateInstance(CLSID_SystemDeviceEnum, nullptr,
        CLSCTX_INPROC_SERVER, IID_ICreateDevEnum, (void**)&pDevEnum);

    if (FAILED(hr)) return false;

    hr = pDevEnum->CreateClassEnumerator(CLSID_VideoInputDeviceCategory, &pEnum, 0);

    if (hr == S_OK) {
        IMoniker* pMoniker = nullptr;
        int currentIndex = 0;

        while (pEnum->Next(1, &pMoniker, nullptr) == S_OK) {
            if (currentIndex == cameraIndex) {
                hr = pMoniker->BindToObject(0, 0, IID_IBaseFilter, (void**)&pCaptureFilter);
                pMoniker->Release();
                break;
            }
            pMoniker->Release();
            currentIndex++;
        }
        pEnum->Release();
    }
    pDevEnum->Release();

    if (!pCaptureFilter) return false;

    pGraph->AddFilter(pCaptureFilter, L"Capture Filter"); 

    // Настройка формата видео
    IAMStreamConfig* pConfig = nullptr;
    hr = pBuild->FindInterface(&PIN_CATEGORY_CAPTURE, &MEDIATYPE_Video,
        pCaptureFilter, IID_IAMStreamConfig, (void**)&pConfig);

    if (SUCCEEDED(hr)) {
        AM_MEDIA_TYPE* pmt;
        pConfig->GetFormat(&pmt);

        VIDEOINFOHEADER* pVih = (VIDEOINFOHEADER*)pmt->pbFormat;
        pVih->bmiHeader.biWidth = width;
        pVih->bmiHeader.biHeight = height;
        pVih->bmiHeader.biCompression = BI_RGB;

        pConfig->SetFormat(pmt);
        pConfig->Release();
    }

    // настройка интерфейса захвата кадра
    hr = CoCreateInstance(CLSID_SampleGrabber, nullptr, CLSCTX_INPROC_SERVER,
        IID_IBaseFilter, (void**)&pNullRenderer);

    if (FAILED(hr)) return false;

    pGraph->AddFilter(pNullRenderer, L"Sample Grabber");
    pNullRenderer->QueryInterface(IID_ISampleGrabber, (void**)&pGrabber);

    AM_MEDIA_TYPE mt;
    ZeroMemory(&mt, sizeof(mt));
    mt.majortype = MEDIATYPE_Video;
    mt.subtype = MEDIASUBTYPE_RGB24;
    pGrabber->SetMediaType(&mt);
    pGrabber->SetBufferSamples(FALSE);
    pGrabber->SetOneShot(FALSE);

    buffer.resize(width * height * 3);
    pCallback = new SampleGrabberCallback(buffer, width, height);
    pGrabber->SetCallback(pCallback, 1);

    // создание рендера для отбрасывания
    IBaseFilter* pNull = nullptr;
    hr = CoCreateInstance(CLSID_NullRenderer, nullptr, CLSCTX_INPROC_SERVER,
        IID_IBaseFilter, (void**)&pNull);
    pGraph->AddFilter(pNull, L"Null Renderer");

    // связываем камеру с графом захвата 
    hr = pBuild->RenderStream(&PIN_CATEGORY_CAPTURE, &MEDIATYPE_Video,
        pCaptureFilter, pNullRenderer, pNull);

    if (FAILED(hr)) return false;

    pGraph->QueryInterface(IID_IMediaControl, (void**)&pControl);
    pControl->Run();

    isInitialized = true;
    return true;
#else
    return false;
#endif
}

void WebcamCapture::cleanup() {
#ifdef _WIN32
    if (pControl) {
        pControl->Stop();
        pControl->Release();
        pControl = nullptr;
    }
    if (pGrabber) {
        pGrabber->Release();
        pGrabber = nullptr;
    }
    if (pNullRenderer) {
        pNullRenderer->Release();
        pNullRenderer = nullptr;
    }
    if (pCaptureFilter) {
        pCaptureFilter->Release();
        pCaptureFilter = nullptr;
    }
    if (pBuild) {
        pBuild->Release();
        pBuild = nullptr;
    }
    if (pGraph) {
        pGraph->Release();
        pGraph = nullptr;
    }
    if (pCallback) {
        delete pCallback;
        pCallback = nullptr;
    }

    CoUninitialize();
#endif
}

bool WebcamCapture::getFrame(sf::Image& image) {
#ifdef _WIN32
    if (!isInitialized || buffer.empty()) return false;

    image.create(width, height);

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int srcIdx = ((height - 1 - y) * width + x) * 3;
            image.setPixel(x, y, sf::Color(
                buffer[srcIdx + 2],  // R
                buffer[srcIdx + 1],  // G
                buffer[srcIdx],      // B
                255
            ));
        }
    }

    return true;
#else
    return false;
#endif
}

int WebcamCapture::getWidth() const {
    return 640;
}

int WebcamCapture::getHeight() const {
    return 480;
}