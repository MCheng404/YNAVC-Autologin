import QtQuick 2.15
import QtQuick.Layouts 1.15

/**
 * GlassCard — 纯色毛玻璃卡片（无渐变、无透明混色）
 * 直接用主题色，不透出窗口背景。
 */
Item {
    id: root

    // 悬停时极轻回弹放大（幅度极小，避免过度花哨）
    scale: 1.0
    Behavior on scale {
        NumberAnimation { duration: 200; easing.type: Easing.OutBack }
    }

    // ── 层 1：纯色底（完全不透明）──
    Rectangle {
        anchors.fill: parent
        radius: 16
        color: themeVM.palette.cardBackground

        Behavior on color {
            ColorAnimation { duration: 200; easing.type: Easing.OutCubic }
        }
    }

    // ── 层 1.5：主题色发光阴影（持久，低透明）──
    // 声明于层 1 之后、层 2 之前，自然绘于底之上、边框之下，无需 z
    Rectangle {
        anchors.fill: parent
        anchors.margins: -4
        radius: 20
        color: "transparent"
        border.color: themeVM.palette.primary
        border.width: 3
        opacity: 0.08

        Behavior on border.color {
            ColorAnimation { duration: 200; easing.type: Easing.OutCubic }
        }
    }

    // ── 层 2：边框 ──
    Rectangle {
        anchors.fill: parent
        radius: 16
        color: "transparent"
        border.color: themeVM.palette.cardBorder
        border.width: 0.6

        Behavior on border.color {
            ColorAnimation { duration: 200; easing.type: Easing.OutCubic }
        }
    }

    // ── 层 3：悬停描边（常态剔除，悬停才参与绘制）──
    Rectangle {
        id: hoverGlow
        anchors.fill: parent
        radius: 16
        color: "transparent"
        border.color: themeVM.palette.primary
        border.width: 1.5
        opacity: 0
        visible: opacity > 0 || cardMouse.containsMouse

        Behavior on opacity {
            NumberAnimation { duration: 150; easing.type: Easing.OutQuad }
        }
    }

    // ── 层 4：悬停外发光（常态剔除，悬停才参与绘制）──
    Rectangle {
        id: outerGlow
        anchors.fill: parent
        anchors.margins: -6
        radius: 22
        color: "transparent"
        border.color: themeVM.palette.primary
        border.width: 1
        opacity: 0
        visible: opacity > 0 || cardMouse.containsMouse

        Behavior on opacity {
            NumberAnimation { duration: 150; easing.type: Easing.OutQuad }
        }
    }

    MouseArea {
        id: cardMouse
        anchors.fill: parent
        hoverEnabled: true
        propagateComposedEvents: true

        onContainsMouseChanged: {
            hoverGlow.opacity = containsMouse ? 0.65 : 0
            outerGlow.opacity = containsMouse ? 0.25 : 0
            root.scale = containsMouse ? 1.012 : 1.0
        }
    }
}
