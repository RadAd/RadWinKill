#include "Window.h"
#include "Windowxx.h"
//#include <tchar.h>
#include <Rad/Format.h>
#include <Rad/Win32/Deleter.h>
#include <Rad/Win32/Win32Error.h>
#include <Psapi.h>

static TCHAR sTitle[] = TEXT("Rad Win Kill");

#define HK_KILL (1)

class RootWindow : public Window
{
    friend WindowManager<RootWindow>;
public:
    static ATOM Register() { return WindowManager<RootWindow>::Register(); }
    static RootWindow* Create() { return WindowManager<RootWindow>::Create(); }

protected:
    static void GetCreateWindow(CREATESTRUCT& cs);
    LRESULT HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam) override;

private:
    BOOL OnCreate(LPCREATESTRUCT lpCreateStruct);
    void OnDestroy();
    void OnHotKey(int idHotKey, UINT fuModifiers, UINT vk);

    static LPCTSTR ClassName() { return TEXT("RadWinKill"); }
};

void RootWindow::GetCreateWindow(CREATESTRUCT& cs)
{
    Window::GetCreateWindow(cs);
    cs.lpszName = sTitle;
    cs.style = WS_OVERLAPPEDWINDOW;
}

BOOL RootWindow::OnCreate(const LPCREATESTRUCT lpCreateStruct)
{
    RegisterHotKey(*this, HK_KILL, MOD_ALT | MOD_CONTROL, VK_F4);
    return TRUE;
}

void RootWindow::OnDestroy()
{
    PostQuitMessage(0);
}

void RootWindow::OnHotKey(int idHotKey, UINT fuModifiers, UINT vk)
{
    switch (idHotKey)
    {
    case HK_KILL:
    {
        HWND hFGWnd = GetForegroundWindow();

        DWORD   pid;
        GetWindowThreadProcessId(hFGWnd, &pid);
        UniqueHANDLE hProcess(OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid));
        if (!hProcess)
        {
            win32_error_code e;
            MessageBox(NULL, e.wmessage().c_str(), sTitle, MB_OK | MB_ICONERROR);
            return;
        }

        TCHAR title[1024];
        if (GetWindowText(hFGWnd, title, ARRAYSIZE(title)) == 0)
            title[0] = L'\0';
        TCHAR module[MAX_PATH];
        DWORD dwModuleSize = ARRAYSIZE(module);
        if (!QueryFullProcessImageName(hProcess.get(), 0, module, &dwModuleSize))
            module[0] = L'\0';

        SetForegroundWindow(*this);
        std::wstring msg = Format(TEXT("Kill window '%s' from process [%d] '%s', are you sure?"), title, pid, module);
        if (MessageBox(*this, msg.c_str(), sTitle, MB_OKCANCEL | MB_ICONEXCLAMATION) == IDOK)
        {
            if (!TerminateProcess(hProcess.get(), 9999))
            {
                win32_error_code e;
                MessageBox(NULL, e.wmessage().c_str(), sTitle, MB_OK | MB_ICONERROR);
            }
        }
    }
    break;
    }
}

LRESULT RootWindow::HandleMessage(const UINT uMsg, const WPARAM wParam, const LPARAM lParam)
{
    LRESULT ret = 0;
    switch (uMsg)
    {
        HANDLE_MSG(WM_CREATE, OnCreate);
        HANDLE_MSG(WM_DESTROY, OnDestroy);
        HANDLE_MSG(WM_HOTKEY, OnHotKey);
    }

    if (!IsHandled())
        ret = Window::HandleMessage(uMsg, wParam, lParam);

    return ret;
}

bool Run(_In_ const LPCTSTR lpCmdLine, _In_ const int nShowCmd)
{
    if (RootWindow::Register() == 0)
    {
        MessageBox(NULL, TEXT("Error registering window class"), sTitle, MB_ICONERROR | MB_OK);
        return false;
    }
    RootWindow* prw = RootWindow::Create();
    if (prw == nullptr)
    {
        MessageBox(NULL, TEXT("Error creating root window"), sTitle, MB_ICONERROR | MB_OK);
        return false;
    }

    //ShowWindow(*prw, nShowCmd);
    return true;
}
