import QtQuick 2.15
import QtQuick.Layouts 1.15
import "../Components"

/**
 * 通知位置选择器（屏幕位置图示风格）
 * 一个迷你屏幕预览框，toast 出现在哪个位置就在对应位置渲染 toast 示意块
 * position: 0=右下角, 1=左下角, 2=顶部居中
 */
Rectangle {
    id: root

    required property var themeVM
    property int position: 0  // 0=右下, 1=左下, 2=顶部居中
    property bool isEnabled: true

    // 从 themeVM 提取调色板
    property var themePalette: themeVM ? themeVM.palette : null

    implicitWidth: 172
    implicitHeight: 108

    // ── 屏幕预览框 ──
    radius: 10
    color: themePalette ? (isEnabled ? themePalette.hoverBackground : themePalette.surfaceVariant) : "#2a2a2a"
    border.color: themePalette ? themePalette.outline : "#555"
    border.width: 0.6
    opacity: isEnabled ? 1.0 : 0.55

    Behavior on opacity { NumberAnimation { duration: 150 } }

    // ── 屏幕底座：桌面示意（中心小房子/显示器符号）──
    LucideIcon {
        anchors.centerIn: parent
        name: "monitor"
        size: 22
        color: themePalette ? themePalette.outline : "#555"
        opacity: 0.7
    }

    // ══════════════════════════════════════════
    // 位置槽位 ×3：右下 / 左下 / 顶部居中
    // ══════════════════════════════════════════
    Repeater {
        model: ListModel {
            ListElement { pos: 0; anchorRight: true;  anchorBottom: true;  anchorCenterH: false; anchorTop: false }
            ListElement { pos: 1; anchorRight: false; anchorBottom: true;  anchorCenterH: false; anchorTop: false }
            ListElement { pos: 2; anchorRight: false; anchorBottom: false; anchorCenterH: true;  anchorTop: true }
        }

        delegate: Item {
            id: slot
            width: 64
            height: 34

            // 槽位锚定（屏幕内边距 8）
            anchors.right:      anchorRight  ? parent.right  : undefined
            anchors.bottom:     anchorBottom ? parent.bottom : undefined
            anchors.horizontalCenter: anchorCenterH ? parent.horizontalCenter : undefined
            anchors.top:        anchorTop    ? parent.top    : undefined
            anchors.margins: 8

            readonly property bool isSelected: root.position === pos
            property bool hovered: false

            // ── 点击热区（略大于示意块）──
            MouseArea {
                anchors.fill: parent
                anchors.margins: -4
                enabled: root.isEnabled
                hoverEnabled: true
                cursorShape: root.isEnabled ? Qt.PointingHandCursor : Qt.ArrowCursor
                onEntered: hovered = true
                onExited: hovered = false
                onClicked: root.position = pos
            }

            // ── toast 示意块 ──
            Rectangle {
                id: toastChip
                width: parent.width
                height: 22
                anchors.verticalCenter: parent.verticalCenter
                radius: 5

                color: {
                    if (!root.isEnabled) return "transparent"
                    if (isSelected) return root.themePalette ? root.themePalette.primary : "#0078d4"
                    if (hovered) return root.themePalette ? root.themePalette.cardBackground : "#3a3a3a"
                    return "transparent"
                }
                border.color: {
                    if (isSelected) return root.themePalette ? root.themePalette.primary : "#0078d4"
                    return root.themePalette ? root.themePalette.outline : "#555"
                }
                border.width: isSelected ? 1 : 0.6
                opacity: isSelected ? 1.0 : (hovered ? 0.9 : 0.5)

                Behavior on color { ColorAnimation { duration: 150 } }
                Behavior on border.color { ColorAnimation { duration: 150 } }
                Behavior on opacity { NumberAnimation { duration: 150 } }

                // ── toast 内容示意：图标点 + 文字线 ──
                Row {
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.left: parent.left
                    anchors.leftMargin: 6
                    spacing: 5

                    // 图标示意圆点
                    Rectangle {
                        width: 8; height: 8; radius: 4
                        anchors.verticalCenter: parent.verticalCenter
                        color: {
                            if (isSelected) return root.themePalette ? root.themePalette.onPrimary : "#fff"
                            return root.themePalette ? root.themePalette.textTertiary : "#888"
                        }
                        Behavior on color { ColorAnimation { duration: 150 } }
                    }

                    // 文字示意横线
                    Rectangle {
                        width: 34; height: 4; radius: 2
                        anchors.verticalCenter: parent.verticalCenter
                        color: {
                            if (isSelected) return root.themePalette ? root.themePalette.onPrimary : "#fff"
                            return root.themePalette ? root.themePalette.textTertiary : "#888"
                        }
                        opacity: isSelected ? 0.9 : 0.6
                        Behavior on color { ColorAnimation { duration: 150 } }
                    }
                }
            }

            // ── 悬停标签（仅在悬停且未选中时提示）──
            Text {
                visible: hovered && !isSelected
                text: pos === 0 ? "右下角" : (pos === 1 ? "左下角" : "顶部居中")
                font.pixelSize: 9
                font.family: "LXGW Neo XiHei Plus, Inter, sans-serif"
                color: root.themePalette ? root.themePalette.textSecondary : "#aaa"
                anchors.bottom: {
                    if (anchorTop) return undefined
                    return parent.top
                }
                anchors.top: anchorTop ? parent.bottom : undefined
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.margins: 2
            }
        }
    }
}
