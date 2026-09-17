import QtQuick 2.15
import QtQuick.Layouts 1.15
import "../Components"

/**
 * 通知位置选择器（屏幕位置图示风格）
 * 一个迷你屏幕预览框，toast 出现在哪个位置就在对应位置渲染 toast 示意块
 * position: 0=右下角, 1=左下角, 2=顶部居中
 *
 * 视觉优化（保持「屏幕位置图示」核心交互不变）：
 * - 顶部加极简标题条、底部加 dock 暗示，增强「这是屏幕」辨识度（纯色，无渐变）
 * - toast 示意块改为「左侧类型色条 + 主/副标题两线」，比例更接近真实通知卡片
 * - 增加纯色半透明阴影叠加，营造浮起感（无渐变）
 * - 未选中占位透明度由 0.5 提升到 0.7，提升可辨识度
 * - 选中切换给示意块加 Easing.OutBack 轻回弹（约 200ms）
 *
 * 进场动画（核心）：不同位置进场方向不同，与真实 ToastWindow 一致
 * - 位置 0/1（右下 / 左下）：从下方滑入（y +24 → 0）
 * - 位置 2（顶部居中）：从上方滑入（y −24 → 0）
 * 选中槽内 toastChip 播放：opacity 0→1、scale 0.92→1（OutBack）、y 从 ±24 滑到 0（OutCubic）
 * 选中后每 2.8s 自动循环重播一次（Timer 仅在「该槽被选中 且 组件可见」时 running）
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
    clip: true

    // ── 屏幕预览框 ──
    radius: 10
    color: themePalette ? (isEnabled ? themePalette.hoverBackground : themePalette.surfaceVariant) : "#2a2a2a"
    border.color: themePalette ? themePalette.outline : "#555"
    border.width: 0.6
    opacity: isEnabled ? 1.0 : 0.55

    Behavior on opacity { NumberAnimation { duration: 150 } }

    // ── 屏幕顶部标题条（纯色）──
    Rectangle {
        id: titleBar
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        height: 12
        radius: 6
        color: themePalette ? themePalette.surfaceVariant : "#333"
    }

    // ── 屏幕底部 dock 暗示（居中短条 + 应用点，纯色）──
    Rectangle {
        id: dock
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
        width: 26
        height: 8
        radius: 4
        color: themePalette ? themePalette.surfaceVariant : "#333"

        Row {
            anchors.centerIn: parent
            spacing: 3
            Repeater {
                model: 3
                Rectangle {
                    width: 3; height: 3; radius: 1.5
                    color: themePalette ? themePalette.outline : "#666"
                }
            }
        }
    }

    // ── 桌面/显示器符号（弱化，作背景）──
    LucideIcon {
        anchors.centerIn: parent
        name: "monitor"
        size: 22
        color: themePalette ? themePalette.outline : "#555"
        opacity: 0.35
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

            // 槽位锚定（屏幕内边距 8；顶部槽下移到标题条之下）
            anchors.right:      anchorRight  ? parent.right  : undefined
            anchors.bottom:     anchorBottom ? parent.bottom : undefined
            anchors.horizontalCenter: anchorCenterH ? parent.horizontalCenter : undefined
            anchors.top:        anchorTop    ? parent.top    : undefined
            anchors.margins: 8
            anchors.topMargin: anchorTop ? 16 : 8

            readonly property bool isSelected: root.position === pos
            // 静止时 y 偏移量：位置 2（顶部）从上方 −24 滑入，其余从下方 +24 滑入
            readonly property int enterY: pos === 2 ? -24 : 24
            property bool hovered: false

            // 播放选中槽位的进场动画演示（方向随位置变化）
            function playEntrance() {
                toastChip.scale = 0.92
                toastChip.opacity = 0
                toastChip.y = 3 + enterY
                toastBounce.restart()   // scale 轻回弹（OutBack）
                toastEnter.restart()    // opacity + y 方向滑入（OutCubic）
            }

            // 监听必须挂在属性所属对象（slot）上，避免 "Cannot assign to non-existent property"
            onIsSelectedChanged: {
                if (isSelected) playEntrance()
                else {
                    toastChip.opacity = hovered ? 0.95 : 0.7
                    toastChip.scale = 1.0
                    toastChip.y = 3
                }
            }
            onHoveredChanged: {
                if (!isSelected) toastChip.opacity = hovered ? 0.95 : 0.7
            }
            Component.onCompleted: {
                if (isSelected) playEntrance()
            }

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

            // ── 纯色半透明阴影（浮起感，无渐变）── 需置于 toastChip 之前以便位于其下
            Rectangle {
                id: toastShadow
                width: toastChip.width
                height: toastChip.height
                radius: 6
                x: toastChip.x + 1.5
                y: toastChip.y + 1.5
                color: "#000000"
                opacity: isSelected ? 0.22 : 0.10
                Behavior on opacity { NumberAnimation { duration: 150 } }
            }

            // ── toast 示意块 ──
            Rectangle {
                id: toastChip
                width: parent.width
                height: 28
                y: 3
                radius: 6

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
                opacity: 0.7

                Behavior on color { ColorAnimation { duration: 150 } }
                Behavior on border.color { ColorAnimation { duration: 150 } }

                // 选中切换轻回弹（先放大再回落，约 200ms）—— 保留
                SequentialAnimation {
                    id: toastBounce
                    NumberAnimation { target: toastChip; property: "scale"; to: 1.06; duration: 90;  easing.type: Easing.OutBack }
                    NumberAnimation { target: toastChip; property: "scale"; to: 1.0;  duration: 110; easing.type: Easing.OutBack }
                }
                // 进场方向滑入 + 淡入（与 ToastWindow 一致；约 320ms，OutCubic）
                ParallelAnimation {
                    id: toastEnter
                    NumberAnimation { target: toastChip; property: "opacity"; from: 0; to: 1.0; duration: 320; easing.type: Easing.OutCubic }
                    NumberAnimation { target: toastChip; property: "y";      to: 3;       duration: 320; easing.type: Easing.OutCubic }
                }

                // ── toast 内容：左侧类型色条 + 主/副标题（更接近真实通知卡片）──
                // 左侧类型色条
                Rectangle {
                    id: accentBar
                    anchors.left: parent.left
                    anchors.verticalCenter: parent.verticalCenter
                    width: 3
                    height: parent.height - 10
                    radius: 1.5
                    color: isSelected
                           ? (root.themePalette ? root.themePalette.success : "#3fb950")
                           : (root.themePalette ? root.themePalette.outline : "#555")
                    opacity: isSelected ? 0.9 : 0.4
                    Behavior on color { ColorAnimation { duration: 150 } }
                }

                // 主标题 + 副标题
                Column {
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.left: parent.left
                    anchors.leftMargin: 9
                    spacing: 3
                    Rectangle {
                        width: 30; height: 4; radius: 2
                        color: isSelected
                               ? (root.themePalette ? root.themePalette.onPrimary : "#fff")
                               : (root.themePalette ? root.themePalette.textTertiary : "#888")
                        opacity: isSelected ? 0.95 : 0.6
                        Behavior on color { ColorAnimation { duration: 150 } }
                    }
                    Rectangle {
                        width: 18; height: 3; radius: 1.5
                        color: isSelected
                               ? (root.themePalette ? root.themePalette.onPrimary : "#fff")
                               : (root.themePalette ? root.themePalette.textTertiary : "#888")
                        opacity: isSelected ? 0.65 : 0.45
                        Behavior on color { ColorAnimation { duration: 150 } }
                    }
                }
            }

            // ── 悬停标签（仅在悬停且未选中时提示）──
            Text {
                visible: hovered && !isSelected
                text: pos === 0 ? "右下角" : (pos === 1 ? "左下角" : "顶部居中")
                font.pixelSize: 11
                font.family: "LXGW Neo XiHei Plus, Inter, sans-serif"
                color: root.themePalette ? root.themePalette.textSecondary : "#aaa"
                anchors.bottom: anchorTop ? undefined : parent.top
                anchors.top: anchorTop ? parent.bottom : undefined
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.margins: 2
            }

            // ── 循环演示：选中且可见时每 2.8s 重播一次进场动画 ──
            Timer {
                id: loopTimer
                interval: 2800
                repeat: true
                running: slot.isSelected && root.isEnabled && root.visible
                onTriggered: playEntrance()
            }
        }
    }
}
