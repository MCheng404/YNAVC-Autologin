import QtQuick 2.15
import QtQuick.Layouts 1.15
import "../Components"

/**
 * 账户信息卡片（用户名/密码）
 *
 * 使用 GlassCard 高斯模糊玻璃背景。
 * 注意：TextInput 无 placeholderText 属性，用 Text overlay 模拟。
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
            text: "账户信息"
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

        // 用户名输入
        ColumnLayout {
            Layout.fillWidth: true
            spacing: 6

            Text {
                text: "用户名"
                font.pixelSize: 11
                font.family: "LXGW Neo XiHei Plus, Inter, sans-serif"
                renderType: Text.NativeRendering
                font.hintingPreference: Font.PreferFullHinting
                font.capitalization: Font.AllUppercase
                font.letterSpacing: 0.5
                color: themeVM.palette.textSecondary
            }

            Rectangle {
                id: usernameBox
                Layout.fillWidth: true
                height: 40
                radius: 10
                color: themeVM.palette.inputBackground
                border.color: usernameInput.activeFocus ? themeVM.palette.primary : themeVM.palette.inputBorder
                border.width: usernameInput.activeFocus ? 2 : 1

                // 焦点外发光（极淡 primaryContainer，纯色无渐变）
                Rectangle {
                    anchors.fill: parent
                    anchors.margins: -3
                    radius: 13
                    color: themeVM.palette.primaryContainer
                    opacity: usernameInput.activeFocus ? 1.0 : 0
                    z: -1
                    Behavior on opacity {
                        NumberAnimation { duration: 150; easing.type: Easing.OutQuad }
                    }
                }

                Behavior on border.color {
                    ColorAnimation { duration: 150; easing.type: Easing.OutQuad }
                }
                Behavior on border.width {
                    NumberAnimation { duration: 150; easing.type: Easing.OutQuad }
                }
                Behavior on color {
                    ColorAnimation { duration: 150; easing.type: Easing.OutQuad }
                }

                TextInput {
                    id: usernameInput
                    anchors.fill: parent
                    anchors.margins: 10
                    verticalAlignment: Text.AlignVCenter
                    font.pixelSize: 13
                    font.family: "JetBrains Mono, LXGW Neo XiHei Plus, monospace"
                    renderType: Text.NativeRendering
                    font.hintingPreference: Font.PreferFullHinting
                    color: themeVM.palette.textPrimary
                    text: settingsVM.username
                    clip: true

                    onTextChanged: {
                        settingsVM.username = text
                    }
                }

                // Placeholder
                Text {
                    anchors.fill: parent
                    anchors.margins: 10
                    verticalAlignment: Text.AlignVCenter
                    font.pixelSize: 13
                    font.family: "JetBrains Mono, LXGW Neo XiHei Plus, monospace"
                    renderType: Text.NativeRendering
                    font.hintingPreference: Font.PreferFullHinting
                    color: themeVM.palette.textTertiary
                    text: "校园网账号（自动补@unicom）"
                    visible: usernameInput.text.length === 0 && !usernameInput.activeFocus
                    enabled: false
                    z: -1
                }
            }
        }

        // 密码输入
        ColumnLayout {
            Layout.fillWidth: true
            spacing: 6

            Text {
                text: "密码"
                font.pixelSize: 11
                font.family: "LXGW Neo XiHei Plus, Inter, sans-serif"
                renderType: Text.NativeRendering
                font.hintingPreference: Font.PreferFullHinting
                font.capitalization: Font.AllUppercase
                font.letterSpacing: 0.5
                color: themeVM.palette.textSecondary
            }

            Rectangle {
                id: passwordBox
                Layout.fillWidth: true
                height: 40
                radius: 10
                color: themeVM.palette.inputBackground
                border.color: passwordInput.activeFocus ? themeVM.palette.primary : themeVM.palette.inputBorder
                border.width: passwordInput.activeFocus ? 2 : 1

                // 焦点外发光（极淡 primaryContainer，纯色无渐变）
                Rectangle {
                    anchors.fill: parent
                    anchors.margins: -3
                    radius: 13
                    color: themeVM.palette.primaryContainer
                    opacity: passwordInput.activeFocus ? 1.0 : 0
                    z: -1
                    Behavior on opacity {
                        NumberAnimation { duration: 150; easing.type: Easing.OutQuad }
                    }
                }

                Behavior on border.color {
                    ColorAnimation { duration: 150; easing.type: Easing.OutQuad }
                }
                Behavior on border.width {
                    NumberAnimation { duration: 150; easing.type: Easing.OutQuad }
                }
                Behavior on color {
                    ColorAnimation { duration: 150; easing.type: Easing.OutQuad }
                }

                TextInput {
                    id: passwordInput
                    anchors.fill: parent
                    anchors.margins: 10
                    verticalAlignment: Text.AlignVCenter
                    font.pixelSize: 13
                    font.family: "JetBrains Mono, LXGW Neo XiHei Plus, monospace"
                    renderType: Text.NativeRendering
                    font.hintingPreference: Font.PreferFullHinting
                    color: themeVM.palette.textPrimary
                    text: settingsVM.password
                    echoMode: TextInput.Password
                    clip: true

                    onTextChanged: {
                        settingsVM.password = text
                    }
                }

                // Placeholder
                Text {
                    anchors.fill: parent
                    anchors.margins: 10
                    verticalAlignment: Text.AlignVCenter
                    font.pixelSize: 13
                    font.family: "JetBrains Mono, LXGW Neo XiHei Plus, monospace"
                    renderType: Text.NativeRendering
                    font.hintingPreference: Font.PreferFullHinting
                    color: themeVM.palette.textTertiary
                    text: "请输入校园网密码"
                    visible: passwordInput.text.length === 0 && !passwordInput.activeFocus
                    enabled: false
                    z: -1
                }
            }
        }
    }
}
