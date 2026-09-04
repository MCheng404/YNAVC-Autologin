import QtQuick 2.15
import QtQuick.Layouts 1.15

/**
 * 自定义开关（弹性动画）
 *
 * required property:
 * - checked: 当前状态
 * - title: 开关标题
 * - description: 描述文字
 * - themeVM: 主题 ViewModel
 *
 * signal:
 * - toggled(bool checked)
 */
Item {
    id: root

    required property bool checked
    required property string title
    required property string description
    required property var themeVM

    signal toggled(bool checked)

    implicitHeight: optionRow.implicitHeight
    height: implicitHeight

    // 选项行
    RowLayout {
        id: optionRow
        anchors.left: parent.left
        anchors.right: parent.right
        spacing: 12

        // 文字区域
        ColumnLayout {
            Layout.fillWidth: true
            spacing: 2

            Text {
                text: root.title
                font.pixelSize: 13
                font.family: "LXGW Neo XiHei Plus, Inter, sans-serif"
                renderType: Text.NativeRendering
                font.hintingPreference: Font.PreferFullHinting
                color: themeVM.palette.textPrimary
                Layout.fillWidth: true
                wrapMode: Text.WordWrap

                Behavior on color {
                    ColorAnimation { duration: 200; easing.type: Easing.OutQuad }
                }
            }

            Text {
                text: root.description
                font.pixelSize: 11
                font.family: "LXGW Neo XiHei Plus, Inter, sans-serif"
                renderType: Text.NativeRendering
                font.hintingPreference: Font.PreferFullHinting
                color: themeVM.palette.textTertiary
                Layout.fillWidth: true
                wrapMode: Text.WordWrap

                Behavior on color {
                    ColorAnimation { duration: 200; easing.type: Easing.OutQuad }
                }
            }
        }

        // 开关控件
        Rectangle {
            id: toggleTrack
            width: 44
            height: 24
            radius: 24
            color: root.checked ? themeVM.palette.toggleActive : themeVM.palette.toggleTrack
            Behavior on color {
                ColorAnimation { duration: 300; easing.type: Easing.InOutCubic }
            }

            // 滑块
            Rectangle {
                id: thumb
                width: 18
                height: 18
                radius: 18
                color: "#ffffff"
                x: root.checked ? 23 : 3
                y: 3

                // 阴影效果
                Rectangle {
                    anchors.fill: parent
                    anchors.margins: -1
                    radius: 19
                    color: "transparent"
                    border.color: root.checked ? "rgba(0,0,0,0.1)" : "rgba(0,0,0,0.15)"
                    border.width: 1
                    z: -1
                }

                Behavior on x {
                    NumberAnimation {
                        duration: 300
                        easing.type: Easing.OutElastic
                        easing.amplitude: 0.8
                        easing.period: 0.4
                    }
                }
            }

            // 整个 track 可点击
            MouseArea {
                anchors.fill: parent
                onClicked: {
                    root.checked = !root.checked
                    root.toggled(root.checked)
                }
            }
        }
    }
}
