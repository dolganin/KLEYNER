# KLEYNER Utility

KLEYNER Utility — консольная утилита на C++17 для очистки временных файлов, кэшей инструментов разработки и некоторых системных папок на Windows и Linux (включая WSL). Все пути задаются в конфиге, перед удалением показывается план и требуется подтверждение.

## Быстрый и безопасный старт

1. **Сначала только симуляция:**  
   `./bin/cleaner --config configs/basic.cfg --dry-run --verbose`
2. **Проверьте план очистки и список путей.** Если видите что-то лишнее — отредактируйте конфиг.
3. **Ограничьте ОС, если нужно:**  
   - только Linux: `--os linux`  
   - только Windows: `--os windows`  
   В WSL по умолчанию безопаснее использовать `--os linux`.
4. **Только после этого запускайте реальную очистку** без `--dry-run`.

## Сборка

Linux/macOS:
```bash
./build.sh
```

Windows (MinGW):
```bat
build.bat
```

Ручная сборка (CMake):
```bash
cmake -S . -B build && cmake --build build
```

Готовый бинарник: `bin/cleaner` или `bin\cleaner.exe`.

## Запуск

Безопасный пример:
```bash
./bin/cleaner --config configs/basic.cfg --dry-run --verbose
```

Обычный запуск после проверки:
```bash
./bin/cleaner --config configs/basic.cfg --verbose
```

### Основные флаги

- `--config <path>` — путь к конфигу.
- `--dry-run` — ничего не удаляет, только показывает, что будет удалено.
- `--verbose` / `-v` — подробный лог.
- `--os <auto|windows|linux|both>` — ограничение по ОС.
- `--clean-windows` — разрешить очистку системных Windows-путей (актуально для WSL).
- `--include-hidden` — включать скрытые файлы и папки (опасно, используйте осознанно).
- `--allow-sudo` / `--sudo` — при отказе в доступе предложит повторить удаление через `sudo`.
- `--cli-clean` — выполнит очистку кэшей через CLI (pip, npm, yarn, pnpm, go, dotnet/nuget).
- `--docker-prune` — `docker system prune -f`.
- `--docker-prune-all` — `docker system prune -f -a`.
- `--docker-prune-volumes` — `docker system prune -f --volumes`.

## Конфигурация

Файл: `configs/basic.cfg` (INI-подобный формат).

Секции:
- `[General]` — общие настройки (verbose, dry_run, os, allow_sudo, cli_clean, docker_prune и т.д.).
- `[Windows]` — пути для Windows.
- `[Linux]` — пути для Linux.
- `[Common]` — общие пути.
- `[Paths]` — дополнительные пути (каждый путь можно перечислять через запятую).

Особенности:
- Поддерживаются маски `*` и `?` (например, `~/.cache/pip`, `/var/log/*.gz`).
- Для Windows можно использовать `%VAR%`, для Linux — `~`.
- Комментарии: строки с `#` или `;`.

## Примеры конфигов

### 1) Минимально безопасный Linux-only (без удаления загрузок)
```ini
[General]
verbose = true
dry_run = true
os = linux
allow_sudo = false
cli_clean = false
docker_prune = false

[Linux]
tmp = /tmp
var_tmp = /var/tmp
user_cache = ~/.cache
pip_cache = ~/.cache/pip
npm_cache = ~/.npm

[Paths]
# дополнительные пути, если нужно
```

### 2) WSL: чистим только Linux, Windows не трогаем
```ini
[General]
verbose = true
dry_run = true
os = linux
clean_windows = false
allow_sudo = false

[Linux]
tmp = /tmp
var_tmp = /var/tmp
user_cache = ~/.cache
downloads =  ; пусто, чтобы не трогать загрузки
```

### 3) Очистка кэшей разработчика + CLI команды
```ini
[General]
verbose = true
dry_run = false
os = linux
cli_clean = true
docker_prune = false

[Linux]
pip_cache = ~/.cache/pip
npm_cache = ~/.npm
yarn_cache = ~/.cache/yarn
pnpm_store = ~/.pnpm-store
go_build_cache = ~/.cache/go-build
go_mod_cache = ~/go/pkg/mod
gradle_cache = ~/.gradle/caches
maven_repo = ~/.m2/repository
```

### 4) Осторожная Docker-очистка (без удаления образов)
```ini
[General]
verbose = true
dry_run = false
os = linux
docker_prune = true
docker_prune_all = false
docker_prune_volumes = false

[Linux]
# можно оставить пустым, если нужны только docker-команды
```

### 5) Windows-only (не в WSL)
```ini
[General]
verbose = true
dry_run = true
os = windows
clean_windows = false

[Windows]
temp = %TEMP%
tmp = %TMP%
chrome_cache = %LocalAppData%\Google\Chrome\User Data\Default\Cache
edge_cache = %LocalAppData%\Microsoft\Edge\User Data\Default\Cache
```

## Как не убить систему

- **Всегда начинайте с `--dry-run`.**
- **Ограничивайте ОС:** в WSL разумно использовать `--os linux`, чтобы не трогать Windows.
- **Проверьте конфиг:**  
  - `downloads` удалит содержимое загрузок, если не убрать этот пункт.  
  - `docker_images`, `containerd` и `docker_prune*` могут удалить образы/контейнеры.  
  - `journal_logs`, `/var/log/*` и прочие системные папки требуют осторожности.
- **Не включайте `--allow-sudo`, если не уверены в путях.** В этом режиме утилита может запускать `sudo rm -rf` по подтверждению.
- **Скрытые файлы (`--include-hidden`) — это риск удалить полезные настройки.**
- **Python окружения:** утилита предложит удалить большие окружения (обычно > 3 GB) отдельным подтверждением.

## Что делает утилита по шагам

1. Загружает конфиг (по умолчанию `configs/basic.cfg`).
2. Строит план очистки и считает размер.
3. Показывает, сколько файлов/папок будет удалено.
4. Спрашивает подтверждение.
5. Удаляет файлы/папки и при включенных опциях запускает CLI-очистку и `docker prune`.

## Разработка

Исходники: `src/`  
Конфиги: `configs/`  
ASCII-арт: `media/`
