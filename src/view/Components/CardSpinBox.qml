import QtQuick 2.15
import QtQuick.Layouts 1.15
import "."

/**
 * 卡片内 SpinBox（居中数字 + 单位标签）
 *
 * required property:
 * - value: 当前值
 * - from: 最小值
 * - to: 最大值
 * - unit: 单位文字
 * - label: 标签文字
 * - enabled: 是否启用
 * - themeVM: 主题 ViewModel
 *
 * signal:
 * - valueChanged(int value)
 *
 * 动画约定：
 * - 按钮按压：scale 1.0 -> 0.92（onPressed），松开/release 取消回到 1.0，Easing.OutBack
 * - 数值变化：scale 1.0 -> 1.08 -> 1.0 轻回弹（onValueChanged 触发）
 * - 长按连续：onPressed 启动 Timer（首延迟 400ms，之后每 80ms 步进），松开停止；
 *   长按一旦触发置 _xxxLongPress 标志，onClicked 中据此跳过单次步进，避免重复跳。
 */
Rectangle {
    id: root

    required property int value
    required property int from
    required property int to
    required property string unit
    required property string label
    required property var themeVM

    height: 84
    radius: 10
    color: themeVM.palette.inputBackground
    border.color: root.enabled ? themeVM.palette.inputBorder : "transparent"
    border.width: 1
    opacity: root.enabled ? 1.0 : 0.55

    Behavior on opacity {
        NumberAnimation { duration: 150; easing.type: Easing.OutQuad }
    }
    Behavior on color {
        ColorAnimation { duration: 200 }
    }
    Behavior on border.color {
        ColorAnimation { duration: 200 }
    }

    // ── 长按连续触发标志（减/加各一）──
    property bool _minusLongPress: false
    property bool _plusLongPress: false
    property bool _valueInit: false

    // 数值变化弹跳（1.0 -> 1.08 -> 1.0）
    SequentialAnimation {
        id: valueBounce
        NumberAnimation { target: valueText; property: "scale"; to: 1.08; duration: 80;  easing.type: Easing.OutBack }
        NumberAnimation { target: valueText; property: "scale"; to: 1.0;  duration: 80;  easing.type: Easing.OutBack }
    }

    // value 改变即弹一下（首帧初始化不弹）
    onValueChanged: {
        if (_valueInit) valueBounce.restart()
        _valueInit = true
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 10
        spacing: 4

        // 标签
        Text {
            text: root.label
            font.pixelSize: 12
            font.capitalization: Font.AllUppercase
            font.letterSpacing: 0.5
            font.family: "LXGW Neo XiHei Plus, Inter, sans-serif"
            renderType: Text.NativeRendering
            font.hintingPreference: Font.PreferFullHinting
            color: themeVM.palette.textTertiary
        }

        // 数值和按钮
        RowLayout {
            Layout.fillWidth: true
            spacing: 4

            // 减少按钮
            Rectangle {
                id: minusButton
                width: 28
                height: 28
                radius: 6
                color: minusMouseArea.containsMouse ? themeVM.palette.hoverBackground : "transparent"

                Behavior on color {
                    ColorAnimation { duration: 150; easing.type: Easing.OutQuad }
                }
                Behavior on scale {
                    NumberAnimation { duration: 120; easing.type: Easing.OutBack }
                }

                LucideIcon {
                    anchors.centerIn: parent
                    name: "minus"
                    size: 16
                    color: themeVM.palette.onSurface
                }

                MouseArea {
                    id: minusMouseArea
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: root.enabled ? Qt.PointingHandCursor : Qt.ArrowCursor
                    onClicked: {
                        if (!root.enabled) return
                        // 长按已连续步进过，松开时跳过这次单击，避免多跳一次
                        if (root._minusLongPress) { root._minusLongPress = false; return }
                        if (root.value > root.from) root.value = root.value - 1
                    }
                    onPressed: {
                        if (!root.enabled) return
                        root._minusLongPress = false
                        minusButton.scale = 0.96
                        minusTimer.interval = 400
                        minusTimer.repeat = false
                        minusTimer.restart()
                    }
                    onReleased: {
                        minusTimer.stop()
                        minusButton.scale = 1.0
                    }
                    onCanceled: {
                        minusTimer.stop()
                        minusButton.scale = 1.0
                    }
                }
            }

            // 数值显示
            Text {
                id: valueText
                Layout.fillWidth: true
                horizontalAlignment: Text.AlignHCenter
                text: root.value.toString()
                font.pixelSize: 24
                font.weight: Font.DemiBold
                font.family: "JetBrains Mono, LXGW Neo XiHei Plus, monospace"
                renderType: Text.NativeRendering
                font.hintingPreference: Font.PreferFullHinting
                color: themeVM.palette.textPrimary
            }

            // 增加按钮
            Rectangle {
                id: plusButton
                width: 28
                height: 28
                radius: 6
                color: plusMouseArea.containsMouse ? themeVM.palette.hoverBackground : "transparent"

                Behavior on color {
                    ColorAnimation { duration: 150; easing.type: Easing.OutQuad }
                }
                Behavior on scale {
                    NumberAnimation { duration: 120; easing.type: Easing.OutBack }
                }

                LucideIcon {
                    anchors.centerIn: parent
                    name: "plus"
                    size: 16
                    color: themeVM.palette.onSurface
                }

                MouseArea {
                    id: plusMouseArea
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: root.enabled ? Qt.PointingHandCursor : Qt.ArrowCursor
                    onClicked: {
                        if (!root.enabled) return
                        if (root._plusLongPress) { root._plusLongPress = false; return }
                        if (root.value < root.to) root.value = root.value + 1
                    }
                    onPressed: {
                        if (!root.enabled) return
                        root._plusLongPress = false
                        plusButton.scale = 0.96
                        plusTimer.interval = 400
                        plusTimer.repeat = false
                        plusTimer.restart()
                    }
                    onReleased: {
                        plusTimer.stop()
                        plusButton.scale = 1.0
                    }
                    onCanceled: {
                        plusTimer.stop()
                        plusButton.scale = 1.0
                    }
                }
            }
        }

        // 单位文字
        Text {
            text: root.unit
            font.pixelSize: 13
            font.family: "LXGW Neo XiHei Plus, Inter, sans-serif"
            renderType: Text.NativeRendering
            font.hintingPreference: Font.PreferFullHinting
            color: themeVM.palette.textTertiary
        }
    }

    // ── 长按连续触发定时器（首延迟 400ms，之后每 80ms 步进）──
    Timer {
        id: minusTimer
        onTriggered: {
            root._minusLongPress = true
            if (root.value > root.from) root.value = root.value - 1
            interval = 80
            repeat = true
            restart()
        }
    }
    Timer {
        id: plusTimer
        onTriggered: {
            root._plusLongPress = true
            if (root.value < root.to) root.value = root.value + 1
            interval = 80
            repeat = true
            restart()
        }
    }
}
