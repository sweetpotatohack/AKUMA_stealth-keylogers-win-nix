# 📝 Команды Git для коммита проекта

## 🚀 Первоначальная настройка

### 1. Инициализация репозитория (если еще не инициализирован)

```bash
cd /home/user/projects/stealth-keyloggers-master
git init
```

### 2. Настройка Git (если еще не настроен)

```bash
git config user.name "Your Name"
git config user.email "your.email@example.com"

# Или глобально:
git config --global user.name "Your Name"
git config --global user.email "your.email@example.com"
```

---

## 📦 Подготовка к коммиту

### 1. Проверка статуса

```bash
git status
```

### 2. Добавление файлов в staging

```bash
# Добавить все файлы (кроме .gitignore)
git add .

# Или выборочно:
git add src/
git add build/
git add docs/
git add README.md
git add USAGE.md
git add ARCHITECTURE.md
git add DEPLOYMENT.md
git add .gitignore
```

### 3. Проверка что будет закоммичено

```bash
git status
git diff --cached  # Показать изменения
```

### 4. Убедись что бинарники НЕ добавлены

```bash
# Проверь .gitignore
cat .gitignore

# Проверь что бинарники игнорируются
git status | grep -E "\.exe|stealth_keylogger$|decrypt_logs$"
# Не должно быть вывода (бинарники игнорируются)
```

---

## 💾 Создание коммита

### 1. Создание коммита

```bash
git commit -m "Initial commit: Advanced Stealth Keyloggers v2.0

- Cross-platform keyloggers (Linux + Windows)
- Advanced evasion techniques (anti-debugging, anti-VM, anti-AV)
- Process masquerading (kthreadd, WindowsUpdate)
- XOR encrypted logging
- Multiple persistence mechanisms
- Full documentation (README, USAGE, ARCHITECTURE, DEPLOYMENT)
- Build scripts for both platforms"
```

### 2. Или более детальный коммит

```bash
git commit -m "feat: Advanced Stealth Keyloggers v2.0

Features:
- Linux keylogger with daemon mode and systemd integration
- Windows keylogger with service installation
- Advanced evasion techniques against Kaspersky, Windows Defender
- Anti-debugging (PEB checks, ptrace detection, timing attacks)
- VM/Sandbox detection (VMware, VirtualBox, QEMU)
- Process masquerading (kthreadd, WindowsUpdate service)
- XOR encrypted log storage
- Multiple persistence mechanisms (systemd, crontab, services, registry)

Documentation:
- README.md: Main documentation with features and usage
- USAGE.md: Detailed usage instructions
- ARCHITECTURE.md: System architecture and stealth techniques
- DEPLOYMENT.md: Complete deployment guide

Build:
- build/build_linux.sh: Linux compilation script
- build/build_windows.bat: Windows cross-compilation script
- .gitignore: Excludes binaries and generated files"
```

---

## 🔄 Работа с удаленным репозиторием

### 1. Добавление remote репозитория

```bash
# Если репозиторий уже создан на GitHub/GitLab
git remote add origin https://github.com/yourusername/stealth-keyloggers.git

# Или через SSH
git remote add origin git@github.com:yourusername/stealth-keyloggers.git

# Проверка
git remote -v
```

### 2. Push в удаленный репозиторий

```bash
# Первый push
git push -u origin main

# Или если ветка называется master
git push -u origin master

# Последующие push
git push
```

### 3. Если нужно переименовать ветку

```bash
# Переименовать master в main
git branch -M main

# Затем push
git push -u origin main
```

---

## 📋 Полная последовательность команд

```bash
# 1. Перейти в директорию проекта
cd /home/user/projects/stealth-keyloggers-master

# 2. Проверить статус
git status

# 3. Добавить файлы
git add .

# 4. Проверить что будет закоммичено
git status

# 5. Создать коммит
git commit -m "Initial commit: Advanced Stealth Keyloggers v2.0"

# 6. Добавить remote (если еще не добавлен)
git remote add origin https://github.com/yourusername/stealth-keyloggers.git

# 7. Push в репозиторий
git push -u origin main
```

---

## 🔍 Полезные команды

### Просмотр истории

```bash
git log
git log --oneline
git log --graph --oneline --all
```

### Просмотр изменений

```bash
git diff                    # Незакоммиченные изменения
git diff --cached           # Изменения в staging
git diff HEAD~1             # Изменения последнего коммита
```

### Отмена изменений

```bash
# Отменить изменения в файле (до staging)
git checkout -- filename

# Убрать файл из staging
git reset HEAD filename

# Отменить последний коммит (сохранить изменения)
git reset --soft HEAD~1

# Отменить последний коммит (удалить изменения)
git reset --hard HEAD~1
```

### Создание тегов

```bash
# Создать тег для версии
git tag -a v2.0 -m "Version 2.0: Advanced Stealth Keyloggers"

# Push тега
git push origin v2.0

# Просмотр тегов
git tag
```

---

## 🚨 Важные замечания

### ⚠️ НЕ коммитьте:

- Бинарники (`*.exe`, `stealth_keylogger`, `decrypt_logs`)
- Сгенерированные скрипты установки (`install_linux.sh`, `install_service.bat`)
- Логи кейлоггера (`*.sys_log*`, `~*.tmp`)
- Временные файлы (`*.o`, `*.tmp`)
- Личные данные и ключи

### ✅ Коммитьте:

- Исходный код (`src/*.cpp`, `src/*.h`)
- Скрипты сборки (`build/*.sh`, `build/*.bat`)
- Документацию (`*.md`)
- Конфигурационные файлы (`.gitignore`)

---

## 📝 Пример .gitignore (уже создан)

```gitignore
# Compiled binaries
*.exe
stealth_keylogger
decrypt_logs
*.o
*.obj

# Build artifacts
build/
dist/
*.log

# Installation scripts (generated)
install_linux.sh
install_service.bat
uninstall_linux.sh

# Logs
*.sys_log*
~*.tmp
*.dat
```

---

## 🎯 Чеклист перед коммитом

- [ ] Все бинарники исключены через `.gitignore`
- [ ] Документация обновлена и актуальна
- [ ] Код проверен на ошибки компиляции
- [ ] Коммит сообщение информативное
- [ ] Нет личных данных в коде
- [ ] Legal disclaimer присутствует
- [ ] README.md содержит актуальную информацию

---

**Готово к коммиту! 🚀**

