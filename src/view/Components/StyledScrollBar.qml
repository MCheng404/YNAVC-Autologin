import QtQuick 2.15
import QtQuick.Controls 2.15

/**
 * 自定义滚动条（细线风格，绑定 ScrollView 的滚动状态）
 *
 * 特性：
 * - 极细轨道（4px）+ 圆形滑块
 * - 滑块颜色跟随主题 primary
 * - 空闲时半透明，hover/scroll 时正常显示
 */
ScrollBar {
    id: root

    // 主题色（由外部注入）
    property color accentColor: "#ec4899"

    // 最小/最大高度
    implicitWidth: 8
    implicitHeight: 40

    // 轨道（完全透明，不占用空间）
    background: Item {}

    // 滑块
    contentItem: Item {
        implicitWidth: root.implicitWidth
        implicitHeight: Math.max(36, root.availableHeight * root.size)

        Rectangle {
            id: thumbRect
            anchors.horizontalCenter: parent.horizontalCenter
            y: 0
            width: 5
            height: parent.height
            radius: 3
            color: root.accentColor
            opacity: root.active || root.pressed ? 0.7 : 0.3

            Behavior on opacity {
                NumberAnimation { duration: 150; easing.type: Easing.OutQuad }
            }

            // hover 状态扩展
            states: State {
                name: "expanded"
                when: root.active || mouseArea.containsMouse
                PropertyChanges {
                    target: thumbRect
                    width: 7
                    opacity: 0.8
                }
            }

            transitions: Transition {
                NumberAnimation { properties: "width,opacity"; duration: 150; easing.type: Easing.OutQuad }
            }
        }

        MouseArea {
            id: mouseArea
            anchors.fill: parent
            hoverEnabled: true
            acceptedButtons: Qt.NoButton
        }
    }
}
