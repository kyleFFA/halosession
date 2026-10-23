#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <tlhelp32.h>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <string>
#include <stdexcept>
#include <cstdint>

#include "../SessionLauncherIdentity.h"
// Internal heartbeat and duplicate-launch checks share the executable identity.
namespace fs = std::filesystem;
static HWND windowHandle{}, statusLabel{}, connectButton{}, retryButton{};
static std::mutex messageMutex;
static std::wstring lastError;
static std::wofstream logFile;
static HMODULE bridge{}; // Lifetime is the process; bridge owns a background thread.
static void (*resetBridge)(){};
constexpr UINT statusMessage = WM_APP + 1, errorMessage = WM_APP + 2;
static constexpr const wchar_t* statuses[] = {
    L"Waiting for MCC", L"Cannot access MCC", L"Launch MCC with anti-cheat disabled",
    L"Connection error", L"Connecting", L"Connection failed", L"Initialising Session",
    L"Session reported an error", L"Connected - open the in-game overlay"
};
static void logCallback(const wchar_t* text) {
    std::lock_guard lock(messageMutex);
    if (logFile) { logFile << text << L'\n'; logFile.flush(); }
}
static void errorCallback(const wchar_t* text) {
    { std::lock_guard lock(messageMutex); lastError = text; }
    logCallback(text);
    PostMessageW(windowHandle, errorMessage, 0, 0);
}
static void statusCallback(int state) {
    PostMessageW(windowHandle, statusMessage, static_cast<WPARAM>(state), 0);
}
template<class T> static T exported(const char* name) {
    auto p = GetProcAddress(bridge, name);
    if (!p) throw std::runtime_error(std::string("Missing connection export: ") + name);
    return reinterpret_cast<T>(p);
}
static bool otherLauncherRunning() {
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snap == INVALID_HANDLE_VALUE) throw std::runtime_error("Cannot check for another Session launcher.");
    PROCESSENTRY32W entry{sizeof(entry)};
    bool found = false;
    if (Process32FirstW(snap, &entry)) do {
        if (entry.th32ProcessID != GetCurrentProcessId() &&
            (_wcsicmp(entry.szExeFile, SessionLauncherIdentity::executableName) == 0 ||
             _wcsicmp(entry.szExeFile, L"PCMExternal.exe") == 0 ||
             _wcsicmp(entry.szExeFile, L"PoorboysCheckpointManager.exe") == 0)) found = true;
    } while (Process32NextW(snap, &entry));
    CloseHandle(snap);
    return found;
}
static void requireFreshGame() {
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snap == INVALID_HANDLE_VALUE) throw std::runtime_error("Cannot inspect game state.");
    PROCESSENTRY32W process{sizeof(process)};
    bool unsafe = false;
    if (Process32FirstW(snap, &process)) do {
        if (_wcsicmp(process.szExeFile, L"MCC-Win64-Shipping.exe") != 0 &&
            _wcsicmp(process.szExeFile, L"MCCWinStore-Win64-Shipping.exe") != 0) continue;
        HANDLE modules = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, process.th32ProcessID);
        if (modules == INVALID_HANDLE_VALUE) { unsafe = true; break; }
        MODULEENTRY32W module{sizeof(module)};
        if (!Module32FirstW(modules, &module)) unsafe = true;
        else do {
            if (_wcsicmp(module.szModule, L"PCMInternal.dll") == 0 ||
                _wcsicmp(module.szModule, L"HCMInternal.dll") == 0) unsafe = true;
        } while (Module32NextW(modules, &module));
        CloseHandle(modules);
    } while (Process32NextW(snap, &process));
    CloseHandle(snap);
    if (unsafe) throw std::runtime_error("MCC already has an internal module loaded, or its modules cannot be checked. Start a fresh MCC session before connecting this candidate.");
}
static void validatePackage() {
    for (const auto* name : {L"PCMInternal.dll", L"HCMInterproc.dll", L"InternalPointerData.xml", L"h3sb/h3sb.dll"})
        if (!fs::is_regular_file(name)) throw std::runtime_error("Incomplete Session package. Extract the entire folder again.");
    // Keep the bridge/internal ABI version contract of the working launcher.
    for (const auto* name : {L"PCMInternal.dll", L"HCMInterproc.dll"}) {
        DWORD ignored{};
        DWORD size = GetFileVersionInfoSizeW(name, &ignored);
        std::string data(size, '\0');
        VS_FIXEDFILEINFO* info{}; UINT count{};
        if (!size || !GetFileVersionInfoW(name, 0, size, data.data()) ||
            !VerQueryValueW(data.data(), L"\\", reinterpret_cast<void**>(&info), &count) ||
            count < sizeof(*info) || info->dwFileVersionMS != MAKELONG(1,3) ||
            info->dwFileVersionLS != MAKELONG(0,9))
            throw std::runtime_error("Session native component version mismatch (expected 3.1.9.0).");
    }
}
static void connect() {
    validatePackage();
    if (otherLauncherRunning()) throw std::runtime_error("Another Session/PCM launcher is running. Use that launcher, or close it before starting a fresh game session.");
    requireFreshGame();
    bridge = LoadLibraryExW(fs::absolute(L"HCMInterproc.dll").c_str(), nullptr,
        LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR | LOAD_LIBRARY_SEARCH_SYSTEM32);
    if (!bridge) throw std::runtime_error("Cannot load the Session connection bridge.");
    auto errorSub = exported<void (*)(void (*)(const wchar_t*))>("ErrorEvent_Subscribe");
    auto logSub = exported<void (*)(void (*)(const wchar_t*))>("LogEvent_Subscribe");
    auto stateSub = exported<void (*)(void (*)(int))>("HookStateChangedEvent_Subscribe");
    resetBridge = exported<void (*)()>("resetStateMachine");
    using Init = uint16_t (*)(bool, int, const char*, const char*, const char*, const char*, int, int, const char*, const char*);
    auto init = exported<Init>("initialiseInterproc");
    errorSub(errorCallback); logSub(logCallback); stateSub(statusCallback);
    // Empty selection satisfies the shared-memory ABI; Session has no checkpoint browser.
    const auto folder = fs::current_path().string();
    if (!init(true, 2, "", "", "", "", 0, 2, "Session", folder.c_str()))
        throw std::runtime_error("Could not initialise Session shared memory.");
    EnableWindow(connectButton, FALSE);
    EnableWindow(retryButton, TRUE);
}
static LRESULT CALLBACK windowProc(HWND hwnd, UINT message, WPARAM w, LPARAM l) {
    if (message == WM_COMMAND) {
        try {
            if (LOWORD(w) == 1) connect();
            if (LOWORD(w) == 2 && resetBridge) { requireFreshGame(); resetBridge(); }
        } catch (const std::exception& e) {
            MessageBoxA(hwnd, e.what(), "Session", MB_OK | MB_ICONERROR);
            // Once a bridge has been loaded, require restarting this launcher after setup failure.
            if (bridge) EnableWindow(connectButton, FALSE);
        }
        return 0;
    }
    if (message == statusMessage) {
        SetWindowTextW(statusLabel, w < std::size(statuses) ? statuses[w] : L"Unknown connection state");
        return 0;
    }
    if (message == errorMessage) {
        std::wstring text; { std::lock_guard lock(messageMutex); text = lastError; }
        MessageBoxW(hwnd, text.c_str(), L"Session", MB_OK | MB_ICONERROR);
        return 0;
    }
    if (message == WM_DESTROY) { PostQuitMessage(0); return 0; }
    return DefWindowProcW(hwnd, message, w, l);
}
int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR args, int show) {
    try {
        wchar_t exe[32768]; auto length = GetModuleFileNameW(nullptr, exe, static_cast<DWORD>(std::size(exe)));
        if (!length || length >= std::size(exe)) return 2;
        fs::current_path(fs::path(exe).parent_path());
        // Exercise the exact heartbeat name lookup and query permission on this
        // process only. No bridge is loaded and no game is accessed.
        if (std::wstring(args) == L"--check-identity") {
            const DWORD found = SessionLauncherIdentity::findProcessId();
            if (found != GetCurrentProcessId()) return 5;
            HANDLE ownProcess = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, TRUE, found);
            if (!ownProcess) return 6;
            DWORD exitCode{};
            const BOOL queried = GetExitCodeProcess(ownProcess, &exitCode);
            CloseHandle(ownProcess);
            return queried && exitCode == STILL_ACTIVE ? 0 : 7;
        }
        // Offline package check: no bridge load, process access, shared memory or injection.
        if (std::wstring(args) == L"--check-package") { validatePackage(); return 0; }
        fs::create_directories(L"Logs");
        logFile.open(fs::path(L"Logs") / (L"Session-launcher-" + std::to_wstring(GetCurrentProcessId()) + L".txt"));
        WNDCLASSW wc{}; wc.hInstance=instance; wc.lpfnWndProc=windowProc;
        wc.lpszClassName=L"SessionLauncher"; wc.hCursor=LoadCursorW(nullptr, IDC_ARROW);
        wc.hbrBackground=reinterpret_cast<HBRUSH>(COLOR_WINDOW+1);
        if (!RegisterClassW(&wc)) return 3;
        windowHandle=CreateWindowW(wc.lpszClassName,L"Session",WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU|WS_MINIMIZEBOX,
            CW_USEDEFAULT,CW_USEDEFAULT,700,350,nullptr,nullptr,instance,nullptr);
        if (!windowHandle) return 4;
        auto control=[&](const wchar_t* cls,const wchar_t* text,int x,int y,int width,int height,int id) {
            HWND h=CreateWindowW(cls,text,WS_CHILD|WS_VISIBLE,x,y,width,height,windowHandle,reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)),instance,nullptr);
            SendMessageW(h,WM_SETFONT,reinterpret_cast<WPARAM>(GetStockObject(DEFAULT_GUI_FONT)),TRUE); return h;
        };
        control(L"STATIC",L"Session - Quick Menu beta",24,22,640,25,0);
        control(L"STATIC",L"Start MCC with anti-cheat disabled, then connect. Keep this window open.\nOpen the in-game overlay with your configured key (default: the key left of 1).",24,58,640,46,0);
        statusLabel=control(L"STATIC",L"Ready to connect",24,120,640,25,0);
        connectButton=control(L"BUTTON",L"Connect",24,160,110,32,1);
        retryButton=control(L"BUTTON",L"Retry",150,160,110,32,2); EnableWindow(retryButton,FALSE);
        control(L"STATIC",L"Credits: Burnt / Poorboy\nBuilt on work by Poorboy, Burnt-o and Harc.\nKyleFFA (badstrafe)\nSee LICENSE and THIRD-PARTY-LICENSES.md.",24,220,640,65,0);
        ShowWindow(windowHandle,show);
        MSG msg{}; while (GetMessageW(&msg,nullptr,0,0)>0) { TranslateMessage(&msg); DispatchMessageW(&msg); }
        // Bridge worker has process lifetime. Do not unload it while it is running.
        ExitProcess(0);
    } catch (const std::exception& e) {
        if (std::wstring(args) != L"--check-package") MessageBoxA(nullptr,e.what(),"Session",MB_OK|MB_ICONERROR);
        return 1;
    }
}
