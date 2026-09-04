import QtQuick 2.15
import QtQuick.Layouts 1.15
import "../Components"

/**
 * 定时认证卡片（开关/间隔/偏移）
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
            text: "定时自动认证"
            font.pixelSize: 11
            font.weight: Font.DemiBold
            font.capitalization: Font.AllUppercase
            font.letterSpacing: 1.2
            font.family: "LXGW Neo XiHei Plus, Inter, sans-serif"
            renderType: Text.NativeRendering
            font.hintingPreference: Font.PreferFullHinting
            color: themeVM.palette.textTertiary
            Layout.leftMargin: 4
        }

        // 启用定时认证
        SwitchToggle {
            Layout.fillWidth: true
            checked: settingsVM.schedEnabled
            title: "启用定时认证"
            description: "每间隔一段时间自动重新登录，增强稳定性"
            themeVM: root.themeVM

            onToggled: function(checked) {
                settingsVM.schedEnabled = checked
            }
        }

        // 间隔设置
        RowLayout {
            Layout.fillWidth: true
            spacing: 10

            // 认证间隔
            CardSpinBox {
                Layout.fillWidth: true
                value: settingsVM.schedInterval
                from: 1
                to: 24
                unit: "小时"
                label: "认证间隔"
                enabled: settingsVM.schedEnabled
                themeVM: root.themeVM

                onValueChanged: {
                    settingsVM.schedInterval = value
                }
            }
        }
    }
}
