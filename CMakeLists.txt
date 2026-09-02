cmake_minimum_required(VERSION 3.20)
project(AutoLogin VERSION 2.13 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

# Qt6 查找
find_package(Qt6 REQUIRED COMPONENTS Core Gui Quick Qml Network Widgets)

# Win32 链接库
if(WIN32)
    link_libraries(iphlpapi dwmapi uxtheme ole32 oleaut32 ws2_32 shlwapi)
endif()

# 源文件
set(SOURCES
    src/main.cpp
    src/App.cpp
    # Platform
    src/platform/NetworkAdapter.cpp
    src/platform/Registry.cpp
    src/platform/HotspotManager.cpp
    src/platform/Notification.cpp
    src/platform/SystemTray.cpp
    src/platform/ToastWindow.cpp
    # Model
    src/model/AuthEngine.cpp
    src/model/ConnectivityChecker.cpp
    src/model/Scheduler.cpp
    src/model/Logger.cpp
    src/model/Settings.cpp
    # ViewModel
    src/viewmodel/AuthViewModel.cpp
    src/viewmodel/SettingsViewModel.cpp
    src/viewmodel/TrayViewModel.cpp
    src/viewmodel/ThemeViewModel.cpp
)

# 头文件（用于 MOC 自动处理）
set(HEADERS
    src/App.h
    # Platform
    src/platform/Win32Defs.h
    src/platform/NetworkAdapter.h
    src/platform/Registry.h
    src/platform/HotspotManager.h
    src/platform/Notification.h
    src/platform/SystemTray.h
    # Model
    src/model/AuthEngine.h
    src/model/ConnectivityChecker.h
    src/model/Scheduler.h
    src/model/Logger.h
    src/model/Settings.h
    # ViewModel
    src/viewmodel/AuthViewModel.h
    src/viewmodel/SettingsViewModel.h
    src/viewmodel/TrayViewModel.h
    src/viewmodel/ThemeViewModel.h
)

# 可执行文件
if(WIN32)
    add_executable(${PROJECT_NAME} WIN32 ${SOURCES} ${HEADERS})
else()
    add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})
endif()

# 链接 Qt6 模块
target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Quick
    Qt6::Qml
    Qt6::Network
    Qt6::Widgets
)

# 包含目录
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/platform
    ${CMAKE_CURRENT_SOURCE_DIR}/src/model
    ${CMAKE_CURRENT_SOURCE_DIR}/src/viewmodel
)

# Windows 特定设置
if(WIN32)
    if(MSVC)
        # MSVC / clang-cl: 直接嵌入 manifest，禁用自动生成
        target_sources(${PROJECT_NAME} PRIVATE app.manifest)
        set_target_properties(${PROJECT_NAME} PROPERTIES
            LINK_FLAGS "/MANIFEST:NO"
        )
    elseif(MINGW)
        # MinGW / llvm-mingw: 通过 .rc 文件嵌入 manifest
        target_sources(${PROJECT_NAME} PRIVATE app.rc)
        # -mwindows 隐藏控制台窗口
        target_link_options(${PROJECT_NAME} PRIVATE -mwindows)
    endif()
endif()

# QML 模块注册（使用 qt6_add_qml_module 管理 QML 文件）
qt6_add_qml_module(${PROJECT_NAME}
    URI AutoLogin
    VERSION 2.13
    QML_FILES
        src/view/Main.qml
        src/view/SettingsWindow.qml
        src/view/Cards/BasicSettingsCard.qml
        src/view/Cards/SystemStatusCard.qml
        src/view/Cards/NetworkSettingsCard.qml
        src/view/Cards/ScheduleCard.qml
        src/view/Cards/AccountCard.qml
        src/view/Cards/AccentColorCard.qml
        src/view/Components/SwitchToggle.qml
        src/view/Components/ThemeToggle.qml
        src/view/Components/CardSpinBox.qml
        src/view/Components/ThemeCard.qml
        src/view/Components/LucideIcon.qml
        src/view/Components/StyledScrollBar.qml
        src/view/Components/ColorDot.qml
        src/view/Components/FloatingScrollBar.qml
        src/view/Components/ToastPositionSelector.qml
        src/view/Components/GlassCard.qml
        src/view/Components/ShadowText.qml
    RESOURCES
        resources/icons/connected.svg
        resources/icons/disconnected.svg
        resources/icons/authenticating.svg
        resources/fonts/embedded/LXGWNeoXiHeiPlus.ttf
        resources/fonts/embedded/InterVariable.ttf
        resources/fonts/embedded/JetBrainsMono-Regular.ttf
)

# 静态 Qt 构建时：将所有 QML 模块插件（QtQuick/Controls/Layouts 等）静态导入 exe
get_target_property(_qt_core_type Qt6::Core TYPE)
if(_qt_core_type STREQUAL "STATIC_LIBRARY")
    qt6_import_qml_plugins(${PROJECT_NAME})
    # 静态链接 MinGW/clang 运行时，消除 libgcc/libstdc++/libwinpthread 依赖
    if(MINGW)
        target_link_options(${PROJECT_NAME} PRIVATE -static)
    endif()
endif()
