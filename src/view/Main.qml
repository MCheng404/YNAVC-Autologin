import QtQuick 2.15
import QtQuick.Window 2.15

/**
 * 根 QML：应用入口
 *
 * 应用启动后不显示主窗口，仅在系统托盘运行
 * 设置窗口通过 trayVM.openSettingsRequested 信号触发
 *
 * 注意：SettingsWindow 有 required property，必须在实例化时用
 * Loader.setSource(url, initialProperties) 同步注入，
 * 不能用 source 静态绑定后在 onLoaded 里赋值（时序上已经太晚了）。
 */
Item {
    id: root

    Component.onCompleted: {
        console.log("Main.qml: Component.onCompleted, Qt.platform.os =", Qt.platform.os)
    }

    // 设置窗口（懒加载，不预设 source — 由 openSettings() 通过 setSource 带属性创建）
    Loader {
        id: settingsLoader
        active: false

        onLoaded: {
            console.log("Main.qml: SettingsWindow onLoaded 触发")
            if (item) {
                item.show()
                item.raise()
                item.requestActivate()
                console.log("Main.qml: SettingsWindow 已显示")
                // 在 Loader 的 onLoaded 中调用（此时 app 已注册）
                if (typeof app !== 'undefined' && app && typeof app.applyWindowMask === 'function') {
                    app.applyWindowMask(item, 24)
                    console.log("Main.qml: applyWindowMask called for SettingsWindow")
                } else {
                    console.warn("Main.qml: app.applyWindowMask not available")
                }
            } else {
                console.log("Main.qml: WARNING - onLoaded 触发但 item 为 null")
            }
        }

        onStatusChanged: {
            console.log("Main.qml: Loader status =", status)
            if (status === Loader.Error) {
                console.log("  source =", source)
                console.log("  Qt.resolvedUrl() =", Qt.resolvedUrl(source))
                if (settingsLoader.errors) {
                    for (var i = 0; i < settingsLoader.errors.length; i++) {
                        console.log("  QML Error", i, ":", settingsLoader.errors[i].toString())
                    }
                }
            }
        }
    }

    // 监听设置窗口请求
    Connections {
        target: trayVM
        function onOpenSettingsRequested() {
            console.log("Main.qml: 收到 openSettingsRequested 信号")
            openSettings()
        }
    }

    function openSettings() {
        console.log("Main.qml: openSettings() 被调用, active =", settingsLoader.active, "status =", settingsLoader.status)

        if (!settingsLoader.active) {
            // 首次打开：用 setSource 同步注入 required properties
            // 这样 SettingsWindow 在实例化时就能拿到 authVM/settingsVM/themeVM
            console.log("Main.qml: 使用 setSource 创建 SettingsWindow（含初始属性）")
            settingsLoader.setSource("qrc:/AutoLogin/src/view/SettingsWindow.qml", {
                "authVM":     authVM,
                "settingsVM": settingsVM,
                "themeVM":    themeVM,
                "trayVM":     trayVM,
                "deviceVM":   deviceVM,
                "logVM":      logVM
            })
            settingsLoader.active = true
        } else if (settingsLoader.item) {
            console.log("Main.qml: 显示已加载的 SettingsWindow")
            settingsLoader.item.show()
            settingsLoader.item.raise()
            settingsLoader.item.requestActivate()
            // 再次打开设置时静默获取一次在线设备。
            // 用 refreshIfStale（10s 节流）而非 refresh —— 反复开关设置窗口时
            // 不应反复登录自助服务（每次 0.3~1s，对服务端也是无谓压力）。
            if (typeof deviceVM !== "undefined" && deviceVM)
                deviceVM.refreshIfStale(10)
        } else {
            console.log("Main.qml: Loader 已激活但 item 还未就绪")
        }
    }

    function closeSettings() {
        if (settingsLoader.item) {
            if (settingsLoader.item.visible)
                settingsLoader.item.requestClose()
            else
                settingsLoader.item.hide()
        }
    }
}
