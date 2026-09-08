import QtQuick 2.15

/**
 * 单个颜色圆点组件（缩小版）
 */
Item {
    id: dotRoot

    required property int colorIndex
    required property string accentColor
    required property var themeVM

    width: 22
    height: 22

    readonly property bool isSelected: themeVM.accentColorIndex === colorIndex

    // 按下回弹
    scale: 1.0
    Behavior on scale {
        NumberAnimation { duration: 140; easing.type: Easing.OutBack }
    }

    // 选中描边（带 scale 弹入）
    Rectangle {
        anchors.centerIn: colorCircle
        width: colorCircle.width + 6
        height: colorCircle.height + 6
        radius: (colorCircle.width + 6) / 2
        color: "transparent"
        border.color: accentColor
        border.width: 1.5
        opacity: isSelected ? 0.9 : 0
        scale: isSelected ? 1 : 0.85
        Behavior on opacity { NumberAnimation { duration: 200; easing.type: Easing.OutBack } }
        Behavior on scale { NumberAnimation { duration: 200; easing.type: Easing.OutBack } }
        z: -2
    }

    // 选中光晕（带 scale 弹入）
    Rectangle {
        anchors.centerIn: colorCircle
        width: colorCircle.width + 10
        height: colorCircle.height + 10
        radius: (colorCircle.width + 10) / 2
        color: "transparent"
        border.color: accentColor
        border.width: 1
        opacity: isSelected ? 0.5 : 0
        scale: isSelected ? 1 : 0.85
        Behavior on opacity { NumberAnimation { duration: 200; easing.type: Easing.OutBack } }
        Behavior on scale { NumberAnimation { duration: 200; easing.type: Easing.OutBack } }
        z: -1
    }

    // 颜色圆点
    Rectangle {
        id: colorCircle
        anchors.centerIn: parent
        width: 14
        height: 14
        radius: 7
        color: accentColor
        z: 1

        // 悬停扩散光晕
        Rectangle {
            anchors.fill: parent
            anchors.margins: -2
            radius: parent.radius + 2
            color: accentColor
            opacity: dotMouseArea.containsMouse ? 0.35 : 0
            Behavior on opacity { NumberAnimation { duration: 150; easing.type: Easing.OutQuad } }
            z: -1
        }
    }

    MouseArea {
        id: dotMouseArea
        anchors.fill: parent
        cursorShape: Qt.PointingHandCursor
        acceptedButtons: Qt.LeftButton
        onPressed: dotRoot.scale = 0.96
        onReleased: dotRoot.scale = 1.0
        onCanceled: dotRoot.scale = 1.0
        onClicked: {
            themeVM.accentColorIndex = colorIndex
        }
    }
}
