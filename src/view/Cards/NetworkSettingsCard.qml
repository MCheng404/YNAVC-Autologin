import QtQuick 2.15
import QtQuick.Layouts 1.15
import "../Components"

/**
 * 网络设置卡片（热点/优先级）
 *
 * 使用 GlassCard 高斯模糊玻璃背景。
 */
GlassCard {
    id: root

    required property var settingsVM
    required property var themeVM


    implicitHeight: col.implicitHeight + 32

    ColumnLayout {
        id: col
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.margins: 16
        spacing: 12

        // 卡片标题
        ShadowText {
            text: "网络设置"
            font.pixelSize: 12
            font.weight: Font.DemiBold
            font.capitalization: Font.AllUppercase
            font.letterSpacing: 1.2
            font.family: "LXGW Neo XiHei Plus, Inter, sans-serif"
            renderType: Text.NativeRendering
            font.hintingPreference: Font.PreferFullHinting
            color: themeVM.palette.textTertiary
            Layout.leftMargin: 4
        }

        // 认证后自动开热点
        SwitchToggle {
            Layout.fillWidth: true
            checked: settingsVM.autoHotspot
            title: "认证后自动开热点"
            description: "登录成功后自动开启 WLAN 热点"
            themeVM: root.themeVM

            onToggled: function(checked) {
                settingsVM.autoHotspot = checked
            }
        }

        // 提高线程优先级
        SwitchToggle {
            Layout.fillWidth: true
            checked: settingsVM.highPriority
            title: "提高线程优先级"
            description: "需要以管理员权限运行"
            themeVM: root.themeVM

            onToggled: function(checked) {
                settingsVM.highPriority = checked
            }
        }
    }
}
