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

// 每个位置（0=右下, 1=左下, 2=顶部居中）已占用的堆叠槽位索引。
// 新 Toast 取最小空闲槽位，避免与已有 Toast 完全重叠；关闭后释放槽位，不做复杂重排。
static QMap<int, QSet<int>> s_occupiedSlots;

QObject *ToastWindow::s_themeVM = nullptr;

void ToastWindow::setThemeViewModel(QObject *vm)
{
    s_themeVM = vm;
}

static int allocSlot(int position)
{
    QSet<int> &slotSet = s_occupiedSlots[position];
    int idx = 0;
    while (slotSet.contains(idx))
        ++idx;
    slotSet.insert(idx);
    return idx;
}

static void freeSlot(int position, int idx)
{
    s_occupiedSlots[position].remove(idx);
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

    // 注入位置参数（0=右下, 1=左下, 2=顶部居中）及堆叠偏移
    m_position = position;
    m_slot = allocSlot(position);
    const int toastStep = 82 + 12;   // 卡片高度 + 间距
    const int stackOffset = m_slot * toastStep;
    engine->rootContext()->setContextProperty("toastPosition", position);
    engine->rootContext()->setContextProperty("toastStackOffset", stackOffset);

    // 内联 QML 通知组件
    const QString qml = R"(
import QtQuick 2.15
import QtQuick.Window 2.15

Window {
    id: root
    title: toastTitle
    visible: true
    flags: Qt.Tool | Qt.FramelessWindowHint | Qt.WindowStaysOnTopHint
    color: "transparent"
    opacity: 0

    // 根据设置计算位置
    // 0=右下角, 1=左下角, 2=顶部居中
    x: {
        if (toastPosition === 1) return 20  // 左下
        if (toastPosition === 2) return (Screen.desktopAvailableWidth - width) / 2  // 顶部居中
        return Screen.desktopAvailableWidth - width - 20  // 右下（默认）
    }

    // 终点 Y（已含堆叠偏移）；入场/离场动画围绕该值滑入/滑出
    property int targetY: {
        if (toastPosition === 2) return 20 + toastStackOffset  // 顶部：向下堆叠
        return Screen.desktopAvailableHeight - height - 20 - toastStackOffset  // 底部：向上堆叠
    }
    y: targetY

    // 固定大小
    width: 320
    height: 82

    // 入场/离场偏移：底部从下方 +24 滑入，顶部从上方 -24 滑入
    property int enterOffset: (toastPosition === 2) ? -24 : 24
    property int leaveOffset: enterOffset

    // 内容容器：承载缩放与阴影。注意 Window 无 scale 属性，故缩放放在内部 Item 上。
    Item {
        id: content
        anchors.fill: parent
        scale: 0.92

        // 阴影
        Rectangle {
            anchors.fill: parent
            anchors.margins: -6
            z: -1
            radius: 16
            color: "#33000000"
        }

        // 内容卡片（radius=10）
        Rectangle {
            id: card
            anchors.fill: parent
            radius: 10
            color: themeVM ? themeVM.palette.surface : "#1e1e2e"

            // 左侧类型色条
            Rectangle {
                width: 4
                height: parent.height - 24
                anchors.verticalCenter: parent.verticalCenter
                x: 10
                radius: 2
                color: accentColor
            }

            // 内容
            Column {
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter
                anchors.leftMargin: 28
                anchors.rightMargin: 16
                spacing: 3

                Text {
                    anchors.left: parent.left
                    anchors.right: parent.right
                    text: toastTitle
                    font.pixelSize: 13
                    font.weight: Font.Medium
                    font.family: "LXGW Neo XiHei Plus, Inter, sans-serif"
                    color: themeVM ? themeVM.palette.textPrimary : "#f1f5f9"
                    renderType: Text.NativeRendering
                    elide: Text.ElideRight
                }

                Text {
                    anchors.left: parent.left
                    anchors.right: parent.right
                    text: toastMessage
                    font.pixelSize: 12
                    font.family: "LXGW Neo XiHei Plus, Inter, sans-serif"
                    color: themeVM ? themeVM.palette.textSecondary : "#94a3b8"
                    renderType: Text.NativeRendering
                    elide: Text.ElideRight
                    maximumLineCount: 2
                    wrapMode: Text.WordWrap
                }
            }
        }
    }

    // 点击关闭
    MouseArea {
        anchors.fill: content
        onClicked: fadeOut.start()
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
        NumberAnimation { target: content; property: "scale";   from: 0.92; to: 1;    duration: 340; easing.type: Easing.OutBack }
        NumberAnimation { target: root;    property: "y";       from: targetY + enterOffset; to: targetY; duration: 340; easing.type: Easing.OutBack }
    }

    // 离场动画：淡出 + 轻微缩小 + 滑出；停止后关闭窗口（销毁唯一真源）
    ParallelAnimation {
        id: fadeOut
        NumberAnimation { target: root;    property: "opacity"; from: 1; to: 0;    duration: 240; easing.type: Easing.InCubic }
        NumberAnimation { target: content; property: "scale";   from: 1; to: 0.96; duration: 240; easing.type: Easing.InCubic }
        NumberAnimation { target: root;    property: "y";       from: targetY; to: targetY + leaveOffset; duration: 240; easing.type: Easing.InCubic }
        onStopped: root.close()
    }

    // 自动关闭：由 fadeOut 收尾，不再硬杀窗口
    Timer {
        interval: 4500
        running: true
        onTriggered: fadeOut.start()
    }

    Component.onCompleted: {
        fadeIn.start()
    }
}
)";

    QQmlComponent component(engine);
    component.setData(qml.toUtf8(), QUrl());

    QObject *obj = component.create(engine->rootContext());
    if (!obj) {
        qWarning() << "ToastWindow: QML create failed:" << component.errors();
        freeSlot(m_position, m_slot);
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
        freeSlot(m_position, m_slot);
        delete engine;
        return;
    }

    // 显示窗口
    m_window->show();
    m_window->raise();
    m_window->requestActivate();

    // 动画为唯一真源：QML Timer(4500) 触发 fadeOut，fadeOut.onStopped -> root.close()
    // -> QQuickWindow::closing -> 释放窗口/引擎/本对象。不再硬杀窗口（修复双计时器 race）。
    auto dispose = [this]() {
        if (m_disposed)
            return;
        m_disposed = true;
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
    if (m_window) {
        m_window->close();
        m_window->deleteLater();
    }
    freeSlot(m_position, m_slot);
}

void ToastWindow::show(const QString &title, const QString &message,
                       const QString &type, int position, QWindow *transientParent)
{
    Q_UNUSED(transientParent);
    // 每个通知创建独立对象，自动 deleteLater 释放
    new ToastWindow(title, message, type, position, transientParent);
}
