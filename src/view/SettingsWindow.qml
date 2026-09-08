import QtQuick 2.15
import QtQuick.Window 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls 2.15
import "Cards"
import "Components"

/**
 * 设置窗口：5 张高斯模糊玻璃卡片 + 主题色选择
 *
 * 所有颜色均通过 themeVM.palette 获取，支持亮暗主题切换
 * 主题模式：Dark / Light / System（跟随系统）
 *
 * 卡片使用 GlassCard 组件（ShaderEffect 高斯模糊 + 多层玻璃叠加）
 * 窗口背景使用 Liquid Glass 风格（噪点纹理 + 折射渐变 + 顶缘高光）
 */
Window {
    id: settingsWindow

    width: 680
    height: 720
    minimumWidth: 560
    minimumHeight: 560
    // 无边框窗口，自己实现拖拽和关闭按钮
    flags: Qt.Window | Qt.FramelessWindowHint | Qt.WindowStaysOnTopHint
    // 背景透明（DWM 圆角遮罩由 C++ DwmSetWindowAttribute 处理）
    // 透明背景让 shadow 溢出部分透出桌面
    color: "transparent"

    // ViewModel 绑定（由 Main.qml 注入）
    required property var authVM
    required property var settingsVM
    required property var themeVM
    required property var trayVM

    // 居中显示
    x: (Screen.width - width) / 2
    y: (Screen.height - height) / 2

    // 圆角遮罩由 Main.qml Loader.onLoaded 调用 app.applyWindowMask() 设置

    // ============================================================
    // 抗闪烁：resize 时暂停所有动画
    // ============================================================
    property bool isResizing: false

    onWidthChanged: {
        isResizing = true
        resizeTimer.restart()
    }
    onHeightChanged: {
        isResizing = true
        resizeTimer.restart()
    }

    Timer {
        id: resizeTimer
        interval: 200
        onTriggered: isResizing = false
    }

    // ============================================================
    // 窗口开合动画根容器
    // QQuickWindow 无 scale 属性，故对 contentRoot 做 scale + opacity
    // ============================================================
    Item {
        id: contentRoot
        anchors.fill: parent
        opacity: 0
        scale: 0.96

        // 打开：scale 0.96→1 + opacity 0→1，约 260ms OutBack
        ParallelAnimation {
            id: openAnim
            NumberAnimation { target: contentRoot; property: "scale"; from: 0.96; to: 1; duration: 260; easing.type: Easing.OutBack }
            NumberAnimation { target: contentRoot; property: "opacity"; from: 0; to: 1; duration: 260; easing.type: Easing.OutBack }
        }

        // 关闭：反向，约 200ms InCubic，动画播完才真正 hide()
        ParallelAnimation {
            id: closeAnim
            NumberAnimation { target: contentRoot; property: "scale"; from: 1; to: 0.96; duration: 200; easing.type: Easing.InCubic }
            NumberAnimation { target: contentRoot; property: "opacity"; from: 1; to: 0; duration: 200; easing.type: Easing.InCubic }
            onStopped: {
                isClosing = false
                settingsWindow.hide()
            }
        }

    // ============================================================
    // Liquid Glass 窗口主体
    // 结构（从底到顶）：
    //   1. 外层阴影（inset shadow 效果）
    //   2. 玻璃渐变底（半透明 + 蓝紫折射）
    //   3. 噪点纹理层（模拟毛玻璃颗粒感）
    //   4. 顶部光泽条（边缘高光）
    //   5. 玻璃边框（微发光）
    // ============================================================
    Item {
        id: windowBg
        anchors.fill: parent
        clip: false

        // ── 1. 外层阴影 ──
        Rectangle {
            anchors.fill: parent
            anchors.margins: -6
            radius: 14
            color: themeVM.palette.cardShadow
            z: 0

            Behavior on color {
                ColorAnimation { duration: 400; easing.type: Easing.InOutCubic }
            }
        }

        // ── 2. 纯色底 ──
        Rectangle {
            anchors.fill: parent
            radius: 8
            color: themeVM.isDark ? "#0f0f1a" : "#f1f5f9"
            z: 1

            Behavior on color {
                ColorAnimation { duration: 400; easing.type: Easing.InOutCubic }
            }
        }

        // ── 3. 噪点纹理层（模拟磨砂玻璃颗粒感）──
        Canvas {
            id: noiseCanvas
            anchors.fill: parent
            z: 2
            opacity: themeVM.isDark ? 0.025 : 0.04

            onPaint: {
                var ctx = getContext("2d")
                ctx.clearRect(0, 0, width, height)
                for (var i = 0; i < 3000; i++) {
                    var x = Math.random() * width
                    var y = Math.random() * height
                    var r = Math.random() * 1.2
                    ctx.fillStyle = Math.random() > 0.5
                        ? "rgba(255,255,255," + (Math.random() * 0.5) + ")"
                        : "rgba(0,0,0," + (Math.random() * 0.3) + ")"
                    ctx.beginPath()
                    ctx.arc(x, y, r, 0, Math.PI * 2)
                    ctx.fill()
                }
            }

            // 首次加载后不再重绘（噪点静态）
            Component.onCompleted: requestPaint()
        }

        // ── 4. 主题色窗口边框 ──
        Rectangle {
            anchors.fill: parent
            radius: 8
            z: 4
            color: "transparent"
            border.color: themeVM.palette.primary
            border.width: 1

            Behavior on border.color {
                ColorAnimation { duration: 400; easing.type: Easing.InOutCubic }
            }
        }
    }

    // ============================================================
    // 内容区域
    // ============================================================
    ColumnLayout {
        id: mainLayout
        anchors.fill: parent
        anchors.margins: 20
        spacing: 14

        // 标题栏（可拖拽）
        Item {
            Layout.fillWidth: true
            height: 52

            // 标题栏拖拽区域
            MouseArea {
                anchors.fill: parent
                property real lastX: 0
                property real lastY: 0
                onPressed: {
                    lastX = mouseX
                    lastY = mouseY
                }
                onMouseXChanged: {
                    settingsWindow.x += (mouseX - lastX)
                }
                onMouseYChanged: {
                    settingsWindow.y += (mouseY - lastY)
                }
            }

            RowLayout {
                anchors.fill: parent
                spacing: 12

                // 大字"设置"标题（靠左）
                ShadowText {
                    text: "设置"
                    font.pixelSize: 24
                    font.weight: Font.Bold
                    font.family: "LXGW Neo XiHei Plus, Inter, sans-serif"
                    renderType: Text.NativeRendering
                    font.hintingPreference: Font.PreferFullHinting
                    color: themeVM.palette.textPrimary
                    shadowOpacity: 0.3
                    Layout.alignment: Qt.AlignVCenter

                    Behavior on color {
                        enabled: !settingsWindow.isResizing
                        ColorAnimation { duration: 300; easing.type: Easing.InOutCubic }
                    }
                }

                Item { Layout.fillWidth: true; Layout.fillHeight: true }

                // 主题切换按钮（三态：Dark/Light/System）
                ThemeToggle {
                    themeMode: themeVM.themeMode
                    isDark: themeVM.isDark
                    onModeClicked: themeVM.toggle()
                }

                // 关闭按钮
                Rectangle {
                    width: 32
                    height: 32
                    radius: 8
                    color: closeMouseArea.containsMouse ? themeVM.palette.closeHover : "transparent"

                    LucideIcon {
                        anchors.centerIn: parent
                        name: "x"
                        size: 16
                        color: closeMouseArea.containsMouse ? "#ef4444" : themeVM.palette.textTertiary
                    }

                    Behavior on color {
                        enabled: !settingsWindow.isResizing
                        ColorAnimation { duration: 150; easing.type: Easing.OutQuad }
                    }

                    MouseArea {
                        id: closeMouseArea
                        anchors.fill: parent
                        hoverEnabled: true
                        onClicked: settingsWindow.requestClose()
                    }
                }
            }
        }

        // 分隔线
        Rectangle {
            Layout.fillWidth: true
            height: 1
            color: themeVM.palette.divider

            Behavior on color {
                enabled: !settingsWindow.isResizing
                ColorAnimation { duration: 300; easing.type: Easing.InOutCubic }
            }
        }

        // ============================================================
        // 卡片区域（两列 Grid + 可滚动）
        // 注意：Flickable clip:true 会裁剪 GlassCard blurEffect 边缘，
        // 但 blur 在卡内渲染，视觉上无影响。
        // ============================================================
        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.minimumHeight: 0

            // 可滚动内容
            Flickable {
                id: flickable
                anchors.left: parent.left
                anchors.right: scrollBar.left
                anchors.top: parent.top
                anchors.bottom: parent.bottom
                anchors.rightMargin: 12
                clip: true
                contentWidth: width
                contentHeight: cardRow.implicitHeight + 32

                // ============================================================
                // 两列卡片：左列 + 右列，顶部对齐
                // 所有卡片均使用 GlassCard 高斯模糊玻璃背景
                // ============================================================
                RowLayout {
                    id: cardRow
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.top: parent.top
                    anchors.margins: 14
                    spacing: 14
                    width: flickable.width - 12

                    // 左列
                    ColumnLayout {
                        Layout.fillWidth: true
                        Layout.fillHeight: false
                        Layout.alignment: Qt.AlignTop
                        spacing: 14

                        BasicSettingsCard {
                            Layout.fillWidth: true
                            settingsVM: settingsWindow.settingsVM
                            themeVM: settingsWindow.themeVM
                        }

                        SystemStatusCard {
                            Layout.fillWidth: true
                            trayVM: settingsWindow.trayVM
                            themeVM: settingsWindow.themeVM
                            active: settingsWindow.visible
                        }

                        ScheduleCard {
                            Layout.fillWidth: true
                            settingsVM: settingsWindow.settingsVM
                            themeVM: settingsWindow.themeVM
                        }
                    }

                    // 右列
                    ColumnLayout {
                        Layout.fillWidth: true
                        Layout.fillHeight: false
                        Layout.alignment: Qt.AlignTop
                        spacing: 14

                        NetworkSettingsCard {
                            Layout.fillWidth: true
                            settingsVM: settingsWindow.settingsVM
                            themeVM: settingsWindow.themeVM
                        }

                        AccountCard {
                            Layout.fillWidth: true
                            settingsVM: settingsWindow.settingsVM
                            themeVM: settingsWindow.themeVM
                        }

                        AccentColorCard {
                            Layout.fillWidth: true
                            settingsVM: settingsWindow.settingsVM
                            themeVM: settingsWindow.themeVM
                        }
                    }
                }
            }

            // 滚动条
            FloatingScrollBar {
                id: scrollBar
                anchors.right: parent.right
                anchors.top: parent.top
                anchors.bottom: parent.bottom
                width: 32
                flickable: flickable
                themeVM: settingsWindow.themeVM
            }
        }

        // ============================================================
        // 底部按钮
        // ============================================================
        RowLayout {
            Layout.fillWidth: true
            spacing: 10

            // 取消按钮
            Rectangle {
                id: cancelBtn
                Layout.fillWidth: true
                height: 40
                radius: 10
                color: cancelMouseArea.containsMouse ? themeVM.palette.hoverBackground : themeVM.palette.surfaceVariant
                border.color: themeVM.palette.outline
                border.width: 1

                Text {
                    anchors.centerIn: parent
                    text: "取消"
                    font.pixelSize: 14
                    font.weight: Font.Medium
                    font.family: "LXGW Neo XiHei Plus, Inter, sans-serif"
                    renderType: Text.NativeRendering
                    font.hintingPreference: Font.PreferFullHinting
                    color: themeVM.palette.textSecondary
                }

                Behavior on color {
                    enabled: !settingsWindow.isResizing
                    ColorAnimation { duration: 150; easing.type: Easing.OutQuad }
                }
                Behavior on border.color {
                    enabled: !settingsWindow.isResizing
                    ColorAnimation { duration: 150; easing.type: Easing.OutQuad }
                }

                MouseArea {
                    id: cancelMouseArea
                    anchors.fill: parent
                    hoverEnabled: true
                    onClicked: settingsWindow.requestClose()
                }
            }

            // 保存按钮
            Rectangle {
                id: saveBtn
                Layout.fillWidth: true
                height: 40
                radius: 10
                color: saveMouseArea.containsMouse ? themeVM.palette.primaryHover : themeVM.palette.primary

                // 悬停外发光
                Rectangle {
                    anchors.fill: parent
                    anchors.margins: saveMouseArea.containsMouse ? -3 : 0
                    radius: parent.radius + (saveMouseArea.containsMouse ? 3 : 0)
                    color: "transparent"
                    border.color: themeVM.palette.primary
                    border.width: 2
                    opacity: saveMouseArea.containsMouse ? 0.5 : 0
                    Behavior on opacity { NumberAnimation { duration: 200; easing.type: Easing.OutQuad } }
                }

                Text {
                    anchors.centerIn: parent
                    text: "保存"
                    font.pixelSize: 14
                    font.weight: Font.Medium
                    font.family: "LXGW Neo XiHei Plus, Inter, sans-serif"
                    renderType: Text.NativeRendering
                    font.hintingPreference: Font.PreferFullHinting
                    color: themeVM.palette.onPrimary
                }

                Behavior on color {
                    enabled: !settingsWindow.isResizing
                    ColorAnimation { duration: 150; easing.type: Easing.OutQuad }
                }

                MouseArea {
                    id: saveMouseArea
                    anchors.fill: parent
                    hoverEnabled: true
                    onClicked: {
                        settingsVM.save()
                        settingsWindow.requestClose()
                    }
                }
            }
        }
    }

    // ── 关闭 contentRoot（窗口开合动画根容器）──
    }

    // ============================================================
    // 窗口开合流程
    // - 打开：visible 变 true 时由 onVisibleChanged 触发 openAnim
    // - 关闭：requestClose() 启动退场动画，closeAnim.onStopped 才真正 hide()
    // ============================================================
    property bool isClosing: false

    function requestClose() {
        if (isClosing)
            return
        isClosing = true
        closeAnim.start()
    }

    onVisibleChanged: {
        if (visible) {
            if (isClosing) {
                isClosing = false
                closeAnim.stop()
            }
            contentRoot.opacity = 0
            contentRoot.scale = 0.96
            openAnim.start()
        }
    }
}
