#include"AdditionalFunc.hpp"
#include "Button.hpp"
#include"State.hpp"
#include"MainMenu.hpp"

static HWND g_hwnd = nullptr;
static WNDPROC g_originalWndProc = nullptr;
static bool g_appRunning = true;

LRESULT CALLBACK OurWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (!g_appRunning) return DefWindowProc(hwnd, msg, wParam, lParam);

    //изменение устройства
    if (msg == WM_DEVICECHANGE) {
        switch (wParam) {
        case DBT_DEVICEARRIVAL:
        case DBT_DEVICEREMOVECOMPLETE:
        case DBT_DEVICEQUERYREMOVEFAILED:
            std::thread([]() {
                if (LabWindow::activeWindow != nullptr) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                    LabWindow::activeWindow->refreshDeviceList();
                }
                }).detach(); 
            break;
        default:
            break;
        }
    }

    if (g_originalWndProc)
        return CallWindowProc(g_originalWndProc, hwnd, msg, wParam, lParam);
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

void HookSFMLWindow(HWND hwnd) {
    g_hwnd = hwnd;
    g_originalWndProc = (WNDPROC)SetWindowLongPtr(hwnd, GWLP_WNDPROC, (LONG_PTR)OurWndProc);

    DEV_BROADCAST_DEVICEINTERFACE_W filter{};
    ZeroMemory(&filter, sizeof(filter));
    filter.dbcc_size = sizeof(filter);
    filter.dbcc_devicetype = DBT_DEVTYP_DEVICEINTERFACE;
    filter.dbcc_classguid = GUID_DEVINTERFACE_USB_DEVICE;
    RegisterDeviceNotificationW(hwnd, &filter, DEVICE_NOTIFY_WINDOW_HANDLE);
}

void UnhookSFMLWindow() {
    g_appRunning = false;

    if (g_hwnd && g_originalWndProc) {
        SetWindowLongPtr(g_hwnd, GWLP_WNDPROC, (LONG_PTR)g_originalWndProc);
        g_originalWndProc = nullptr;
        g_hwnd = nullptr;
    }
}
int main()
{
    std::string exePath = getExecutablePath();
    SetCurrentDirectoryA(exePath.c_str());

    sf::RenderWindow window(sf::VideoMode(800, 600), "Laboratory Works Interface", sf::Style::Resize | sf::Style::Close);
    window.setFramerateLimit(60);
    HWND hwnd = window.getSystemHandle();
    HookSFMLWindow(hwnd);

    MainMenu mainMenu(window);
    sf::Clock clock;
    State currentState = State::MainMenu;

    while (window.isOpen())
    {
        float deltaTime = clock.restart().asSeconds();
        sf::Event event;

        while (window.pollEvent(event))
        {
            if (event.type == sf::Event::Closed)
                window.close();

            else if (event.type == sf::Event::Resized)
            {
                sf::FloatRect visibleArea(0, 0, event.size.width, event.size.height);
                window.setView(sf::View(visibleArea));
                mainMenu.updateWindowSize();
            }

            if (currentState == State::MainMenu)
                mainMenu.handleEvent(event);
        }

        if (currentState == State::MainMenu)
        {
            mainMenu.update(deltaTime);

            if (!mainMenu.isInTransition() && mainMenu.getNextState() != State::MainMenu)
            {
                currentState = mainMenu.getNextState();
                mainMenu.resetState();
            }
        }

        window.clear();

        if (currentState == State::MainMenu)
        {
            mainMenu.draw();
        }
        else
        {
            int labIndex = static_cast<int>(currentState) - 1;

            LabWindow* lab = mainMenu.getLabByIndex(labIndex);
            if (lab)
            {
                lab->run();
                currentState = State::MainMenu;
                mainMenu.resetButtonState();
            }
            else
            {
                std::cerr << "Invalid lab index!\n";
                currentState = State::MainMenu;
            }
        }


        window.display();
    }
    UnhookSFMLWindow();
    return 0;
}



