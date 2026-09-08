import QtQuick 2.15
import QtQuick.Layouts 1.15
import "../Components"

/**
 * 基本设置卡片（点击认证/自启/通知）
 *
 * 使用 GlassCard 高斯模糊玻璃背景，替换旧的多层 Rectangle 叠加。
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
            text: "基本功能"
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

        // 点击托盘图标立即认证
        SwitchToggle {
            Layout.fillWidth: true
            checked: settingsVM.clickToAuth
            title: "单击托盘图标立即认证"
            description: "点击系统托盘图标时重新登录"
            themeVM: root.themeVM

            onToggled: function(checked) {
                settingsVM.clickToAuth = checked
            }
        }

        // 开机自动启动
        SwitchToggle {
            Layout.fillWidth: true
            checked: settingsVM.autostart
            title: "开机自动启动"
            description: "登录系统后自动后台运行"
            themeVM: root.themeVM

            onToggled: function(checked) {
                settingsVM.autostart = checked
            }
        }

        // 系统通知提醒
        SwitchToggle {
            Layout.fillWidth: true
            checked: settingsVM.notifications
            title: "系统通知提醒"
            description: "登录/登出/错误时弹出通知"
            themeVM: root.themeVM

            onToggled: function(checked) {
                settingsVM.notifications = checked
            }
        }

        // 通知显示位置
        ColumnLayout {
            Layout.fillWidth: true
            spacing: 6

            Text {
                text: "通知位置"
                font.pixelSize: 12
                font.capitalization: Font.AllUppercase
                font.letterSpacing: 0.5
                font.family: "LXGW Neo XiHei Plus, Inter, sans-serif"
                renderType: Text.NativeRendering
                font.hintingPreference: Font.PreferFullHinting
                color: themeVM.palette.textSecondary
                Layout.leftMargin: 4
            }

            ToastPositionSelector {
                position: settingsVM.toastPosition
                isEnabled: settingsVM.notifications
                themeVM: root.themeVM

                onPositionChanged: {
                    settingsVM.toastPosition = position
                }
            }
        }
    }
}
