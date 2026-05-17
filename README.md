<<<<<<< Updated upstream
# Web Agent

Веб-агент для удаленного выполнения команд на C++.

## Сборка
```bash
mkdir build && cd build
cmake ..
make./web_agent config/agent.conf
=======
# 🌐 Web Agent

[![C++](https://img.shields.io/badge/C%2B%2B-17-blue.svg)](https://isocpp.org/)
[![CMake](https://img.shields.io/badge/CMake-3.10+-green.svg)](https://cmake.org/)
[![Build](https://img.shields.io/badge/build-passing-brightgreen.svg)]()
[![Tests](https://img.shields.io/badge/tests-8%20passed-brightgreen.svg)]()
[![License](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE)

> **Агент для автоматического выполнения задач по расписанию с HTTP-интерфейсом**

## 📋 О проекте

Web Agent — это легковесное C++ приложение, которое:
- 🔄 Автоматически получает задачи с сервера
- ⚙️ Выполняет их по расписанию
- 📤 Отправляет результаты обратно
- 📝 Ведёт подробное логирование

### 🎯 Основные возможности

| Функция | Описание |
|---------|----------|
| 📁 Конфигурация | Гибкая настройка через INI-файлы |
| 🌐 HTTP клиент | Встроенный клиент для общения с сервером |
| ⏰ Планировщик | Выполнение задач с заданным интервалом |
| 📊 Логирование | Детальное логирование всех операций |
| 🧪 Тестирование | 8 модульных тестов для проверки работы |
| 🔄 Жизненный цикл | Полное управление состояниями задач |

## 🚀 Быстрый старт

### Установка зависимостей

```bash
# Ubuntu/Debian
sudo apt update
sudo apt install build-essential cmake libcurl4-openssl-dev nlohmann-json3-dev
>>>>>>> Stashed changes
