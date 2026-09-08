import QtQuick 2.15
import QtQuick.Layouts 1.15
import "../Components"

/**
 * 主题色选择卡片
 *
 * 使用 GlassCard 毛玻璃背景。
 */
GlassCard {
    id: root

    required property var settingsVM
    required property var themeVM

    implicitHeight: col.implicitHeight + 32

    // ── Flow 流动渐变状态 ──
    property bool flowEnabled: false

    Timer {
        id: flowTimer
        interval: 120
        repeat: true
        running: flowEnabled
        onTriggered: {
            // 循环切换 16 种主题色
            var idx = themeVM.accentColorIndex
            themeVM.accentColorIndex = (idx + 1) % 16
        }
    }

    ColumnLayout {
        id: col
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.margins: 16
        spacing: 12

        // 卡片标题 + Flow 切换按钮
        RowLayout {
            Layout.fillWidth: true
            spacing: 8
            Layout.leftMargin: 4

            ShadowText {
                text: "主题色彩"
                font.pixelSize: 11
                font.weight: Font.DemiBold
                font.capitalization: Font.AllUppercase
                font.letterSpacing: 1.2
                font.family: "LXGW Neo XiHei Plus, Inter, sans-serif"
                renderType: Text.NativeRendering
                font.hintingPreference: Font.PreferFullHinting
                color: themeVM.palette.textTertiary
            }

            Item { Layout.fillWidth: true }

            // Flow 切换按钮
            Rectangle {
                width: 36
                height: 18
                radius: 9
                color: root.flowEnabled ? themeVM.palette.primary : "transparent"
                border.color: root.flowEnabled ? themeVM.palette.primary : themeVM.palette.textTertiary
                border.width: 1
                opacity: root.flowEnabled ? 1 : 0.6

                Behavior on color { ColorAnimation { duration: 200 } }
                Behavior on border.color { ColorAnimation { duration: 200 } }
                Behavior on opacity { NumberAnimation { duration: 200 } }

                Text {
                    anchors.centerIn: parent
                    text: "Flow"
                    font.pixelSize: 8
                    font.weight: Font.Bold
                    font.family: "Inter, sans-serif"
                    color: root.flowEnabled ? "#fff" : themeVM.palette.textTertiary
                }

                MouseArea {
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    onClicked: {
                        root.flowEnabled = !root.flowEnabled
                    }
                }
            }
        }

        // 8x2 颜色网格
        GridLayout {
            Layout.fillWidth: true
            columns: 8
            rowSpacing: 8
            columnSpacing: 8

            ColorDot {
                colorIndex: 0
                accentColor: "" + themeVM.accentColors[0]
                themeVM: root.themeVM
            }
            ColorDot {
                colorIndex: 1
                accentColor: "" + themeVM.accentColors[1]
                themeVM: root.themeVM
            }
            ColorDot {
                colorIndex: 2
                accentColor: "" + themeVM.accentColors[2]
                themeVM: root.themeVM
            }
            ColorDot {
                colorIndex: 3
                accentColor: "" + themeVM.accentColors[3]
                themeVM: root.themeVM
            }
            ColorDot {
                colorIndex: 4
                accentColor: "" + themeVM.accentColors[4]
                themeVM: root.themeVM
            }
            ColorDot {
                colorIndex: 5
                accentColor: "" + themeVM.accentColors[5]
                themeVM: root.themeVM
            }
            ColorDot {
                colorIndex: 6
                accentColor: "" + themeVM.accentColors[6]
                themeVM: root.themeVM
            }
            ColorDot {
                colorIndex: 7
                accentColor: "" + themeVM.accentColors[7]
                themeVM: root.themeVM
            }
            ColorDot {
                colorIndex: 8
                accentColor: "" + themeVM.accentColors[8]
                themeVM: root.themeVM
            }
            ColorDot {
                colorIndex: 9
                accentColor: "" + themeVM.accentColors[9]
                themeVM: root.themeVM
            }
            ColorDot {
                colorIndex: 10
                accentColor: "" + themeVM.accentColors[10]
                themeVM: root.themeVM
            }
            ColorDot {
                colorIndex: 11
                accentColor: "" + themeVM.accentColors[11]
                themeVM: root.themeVM
            }
            ColorDot {
                colorIndex: 12
                accentColor: "" + themeVM.accentColors[12]
                themeVM: root.themeVM
            }
            ColorDot {
                colorIndex: 13
                accentColor: "" + themeVM.accentColors[13]
                themeVM: root.themeVM
            }
            ColorDot {
                colorIndex: 14
                accentColor: "" + themeVM.accentColors[14]
                themeVM: root.themeVM
            }
            ColorDot {
                colorIndex: 15
                accentColor: "" + themeVM.accentColors[15]
                themeVM: root.themeVM
            }
        }

        // 当前选中颜色的文字提示
        Text {
            text: "当前主题色"
            font.pixelSize: 13
            renderType: Text.NativeRendering
            font.hintingPreference: Font.PreferFullHinting
            color: themeVM.palette.textTertiary
            Layout.leftMargin: 4
        }

        // 颜色预览
        RowLayout {
            spacing: 10

            Rectangle {
                implicitWidth: 32
                implicitHeight: 32
                radius: 8
                color: themeVM.palette.primary

                Behavior on color {
                    ColorAnimation { duration: 300; easing.type: Easing.InOutCubic }
                }
            }

            Text {
                text: themeVM.palette.primary
                font.pixelSize: 13
                font.weight: Font.Medium
                renderType: Text.NativeRendering
                font.hintingPreference: Font.PreferFullHinting
                color: themeVM.palette.textPrimary

                Behavior on color {
                    ColorAnimation { duration: 300; easing.type: Easing.InOutCubic }
                }
            }
        }
    }
}
