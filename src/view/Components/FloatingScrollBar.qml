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
            width: handle.width + 8
            height: handle.height + 8
            radius: handleRadius + 4
            color: themeVM.palette.primary
            opacity: handleMouseArea.containsMouse ? 0.3 : 0
            z: -1

            Behavior on opacity {
                NumberAnimation { duration: 200; easing.type: Easing.OutQuad }
            }
        }

        // 滑块本体
        Rectangle {
            id: handle
            anchors.fill: parent
            radius: handleRadius
            color: handleMouseArea.containsMouse || handleMouseArea.pressed ? handleHoverColor : handleColor

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
                }
            })
        }
    }
}
