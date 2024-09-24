#include "GuiAlbumManager.h"
#include <iostream>
#include <string>
#include "MyException.h"
#include "AlbumNotOpenException.h"
#include <vector>
#include <commdlg.h>
#include <iomanip>
#include <chrono>
#include <ctime>
#include <chrono>
#include <iomanip>
#include <sstream>

IDataAccess* dataAccessPtr;

//galley related
Album m_openAlbum;
std::string m_currentAlbumName;
std::string m_currentPictureName;
int m_openUserId;

//windows
HWND g_hMainWindow;
HWND g_hCreateUserFrame;
HWND g_hOpenUserFrame;
HWND g_hCreateAlbumFrame;
HWND g_hOpenAlbumFrame;
HWND g_hCreatePictureFrame;
HWND g_hOpenPictureFrame;
HWND g_hTagUserFrame;

//main
HWND g_hUserNameInput;
HWND g_hComboBox;
HWND g_topTaggedUser;
HWND g_topTaggedPicture;
HWND g_hUserStatistics;

// album
HWND g_albumComboBox;
HWND g_hAlbumNameInput;
HWND g_topTaggedPictureOfUser;

//picture
HWND g_hPictureComboBox;
HWND g_hPictureNameInput;
HWND g_hFilePathDisplay;
std::string fullPicPath;

// tags
HWND g_hUsersTaggedComboBox;
HWND g_hUsersNotTaggedComboBox;



GuiAlbumManager::GuiAlbumManager(IDataAccess& dataAccess) : m_dataAccess(dataAccess) {
    m_dataAccess.open();
    dataAccessPtr = &dataAccess;
}


// ******************* Users ******************* 
void GuiAlbumManager::CreateMainWindow()
{
    WNDCLASS wc = {};
    wc.lpfnWndProc = MainWindowProc;
    wc.hInstance = GetModuleHandle(nullptr);
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.lpszClassName = "MainWindowClass";
    RegisterClass(&wc);

    g_hMainWindow = CreateWindowEx(0, "MainWindowClass", "Main Window", WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 400, 300, nullptr, nullptr, GetModuleHandle(nullptr), this);

    CreateCreateUserFrame();
    CreateOpenUserFrame();

    g_hComboBox = CreateWindow("COMBOBOX", "", CBS_DROPDOWN | CBS_HASSTRINGS | WS_CHILD | WS_VISIBLE,
        100, 50, 200, 50, g_hMainWindow, nullptr, nullptr, nullptr);

    g_topTaggedUser = CreateWindow("STATIC", "top tagged", WS_VISIBLE | WS_CHILD | SS_CENTER, 
        50, 220, 300, 18, g_hMainWindow, nullptr, nullptr, nullptr);

    g_topTaggedPicture = CreateWindow("STATIC", "top tagged", WS_VISIBLE | WS_CHILD | SS_CENTER,
        50, 240, 300, 18, g_hMainWindow, nullptr, nullptr, nullptr);

    updateMainOptions();

    ShowWindow(g_hMainWindow, SW_SHOW);
}

LRESULT GuiAlbumManager::MainWindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    GuiAlbumManager* manager = reinterpret_cast<GuiAlbumManager*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
    if (manager == nullptr && msg == WM_CREATE) {
        manager = reinterpret_cast<GuiAlbumManager*>(((LPCREATESTRUCT)lParam)->lpCreateParams);
        SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(manager));
    }

    if (manager != nullptr) {
        return manager->MainWindowHandler(hwnd, msg, wParam, lParam);
    }

    return DefWindowProc(hwnd, msg, wParam, lParam);
}

LRESULT GuiAlbumManager::MainWindowHandler(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg) {
    case WM_CREATE: {
        CreateWindow("STATIC", "Users", WS_VISIBLE | WS_CHILD | SS_CENTER, 100, 10, 200, 30, hwnd, nullptr, nullptr, nullptr);

        CreateWindow("BUTTON", "Open User", WS_VISIBLE | WS_CHILD,
            100, 100, 200, 50, hwnd, reinterpret_cast<HMENU>(1), nullptr, nullptr);

        CreateWindow("BUTTON", "Create User", WS_VISIBLE | WS_CHILD,
            100, 160, 100, 50, hwnd, reinterpret_cast<HMENU>(2), nullptr, nullptr);

        CreateWindow("BUTTON", "Delete User", WS_VISIBLE | WS_CHILD,
            200, 160, 100, 50, hwnd, reinterpret_cast<HMENU>(3), nullptr, nullptr);
        break;
    }
    case WM_COMMAND: {
        int wmId = LOWORD(wParam);
        switch (wmId) {
        case 1: {
            ShowWindow(g_hOpenUserFrame, SW_SHOW);
            ShowWindow(g_hMainWindow, SW_HIDE);
            m_openUserId = getSelectedUserId();
            updateAlbumOptions();
            break;
        }
        case 2:
            ShowWindow(g_hCreateUserFrame, SW_SHOW);
            ShowWindow(g_hMainWindow, SW_HIDE);
            break;
        case 3: {
            try {
                m_openUserId = getSelectedUserId();
                m_dataAccess.deleteUser(m_dataAccess.getUser(m_openUserId));
                updateMainOptions();
            }
            catch (std::exception& e) {
                MessageBox(NULL, e.what(), "Error", MB_ICONERROR | MB_OK);
            }
        }

        }
        break;
    }
    case WM_DESTROY: {
        PostQuitMessage(0);
        break;
    }
    default:
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}

int GuiAlbumManager::getSelectedUserId()
{
    int selectedIndex = SendMessage(g_hComboBox, CB_GETCURSEL, 0, 0);
    if (selectedIndex != CB_ERR) {
        int textLength = SendMessage(g_hComboBox, CB_GETLBTEXTLEN, selectedIndex, 0);
        if (textLength != CB_ERR) {
            std::vector<char> buffer(textLength + 1);
            SendMessage(g_hComboBox, CB_GETLBTEXT, selectedIndex, reinterpret_cast<LPARAM>(buffer.data()));
            std::string selectedItem(buffer.data());

            size_t spacePos = selectedItem.find(' ');
            if (spacePos != std::string::npos) {
                std::string idStr = selectedItem.substr(0, spacePos);
                return std::stoi(idStr);
            }
        }
    }

    return 0;
} 

void GuiAlbumManager::updateMainOptions()
{
    try {
        SendMessage(g_hComboBox, CB_RESETCONTENT, 0, 0);

        std::list<User> users = dataAccessPtr->getUsers();

        for (auto user : users) {
            std::string itemName = std::to_string(user.getId()) + " - " + user.getName();
            SendMessage(g_hComboBox, (UINT)CB_ADDSTRING, (WPARAM)0, (LPARAM)(itemName.c_str()));
        }

        SendMessage(g_hComboBox, CB_SETCURSEL, (WPARAM)0, (LPARAM)0);

        try {
            std::string display = "The most tagged user is: " + dataAccessPtr->getTopTaggedUser().getName();
            SetWindowText(g_topTaggedUser, display.c_str());
        }
        catch (std::exception& e) {
            std::string display = e.what();
            SetWindowText(g_topTaggedUser, display.c_str());
        }

        try {
            std::string display = "The most tagged picture is: " + dataAccessPtr->getTopTaggedPicture().getName();
            SetWindowText(g_topTaggedPicture, display.c_str());
        }
        catch (std::exception& e) {
            std::string display = e.what();
            SetWindowText(g_topTaggedPicture, display.c_str());
        }
        
    }
    catch (std::exception& e) {
        MessageBox(NULL, e.what(), "Error", MB_ICONERROR | MB_OK);
    }
}

void GuiAlbumManager::CreateCreateUserFrame()
{
    WNDCLASS wc = {};
    wc.lpfnWndProc = CreateUserFrameProc;
    wc.hInstance = GetModuleHandle(nullptr);
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.lpszClassName = "CreateUserFrameClass";
    RegisterClass(&wc);

    g_hCreateUserFrame = CreateWindowEx(0, "CreateUserFrameClass", "Create User", WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 400, 300, nullptr, nullptr, GetModuleHandle(nullptr), nullptr);

    SetWindowLongPtr(g_hCreateUserFrame, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(g_hMainWindow));

    CreateWindow("BUTTON", "Back", WS_VISIBLE | WS_CHILD,
        10, 10, 80, 30, g_hCreateUserFrame, reinterpret_cast<HMENU>(1), nullptr, nullptr);

    CreateWindow("STATIC", "Enter new User Name:", WS_VISIBLE | WS_CHILD | SS_CENTER, 100, 70, 200, 30, g_hCreateUserFrame, nullptr, nullptr, nullptr);

    g_hUserNameInput = CreateWindow("EDIT", "", WS_VISIBLE | WS_CHILD | WS_BORDER | ES_AUTOHSCROLL,
        100, 100, 200, 25, g_hCreateUserFrame, nullptr, nullptr, nullptr);

    CreateWindow("BUTTON", "Create", WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
        150, 150, 100, 30, g_hCreateUserFrame, reinterpret_cast<HMENU>(2), nullptr, nullptr);
}


LRESULT GuiAlbumManager::CreateUserFrameProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    GuiAlbumManager* manager = reinterpret_cast<GuiAlbumManager*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
    if (manager == nullptr && msg == WM_CREATE) {
        manager = reinterpret_cast<GuiAlbumManager*>(((LPCREATESTRUCT)lParam)->lpCreateParams);
        SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(manager));
    }

    if (manager != nullptr) {
        return manager->CreateUserFrameHandler(hwnd, msg, wParam, lParam);
    }

    return DefWindowProc(hwnd, msg, wParam, lParam);
}

LRESULT GuiAlbumManager::CreateUserFrameHandler(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg) {
    case WM_CREATE: {
        g_hUserNameInput = CreateWindow("EDIT", "", WS_VISIBLE | WS_CHILD | WS_BORDER | ES_AUTOHSCROLL,
            100, 100, 200, 25, hwnd, nullptr, nullptr, nullptr);
        break;
    }
    case WM_COMMAND: {
        int wmId = LOWORD(wParam);
        switch (wmId) {
        case 1: {
            HWND hMainWindow = reinterpret_cast<HWND>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
            ShowWindow(hMainWindow, SW_SHOW);
            ShowWindow(hwnd, SW_HIDE);
            updateMainOptions();
            break;
        }
        case 2: {
            try {
                int len = GetWindowTextLength(g_hUserNameInput) + 1;
                if (len == 1) {
                    MessageBox(hwnd, "Please enter a username.", "Error", MB_OK | MB_ICONERROR);
                    return 0;
                }

                char* username = new char[len];
                GetWindowText(g_hUserNameInput, username, len);

                std::string name = username;
                User user(0, name); 
                dataAccessPtr->createUser(user);

                delete[] username;

                HWND hMainWindow = reinterpret_cast<HWND>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
                ShowWindow(hMainWindow, SW_SHOW);
                ShowWindow(hwnd, SW_HIDE);

                updateMainOptions();
            }
            catch (std::exception& e) {
                MessageBox(NULL, e.what(), "Error", MB_ICONERROR | MB_OK);
            }
            break;
        }
        }

        break;
    }
    case WM_DESTROY: {
        PostQuitMessage(0);
        break;
    }
    default:
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}



// ******************* Album ******************* 
void GuiAlbumManager::CreateOpenUserFrame()
{
    WNDCLASS wc = {};
    wc.lpfnWndProc = OpenUserFrameProc;
    wc.hInstance = GetModuleHandle(nullptr);
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.lpszClassName = "OpenUserFrameClass";
    RegisterClass(&wc);

    g_hOpenUserFrame = CreateWindowEx(0, "OpenUserFrameClass", "Open User", WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 400, 350, nullptr, nullptr, GetModuleHandle(nullptr), nullptr);

    CreateOpenAlbumFrame();
    CreateCreateAlbumFrame();

    SetWindowLongPtr(g_hOpenUserFrame, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(g_hMainWindow));


    CreateWindow("BUTTON", "Back", WS_VISIBLE | WS_CHILD,
        10, 10, 80, 30, g_hOpenUserFrame, reinterpret_cast<HMENU>(1), nullptr, nullptr);

    CreateWindow("STATIC", "Albums", WS_VISIBLE | WS_CHILD | SS_CENTER,
        100, 10, 200, 30, g_hOpenUserFrame, nullptr, nullptr, nullptr);

    g_albumComboBox = CreateWindow("COMBOBOX", "", CBS_DROPDOWN | CBS_HASSTRINGS | WS_CHILD | WS_VISIBLE,
        100, 50, 200, 50, g_hOpenUserFrame, nullptr, nullptr, nullptr);

    CreateWindow("BUTTON", "Open Album", WS_VISIBLE | WS_CHILD,
        100, 100, 200, 50, g_hOpenUserFrame, reinterpret_cast<HMENU>(2), nullptr, nullptr);

    CreateWindow("BUTTON", "Delete Album", WS_VISIBLE | WS_CHILD,
        100, 160, 100, 50, g_hOpenUserFrame, reinterpret_cast<HMENU>(3), nullptr, nullptr);

    CreateWindow("BUTTON", "Create Album", WS_VISIBLE | WS_CHILD,
        200, 160, 100, 50, g_hOpenUserFrame, reinterpret_cast<HMENU>(4), nullptr, nullptr);

    g_topTaggedPictureOfUser = CreateWindow("STATIC", "top tagged", WS_VISIBLE | WS_CHILD | SS_CENTER,
        50, 220, 300, 18, g_hOpenUserFrame, nullptr, nullptr, nullptr);

    g_hUserStatistics = CreateWindow("STATIC", "statistics", WS_VISIBLE | WS_CHILD | SS_CENTER,
        50, 238, 300, 70, g_hOpenUserFrame, nullptr, nullptr, nullptr);

    ShowWindow(g_hMainWindow, SW_SHOW);
}

LRESULT GuiAlbumManager::OpenUserFrameProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    GuiAlbumManager* manager = reinterpret_cast<GuiAlbumManager*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
    if (manager != nullptr) {
        return manager->OpenUserFrameHandler(hwnd, msg, wParam, lParam);
    }

    return DefWindowProc(hwnd, msg, wParam, lParam);
}

LRESULT GuiAlbumManager::OpenUserFrameHandler(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg) {
    case WM_COMMAND: {
        int wmId = LOWORD(wParam);
        switch (wmId) {
        case 1: {
            HWND hMainWindow = reinterpret_cast<HWND>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
            ShowWindow(hMainWindow, SW_SHOW);
            ShowWindow(hwnd, SW_HIDE);
            updateMainOptions();
            break;
        }
            
        case 2: {
            try {
                HWND hOpenAlbumFrame = reinterpret_cast<HWND>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
                ShowWindow(g_hOpenAlbumFrame, SW_SHOW);
                ShowWindow(hwnd, SW_HIDE);
                m_currentAlbumName = getSelectedAlbumName();
                updatePictureOptions();

                m_openAlbum = dataAccessPtr->openAlbum(m_currentAlbumName);
            }
            catch (std::exception& e) {
                MessageBox(NULL, e.what(), "Error", MB_ICONERROR | MB_OK);
            }
            break;
        }

        case 3: {
            try {
                m_currentAlbumName = getSelectedAlbumName();
                dataAccessPtr->deleteAlbum(m_currentAlbumName, m_openUserId);
                updateAlbumOptions();
            }
            catch (std::exception& e) {
                MessageBox(NULL, e.what(), "Error", MB_ICONERROR | MB_OK);
            }
            
            break;
        }
        case 4: {
            ShowWindow(g_hCreateAlbumFrame, SW_SHOW);
            ShowWindow(hwnd, SW_HIDE);
            break;
        }
        }
        break;
    }
    case WM_DESTROY: {
        PostQuitMessage(0);
        break;
    }
    default:
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}

std::string GuiAlbumManager::getSelectedAlbumName()
{
    int selectedIndex = SendMessage(g_albumComboBox, CB_GETCURSEL, 0, 0);
    if (selectedIndex != CB_ERR) {
        int textLength = SendMessage(g_albumComboBox, CB_GETLBTEXTLEN, selectedIndex, 0);
        if (textLength != CB_ERR) {
            std::vector<char> buffer(textLength + 1);
            SendMessage(g_albumComboBox, CB_GETLBTEXT, selectedIndex, reinterpret_cast<LPARAM>(buffer.data()));
            std::string selectedItem(buffer.data());

            return selectedItem;
            
        }
    }

    return "";
}

void GuiAlbumManager::updateAlbumOptions()
{
    try {
        SendMessage(g_albumComboBox, CB_RESETCONTENT, 0, 0);

        std::list<Album> albums = dataAccessPtr->getAlbumsOfUser(dataAccessPtr->getUser(m_openUserId));

        for (auto album : albums) {
            std::string itemName = album.getName();
            SendMessage(g_albumComboBox, (UINT)CB_ADDSTRING, (WPARAM)0, (LPARAM)(itemName.c_str()));
        }

        SendMessage(g_albumComboBox, CB_SETCURSEL, (WPARAM)0, (LPARAM)0);

        try {
            User user(m_openUserId, ""); // name doesn't matter
            std::list<Picture> pics = dataAccessPtr->getTaggedPicturesOfUser(user);
            std::string display = "The most tagged pictures are: ";
            if (!pics.empty()) {
                auto it = pics.begin();
                display += it->getName();
                ++it;
                for (; it != pics.end(); ++it) {
                    display += ", " + it->getName();
                }
            }
            
            SetWindowText(g_topTaggedPictureOfUser, display.c_str());
            
        }
        catch (std::exception& e) {
            std::string display = e.what();
            SetWindowText(g_topTaggedPictureOfUser, display.c_str());
        }
        try {
            User user(m_openUserId, ""); // name doesn't matter
            std::string display = "Albums owned: " + std::to_string(dataAccessPtr->countAlbumsOwnedOfUser(user)) +
                "\n Albums Tagged: " + std::to_string(dataAccessPtr->countAlbumsTaggedOfUser(user)) +
                "\n Tags of user: " + std::to_string(dataAccessPtr->countTagsOfUser(user)) +
                "\n Average tags per album: " + std::to_string(dataAccessPtr->averageTagsPerAlbumOfUser(user));

            SetWindowText(g_hUserStatistics, display.c_str());

        }
        catch (std::exception& e) {
            std::string display = e.what();
            SetWindowText(g_hUserStatistics, display.c_str());
        }
        
    }
    catch (std::exception& e) {
        MessageBox(NULL, e.what(), "Error", MB_ICONERROR | MB_OK);
    }
}

void GuiAlbumManager::CreateCreateAlbumFrame()
{
    WNDCLASS wc = {};
    wc.lpfnWndProc = CreateAlbumFrameProc;
    wc.hInstance = GetModuleHandle(nullptr);
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.lpszClassName = "CreateAlbumFrameClass";
    RegisterClass(&wc);

    g_hCreateAlbumFrame = CreateWindowEx(0, "CreateAlbumFrameClass", "Create Album", WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 400, 300, nullptr, nullptr, GetModuleHandle(nullptr), nullptr);

    SetWindowLongPtr(g_hCreateAlbumFrame, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(g_hMainWindow));

    CreateWindow("BUTTON", "Back", WS_VISIBLE | WS_CHILD,
        10, 10, 80, 30, g_hCreateAlbumFrame, reinterpret_cast<HMENU>(1), nullptr, nullptr);

    CreateWindow("STATIC", "Enter new Album Name:", WS_VISIBLE | WS_CHILD | SS_CENTER, 100, 70, 200, 30, g_hCreateAlbumFrame, nullptr, nullptr, nullptr);

    g_hAlbumNameInput = CreateWindow("EDIT", "", WS_VISIBLE | WS_CHILD | WS_BORDER | ES_AUTOHSCROLL,
        100, 100, 200, 25, g_hCreateAlbumFrame, nullptr, nullptr, nullptr);

    CreateWindow("BUTTON", "Create", WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
        150, 150, 100, 30, g_hCreateAlbumFrame, reinterpret_cast<HMENU>(2), nullptr, nullptr);
}

LRESULT GuiAlbumManager::CreateAlbumFrameProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    GuiAlbumManager* manager = reinterpret_cast<GuiAlbumManager*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
    if (manager == nullptr && msg == WM_CREATE) {
        manager = reinterpret_cast<GuiAlbumManager*>(((LPCREATESTRUCT)lParam)->lpCreateParams);
        SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(manager));
    }

    if (manager != nullptr) {
        return manager->CreateAlbumFrameHandler(hwnd, msg, wParam, lParam);
    }

    return DefWindowProc(hwnd, msg, wParam, lParam);
}

LRESULT GuiAlbumManager::CreateAlbumFrameHandler(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg) {
    case WM_CREATE: {
        break;
    }
    case WM_COMMAND: {
        int wmId = LOWORD(wParam);
        switch (wmId) {
        case 1: {
            ShowWindow(g_hOpenUserFrame, SW_SHOW);
            ShowWindow(hwnd, SW_HIDE);
            break;
        }
        case 2: {
            try {
                int len = GetWindowTextLength(g_hAlbumNameInput) + 1; // add 1 for null terminator
                if (len == 1) {
                    MessageBox(hwnd, "Please enter a username.", "Error", MB_OK | MB_ICONERROR);
                    return 0;
                }
                char* albumname = new char[len];
                GetWindowText(g_hAlbumNameInput, albumname, len);

                std::string name = albumname;
                Album album(m_openUserId, name);
                dataAccessPtr->createAlbum(album);

                delete[] albumname;

                ShowWindow(g_hOpenUserFrame, SW_SHOW);
                ShowWindow(hwnd, SW_HIDE);

                updateAlbumOptions();
            }
            catch (std::exception& e) {
                MessageBox(NULL, e.what(), "Error", MB_ICONERROR | MB_OK);
            }
            break;
        }
        }

        break;
    }
    case WM_DESTROY: {
        PostQuitMessage(0);
        break;
    }
    default:
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}

// ******************* Picture ******************* 
void GuiAlbumManager::CreateOpenAlbumFrame()
{
    WNDCLASS wc = {};
    wc.lpfnWndProc = OpenAlbumFrameProc;
    wc.hInstance = GetModuleHandle(nullptr);
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.lpszClassName = "OpenAlbumFrameClass";
    RegisterClass(&wc);
    CreateCreatePictureFrame();
    CreateOpenPictureFrame();

    g_hOpenAlbumFrame = CreateWindowEx(0, "OpenAlbumFrameClass", "Open User", WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 400, 300, nullptr, nullptr, GetModuleHandle(nullptr), nullptr);

    SetWindowLongPtr(g_hOpenAlbumFrame, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(g_hMainWindow));

    CreateWindow("BUTTON", "Back", WS_VISIBLE | WS_CHILD,
        10, 10, 80, 30, g_hOpenAlbumFrame, reinterpret_cast<HMENU>(1), nullptr, nullptr);

    CreateWindow("STATIC", "Pictures", WS_VISIBLE | WS_CHILD | SS_CENTER,
        100, 10, 200, 30, g_hOpenAlbumFrame, nullptr, nullptr, nullptr);

    g_hPictureComboBox = CreateWindow("COMBOBOX", "", CBS_DROPDOWN | CBS_HASSTRINGS | WS_CHILD | WS_VISIBLE,
        100, 50, 200, 50, g_hOpenAlbumFrame, nullptr, nullptr, nullptr);

    CreateWindow("BUTTON", "Open With Paint", WS_VISIBLE | WS_CHILD,
        60, 100, 140, 50, g_hOpenAlbumFrame, reinterpret_cast<HMENU>(2), nullptr, nullptr);

    CreateWindow("BUTTON", "Open With IrfanView", WS_VISIBLE | WS_CHILD,
        200, 100, 140, 50, g_hOpenAlbumFrame, reinterpret_cast<HMENU>(3), nullptr, nullptr);

    CreateWindow("BUTTON", "Delete Picture", WS_VISIBLE | WS_CHILD,
        50, 160, 100, 50, g_hOpenAlbumFrame, reinterpret_cast<HMENU>(4), nullptr, nullptr);

    CreateWindow("BUTTON", "Create Picture", WS_VISIBLE | WS_CHILD,
        150, 160, 100, 50, g_hOpenAlbumFrame, reinterpret_cast<HMENU>(5), nullptr, nullptr);

    CreateWindow("BUTTON", "Users tagged", WS_VISIBLE | WS_CHILD,
        250, 160, 100, 50, g_hOpenAlbumFrame, reinterpret_cast<HMENU>(6), nullptr, nullptr);


    ShowWindow(g_hMainWindow, SW_SHOW);
}

LRESULT GuiAlbumManager::OpenAlbumFrameProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    GuiAlbumManager* manager = reinterpret_cast<GuiAlbumManager*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
    if (manager != nullptr) {
        return manager->OpenAlbumFrameHandler(hwnd, msg, wParam, lParam);
    }

    return DefWindowProc(hwnd, msg, wParam, lParam);
}

LRESULT GuiAlbumManager::OpenAlbumFrameHandler(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg) {
    case WM_COMMAND: {
        int wmId = LOWORD(wParam);
        switch (wmId) {
        case 1: {
            ShowWindow(g_hOpenUserFrame, SW_SHOW);
            ShowWindow(hwnd, SW_HIDE);
            break;
        }
        case 2: {
            try {
                m_currentPictureName = getSelectedPictureName();
                std::string cmd = "C:\\Windows\\system32\\mspaint.exe \"" +
                    dataAccessPtr->openAlbum(m_currentAlbumName).getPicture(m_currentPictureName).getPath() + "\"";
                launchImageProccess(cmd);
            }
            catch (std::exception& e) {
                MessageBox(NULL, e.what(), "Error", MB_ICONERROR | MB_OK);
            }
            break;
        }
        case 3: {
            try {
                m_currentPictureName = getSelectedPictureName();
                std::string cmd = "\"C:\\Program Files\\IrfanView\\i_view64.exe\" \"" +
                    dataAccessPtr->openAlbum(m_currentAlbumName).getPicture(m_currentPictureName).getPath() + "\"";
                launchImageProccess(cmd);
            }
            catch (std::exception& e) {
                MessageBox(NULL, e.what(), "Error", MB_ICONERROR | MB_OK);
            }
            break;
        }
        case 4: {
            try {
                m_currentPictureName = getSelectedPictureName();
                dataAccessPtr->removePictureFromAlbumByName(m_currentAlbumName, m_currentPictureName);
                updatePictureOptions();
            }
            catch (std::exception& e) {
                MessageBox(NULL, e.what(), "Error", MB_ICONERROR | MB_OK);
            }
            break;
        }
        case  5: {
            ShowWindow(g_hCreatePictureFrame, SW_SHOW);
            ShowWindow(hwnd, SW_HIDE);
            break;
        }
        case 6: {
            m_currentPictureName = getSelectedPictureName();
            updateTaggedInPictureOptions();
            ShowWindow(g_hOpenPictureFrame, SW_SHOW);
            ShowWindow(hwnd, SW_HIDE);
            break;
        }
        }

        break;
    }
    case WM_DESTROY: {
        PostQuitMessage(0);
        break;
    }
    default:
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}

void GuiAlbumManager::updatePictureOptions()
{
    try {
        SendMessage(g_hPictureComboBox, CB_RESETCONTENT, 0, 0);

        Album album = dataAccessPtr->openAlbum(m_currentAlbumName);

        for (auto picture : album.getPictures()) {
            std::string itemName = picture.getName();
            SendMessage(g_hPictureComboBox, (UINT)CB_ADDSTRING, (WPARAM)0, (LPARAM)(itemName.c_str()));
        }

        SendMessage(g_hPictureComboBox, CB_SETCURSEL, (WPARAM)0, (LPARAM)0);
    }
    catch (std::exception& e) {
        MessageBox(NULL, e.what(), "Error", MB_ICONERROR | MB_OK);
    }
}

std::string GuiAlbumManager::getSelectedPictureName()
{
    int selectedIndex = SendMessage(g_hPictureComboBox, CB_GETCURSEL, 0, 0);
    if (selectedIndex != CB_ERR) {
        int textLength = SendMessage(g_hPictureComboBox, CB_GETLBTEXTLEN, selectedIndex, 0);
        if (textLength != CB_ERR) {
            std::vector<char> buffer(textLength + 1);
            SendMessage(g_hPictureComboBox, CB_GETLBTEXT, selectedIndex, reinterpret_cast<LPARAM>(buffer.data()));
            std::string selectedItem(buffer.data());

            return selectedItem;

        }
    }

    return "";
}

void GuiAlbumManager::launchImageProccess(std::string cmd)
{
    STARTUPINFO info = { sizeof(info) };
    PROCESS_INFORMATION processInfo;
    if (CreateProcessA(NULL, const_cast<LPSTR>(cmd.c_str()), NULL, NULL, FALSE, 0, NULL, NULL, &info, &processInfo))
    {
        while (true) {
            if (WaitForSingleObject(processInfo.hProcess, 0) == WAIT_OBJECT_0) {
                break;
            }
            if (GetAsyncKeyState(VK_CONTROL) & 0x8000 && GetAsyncKeyState('C') & 0x8000) {
                TerminateProcess(processInfo.hProcess, 0);
                break;
            }
        }
        WaitForSingleObject(processInfo.hProcess, INFINITE);
        CloseHandle(processInfo.hProcess);
        CloseHandle(processInfo.hThread);
    }

}

void GuiAlbumManager::CreateCreatePictureFrame()
{
    WNDCLASS wc = {};
    wc.lpfnWndProc = CreatePictureFrameProc;
    wc.hInstance = GetModuleHandle(nullptr);
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.lpszClassName = "CreateCreatePictureClass";
    RegisterClass(&wc);

    g_hCreatePictureFrame = CreateWindowEx(0, "CreateCreatePictureClass", "Create Picture", WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 400, 300, nullptr, nullptr, GetModuleHandle(nullptr), nullptr);

    SetWindowLongPtr(g_hCreatePictureFrame, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(g_hMainWindow));

    CreateWindow("BUTTON", "Back", WS_VISIBLE | WS_CHILD,
        10, 10, 80, 30, g_hCreatePictureFrame, reinterpret_cast<HMENU>(1), nullptr, nullptr);

    CreateWindow("STATIC", "Enter new Picture Name:", WS_VISIBLE | WS_CHILD | SS_CENTER, 
        100, 50, 200, 30, g_hCreatePictureFrame, nullptr, nullptr, nullptr);

    g_hPictureNameInput = CreateWindow("EDIT", "", WS_VISIBLE | WS_CHILD | WS_BORDER | ES_AUTOHSCROLL,
        100, 80, 200, 25, g_hCreatePictureFrame, nullptr, nullptr, nullptr);

    CreateWindow("BUTTON", "Browse", WS_VISIBLE | WS_CHILD,
        300, 125, 50, 30, g_hCreatePictureFrame, reinterpret_cast<HMENU>(3), nullptr, nullptr);

    g_hFilePathDisplay = CreateWindow("STATIC", "File path will appear here", WS_VISIBLE | WS_CHILD | SS_CENTER,
        50, 125, 240, 30, g_hCreatePictureFrame, nullptr, nullptr, nullptr);

    CreateWindow("BUTTON", "Create", WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
        150, 200, 100, 30, g_hCreatePictureFrame, reinterpret_cast<HMENU>(2), nullptr, nullptr);
}

LRESULT GuiAlbumManager::CreatePictureFrameProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    GuiAlbumManager* manager = reinterpret_cast<GuiAlbumManager*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
    if (manager == nullptr && msg == WM_CREATE) {
        manager = reinterpret_cast<GuiAlbumManager*>(((LPCREATESTRUCT)lParam)->lpCreateParams);
        SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(manager));
    }

    if (manager != nullptr) {
        return manager->CreatePictureFrameHandler(hwnd, msg, wParam, lParam);
    }

    return DefWindowProc(hwnd, msg, wParam, lParam);
}

LRESULT GuiAlbumManager::CreatePictureFrameHandler(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg) {
    case WM_CREATE: {
        break;
    }
    case WM_COMMAND: {
        int wmId = LOWORD(wParam);
        switch (wmId) {
        case 1: {
            ShowWindow(g_hOpenAlbumFrame, SW_SHOW);
            ShowWindow(hwnd, SW_HIDE);
            break;
        }
        case 2: {
            try {
                int len = GetWindowTextLength(g_hPictureNameInput) + 1; // add 1 for null terminator
                if (len == 1) {
                    MessageBox(hwnd, "Please enter a name.", "Error", MB_OK | MB_ICONERROR);
                    return 0;
                }

                char* albumname = new char[len];
                GetWindowText(g_hPictureNameInput, albumname, len);

                auto now = std::chrono::system_clock::now();
                auto timeT = std::chrono::system_clock::to_time_t(now);

                std::ostringstream oss;
                oss << std::put_time(std::localtime(&timeT), "%Y-%m-%d");
                std::string dateStr = oss.str();

                std::string name = albumname;
                Picture pic(0, name);
                pic.setPath(fullPicPath);
                pic.setCreationDate(dateStr);

                dataAccessPtr->addPictureToAlbumByName(m_currentAlbumName, pic);

                // Clean up memory
                delete[] albumname;

                ShowWindow(g_hOpenAlbumFrame, SW_SHOW);
                ShowWindow(hwnd, SW_HIDE);

                updatePictureOptions();
            }
            catch (std::exception& e) {
                MessageBox(NULL, e.what(), "Error", MB_ICONERROR | MB_OK);
            }
            
            break;
        }
        case 3: {
            OPENFILENAME ofn;       
            char szFile[260];    
            ZeroMemory(&ofn, sizeof(ofn));
            ofn.lStructSize = sizeof(ofn);
            ofn.hwndOwner = hwnd;
            ofn.lpstrFile = szFile;
            ofn.lpstrFile[0] = '\0';
            ofn.nMaxFile = sizeof(szFile);
            ofn.lpstrFilter = "Image Files\0*.JPG;*.JPEG;*.PNG;*.BMP;*.GIF\0All Files\0*.*\0";
            ofn.nFilterIndex = 1;
            ofn.lpstrFileTitle = NULL;
            ofn.nMaxFileTitle = 0;
            ofn.lpstrInitialDir = NULL;
            ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

            if (GetOpenFileName(&ofn) == TRUE)
            {
                SetWindowText(g_hAlbumNameInput, ofn.lpstrFile);
                SetWindowText(g_hFilePathDisplay, ofn.lpstrFile);
                fullPicPath = ofn.lpstrFile;
            }
            break;
        }
        }

        break;
    }
    case WM_DESTROY: {
        PostQuitMessage(0);
        break;
    }
    default:
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}

// ******************* Tags ******************* 
void GuiAlbumManager::CreateOpenPictureFrame()
{
    WNDCLASS wc = {};
    wc.lpfnWndProc = OpenPictureFrameProc;
    wc.hInstance = GetModuleHandle(nullptr);
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.lpszClassName = "OpenPictureFrameClass";
    RegisterClass(&wc);

    CreateTagUserFrame();

    g_hOpenPictureFrame = CreateWindowEx(0, "OpenPictureFrameClass", "Tags", WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 400, 300, nullptr, nullptr, GetModuleHandle(nullptr), nullptr);

    SetWindowLongPtr(g_hOpenPictureFrame, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(g_hMainWindow));

    CreateWindow("BUTTON", "Back", WS_VISIBLE | WS_CHILD,
        10, 10, 80, 30, g_hOpenPictureFrame, reinterpret_cast<HMENU>(1), nullptr, nullptr);

    CreateWindow("STATIC", "Usres tagged in picture:", WS_VISIBLE | WS_CHILD | SS_CENTER,
        100, 10, 200, 30, g_hOpenPictureFrame, nullptr, nullptr, nullptr);

    g_hUsersTaggedComboBox = CreateWindow("COMBOBOX", "", CBS_DROPDOWN | CBS_HASSTRINGS | WS_CHILD | WS_VISIBLE,
        100, 50, 200, 50, g_hOpenPictureFrame, nullptr, nullptr, nullptr);

    CreateWindow("BUTTON", "Delete Tag", WS_VISIBLE | WS_CHILD,
        100, 160, 100, 50, g_hOpenPictureFrame, reinterpret_cast<HMENU>(2), nullptr, nullptr);

    CreateWindow("BUTTON", "Tag User", WS_VISIBLE | WS_CHILD,
        200, 160, 100, 50, g_hOpenPictureFrame, reinterpret_cast<HMENU>(3), nullptr, nullptr);


    ShowWindow(g_hMainWindow, SW_SHOW);
}

LRESULT GuiAlbumManager::OpenPictureFrameProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    GuiAlbumManager* manager = reinterpret_cast<GuiAlbumManager*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
    if (manager == nullptr && msg == WM_CREATE) {
        manager = reinterpret_cast<GuiAlbumManager*>(((LPCREATESTRUCT)lParam)->lpCreateParams);
        SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(manager));
    }

    if (manager != nullptr) {
        return manager->OpenPictureFrameHandler(hwnd, msg, wParam, lParam);
    }

    return DefWindowProc(hwnd, msg, wParam, lParam);
}

LRESULT GuiAlbumManager::OpenPictureFrameHandler(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg) {
    case WM_COMMAND: {
        int wmId = LOWORD(wParam);
        switch (wmId) {
        case 1: {
            ShowWindow(g_hOpenAlbumFrame, SW_SHOW);
            ShowWindow(hwnd, SW_HIDE);
            break;
        }
        case 3: {
            ShowWindow(g_hTagUserFrame, SW_SHOW);
            ShowWindow(hwnd, SW_HIDE);
            updateNotTaggedInPictureOptions();
            break;
        }
        case 2: {
            try {
                int userId = getSelectedUserToUntag();
                dataAccessPtr->untagUserInPicture(m_currentAlbumName, m_currentPictureName, userId);
                updateTaggedInPictureOptions();
                updateNotTaggedInPictureOptions();
            }
            catch (std::exception& e) {
                MessageBox(NULL, e.what(), "Error", MB_ICONERROR | MB_OK);
            }
            break;
        }
        }
        break;
    }
    case WM_DESTROY: {
        PostQuitMessage(0);
        break;
    }
    default:
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}

void GuiAlbumManager::updateTaggedInPictureOptions()
{
    try {
        SendMessage(g_hUsersTaggedComboBox, CB_RESETCONTENT, 0, 0);

        std::set<int> taggedUsers = dataAccessPtr->openAlbum(m_currentAlbumName).getPicture(m_currentPictureName).getUserTags();

        for (auto userID : taggedUsers) {
            User user = dataAccessPtr->getUser(userID);
            std::string itemName = std::to_string(userID) + " - " + user.getName();
            SendMessage(g_hUsersTaggedComboBox, (UINT)CB_ADDSTRING, (WPARAM)0, (LPARAM)(itemName.c_str()));
        }

        SendMessage(g_hUsersTaggedComboBox, CB_SETCURSEL, (WPARAM)0, (LPARAM)0);
    }
    catch (std::exception& e) {
        MessageBox(NULL, e.what(), "Error", MB_ICONERROR | MB_OK);
    }
}

int GuiAlbumManager::getSelectedUserToUntag()
{
    int selectedIndex = SendMessage(g_hUsersTaggedComboBox, CB_GETCURSEL, 0, 0);
    if (selectedIndex != CB_ERR) {
        int textLength = SendMessage(g_hUsersTaggedComboBox, CB_GETLBTEXTLEN, selectedIndex, 0);
        if (textLength != CB_ERR) {
            std::vector<char> buffer(textLength + 1);
            SendMessage(g_hUsersTaggedComboBox, CB_GETLBTEXT, selectedIndex, reinterpret_cast<LPARAM>(buffer.data()));
            std::string selectedItem(buffer.data());

            size_t spacePos = selectedItem.find(' ');
            if (spacePos != std::string::npos) {
                std::string idStr = selectedItem.substr(0, spacePos);
                return std::stoi(idStr);
            }
        }
    }

    return 0;
}

void GuiAlbumManager::CreateTagUserFrame()
{
    WNDCLASS wc = {};
    wc.lpfnWndProc = CreateTagUserFrameProc;
    wc.hInstance = GetModuleHandle(nullptr);
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.lpszClassName = "TagUserFrameClass";
    RegisterClass(&wc);

    g_hTagUserFrame = CreateWindowEx(0, "TagUserFrameClass", "Tags", WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 400, 300, nullptr, nullptr, GetModuleHandle(nullptr), nullptr);

    SetWindowLongPtr(g_hTagUserFrame, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(g_hMainWindow));

    CreateWindow("BUTTON", "Back", WS_VISIBLE | WS_CHILD,
        10, 10, 80, 30, g_hTagUserFrame, reinterpret_cast<HMENU>(1), nullptr, nullptr);

    CreateWindow("STATIC", "Select from users not tagged in picture:", WS_VISIBLE | WS_CHILD | SS_CENTER,
        100, 10, 200, 30, g_hTagUserFrame, nullptr, nullptr, nullptr);

    g_hUsersNotTaggedComboBox = CreateWindow("COMBOBOX", "", CBS_DROPDOWN | CBS_HASSTRINGS | WS_CHILD | WS_VISIBLE,
        100, 50, 200, 50, g_hTagUserFrame, nullptr, nullptr, nullptr);

    CreateWindow("BUTTON", "Tag user", WS_VISIBLE | WS_CHILD,
        100, 160, 200, 50, g_hTagUserFrame, reinterpret_cast<HMENU>(2), nullptr, nullptr);


    ShowWindow(g_hMainWindow, SW_SHOW);
}

LRESULT GuiAlbumManager::CreateTagUserFrameProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    GuiAlbumManager* manager = reinterpret_cast<GuiAlbumManager*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
    if (manager == nullptr && msg == WM_CREATE) {
        manager = reinterpret_cast<GuiAlbumManager*>(((LPCREATESTRUCT)lParam)->lpCreateParams);
        SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(manager));
    }

    if (manager != nullptr) {
        return manager->CreateTagUserFrameHandler(hwnd, msg, wParam, lParam);
    }

    return DefWindowProc(hwnd, msg, wParam, lParam);
}

LRESULT GuiAlbumManager::CreateTagUserFrameHandler(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg) {
    case WM_COMMAND: {
        int wmId = LOWORD(wParam);
        switch (wmId) {
        case 1: {
            ShowWindow(g_hOpenPictureFrame, SW_SHOW);
            ShowWindow(hwnd, SW_HIDE);
            break;
        }
        case 2: {
            try {
                int userId = getSelectedUserToTag();
                dataAccessPtr->tagUserInPicture(m_currentAlbumName, m_currentPictureName, userId);
                updateTaggedInPictureOptions();
                updateNotTaggedInPictureOptions();

                ShowWindow(g_hOpenPictureFrame, SW_SHOW);
                ShowWindow(hwnd, SW_HIDE);
            }
            catch (std::exception& e) {
                MessageBox(NULL, e.what(), "Error", MB_ICONERROR | MB_OK);
            }
            break;
        }
        }
        break;
    }
    case WM_DESTROY: {
        PostQuitMessage(0);
        break;
    }
    default:
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}

void GuiAlbumManager::updateNotTaggedInPictureOptions()
{
    try {
        SendMessage(g_hUsersNotTaggedComboBox, CB_RESETCONTENT, 0, 0);

        std::set<int> taggedUsers = dataAccessPtr->openAlbum(m_currentAlbumName).getPicture(m_currentPictureName).getUserTags();
        std::list<User> users = dataAccessPtr->getUsers();

        for (auto user : users) {
            auto it = taggedUsers.find(user.getId());
            if (it == taggedUsers.end()) {
                std::string itemName = std::to_string(user.getId()) + " - " + user.getName();
                SendMessage(g_hUsersNotTaggedComboBox, (UINT)CB_ADDSTRING, (WPARAM)0, (LPARAM)(itemName.c_str()));
            }
        }

        SendMessage(g_hUsersNotTaggedComboBox, CB_SETCURSEL, (WPARAM)0, (LPARAM)0);
    }
    catch (std::exception& e) {
        MessageBox(NULL, e.what(), "Error", MB_ICONERROR | MB_OK);
    }
}

int GuiAlbumManager::getSelectedUserToTag()
{
    int selectedIndex = SendMessage(g_hUsersNotTaggedComboBox, CB_GETCURSEL, 0, 0);
    if (selectedIndex != CB_ERR) {
        int textLength = SendMessage(g_hUsersNotTaggedComboBox, CB_GETLBTEXTLEN, selectedIndex, 0);
        if (textLength != CB_ERR) {
            std::vector<char> buffer(textLength + 1);
            SendMessage(g_hUsersNotTaggedComboBox, CB_GETLBTEXT, selectedIndex, reinterpret_cast<LPARAM>(buffer.data()));
            std::string selectedItem(buffer.data());

            size_t spacePos = selectedItem.find(' ');
            if (spacePos != std::string::npos) {
                std::string idStr = selectedItem.substr(0, spacePos);
                return std::stoi(idStr);
            }
        }
    }

    return 0;
}