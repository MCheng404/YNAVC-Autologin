import QtQuick 2.15
import QtQuick.Layouts 1.15
import "../Components"

/**
 * 系统状态卡片（时钟 + 运行时间 + 上次认证间隔）
 *
 * 使用 GlassCard 高斯模糊玻璃背景。
 * appStartMs 由 C++ TrayViewModel 构造函数在应用启动时记录，
 * 比 QML 的 Component.onCompleted 更早、更可靠。
 */
GlassCard {
    id: root

    // ---- 公开属性 ----
    required property var trayVM
    required property var themeVM

    // 可见性开关：由 SettingsWindow 注入 settingsWindow.visible，
    // 窗口隐藏时暂停 Timer，避免无谓刷新耗电
    property bool active: true


    implicitHeight: col.implicitHeight + 32

    // ---- 辅助函数 ----

    function fmtHMS(totalMs) {
        if (!totalMs || totalMs <= 0) return "00:00:00"
        var secs = Math.floor(totalMs / 1000)
        var hours = Math.floor(secs / 3600)
        secs = secs % 3600
        var minutes = Math.floor(secs / 60)
        secs = secs % 60
        var hh = String(hours).padStart(2, "0")
        var mm = String(minutes).padStart(2, "0")
        var ss = String(secs).padStart(2, "0")
        return hh + ":" + mm + ":" + ss
    }

    // ---- 动态数据 ----

    // 当前系统时间字符串（每秒刷新）
    property string timeStr: "00:00:00"

    // 当前系统日期字符串
    property string dateStr: (function() {
        var d = new Date()
        var week = ["周日", "周一", "周二", "周三", "周四", "周五", "周六"]
        return d.getFullYear() + "年" +
               String(d.getMonth() + 1).padStart(2, "0") + "月" +
               String(d.getDate()).padStart(2, "0") + "日  " +
               week[d.getDay()]
    })()
    property string runtimeStr: "00:00:00"

    // 距上次认证
    property string sinceAuthStr: "从未"

    // 联网状态（数据源：trayVM.iconSource —— connected/disconnected/authenticating）
    property string statusKey: trayVM ? trayVM.iconSource : "disconnected"
    property color statusDotColor: {
        if (statusKey === "connected") return themeVM.palette.success
        if (statusKey === "authenticating") return themeVM.palette.warning
        return themeVM.palette.textTertiary
    }
    property string statusLabel: {
        if (statusKey === "connected") return "已连接"
        if (statusKey === "authenticating") return "认证中"
        return "未连接"
    }

    // 下次定时认证倒计时文案（数据源：trayVM.nextAuthSec，秒）
    property string nextAuthStr: "—"

    // 每秒刷新——同时更新 timeStr/runtimeStr/sinceAuthStr/nextAuthStr
    // 仅在 active（窗口可见）时运行；恢复可见时 triggeredOnStart 立即刷新一次
    Timer {
        id: ticker
        interval: 1000
        repeat: true
        running: root.active
        triggeredOnStart: true
        onTriggered: {
            var now = Date.now()

            // 时钟
            var d = new Date(now)
            timeStr = String(d.getHours()).padStart(2, "0") + ":" +
                       String(d.getMinutes()).padStart(2, "0") + ":" +
                       String(d.getSeconds()).padStart(2, "0")

            // 日期
            var week = ["周日", "周一", "周二", "周三", "周四", "周五", "周六"]
            dateStr = d.getFullYear() + "年" +
                       String(d.getMonth() + 1).padStart(2, "0") + "月" +
                       String(d.getDate()).padStart(2, "0") + "日  " +
                       week[d.getDay()]

            // 运行时间
            var startMs = trayVM ? trayVM.appStartMs : 0
            runtimeStr = startMs > 0 ? fmtHMS(now - startMs) : "00:00:00"

            // 距上次认证
            var last = trayVM ? trayVM.lastAuthMs : 0
            sinceAuthStr = last > 0 ? fmtHMS(now - last) : "从未"

            // 下次定时认证倒计时
            var nextSec = trayVM ? trayVM.nextAuthSec : -1
            if (nextSec <= 0) {
                nextAuthStr = "未启用"
            } else {
                var remain = nextSec - Math.floor(now / 1000)
                nextAuthStr = remain > 0 ? fmtHMS(remain * 1000) : "即将认证"
            }
        }
    }

    // ---- 布局 ----
    ColumnLayout {
        id: col
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.margins: 16
        spacing: 16

        // 卡片标题
        ShadowText {
            text: "系统状态"
            font.pixelSize: 11
            font.weight: Font.DemiBold
            font.capitalization: Font.AllUppercase
            font.letterSpacing: 1.2
            font.family: "LXGW Neo XiHei Plus, Inter, sans-serif"
            renderType: Text.NativeRendering
            font.hintingPreference: Font.PreferFullHinting
            color: themeVM.palette.textTertiary
            Layout.leftMargin: 4
        }

        // 联网状态（彩色圆点 + 文字）
        RowLayout {
            Layout.fillWidth: true
            spacing: 8
            Layout.leftMargin: 4

            Rectangle {
                width: 10
                height: 10
                radius: 5
                color: root.statusDotColor
            }

            Text {
                text: root.statusLabel
                font.pixelSize: 12
                font.weight: Font.Normal
                font.family: "LXGW Neo XiHei Plus, Inter, sans-serif"
                renderType: Text.NativeRendering
                font.hintingPreference: Font.PreferFullHinting
                color: themeVM.palette.textSecondary
            }
        }

        // 时间 + 日期（大号字体，居中）
        ColumnLayout {
            Layout.fillWidth: true
            spacing: 6

            Text {
                text: root.timeStr
                font.pixelSize: 34
                font.weight: Font.Light
                font.family: "JetBrains Mono, LXGW Neo XiHei Plus, monospace"
                renderType: Text.NativeRendering
                font.hintingPreference: Font.PreferFullHinting
                color: themeVM.palette.textPrimary
                Layout.alignment: Qt.AlignHCenter
            }

            Text {
                text: root.dateStr
                font.pixelSize: 12
                font.weight: Font.Normal
                font.family: "LXGW Neo XiHei Plus, Inter, sans-serif"
                renderType: Text.NativeRendering
                font.hintingPreference: Font.PreferFullHinting
                color: themeVM.palette.textTertiary
                Layout.alignment: Qt.AlignHCenter
            }
        }

        // 分隔线
        Rectangle {
            Layout.fillWidth: true
            height: 1
            color: themeVM.palette.divider
        }

        // 统计行
        RowLayout {
            Layout.fillWidth: true
            spacing: 0

            // 运行时间
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 10

                    Text {
                        text: "运行时间"
                    font.pixelSize: 11
                    font.weight: Font.Normal
                    font.family: "LXGW Neo XiHei Plus, Inter, sans-serif"
                    renderType: Text.NativeRendering
                    color: themeVM.palette.textTertiary
                    Layout.alignment: Qt.AlignHCenter
                }

                Text {
                    text: root.runtimeStr
                    font.pixelSize: 18
                    font.weight: Font.Medium
                    font.family: "JetBrains Mono, LXGW Neo XiHei Plus, monospace"
                    renderType: Text.NativeRendering
                    color: themeVM.palette.textSecondary
                    Layout.alignment: Qt.AlignHCenter
                }
            }

            // 竖线分隔
            Rectangle {
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                width: 1
                Layout.fillHeight: true
                color: themeVM.palette.divider
            }

            // 距上次认证
            ColumnLayout {
                Layout.fillWidth: true
                spacing: 8

                Text {
                    text: "距上次认证"
                    font.pixelSize: 11
                    font.weight: Font.Normal
                    font.family: "LXGW Neo XiHei Plus, Inter, sans-serif"
                    renderType: Text.NativeRendering
                    color: themeVM.palette.textTertiary
                    Layout.alignment: Qt.AlignHCenter
                }

                Text {
                    text: root.sinceAuthStr
                    font.pixelSize: 18
                    font.weight: Font.Medium
                    font.family: "JetBrains Mono, LXGW Neo XiHei Plus, monospace"
                    renderType: Text.NativeRendering
                    color: themeVM.palette.textSecondary
                    Layout.alignment: Qt.AlignHCenter
                }
            }
        }

        // 下次定时认证倒计时
        RowLayout {
            Layout.fillWidth: true
            spacing: 8
            Layout.leftMargin: 4

            Text {
                text: "下次定时认证"
                font.pixelSize: 11
                font.weight: Font.Normal
                font.family: "LXGW Neo XiHei Plus, Inter, sans-serif"
                renderType: Text.NativeRendering
                font.hintingPreference: Font.PreferFullHinting
                color: themeVM.palette.textTertiary
            }

            Item { Layout.fillWidth: true }

            Text {
                text: root.nextAuthStr
                font.pixelSize: 14
                font.weight: Font.Medium
                font.family: "JetBrains Mono, LXGW Neo XiHei Plus, monospace"
                renderType: Text.NativeRendering
                font.hintingPreference: Font.PreferFullHinting
                color: themeVM.palette.textSecondary
            }
        }
    }
}
