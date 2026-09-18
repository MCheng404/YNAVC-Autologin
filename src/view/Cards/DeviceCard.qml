import QtQuick 2.15
import QtQuick.Layouts 1.15
import "../Components"

/**
 * 在线设备卡片
 *
 * 使用 GlassCard 纯色毛玻璃背景（无渐变、无透明混色）。
 * 数据源：deviceVM（C++ 注入的上下文属性）
 *   devices       QVariantList，每项含 sessionId/ip/mac/terminalType/loginTime/useTime/isSelf
 *   loading       是否刷新中
 *   errorMessage  错误信息（非空时显示）
 *   account       已脱敏账号
 *   refresh() / kick(id) / kickAllExceptSelf()
 */
GlassCard {
    id: root

    required property var deviceVM
    required property var themeVM

    implicitHeight: col.implicitHeight + 32

    // 卡片首次创建（= 首次打开设置窗口）时静默获取一次在线设备，
    // 用户无需手动点「刷新」。后续每次打开设置由 Main.qml 调
    // refreshIfStale() 兜底（带节流）。
    Component.onCompleted: {
        if (deviceVM) deviceVM.refresh()
    }

    // ── 辅助函数 ──

    // 去掉终端类型前缀的 '#'
    function cleanType(t) {
        if (!t) return ""
        return ("" + t).replace(/^#/, "")
    }

    // 将分钟数转成中文可读时长
    function fmtUseTime(min) {
        min = Math.floor(Number(min))
        if (!min || min <= 0) return "0 分钟"
        if (min < 60) return min + " 分钟"
        var h = Math.floor(min / 60)
        var m = min % 60
        return m > 0 ? (h + " 小时 " + m + " 分") : (h + " 小时")
    }

    // MAC 显示格式：去掉 ':' 与 '-' 分隔符。
    function fmtMac(m) {
        if (!m) return ""
        return ("" + m).replace(/[:-]/g, "").toUpperCase()
    }

    // MAC 短格式：只取后 6 位（无分隔符、大写）。
    // 设置窗口左列卡片可用宽度实测仅约 233px（w = parentW = 233），
    // 而「完整 MAC(≈96px) + IP(≈96px) + 下线按钮(≈55px) + 间距」≈ 270px 必然超宽。
    // MAC 的后 3 字节本身即设备唯一标识，后 6 位足够辨识，故列表里用短格式；
    // 完整格式仍保留在 fmtMac() 中，需要时可随时切回。
    function fmtMacShort(m) {
        if (!m) return ""
        var s = ("" + m).replace(/[:-]/g, "").toUpperCase()
        return s.length > 6 ? s.slice(-6) : s
    }

    // 设备总数：缓存一次跨边界读取，供 overflowCount / showList 复用，避免重复访问 deviceVM.devices
    property int deviceCount: deviceVM.devices ? deviceVM.devices.length : 0

    // 可见设备（本机置顶，其余保持原序，最多 6 行，超出截断并显示"还有 N 台…"）
    property var visibleDevices: {
        var all = deviceVM.devices || []
        var self = [], others = []
        for (var i = 0; i < all.length; i++) {
            if (all[i] && all[i].isSelf === true) self.push(all[i])
            else others.push(all[i])
        }
        return self.concat(others).slice(0, 6)
    }
    property int overflowCount: Math.max(0, deviceCount - 6)

    property bool showList: !deviceVM.loading && deviceCount > 0

    ColumnLayout {
        id: col
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.margins: 16
        spacing: 12

        // ── 卡片标题 + 脱敏账号 ──
        RowLayout {
            Layout.fillWidth: true
            Layout.leftMargin: 4
            spacing: 8

            ShadowText {
                text: "在线设备"
                font.pixelSize: 12
                font.weight: Font.DemiBold
                font.capitalization: Font.AllUppercase
                font.letterSpacing: 1.2
                font.family: "LXGW Neo XiHei Plus, Inter, sans-serif"
                color: themeVM.palette.textTertiary
            }

            Item { Layout.fillWidth: true }

            Text {
                text: deviceVM.account || ""
                font.pixelSize: 13
                font.family: "LXGW Neo XiHei Plus, Inter, sans-serif"
                color: themeVM.palette.textTertiary
                elide: Text.ElideRight
                Layout.maximumWidth: 160
            }
        }

        // ── 操作行：刷新 / 全部下线 ──
        RowLayout {
            Layout.fillWidth: true
            Layout.leftMargin: 4
            spacing: 8

            // 刷新（普通按钮）
            Rectangle {
                id: refreshBtn
                height: 34
                radius: 8
                Layout.preferredWidth: 84
                color: refreshMouse.containsMouse ? themeVM.palette.hoverBackground : themeVM.palette.surfaceVariant
                border.color: themeVM.palette.outline
                border.width: 1
                scale: 1
                opacity: deviceVM.loading ? 0.55 : 1

                Behavior on color { ColorAnimation { duration: 150; easing.type: Easing.OutQuad } }
                Behavior on border.color { ColorAnimation { duration: 150; easing.type: Easing.OutQuad } }
                Behavior on opacity { NumberAnimation { duration: 150; easing.type: Easing.OutQuad } }
                Behavior on scale { NumberAnimation { duration: 140; easing.type: Easing.OutBack } }

                Text {
                    anchors.centerIn: parent
                    text: "刷新"
                    font.pixelSize: 14
                    font.weight: Font.Medium
                    font.family: "LXGW Neo XiHei Plus, Inter, sans-serif"
                    color: themeVM.palette.textPrimary
                }

                MouseArea {
                    id: refreshMouse
                    anchors.fill: parent
                    hoverEnabled: true
                    enabled: !deviceVM.loading
                    onClicked: deviceVM.refresh()
                    onPressed: refreshBtn.scale = 0.96
                    onReleased: refreshBtn.scale = 1
                    onCanceled: refreshBtn.scale = 1
                }
            }

            // 全部下线（危险色，克制）
            Rectangle {
                id: kickAllBtn
                height: 34
                radius: 8
                Layout.preferredWidth: 96
                color: kickAllMouse.containsMouse
                      ? Qt.rgba(themeVM.palette.error.r, themeVM.palette.error.g, themeVM.palette.error.b, 0.12)
                      : themeVM.palette.hoverBackground
                border.color: themeVM.palette.outline
                border.width: 1
                scale: 1
                opacity: deviceVM.loading ? 0.55 : 1

                Behavior on color { ColorAnimation { duration: 150; easing.type: Easing.OutQuad } }
                Behavior on opacity { NumberAnimation { duration: 150; easing.type: Easing.OutQuad } }
                Behavior on scale { NumberAnimation { duration: 140; easing.type: Easing.OutBack } }

                Text {
                    anchors.centerIn: parent
                    text: "全部下线"
                    font.pixelSize: 14
                    font.weight: Font.Medium
                    font.family: "LXGW Neo XiHei Plus, Inter, sans-serif"
                    color: themeVM.palette.error
                }

                MouseArea {
                    id: kickAllMouse
                    anchors.fill: parent
                    hoverEnabled: true
                    enabled: !deviceVM.loading
                    onClicked: deviceVM.kickAllExceptSelf()
                    onPressed: kickAllBtn.scale = 0.96
                    onReleased: kickAllBtn.scale = 1
                    onCanceled: kickAllBtn.scale = 1
                }
            }

            Item { Layout.fillWidth: true }
        }

        // ── 设备列表 / 占位 ──
        ColumnLayout {
            id: listArea
            Layout.fillWidth: true
            Layout.leftMargin: 4
            spacing: 0

            // 占位：读取中 / 暂无设备
            Text {
                Layout.fillWidth: true
                Layout.topMargin: 14
                Layout.bottomMargin: 14
                horizontalAlignment: Text.AlignHCenter
                text: deviceVM.loading ? "读取中…" : "暂无在线设备"
                font.pixelSize: 13
                font.family: "LXGW Neo XiHei Plus, Inter, sans-serif"
                color: themeVM.palette.textTertiary
                visible: !root.showList
            }

            // 设备列表（独立长方形卡片，竖向排列，卡片间距 8）
            Column {
                id: devColumn
                Layout.fillWidth: true
                Layout.leftMargin: 4
                spacing: 8
                visible: root.showList

                Repeater {
                    id: devRepeater
                    // 故意保留 slice（最多 6）：避免为超出设备创建隐藏 delegate（visible:false 仍会构造对象/求值绑定），也更省一次跨边界读取
                    model: root.visibleDevices

                    // delegate 根对象：Item + 显式 width 锁定卡片宽度 = 列表容器宽度
                    Item {
                        id: cardRoot
                        width: parent.width
                        height: 68

                            // 本机卡片：描边发光层（声明于内容之前 = 绘于下方，不用 z）
                            Rectangle {
                                anchors.fill: parent
                                anchors.margins: -2
                                radius: 10
                                color: "transparent"
                                border.color: themeVM.palette.primary
                                border.width: 1.5
                                opacity: 0.4
                                visible: modelData.isSelf === true
                            }

                            // 卡片底色
                            Rectangle {
                                anchors.fill: parent
                                radius: 8
                                color: themeVM.palette.surfaceVariant
                            }
                            // 本机淡底（叠加在 surfaceVariant 之上）
                            Rectangle {
                                anchors.fill: parent
                                radius: 8
                                color: Qt.rgba(themeVM.palette.primary.r, themeVM.palette.primary.g, themeVM.palette.primary.b, 0.06)
                                visible: modelData.isSelf === true
                            }

                            // 第一行：左 终端类型 + 本机标签；右 使用时长
                            Item {
                                id: line1
                                anchors.left: parent.left
                                anchors.right: parent.right
                                anchors.top: parent.top
                                anchors.topMargin: 10
                                height: 18

                                Row {
                                    anchors.left: parent.left
                                    anchors.verticalCenter: parent.verticalCenter
                                    spacing: 6

                                    Text {
                                        anchors.verticalCenter: parent.verticalCenter
                                        text: root.cleanType(modelData.terminalType)
                                        font.pixelSize: 13
                                        font.family: "LXGW Neo XiHei Plus, Inter, sans-serif"
                                        color: themeVM.palette.textSecondary
                                    }

                                    // 本机标签（primary 12% 透明底 + primary 文字）
                                    Rectangle {
                                        anchors.verticalCenter: parent.verticalCenter
                                        visible: modelData.isSelf === true
                                        height: 18
                                        radius: 5
                                        color: Qt.rgba(themeVM.palette.primary.r, themeVM.palette.primary.g, themeVM.palette.primary.b, 0.12)
                                        width: selfTagTxt.width + 12

                                        Text {
                                            id: selfTagTxt
                                            anchors.centerIn: parent
                                            text: "本机"
                                            font.pixelSize: 10
                                            font.family: "LXGW Neo XiHei Plus, Inter, sans-serif"
                                            color: themeVM.palette.primary
                                        }
                                    }
                                }

                                Text {
                                    anchors.right: parent.right
                                    anchors.verticalCenter: parent.verticalCenter
                                    text: root.fmtUseTime(modelData.useTime)
                                    font.pixelSize: 13
                                    font.family: "JetBrains Mono, LXGW Neo XiHei Plus, monospace"
                                    color: themeVM.palette.textTertiary
                                }
                            }

                            // 第二行：左 MAC；中右 IP（约束宽度 + elide 兜底）；右 下线按钮
                            Item {
                                id: line2
                                anchors.left: parent.left
                                anchors.right: parent.right
                                anchors.top: line1.bottom
                                anchors.topMargin: 4
                                height: 26

                                Text {
                                    id: macTxt
                                    anchors.left: parent.left
                                    anchors.verticalCenter: parent.verticalCenter
                                    text: root.fmtMacShort(modelData.mac)
                                    font.pixelSize: 13
                                    font.family: "JetBrains Mono, LXGW Neo XiHei Plus, monospace"
                                    color: themeVM.palette.textSecondary
                                }

                                Rectangle {
                                    id: kickBtn
                                    anchors.right: parent.right
                                    anchors.verticalCenter: parent.verticalCenter
                                    visible: modelData.isSelf !== true
                                    height: 26
                                    radius: 6
                                    color: kickMouse.containsMouse
                                          ? Qt.rgba(themeVM.palette.error.r, themeVM.palette.error.g, themeVM.palette.error.b, 0.12)
                                          : "transparent"
                                    width: kickTxt.width + 14

                                    Text {
                                        id: kickTxt
                                        anchors.centerIn: parent
                                        text: "下线"
                                        font.pixelSize: 13
                                        font.family: "LXGW Neo XiHei Plus, Inter, sans-serif"
                                        color: kickMouse.containsMouse ? themeVM.palette.error : themeVM.palette.textSecondary
                                    }

                                    Behavior on color { ColorAnimation { duration: 150; easing.type: Easing.OutQuad } }

                                    MouseArea {
                                        id: kickMouse
                                        anchors.fill: parent
                                        hoverEnabled: true
                                        onClicked: deviceVM.kick(modelData.sessionId)
                                    }
                                }

                                Text {
                                    id: ipTxt
                                    anchors.left: macTxt.right
                                    anchors.leftMargin: 10
                                    anchors.right: kickBtn.left
                                    anchors.rightMargin: 10
                                    anchors.verticalCenter: parent.verticalCenter
                                    text: modelData.ip || ""
                                    elide: Text.ElideRight
                                    font.pixelSize: 13
                                    font.family: "JetBrains Mono, LXGW Neo XiHei Plus, monospace"
                                    color: themeVM.palette.textPrimary
                                }
                            }
                }
            }

                // 超出 6 行提示
                Text {
                    Layout.fillWidth: true
                    Layout.topMargin: 8
                    horizontalAlignment: Text.AlignHCenter
                    text: "还有 " + root.overflowCount + " 台…"
                    font.pixelSize: 13
                    font.family: "LXGW Neo XiHei Plus, Inter, sans-serif"
                    color: themeVM.palette.textTertiary
                    visible: root.overflowCount > 0
                }
            }
        }

        // ── 错误态 ──
        Text {
            Layout.fillWidth: true
            Layout.leftMargin: 4
            text: deviceVM.errorMessage || ""
            font.pixelSize: 13
            font.family: "LXGW Neo XiHei Plus, Inter, sans-serif"
            color: themeVM.palette.error
            wrapMode: Text.Wrap
            visible: (deviceVM.errorMessage || "") !== ""
        }
    }
}
