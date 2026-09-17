#include "ToastWindow.h"

#include <QGuiApplication>
#include <QScreen>
#include <QTimer>
#include <QPropertyAnimation>
#include <QQmlEngine>
#include <QQmlComponent>
#include <QQmlContext>
#include <QSet>
#include <QMap>
#include <QList>

// 每个位置（0=右下, 1=左下, 2=顶部居中）按创建顺序维护的存活通知根窗口列表。
// 新通知插入列表头部（最底部），其余整体上移；关闭时移除并触发其余平滑归位。
static QMap<int, QList<QQuickWindow*>> s_stack;

QObject *ToastWindow::s_themeVM = nullptr;

void ToastWindow::setThemeViewModel(QObject *vm)
{
    s_themeVM = vm;
}

// 重排某位置下所有存活通知：按列表顺序重分配堆叠偏移（QML 端 y 平滑过渡）。
static void relayoutStack(int position)
{
    auto it = s_stack.find(position);
    if (it == s_stack.end())
        return;
    const int gap = 12;
    int offset = 0;
    for (QQuickWindow *w : *it) {
        if (!w)
            continue;
        w->setProperty("toastStackOffset", offset);
        offset += w->height() + gap;   // 按各通知实际高度累加，间距恒定
    }
}

ToastWindow::ToastWindow(const QString &title, const QString &message,
                         const QString &type, int position, QWindow *transientParent)
{
    // 优先使用 App 注入的 themeVM；未注入时回退到遍历窗口的 QML 上下文查找
    QObject *themeVM = s_themeVM;
    if (!themeVM) {
        for (QWindow *w : QGuiApplication::allWindows()) {
            QQmlEngine *qe = qmlEngine(w);
            if (!qe)
                continue;
            QVariant vm = qe->rootContext()->contextProperty("themeVM");
            if (vm.value<QObject*>()) {
                themeVM = vm.value<QObject*>();
                break;
            }
        }
    }

    // 创建独立 QML 引擎
    QQmlEngine *engine = new QQmlEngine(this);

    // 设置 themeVM（如果找到主窗口）
    if (themeVM) {
        engine->rootContext()->setContextProperty("themeVM", themeVM);
    }

    // 注入位置参数（0=右下, 1=左下, 2=顶部居中）
    m_position = position;
    engine->rootContext()->setContextProperty("toastPosition", position);

    // 内联 QML 通知组件
    const QString qml = R"(
import QtQuick 2.15
import QtQuick.Window 2.15
import QtQuick.Shapes 1.15

Window {
    id: root
    title: toastTitle
    visible: true
    flags: Qt.Tool | Qt.FramelessWindowHint | Qt.WindowStaysOnTopHint
    color: "transparent"
    opacity: 0

    // 堆叠偏移（由 C++ 运行时按存活列表重排写入），用于平滑归位
    property int toastStackOffset: 0

    // 首帧后才允许堆叠归位动画，避免新窗口创建时 y 跳变触发 Behavior，
    // 与 fadeIn 的 content.y 滑入叠加成"飘两下"
    property bool stackAnimReady: false

    // 根据设置计算位置
    // 0=右下角, 1=左下角, 2=顶部居中
    x: {
        if (toastPosition === 1) return 20
        if (toastPosition === 2) return (Screen.desktopAvailableWidth - width) / 2
        return Screen.desktopAvailableWidth - width - 20
    }

    // 终点 Y（已含堆叠偏移）；入场/离场动画围绕该值滑入/滑出
    property int targetY: {
        if (toastPosition === 2) return 20 + toastStackOffset
        return Screen.desktopAvailableHeight - height - 20 - toastStackOffset
    }
    y: targetY

    // 堆叠归位：偏移变化时平滑过渡（入场滑入由 content.y 负责，互不冲突）
    Behavior on y {
        enabled: root.stackAnimReady
        NumberAnimation { duration: 320; easing.type: Easing.OutCubic }
    }

    width: 340
    // 长文本自适应高度（88~132）
    height: Math.min(132, Math.max(88, body.implicitHeight + 30))

    // 入场/离场偏移：底部从下方 +24 滑入，顶部从上方 -24 滑入
    property int enterOffset: (toastPosition === 2) ? -24 : 24
    property int leaveOffset: enterOffset

    // 悬停态：用于显示关闭按钮 + 暂停自动关闭
    property bool hovering: false

    // 内容容器：承载缩放、滑入、阴影。注意 Window 无 scale 属性，故缩放/滑入放在内部 Item 上。
    Item {
        id: content
        width: parent.width
        height: parent.height
        scale: 0.92
        y: enterOffset

        // 多层柔和阴影（纯色叠加，无渐变）：合并为 2 层，去掉中间过渡层以减 overdraw。
        // 保留最内层(-4,较强,实体接触感)与最外层(-12,最弱最扩散)两个极值，观感基本不变。
        Rectangle { anchors.fill: parent; anchors.margins: -4;  radius: 14; color: "#1f000000"; z: -1 }
        Rectangle { anchors.fill: parent; anchors.margins: -12; radius: 22; color: "#0a000000"; z: -3 }

        // 内容卡片（radius=10）
        Rectangle {
            id: card
            anchors.fill: parent
            radius: 10
            color: themeVM ? themeVM.palette.surface : "#1e1e2e"

            // 圆角图标容器 + 类型图标（自绘，纯色无渐变）
            Rectangle {
                id: iconBox
                width: 30; height: 30
                radius: 8
                anchors.left: parent.left
                anchors.leftMargin: 12
                anchors.verticalCenter: parent.verticalCenter
                // 注意：这里不能用 opacity 来做"淡背景"——opacity 会作用于整棵子树，
                // 把里面的图标一起压到同样透明度，图标会糊得看不清。
                // 必须用带 alpha 的颜色，让"背景淡、图标实"。
                color: Qt.rgba(accentColor.r, accentColor.g, accentColor.b, 0.20)

                // ── 图标动效 ──
                // Rectangle 的 transformOrigin 默认是 Center，scale 绕中心缩放。
                // 注意 iconEnter 与 iconBreathe 都会改 scale：必须串行 ——
                // 进场结束后再启动呼吸，否则两个动画会互相打架。
                ParallelAnimation {
                    id: iconEnter
                    NumberAnimation { target: iconBox; property: "scale"; from: 0.30; to: 1.0; duration: 340; easing.type: Easing.OutBack }
                    NumberAnimation { target: iconRot; property: "angle"; from: -25;  to: 0;   duration: 340; easing.type: Easing.OutBack }
                    onStopped: iconBreathe.start()
                }
                // 持续呼吸：极轻微缩放，让图标"活着"，但不喧宾夺主
                SequentialAnimation {
                    id: iconBreathe
                    loops: Animation.Infinite
                    NumberAnimation { target: iconBox; property: "scale"; to: 1.07; duration: 900; easing.type: Easing.InOutSine }
                    NumberAnimation { target: iconBox; property: "scale"; to: 1.00; duration: 900; easing.type: Easing.InOutSine }
                }
                // error 类型额外抖一下，强化"出错了"的感知
                SequentialAnimation {
                    id: iconShake
                    NumberAnimation { target: iconShift; property: "x"; to: -3; duration: 60 }
                    NumberAnimation { target: iconShift; property: "x"; to:  3; duration: 60 }
                    NumberAnimation { target: iconShift; property: "x"; to: -2; duration: 60 }
                    NumberAnimation { target: iconShift; property: "x"; to:  0; duration: 60 }
                }
                Timer {
                    id: iconShakeTimer
                    interval: 420
                    onTriggered: iconShake.start()
                }
                transform: [
                    Translate { id: iconShift; x: 0 },
                    Rotation  { id: iconRot; origin.x: 15; origin.y: 15; angle: 0 }
                ]
                Component.onCompleted: {
                    iconEnter.start()
                    if (root.toastType === "error")
                        iconShakeTimer.start()
                }

                // 注意：strokeColor/fillColor/strokeWidth/capStyle/joinStyle 都是
                // ShapePath 的属性，PathSvg 只有 path（+ viewBox）。
                // 早期版本把这些样式属性直接写在 PathSvg 上，导致
                // "Cannot assign to non-existent property" → 整个 Toast 创建失败。
                Shape {
                    id: shapeSuccess
                    anchors.centerIn: parent
                    width: 18; height: 18
                    visible: root.toastType === "success"
                    ShapePath {
                        strokeColor: accentColor
                        strokeWidth: 2.8
                        fillColor: "transparent"
                        capStyle: ShapePath.RoundCap
                        joinStyle: ShapePath.RoundJoin
                        PathSvg { path: "M4 9 L8 13 L15 5" }
                    }
                }
                Shape {
                    id: shapeError
                    anchors.centerIn: parent
                    width: 18; height: 18
                    visible: root.toastType === "error"
                    ShapePath {
                        strokeColor: accentColor
                        strokeWidth: 2.8
                        fillColor: "transparent"
                        capStyle: ShapePath.RoundCap
                        PathSvg { path: "M9 4 L9 12" }
                    }
                    ShapePath {
                        strokeColor: "transparent"
                        fillColor: accentColor
                        PathSvg { path: "M9 16 m-1.6 0 a1.6 1.6 0 1 0 3.2 0 a1.6 1.6 0 1 0 -3.2 0" }
                    }
                }
                Shape {
                    id: shapeInfo
                    anchors.centerIn: parent
                    width: 18; height: 18
                    visible: root.toastType === "info"
                    ShapePath {
                        strokeColor: accentColor
                        strokeWidth: 2.8
                        fillColor: "transparent"
                        capStyle: ShapePath.RoundCap
                        PathSvg { path: "M9 6 L9 11" }
                    }
                    ShapePath {
                        strokeColor: "transparent"
                        fillColor: accentColor
                        PathSvg { path: "M9 15.5 m-1.6 0 a1.6 1.6 0 1 0 3.2 0 a1.6 1.6 0 1 0 -3.2 0" }
                    }
                }
            }

            // 内容
            Column {
                id: body
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter
                anchors.leftMargin: 52
                anchors.rightMargin: 30
                spacing: 3

                Text {
                    anchors.left: parent.left
                    anchors.right: parent.right
                    text: toastTitle
                    font.pixelSize: 14
                    font.weight: Font.Medium
                    font.family: "LXGW Neo XiHei Plus, Inter, sans-serif"
                    color: themeVM ? themeVM.palette.textPrimary : "#f1f5f9"
                    elide: Text.ElideRight
                }

                Text {
                    anchors.left: parent.left
                    anchors.right: parent.right
                    text: toastMessage
                    font.pixelSize: 13
                    font.family: "LXGW Neo XiHei Plus, Inter, sans-serif"
                    color: themeVM ? themeVM.palette.textSecondary : "#94a3b8"
                    elide: Text.ElideRight
                    maximumLineCount: 2
                    wrapMode: Text.WordWrap
                }
            }

            // 进度条（随倒计时收缩，纯色）
            Rectangle {
                id: progressTrack
                height: 3
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                anchors.leftMargin: 12
                anchors.rightMargin: 12
                anchors.bottomMargin: 6
                radius: 1.5
                color: themeVM ? themeVM.palette.divider : "#000000"
                opacity: 0.5
                Rectangle {
                    id: progressBar
                    anchors.left: parent.left
                    anchors.top: parent.top
                    anchors.bottom: parent.bottom
                    width: parent.width
                    radius: 1.5
                    color: accentColor
                    transformOrigin: Item.Left
                    transform: Scale { id: progressScale; xScale: 1 }
                }
            }
        }
    }

    // 主交互区：点击关闭 + hover 暂停/恢复
    MouseArea {
        anchors.fill: content
        hoverEnabled: true
        onEntered: { root.hovering = true; progressAnim.pause() }
        onExited:  { root.hovering = false; progressAnim.resume() }
        onClicked: fadeOut.start()
    }

    // 关闭按钮（右上角，hover 显示，点击立即关闭）
    Rectangle {
        id: closeBtn
        width: 22; height: 22
        radius: 6
        anchors.top: parent.top
        anchors.right: parent.right
        anchors.topMargin: 8
        anchors.rightMargin: 8
        color: themeVM ? themeVM.palette.hoverBackground : "#2a2a3a"
        // 常态(未 hover)opacity=0 时直接剔除绘制批次；hover 时立即可见，
        // opacity 过渡(160ms OutQuad)的进出场动画照常播放（淡入时 hovering 已为真，
        // 淡出时 opacity 仍 >0 故 visible 保持真直到动画归零）。
        visible: root.hovering || opacity > 0
        opacity: root.hovering ? 1 : 0
        Behavior on opacity { NumberAnimation { duration: 160; easing.type: Easing.OutQuad } }
        MouseArea {
            anchors.fill: parent
            cursorShape: Qt.PointingHandCursor
            onEntered: { root.hovering = true; progressAnim.pause() }
            onExited:  { root.hovering = false; progressAnim.resume() }
            onClicked: fadeOut.start()
        }
        Text {
            anchors.centerIn: parent
            text: "×"
            font.pixelSize: 14
            font.family: "LXGW Neo XiHei Plus, Inter, sans-serif"
            color: themeVM ? themeVM.palette.textTertiary : "#cbd5e1"
        }
    }

    // 动态属性
    property string toastTitle: ""
    property string toastMessage: ""
    property string toastType: "info"
    property color accentColor: {
        if (root.toastType === "error")   return "#ef4444"
        if (root.toastType === "success") return "#4ade80"
        return "#60a5fa"
    }

    // 入场动画：淡入 + 轻微放大回弹 + 滑入（ParallelAnimation 组合）
    ParallelAnimation {
        id: fadeIn
        NumberAnimation { target: root;    property: "opacity"; from: 0;    to: 1;    duration: 320; easing.type: Easing.OutCubic }
        NumberAnimation { target: content; property: "scale";   from: 0.92; to: 1;    duration: 320; easing.type: Easing.OutBack }
        NumberAnimation { target: content; property: "y";       from: enterOffset; to: 0; duration: 320; easing.type: Easing.OutCubic }
    }

    // 离场动画：淡出 + 轻微缩小 + 滑出；停止后关闭窗口（销毁唯一真源）
    ParallelAnimation {
        id: fadeOut
        NumberAnimation { target: root;    property: "opacity"; from: 1; to: 0;    duration: 240; easing.type: Easing.InCubic }
        NumberAnimation { target: content; property: "scale";   from: 1; to: 0.96; duration: 240; easing.type: Easing.InCubic }
        NumberAnimation { target: content; property: "y";       from: 0; to: leaveOffset; duration: 240; easing.type: Easing.InCubic }
        onStopped: root.close()
    }

    // 进度条收缩动画：作为自动关闭的唯一真源（hover 时 pause/resume）；
    // 走完即触发离场，与进度条完全对齐，不再有双计时器漂移。
    // 用 transform 缩放替代改 width，避免每帧触发布局重排（父容器/兄弟项不变）。
    NumberAnimation {
        id: progressAnim
        target: progressScale
        property: "xScale"
        from: 1
        to: 0
        duration: 4500
        easing.type: Easing.OutQuad
        onStopped: fadeOut.start()
    }

    Component.onCompleted: {
        fadeIn.start()
        progressAnim.start()
        Qt.callLater(function() { root.stackAnimReady = true })   // 首帧之后再允许堆叠动画
    }
}
)";

    QQmlComponent component(engine);
    component.setData(qml.toUtf8(), QUrl());

    QObject *obj = component.create(engine->rootContext());
    if (!obj) {
        qWarning() << "ToastWindow: QML create failed:" << component.errors();
        delete engine;
        return;
    }

    // 设置属性
    obj->setProperty("toastTitle",   title);
    obj->setProperty("toastMessage", message);
    obj->setProperty("toastType",    type);

    // 获取窗口
    m_window = qobject_cast<QQuickWindow*>(obj);
    if (!m_window) {
        // QML 根可能是 Item，向上查找 Window
        QObject *parent = obj;
        while (parent && !qobject_cast<QWindow*>(parent)) {
            parent = parent->property("parent").value<QObject*>();
        }
        m_window = qobject_cast<QQuickWindow*>(parent);
    }

    if (!m_window) {
        m_window = qobject_cast<QQuickWindow*>(obj);
    }

    if (!m_window) {
        qWarning() << "ToastWindow: cannot get QQuickWindow";
        delete obj;
        delete engine;
        return;
    }

    // 加入存活列表并重排本位置所有通知（其余平滑上移）
    s_stack[m_position].append(m_window);
    relayoutStack(m_position);

    // 窗口高度由内容决定、稳定后才会正确；高度变化（行数不同）时全量重排，
    // 保证任意高度的通知间距恒定。改 y 不影响 height，不会递归。
    QObject::connect(m_window, &QWindow::heightChanged, this, [position]() {
        relayoutStack(position);
    });

    // 显示窗口
    m_window->show();
    m_window->raise();
    m_window->requestActivate();

    // 动画为唯一真源：progressAnim(4500) 走完触发 fadeOut，fadeOut.onStopped -> root.close()
    // -> QQuickWindow::closing -> 释放窗口/引擎/本对象。不再硬杀窗口（修复双计时器 race）。
    auto dispose = [this]() {
        if (m_disposed)
            return;
        m_disposed = true;
        s_stack[m_position].removeAll(m_window);
        relayoutStack(m_position);     // 其余通知平滑归位
        if (m_window)
            m_window->deleteLater();   // create() 返回的顶层对象由调用方负责释放
        this->deleteLater();           // 引擎为子对象，随本对象一起释放
    };

    QObject::connect(m_window, &QQuickWindow::closing, this, dispose);

    // 安全兜底：异常情况下（如 QML 计时器未触发）防泄漏。正常路径已被 closing 取消。
    QTimer::singleShot(10000, this, [this, dispose]() {
        if (m_disposed)
            return;
        if (m_window && m_window->isVisible())
            m_window->close();         // 触发 closing -> dispose
        else
            dispose();
    });
}

ToastWindow::~ToastWindow()
{
    m_disposed = true;   // 防止 closing 信号再次进入 dispose（析构中二次 deleteLater 不安全）
    if (m_window)
        s_stack[m_position].removeAll(m_window);
    if (m_window) {
        m_window->close();
        m_window->deleteLater();
    }
}

void ToastWindow::show(const QString &title, const QString &message,
                       const QString &type, int position, QWindow *transientParent)
{
    Q_UNUSED(transientParent);
    // 每个通知创建独立对象，自动 deleteLater 释放
    new ToastWindow(title, message, type, position, transientParent);
}
