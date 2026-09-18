import QtQuick 2.15
import QtQuick.Layouts 1.15
import "../Components"

/**
 * 运行日志卡片（纯平毛玻璃）
 *
 * 数据来源：logVM（可绑定占位接口）
 *   - 当前 Logger 尚未暴露给 QML（无 Q_PROPERTY / 内存缓冲 / setContextProperty），
 *     故本卡片先用 property var logVM: null 占位，不依赖任何 C++ 改动。
 *   - 待 C++ 侧为 Logger 增加 Q_PROPERTY(QStringList recentLogs) + recentLogsChanged 信号，
 *     并在 App::run() 中 engine.rootContext()->setContextProperty("logVM", ...) 注入后，
 *     本卡片即可显示真实的最近若干条运行日志。
 *   - logVM 为 null         → 显示「日志功能待接入」
 *   - 注入但 recentLogs 为空 → 显示「暂无日志」
 *
 * 视觉：继承 GlassCard（纯平毛玻璃，无渐变），标题 12px 大写 + letterSpacing 1.2，
 *       等宽字体 13px 显示日志，卡内 Flickable 固定高度 180 可滚动。
 */
GlassCard {
    id: root

    // ---- 公开属性 ----
    required property var themeVM

    // 日志数据源（占位接口，默认 null —— 由 C++ 后续注入）
    property var logVM: null

    implicitHeight: col.implicitHeight + 32

    // ---- 派生状态 ----
    readonly property bool hasVM: logVM !== null && logVM !== undefined
    readonly property var logLines: {
        if (hasVM && logVM.recentLogs !== undefined && logVM.recentLogs !== null)
            return logVM.recentLogs
        return []
    }
    readonly property bool hasLogs: logLines.length > 0

    // ---- 布局 ----
    ColumnLayout {
        id: col
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.margins: 16
        spacing: 16

        // 标题行：标题 + 右侧「复制全部」按钮
        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            ShadowText {
                text: "运行日志"
                font.pixelSize: 12
                font.weight: Font.DemiBold
                font.capitalization: Font.AllUppercase
                font.letterSpacing: 1.2
                font.family: "LXGW Neo XiHei Plus, Inter, sans-serif"
                color: themeVM.palette.textTertiary
                Layout.alignment: Qt.AlignVCenter
            }

            Item { Layout.fillWidth: true; Layout.fillHeight: true }

            // 复制全部：将当前日志文本写入系统剪贴板
            Text {
                id: copyBtn
                text: "复制全部"
                font.pixelSize: 13
                font.weight: Font.Medium
                font.family: "LXGW Neo XiHei Plus, Inter, sans-serif"
                color: copyMouse.containsMouse ? themeVM.palette.primary : themeVM.palette.textTertiary
                Layout.alignment: Qt.AlignVCenter

                Behavior on color {
                    ColorAnimation { duration: 150; easing.type: Easing.OutQuad }
                }

                MouseArea {
                    id: copyMouse
                    anchors.fill: parent
                    hoverEnabled: true
                    onClicked: {
                        clipboard.setText(root.logLines.join("\n"))
                    }
                }
            }
        }

        // 日志内容区（卡内可滚动，固定高度 180）
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 180
            radius: 10
            color: themeVM.palette.surfaceVariant
            clip: true

            // 待接入 / 空态
            Text {
                anchors.centerIn: parent
                visible: !root.hasVM || !root.hasLogs
                text: root.hasVM ? "暂无日志" : "日志功能待接入"
                font.pixelSize: 13
                font.family: "LXGW Neo XiHei Plus, Inter, sans-serif"
                color: themeVM.palette.textTertiary
            }

            // 日志列表（等宽字体，可滚动）
            Flickable {
                anchors.fill: parent
                anchors.margins: 12
                visible: root.hasLogs
                contentWidth: width
                contentHeight: logCol.implicitHeight
                clip: true

                ColumnLayout {
                    id: logCol
                    anchors.left: parent.left
                    anchors.right: parent.right
                    spacing: 6

                    Repeater {
                        model: root.logLines

                        Text {
                            Layout.fillWidth: true
                            text: modelData
                            font.pixelSize: 13
                            font.family: "JetBrains Mono, LXGW Neo XiHei Plus, monospace"
                            color: themeVM.palette.textSecondary
                            wrapMode: Text.WrapAnywhere
                        }
                    }
                }
            }
        }
    }
}
