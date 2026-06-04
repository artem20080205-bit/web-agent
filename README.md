# Web-Agent — Кроссплатформенный веб-агент

Версия 1.0 | C++17 | Windows / Linux / macOS

---

## Содержание

1. [Обзор архитектуры](#архитектура)
2. [Структура проекта](#структура)
3. [Зависимости](#зависимости)
4. [Сборка](#сборка)
5. [Конфигурация](#конфигурация)
6. [Запуск](#запуск)
7. [Демонстрация](#демонстрация)
8. [Web API](#web-api)
9. [Описание модулей](#модули)

---

## Архитектура

```
Сервер (xdev.arkcom.ru)
       ↑ ↓ HTTPS/JSON
  ┌─────────────┐
  │  Web-Agent  │
  │  main.cpp   │
  │      │      │
  │  Agent      │  ← главный цикл: регистрация → поллинг → выполнение → отправка
  │      │      │
  │  HttpClient │  ← libcurl: POST JSON, POST multipart
  │  TaskExecutor  ← запуск команд, сбор файлов
  │  Logger     │  ← stdout + файл
  │  Config     │  ← agent.json
  └─────────────┘
```

### Жизненный цикл агента

```
Старт
  │
  ▼
Ждём доступность сервера (GET /wa_reg/)
  │
  ▼
Регистрация POST /wa_reg/  →  access_code
  │
  ▼
┌─ Цикл поллинга ─────────────────────────────────┐
│  POST /wa_task/  {UID, access_code}              │
│     │                                            │
│  ┌──┴──────────────────────┐                     │
│  │ code=0  → WAIT, пауза   │                     │
│  │ code=1  → есть задание  │                     │
│  │ code<0  → ошибка/backoff│                     │
│  └──────────────────────── ┘                     │
│     │ code=1                                     │
│     ▼                                            │
│  TaskExecutor.execute(task_code, options)        │
│     │                                            │
│     ▼                                            │
│  POST /wa_result/  multipart {result + files}   │
│     │                                            │
│  sleep(poll_interval)                            │
└─────────────────────────────────────────────────┘
```

---

## Структура проекта

```
webagent/
├── CMakeLists.txt          — система сборки
├── config/
│   └── agent.json          — конфигурация агента
├── include/
│   ├── agent.h             — главный класс агента
│   ├── config.h            — загрузка конфигурации
│   ├── http_client.h       — HTTP-клиент на libcurl
│   ├── logger.h            — потокобезопасный логгер
│   └── task_executor.h     — исполнитель задач
├── src/
│   └── main.cpp            — точка входа
├── tests/
│   ├── test_config.cpp     — тест загрузки конфига
│   ├── test_http.cpp       — тест HTTP-клиента
│   └── test_executor.cpp   — тест исполнителя задач
├── scripts/
│   ├── build.sh            — сборка Linux/macOS
│   ├── run.sh              — запуск одного агента
│   ├── run_multiple.sh     — запуск нескольких агентов
│   └── build_windows.bat   — сборка Windows
└── docs/
    └── README.md           — этот файл
```

---

## Зависимости

| Зависимость       | Версия   | Назначение                |
|-------------------|----------|---------------------------|
| CMake             | ≥ 3.14   | Система сборки            |
| GCC / Clang / MSVC| C++17    | Компилятор                |
| libcurl           | ≥ 7.68   | HTTP/HTTPS запросы        |
| nlohmann/json     | ≥ 3.2    | Парсинг JSON              |

### Linux (Ubuntu / Debian)
```bash
sudo apt-get install -y cmake g++ libcurl4-openssl-dev nlohmann-json3-dev
```

### macOS
```bash
brew install cmake curl nlohmann-json
```

### Windows
Установите [vcpkg](https://github.com/microsoft/vcpkg), затем:
```
vcpkg install curl:x64-windows nlohmann-json:x64-windows
```

---

## Сборка

### Linux / macOS (автоматически)
```bash
chmod +x scripts/build.sh
./scripts/build.sh
```

### Вручную
```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j4
```

### Windows (Visual Studio + vcpkg)
```bat
scripts\build_windows.bat
```

Бинарный файл после сборки: `build/webagent` (или `build/Release/webagent.exe` на Windows)

---

## Конфигурация

Файл: `config/agent.json`

```json
{
    "uid":                   "agent-001",
    "descr":                 "web-agent",
    "server_url":            "https://xdev.arkcom.ru:9999/app/webagent1/api",
    "tasks_dir":             "./tasks",
    "results_dir":           "./results",
    "log_file":              "./webagent.log",
    "poll_interval_sec":     10,
    "max_poll_interval_sec": 120,
    "ssl_verify":            false
}
```

| Поле                   | Тип    | Описание                                               |
|------------------------|--------|--------------------------------------------------------|
| `uid`                  | string | Уникальный идентификатор агента                        |
| `descr`                | string | Описание агента                                        |
| `server_url`           | string | Базовый URL сервера (без завершающего `/`)             |
| `tasks_dir`            | string | Папка для хранения задач                               |
| `results_dir`          | string | Папка для файлов-результатов                           |
| `log_file`             | string | Путь к файлу журнала                                   |
| `poll_interval_sec`    | int    | Базовый интервал опроса сервера (секунды)              |
| `max_poll_interval_sec`| int    | Максимальный интервал при backoff                      |
| `ssl_verify`           | bool   | Проверять SSL-сертификат сервера                       |

---

## Запуск

### Один агент
```bash
./build/webagent --config config/agent.json
```

### С переопределением параметров
```bash
./build/webagent --config config/agent.json --uid agent-042 --server https://myserver.com/api
```

### Несколько агентов параллельно
```bash
./scripts/run_multiple.sh
```
Это запустит 3 агента с UID `agent-001`, `agent-002`, `agent-003`.

### Фоновый режим (daemon)
```bash
nohup ./build/webagent --config config/agent.json > /dev/null 2>&1 &
echo "PID: $!"
```

### Остановка
```
Ctrl+C   — graceful shutdown (SIGINT)
kill PID — то же через SIGTERM
```

---

## Демонстрация

### Шаг 1 — Сборка
```bash
./scripts/build.sh
```

### Шаг 2 — Запуск агента
```bash
./build/webagent --config config/agent.json
```
Вы увидите:
```
[2024-01-15 12:00:00] [INFO]    Web-Agent v1.0 starting...
[2024-01-15 12:00:00] [INFO]    Config: uid=agent-001 server=https://xdev.arkcom.ru:9999/app/webagent1/api
[2024-01-15 12:00:01] [INFO]    Checking server availability...
[2024-01-15 12:00:01] [INFO]    Server is reachable.
[2024-01-15 12:00:01] [INFO]    Registering agent at: .../wa_reg/
[2024-01-15 12:00:01] [INFO]    Registered successfully. access_code: 594807-...
[2024-01-15 12:00:01] [DEBUG]   Polling for task...
[2024-01-15 12:00:01] [DEBUG]   No task. Status: WAIT
```

### Шаг 3 — Выдача задания через сервер
На стороне сервера (или через UI заказчика) создаётся задание с `task_code=CONF` и `options`:
```json
{"command": "echo Hello from server > /tmp/result.txt && cat /tmp/result.txt"}
```

Агент получит задание на следующем опросе и вы увидите:
```
[INFO]    Task received: code=CONF session=bvLeD2gv-...
[INFO]    Executing task: CONF session: bvLeD2gv-...
[INFO]    Running command: echo Hello from server ...
[INFO]    Command completed successfully
[INFO]    Sending result to server, files: 1
[INFO]    Result accepted by server. msg: ok
```

### Шаг 4 — Проверка журнала
```bash
cat webagent.log
```

### Шаг 5 — Тесты
```bash
cd build && ctest --output-on-failure
```

---

## Web API

### POST /wa_reg/ — Регистрация агента

Запрос:
```json
{"UID": "007", "descr": "web-agent"}
```
Ответ (успех):
```json
{"code_responce": "0", "msg": "Регистрация прошла успешно", "access_code": "594807-..."}
```
Ответ (уже зарегистрирован):
```json
{"code_responce": "-3", "msg": "Такой агент уже зарегистрирован"}
```

### POST /wa_task/ — Запрос задания

Запрос:
```json
{"UID": "007", "descr": "web-agent", "access_code": "594807-..."}
```
Ответ (есть задание):
```json
{"code_responce": "1", "task_code": "CONF", "options": "{...}", "session_id": "...", "status": "RUN"}
```
Ответ (нет задания):
```json
{"code_responce": "0", "status": "WAIT"}
```

### POST /wa_result/ — Отправка результата

Тип: `multipart/form-data`

| Поле          | Тип    | Описание                        |
|---------------|--------|---------------------------------|
| `result_code` | string | 0 = OK, < 0 = ошибка            |
| `result`      | string | JSON с метаинформацией          |
| `file1..fileN`| file   | Файлы с результатами            |

---

## Модули

### `Config` (include/config.h)
Загружает JSON-конфигурацию. Все поля имеют значения по умолчанию.

### `Logger` (include/logger.h)
Потокобезопасный логгер. Уровни: DEBUG, INFO, WARNING, ERROR. Пишет в stdout и файл.

### `HttpClient` (include/http_client.h)
Обёртка над libcurl:
- `post_json(url, json)` — POST application/json
- `post_multipart(url, fields, files)` — POST multipart/form-data
- `is_reachable(url)` — проверка доступности

### `TaskExecutor` (include/task_executor.h)
Выполняет задания:
- `CONF` / `RUN` — запускает команду/программу из `options.command`
- `UPLOAD` — собирает файлы из `results_dir`
- Сохраняет вывод программы в `results_dir/output_<session>.txt`

### `Agent` (include/agent.h)
Главный класс. Управляет:
- Ожиданием сервера с экспоненциальным backoff
- Регистрацией и кэшированием `access_code`
- Циклом поллинга задач
- Делегированием выполнения `TaskExecutor`
- Отправкой результатов
