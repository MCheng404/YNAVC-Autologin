import QtQuick 2.15
import "."

/**
 * 主题三态切换按钮：Dark → Light → System → Dark
 *
 * 显示当前模式图标 + 文字提示
 * 200ms 颜色过渡
 */
Item {
    id: root

    required property int themeMode   // 0=Dark, 1=Light, 2=System
    required property bool isDark

    signal modeClicked()

    width: 84
    height: 36

    // 按下回弹
    scale: 1.0
    Behavior on scale {
        NumberAnimation { duration: 140; easing.type: Easing.OutBack }
    }

    // 当前是否暗色，用于决定文字颜色
    readonly property color _textColor: root.isDark ? "#e2e8f0" : "#1e293b"
    readonly property color _bgColor: root.isDark ? "#252540" : "#f1f5f9"
    readonly property color _bgHover: root.isDark ? "#1e293b" : "#e2e8f0"
    readonly property color _borderColor: root.isDark ? "#2d2d4a" : "#e2e8f0"

    Rectangle {
        anchors.fill: parent
        radius: 18
        color: mouseArea.containsMouse ? root._bgHover : root._bgColor
        border.color: root._borderColor
        border.width: 1

        Behavior on color {
            ColorAnimation { duration: 150; easing.type: Easing.OutQuad }
        }
        Behavior on border.color {
            ColorAnimation { duration: 150; easing.type: Easing.OutQuad }
        }

        Row {
            anchors.centerIn: parent
            spacing: 5

            // 图标
            LucideIcon {
                anchors.verticalCenter: parent.verticalCenter
                name: {
                    switch (root.themeMode) {
                    case 0: return "moon"
                    case 1: return "sun"
                    case 2: return "monitor"
                    default: return "moon"
                    }
                }
                size: 16
                color: root._textColor
            }

            // 模式文字
            Text {
                anchors.verticalCenter: parent.verticalCenter
                text: {
                    switch (root.themeMode) {
                    case 0: return "暗色"
                    case 1: return "亮色"
                    case 2: return "自动"
                    default: return "暗色"
                    }
                }
                font.pixelSize: 13
                font.weight: Font.Medium
                font.family: "LXGW Neo XiHei Plus, Inter, sans-serif"
                color: root._textColor

                Behavior on color {
                    ColorAnimation { duration: 150; easing.type: Easing.OutQuad }
                }
            }
        }

        MouseArea {
            id: mouseArea
            anchors.fill: parent
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            onPressed: root.scale = 0.97
            onReleased: root.scale = 1.0
            onCanceled: root.scale = 1.0
            onClicked: root.modeClicked()
        }
    }
}
