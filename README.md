# YNAVC-Autologin

[![C++17](https://img.shields.io/badge/C%2B%2B-17-blue)](https://en.cppreference.com/w/cpp/17) ![Qt6.11](https://img.shields.io/badge/Qt-6.11-green) ![Platform](https://img.shields.io/badge/Platform-Windows%2010/11-blueviolet) ![Version](https://img.shields.io/badge/Version-2.13-purple) ![License](https://img.shields.io/badge/License-CC%20BY--NC--SA%204.0-red) [![Download](https://img.shields.io/badge/Download-v2.13%20Single%20EXE-brightgreen)](https://github.com/MCheng404/autologin/releases/latest)

> **English** | [中文](#中文) | [Русский](#русский)

云南农业职业技术学院校园网自动认证服务，仅供交流学习使用。

---

## English

### Introduction

YNAVC-Autologin is a campus network auto-authentication service for Yunnan Agricultural Vocational and Technical College. It supports HTTP portal authentication with automatic reconnection, schedule-based login, and a modern Qt6 QML UI.

> **⚠️ This project is for educational and exchange purposes only. Commercial use is strictly prohibited.**

### Features

| Feature | Description |
|---------|-------------|
| **Auto Login** | HTTP portal authentication with auto-reconnect on disconnect |
| **Schedule** | Login/logout at specified times (e.g. 03:55–05:05) |
| **Modern UI** | 10 cards, 16 components, Light/Dark/System themes |
| **Account Management** | Multi-account support with credential storage |
| **Network Detection** | Gateway ping + HTTP status check |
| **System Tray** | Minimize to tray, startup with Windows |

### Tech Stack

| Layer | Technology |
|-------|------------|
| Model | C++17, AuthEngine, ConnectivityChecker, Scheduler |
| View | QML, Qt Quick, Qt Quick Controls 2 |
| Platform | Win32/WinRT API |

### Build Requirements

- Qt 6.11+ (Qt Creator, Qt Network, Qt Widgets)
- CMake 3.21+
- Ninja
- LLVM MinGW (llvm-mingw-w64)

#### Build with LLVM MinGW

```bash
mkdir build && cd build
cmake .. -G "Ninja"   -DCMAKE_PREFIX_PATH=D:/Qt/6.11.1/llvm-mingw_64   -DCMAKE_CXX_COMPILER=D:/Qt/Tools/llvm-mingw1706_64/bin/clang++.exe   -DCMAKE_MAKE_PROGRAM=E:/Tools/ninja/ninja.exe
ninja
windeployqt --no-translations YNAVC-Autologin.exe
```

#### Single-File (Static) Build

The published `AutoLogin.exe` is fully static — all Qt libraries, QML modules and C++ runtime are linked into the binary. It runs on any Windows 10/11 x64 machine with no installation and no external DLLs.

Building it requires a **statically-built Qt** (the official online installer only ships shared libs):

```bash
# 1. Build static Qt 6.11.1 (takes 20–40 min)
cmake -DQT_BUILD_SUBMODULES=qtbase;qtshadertools;qtdeclarative;qtsvg ^
  -DBUILD_SHARED_LIBS=OFF -DCMAKE_INSTALL_PREFIX=D:/Qt/6.11.1-llvm-mingw-static ^
  -DCMAKE_CXX_COMPILER=D:/Qt/Tools/llvm-mingw1706_64/bin/clang++.exe ^
  -DCMAKE_MAKE_PROGRAM=D:/Qt/Tools/Ninja/ninja.exe -DQT_QMAKE_TARGET_MKSPEC=win32-clang-g++ ^
  -DQT_BUILD_EXAMPLES=FALSE -DQT_BUILD_TESTS=FALSE -DCMAKE_BUILD_TYPE=Release ^
  -DFEATURE_static_runtime=ON -DINPUT_opengl=no -G Ninja D:/Qt/6.11.1/Src
cmake --build . --parallel && cmake --install .

# 2. Build the app against it
cmake -B build_static -G Ninja -DCMAKE_BUILD_TYPE=Release ^
  -DCMAKE_PREFIX_PATH=D:/Qt/6.11.1-llvm-mingw-static ^
  -DCMAKE_CXX_COMPILER=D:/Qt/Tools/llvm-mingw1706_64/bin/clang++.exe
cmake --build build_static --parallel
```

> The project's `CMakeLists.txt` detects a static Qt automatically and calls `qt6_import_qml_plugins()` so every QML module is baked into the exe.

### Network Configuration

| Item | Value |
|------|-------|
| Gateway | `172.30.255.2` |
| Auth Server | `connect.rom.miui.com` |
| Timeout | 30s |
| Retry Interval | 300s |
| Schedule Default | 03:55–05:05 |

### License

This project is licensed under [CC BY-NC-SA 4.0](https://creativecommons.org/licenses/by-nc-sa/4.0/).

**Commercial use is strictly prohibited.** You may share and adapt this work for non-commercial purposes with proper attribution.

---

## 中文

### 简介

YNAVC-Autologin（云南农业职业技术学院自动认证服务）是一款校园网自动认证工具，支持 HTTP Portal 认证、断线自动重连、定时登录/注销，以及现代化的 Qt6 QML 界面。

> **⚠️ 本项目仅供交流学习使用，严禁用于商业用途。**

### 功能特性

| 功能 | 说明 |
|------|------|
| **自动登录** | HTTP Portal 认证，断线自动重连 |
| **定时计划** | 指定时间段自动登录/注销（如 03:55–05:05） |
| **现代界面** | 10 个卡片、16 个组件，支持明亮/暗黑/系统主题 |
| **账号管理** | 多账号支持，凭据本地安全存储 |
| **网络检测** | 网关 Ping + HTTP 状态双重检测 |
| **系统托盘** | 最小化到托盘，开机自启 |

### 技术栈

| 层级 | 技术 |
|------|------|
| Model | C++17, AuthEngine, ConnectivityChecker, Scheduler |
| View | QML, Qt Quick, Qt Quick Controls 2 |
| Platform | Win32/WinRT API |

### 构建环境

- Qt 6.11+（Qt Creator, Qt Network, Qt Widgets）
- CMake 3.21+
- Ninja
- LLVM MinGW (llvm-mingw-w64)

#### 使用 LLVM MinGW 构建

```bash
mkdir build && cd build
cmake .. -G "Ninja" \
  -DCMAKE_PREFIX_PATH=D:/Qt/6.11.1/llvm-mingw_64 \
  -DCMAKE_CXX_COMPILER=D:/Qt/Tools/llvm-mingw1706_64/bin/clang++.exe \
  -DCMAKE_MAKE_PROGRAM=E:/Tools/ninja/ninja.exe
ninja
windeployqt --no-translations YNAVC-Autologin.exe
```

### 网络配置

| 项目 | 值 |
|------|-----|
| 网关 | `172.30.255.2` |
| 认证服务器 | `connect.rom.miui.com` |
| 超时时间 | 30 秒 |
| 重试间隔 | 300 秒 |
| 默认定时 | 03:55–05:05 |

### 开源协议

本项目采用 [CC BY-NC-SA 4.0](https://creativecommons.org/licenses/by-nc-sa/4.0/) 协议。

**严禁商业用途。** 仅允许非商业性质的分享和改编，且需注明出处。

---

## Русский

### Описание

YNAVC-Autologin — служба автоматической аутентификации сети кампуса Яньнаньского сельскохозяйственного профессионально-технического колледжа. Поддерживает HTTP-портальную аутентификацию, автоматическое переподключение, планирование входа и современный интерфейс на Qt6 QML.

> **⚠️ Этот проект предназначен только для учебных и обменных целей. Коммерческое использование строго запрещено.**

### Возможности

| Функция | Описание |
|---------|----------|
| **Авто вход** | HTTP-портальная аутентификация с автопереподключением |
| **Расписание** | Автоматический вход/выход в заданное время |
| **Современный UI** | 10 карточек, 16 компонентов, светлая/тёмная/системная темы |
| **Учётные записи** | Многопользовательская поддержка с безопасным хранением |
| **Проверка сети** | Пинг шлюза + HTTP-проверка статуса |
| **Системный трей** | Сворачивание в трей, автозапуск с Windows |

### Технологический стек

| Уровень | Технология |
|---------|-----------|
| Model | C++17, AuthEngine, ConnectivityChecker, Scheduler |
| View | QML, Qt Quick, Qt Quick Controls 2 |
| Platform | Win32/WinRT API |

### Требования для сборки

- Qt 6.11+ (Qt Creator, Qt Network, Qt Widgets)
- CMake 3.21+
- Ninja
- LLVM MinGW (llvm-mingw-w64)

#### Сборка с LLVM MinGW

```bash
mkdir build && cd build
cmake .. -G "Ninja" \
  -DCMAKE_PREFIX_PATH=D:/Qt/6.11.1/llvm-mingw_64 \
  -DCMAKE_CXX_COMPILER=D:/Qt/Tools/llvm-mingw1706_64/bin/clang++.exe \
  -DCMAKE_MAKE_PROGRAM=E:/Tools/ninja/ninja.exe
ninja
windeployqt --no-translations YNAVC-Autologin.exe
```

### Сетевая конфигурация

| Параметр | Значение |
|----------|----------|
| Шлюз | `172.30.255.2` |
| Сервер аутентификации | `connect.rom.miui.com` |
| Тайм-аут | 30 сек |
| Интервал повтора | 300 сек |
| Расписание по умолчанию | 03:55–05:05 |

### Лицензия

Этот проект лицензирован по [CC BY-NC-SA 4.0](https://creativecommons.org/licenses/by-nc-sa/4.0/).

**Коммерческое использование строго запрещено.** Вы можете распространять и адаптировать эту работу в некоммерческих целях с указанием авторства.

---

> ⚠️ **Disclaimer / 免责声明 / Отказ от ответственности**
>
> This project is provided as-is for educational and exchange purposes only. The authors are not responsible for any consequences arising from the use of this software. Commercial use is strictly prohibited.
>
> 本项目仅供交流学习使用，作者不对因使用本软件而产生的任何后果负责。严禁商业用途。
>
> Этот проект предоставляется как есть исключительно в учебных и обменных целях. Авторы не несут ответственности за любые последствия, возникающие в результате использования этого программного обеспечения. Коммерческое использование строго запрещено.
