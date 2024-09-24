#include <Windows.h>
#include "MemoryAccess.h"
#include "Album.h"
#include "User.h"

class GuiAlbumManager {
public:
    GuiAlbumManager(IDataAccess& dataAccess);

    void CreateMainWindow();

private:
    //management
    IDataAccess& m_dataAccess;
    

    // Main - Users
    static LRESULT CALLBACK MainWindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    LRESULT MainWindowHandler(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    int getSelectedUserId();
    void updateMainOptions();

    // Create user
    void CreateCreateUserFrame();
    static LRESULT CALLBACK CreateUserFrameProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    LRESULT CreateUserFrameHandler(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

    // Open user -Albums
    void CreateOpenUserFrame();
    static LRESULT CALLBACK OpenUserFrameProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    LRESULT OpenUserFrameHandler(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    std::string getSelectedAlbumName();
    void updateAlbumOptions();

    // Create album
    void CreateCreateAlbumFrame();
    static LRESULT CALLBACK CreateAlbumFrameProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    LRESULT CreateAlbumFrameHandler(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

    // Open Album - pictures
    void CreateOpenAlbumFrame();
    static LRESULT CALLBACK OpenAlbumFrameProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    LRESULT OpenAlbumFrameHandler(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    void updatePictureOptions();
    std::string getSelectedPictureName();
    void launchImageProccess(std::string cmd);;

    // create picture
    void CreateCreatePictureFrame();
    static LRESULT CALLBACK CreatePictureFrameProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    LRESULT CreatePictureFrameHandler(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

    // Open Picture - tags
    void CreateOpenPictureFrame();
    static LRESULT CALLBACK OpenPictureFrameProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    LRESULT OpenPictureFrameHandler(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    void updateTaggedInPictureOptions();
    int getSelectedUserToUntag();

    // Tag user
    void CreateTagUserFrame();
    static LRESULT CALLBACK CreateTagUserFrameProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    LRESULT CreateTagUserFrameHandler(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    void updateNotTaggedInPictureOptions();
    int getSelectedUserToTag();
};