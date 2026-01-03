# Advanced Stealth Keyloggers

This repository contains cross-platform keyloggers designed for security competitions and penetration testing. The keyloggers implement advanced evasion techniques to avoid detection by modern antivirus solutions and EDR systems.

## ⚠️ Legal Disclaimer

This software is intended for:
- Security competitions
- Authorized penetration testing
- Educational purposes
- Research environments

**DO NOT USE** on systems you don't own or without explicit written permission.

## Features

### Windows Keylogger
- Low-level keyboard hooks (WH_KEYBOARD_LL)
- Multiple anti-debugging techniques
- Process hiding and masquerading
- Service installation with persistence
- Registry-based persistence
- Encrypted log storage
- Anti-VM and sandbox detection
- Memory protection mechanisms

### Linux Keylogger
- Direct input device monitoring (/dev/input/eventX)
- Process name masquerading
- Daemon mode operation
- Anti-ptrace protection
- Systemd service persistence
- Crontab backup persistence
- X11 window title capture
- Encrypted logging

### Common Features
- XOR encryption of logged data
- Timestamp and window context logging
- Multiple persistence mechanisms
- Anti-analysis techniques
- Minimal resource footprint

## File Structure

```
stealth-keyloggers/
├── windows_keylogger.cpp       # Windows implementation
├── linux_keylogger.cpp        # Linux implementation
├── evasion_techniques.h        # Common evasion library
├── decrypt_logs.cpp           # Log decryption utility
├── build_windows.bat          # Windows build script
├── build_linux.sh            # Linux build script
└── KEYLOGGER_README.md        # This file
```

## Building

### Windows (Cross-compilation from Linux)

1. Install MinGW-w64:
```bash
sudo apt-get install mingw-w64
```

2. Run build script:
```bash
chmod +x build_windows.bat
./build_windows.bat
```

### Linux

1. Install build dependencies:
```bash
sudo apt-get install build-essential upx-ucl xdotool
```

2. Run build script:
```bash
chmod +x build_linux.sh
./build_linux.sh
```

## Usage

### Windows

1. **Manual execution:**
```cmd
stealth_keylogger.exe
```

2. **Service installation (as Administrator):**
```cmd
install_service.bat
```

3. **Log decryption:**
```cmd
decrypt_logs.exe "C:\Users\%USERNAME%\AppData\Local\Temp\sys_backup.txt"
```

### Linux

1. **Manual execution:**
```bash
sudo ./stealth_keylogger
```

2. **System installation:**
```bash
sudo ./install_linux.sh
```

3. **Log decryption:**
```bash
./decrypt_logs /tmp/.sys_log
```

4. **Uninstallation:**
```bash
sudo ./uninstall_linux.sh
```

## Evasion Techniques Implemented

### Anti-Debugging
- IsDebuggerPresent() checks
- PEB BeingDebugged flag detection
- Remote debugger detection
- Hardware breakpoint detection
- Timing-based detection
- Ptrace detection (Linux)

### Anti-VM/Sandbox
- Registry key detection (Windows)
- DMI/SMBIOS checks (Linux)
- Timing delays
- Resource availability checks
- Virtual device detection

### Stealth Features
- Process name masquerading
- String obfuscation
- Dynamic API resolution
- Memory protection
- Service/daemon mode
- Hidden file locations

## Log Format

Encrypted logs contain entries in the format:
```
YYYY-MM-DD HH:MM:SS | Window Title | Keystroke
```

Example decrypted output:
```
2023-10-16 14:30:15 | Notepad - Untitled | hello world[ENTER]
2023-10-16 14:30:20 | Firefox | admin
2023-10-16 14:30:25 | Firefox | [TAB]password123[ENTER]
```

## Competition Strategy

1. **Pre-deployment:**
   - Test against target antivirus
   - Verify all evasion techniques
   - Prepare multiple variants

2. **During competition:**
   - Monitor for detection
   - Have backup deployment methods
   - Use different persistence mechanisms

3. **Post-deployment:**
   - Minimize network traffic
   - Rotate log storage locations
   - Update signatures if detected

## Countermeasures (Blue Team)

To detect these keyloggers:

1. **Process monitoring:** Look for suspicious process names
2. **Network monitoring:** Monitor unusual DNS queries
3. **File system monitoring:** Watch for hidden files
4. **Registry monitoring:** Monitor Run key changes
5. **Service monitoring:** Check for unusual services
6. **Input monitoring:** Monitor input device access
7. **Memory analysis:** Scan for injection techniques

## License

Educational/Research use only. Not for malicious purposes.