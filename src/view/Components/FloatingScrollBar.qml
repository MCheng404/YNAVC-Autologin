import QtQuick 2.15

/**
 * 自定义浮动滚动条
 * 完全替代 Qt 原生滚动条，避免样式冲突
 */
Item {
    id: scrollBarRoot

    required property var themeVM
    property alias flickable: scrollBarRoot._flickable
    property int thickness: 8
    property int handleRadius: 4
    property color trackColor: themeVM.isDark ? "#1a1a2e" : "#e8e8e8"
    property color handleColor: themeVM.isDark ? "#4a4a6a" : "#b0b0b0"
    property color handleHoverColor: themeVM.palette.primary
    property real padding: 0
    property Flickable _flickable: null

    // 显隐状态：滚动中或悬停内容区时淡入，空闲淡出
    property bool _idle: true
    property bool _hovering: false
    opacity: (scrollBarRoot.visible && (_hovering || !_idle)) ? 1 : 0
    Behavior on opacity {
        NumberAnimation { duration: 160; easing.type: Easing.OutQuad }
    }

    // 统一的高度计算（ratio = 可视比例，handle 越小内容越多）
    property real hRatio: _flickable ? (_flickable.height / Math.max(1, _flickable.contentHeight)) : 1
    property real handleHH: Math.max(36, Math.min(96, track.height * hRatio))

    visible: _flickable && _flickable.contentHeight > _flickable.height
    anchors.right: parent.right
    anchors.top: parent.top
    anchors.bottom: parent.bottom
    anchors.rightMargin: 8  // 滚动条距容器右边缘间距
    width: 32

    // 滚动条轨道（居中对齐）
    Rectangle {
        id: track
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.horizontalCenter: parent.horizontalCenter
        width: thickness
        radius: thickness / 2
        color: trackColor

        Behavior on color {
            ColorAnimation { duration: 150; easing.type: Easing.OutQuad }
        }

        // 轨道悬停探测（不抢点击，仅用于显隐）
        MouseArea {
            id: trackMouseArea
            anchors.fill: parent
            hoverEnabled: true
            acceptedButtons: Qt.NoButton
            onContainsMouseChanged: {
                if (containsMouse) {
                    _hovering = true
                    _idle = false
                    _hideTimer.stop()
                } else {
                    _hovering = false
                    _hideTimer.restart()
                }
            }
        }
    }

    // 滚动条滑块容器（负责 y 定位和拖拽）
    Item {
        id: handleContainer
        width: thickness
        height: scrollBarRoot.handleHH
        anchors.horizontalCenter: parent.horizontalCenter
        // posRatio 钳制在 [0, 1] 防止越界
        property real posRatio: _flickable ? Math.max(0, Math.min(1,
            _flickable.contentY / Math.max(1, _flickable.contentHeight - _flickable.height)
        )) : 0
        y: track.y + (track.height - height) * posRatio

        // 滑块外发光（悬停时主题色光晕）
        Rectangle {
            anchors.centerIn: handle
            width: handle.width * handleScale.xScale + 8
            height: handle.height + 8
            radius: handleRadius + 4
            color: themeVM.palette.primary
            opacity: handleMouseArea.containsMouse ? 0.3 : 0
            visible: opacity > 0 || handleMouseArea.containsMouse

            Behavior on opacity {
                NumberAnimation { duration: 150; easing.type: Easing.OutQuad }
            }
        }

        // 滑块本体
        Rectangle {
            id: handle
            anchors.centerIn: parent
            width: scrollBarRoot.thickness
            height: parent.height
            radius: handleRadius
            color: handleMouseArea.containsMouse || handleMouseArea.pressed ? handleHoverColor : handleColor
            transform: Scale {
                id: handleScale
                origin.x: handle.width / 2
                origin.y: handle.height / 2
                xScale: (handleMouseArea.containsMouse || handleMouseArea.pressed) ? (scrollBarRoot.thickness + 3) / scrollBarRoot.thickness : 1
                Behavior on xScale {
                    NumberAnimation { duration: 150; easing.type: Easing.OutQuad }
                }
            }

            Behavior on color {
                ColorAnimation { duration: 150; easing.type: Easing.OutQuad }
            }
        }

        // 鼠标拖拽（拖动容器本身）
        MouseArea {
            id: handleMouseArea
            anchors.fill: parent
            hoverEnabled: true
            drag.target: parent
            drag.axis: Drag.YAxis
            drag.minimumY: track.y
            drag.maximumY: track.y + track.height - handleContainer.height

            onContainsMouseChanged: {
                if (containsMouse) {
                    _hovering = true
                    _idle = false
                    _hideTimer.stop()
                } else {
                    _hovering = false
                    _hideTimer.restart()
                }
            }

            onMouseYChanged: {
                if (pressed && _flickable) {
                    var ratio = (handleContainer.y - track.y) / Math.max(1, track.height - handleContainer.height)
                    _flickable.contentY = ratio * (_flickable.contentHeight - _flickable.height)
                }
            }
        }
    }

    // 监听 flickable 滚动，同步滑块位置
    onFlickableChanged: {
        if (_flickable) {
            _flickable.onContentYChanged.connect(function() {
                if (_flickable) {
                    // posRatio 钳制 [0, 1] 防止越界
                    var p = Math.max(0, Math.min(1,
                        _flickable.contentY / Math.max(1, _flickable.contentHeight - _flickable.height)
                    ))
                    handleContainer.y = track.y + (track.height - handleContainer.height) * p
                    // 滚动时保持可见，空闲后淡出
                    _idle = false
                    _hideTimer.restart()
                }
            })
        }
    }

    // 空闲淡出定时器
    Timer {
        id: _hideTimer
        interval: 1500
        onTriggered: _idle = true
    }
}
