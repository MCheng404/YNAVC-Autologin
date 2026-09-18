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

            // 设备列表
            ColumnLayout {
                Layout.fillWidth: true
                spacing: 0
                visible: root.showList

                Repeater {
                    id: devRepeater
                    // 故意保留 slice（最多 6）：避免为超出设备创建隐藏 delegate（visible:false 仍会构造对象/求值绑定），也更省一次跨边界读取
                    model: root.visibleDevices

                    // delegate 根对象：所有 onXxxChanged 必须挂在这里（与属性定义同源）
                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 0

                        // 行间分隔线（首行不显示）
                        Rectangle {
                            Layout.fillWidth: true
                            Layout.leftMargin: 0
                            Layout.rightMargin: 0
                            height: 1
                            color: themeVM.palette.divider
                            visible: index > 0
                            Layout.bottomMargin: 8
                        }

                        // 单行容器：发光层 / 底色层声明在内容之前 → 自然绘于内容下方，不用 z（避免打断批处理）
                        Item {
                            Layout.fillWidth: true
                            Layout.topMargin: 8
                            Layout.bottomMargin: 8
                            implicitHeight: rowLayout.implicitHeight

                            // 本机发光层（玻璃描边式，仅 isSelf）：声明于内容之前 = 绘于下方
                            Rectangle {
                                anchors.fill: parent
                                anchors.margins: -4
                                radius: 12
                                color: "transparent"
                                border.color: themeVM.palette.primary
                                border.width: 1.5
                                opacity: 0.4
                                visible: modelData.isSelf === true
                            }

                            // 本机极淡底色层（仅 isSelf）
                            Rectangle {
                                anchors.fill: parent
                                radius: 8
                                color: Qt.rgba(themeVM.palette.primary.r, themeVM.palette.primary.g, themeVM.palette.primary.b, 0.06)
                                visible: modelData.isSelf === true
                            }

                            // 单行内容
                            RowLayout {
                                id: rowLayout
                                anchors.fill: parent
                                spacing: 10

                            // 左：终端类型 + 本机标签（轻量 Row 定位器，间距由 spacing 控制）
                            Row {
                                spacing: 6
                                Layout.alignment: Qt.AlignVCenter

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
                                    // 宽度按文字自适应
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

                            // 中：IP / MAC（等宽字体，轻量 Column 定位器）
                            Column {
                                spacing: 2
                                Layout.alignment: Qt.AlignVCenter

                                Text {
                                    text: modelData.ip || ""
                                    font.pixelSize: 13
                                    font.family: "JetBrains Mono, LXGW Neo XiHei Plus, monospace"
                                    color: themeVM.palette.textPrimary
                                }
                                Text {
                                    text: modelData.mac || ""
                                    font.pixelSize: 13
                                    font.family: "JetBrains Mono, LXGW Neo XiHei Plus, monospace"
                                    color: themeVM.palette.textSecondary
                                }
                            }

                            Item { Layout.fillWidth: true }

                            // 右：使用时长
                            Text {
                                text: root.fmtUseTime(modelData.useTime)
                                font.pixelSize: 13
                                font.family: "JetBrains Mono, LXGW Neo XiHei Plus, monospace"
                                color: themeVM.palette.textTertiary
                                Layout.alignment: Qt.AlignVCenter
                            }

                            // 最右：下线（本机不显示，避免误踢自己）
                            Rectangle {
                                id: kickBtn
                                visible: modelData.isSelf !== true
                                height: 26
                                radius: 6
                                color: kickMouse.containsMouse
                                      ? Qt.rgba(themeVM.palette.error.r, themeVM.palette.error.g, themeVM.palette.error.b, 0.12)
                                      : "transparent"
                                width: kickTxt.width + 14
                                Layout.alignment: Qt.AlignVCenter

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
