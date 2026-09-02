#include "ToastWindow.h"

#include <QGuiApplication>
#include <QScreen>
#include <QTimer>
#include <QPropertyAnimation>
#include <QQmlEngine>
#include <QQmlComponent>
#include <QQmlContext>

ToastWindow::ToastWindow(const QString &title, const QString &message,
                         const QString &type, int position, QWindow *transientParent)
{
    // 查找主窗口以获取 themeVM
    QObject *themeVM = nullptr;
    for (QWindow *w : QGuiApplication::allWindows()) {
        if (w->isVisible() && !w->title().isEmpty()) {
            QQmlEngine *qe = qmlEngine(w);
            if (qe) {
                QVariant vm = qe->rootContext()->contextProperty("themeVM");
                if (vm.value<QObject*>()) {
                    themeVM = vm.value<QObject*>();
                    break;
                }
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
    engine->rootContext()->setContextProperty("toastPosition", position);

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

    // 根据设置计算位置
    // 0=右下角, 1=左下角, 2=顶部居中
    x: {
        if (toastPosition === 1) return 20  // 左下
        if (toastPosition === 2) return (Screen.desktopAvailableWidth - width) / 2  // 顶部居中
        return Screen.desktopAvailableWidth - width - 20  // 右下（默认）
    }
    y: {
        if (toastPosition === 2) return 20  // 顶部居中：顶部留 20px
        return Screen.desktopAvailableHeight - height - 20  // 底部两种：底部留 20px
    }

    // 固定大小
    width: 320
    height: 82

    // 圆角遮罩（Qt.Window 的 clip 由 DWM 圆角处理）
    // 内容卡片（radius=10）
    Rectangle {
        id: card
        anchors.fill: parent
        anchors.margins: 0
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

        // 阴影
        Rectangle {
            anchors.fill: parent
            anchors.margins: -6
            z: -1
            radius: 16
            color: "#33000000"
        }
    }

    // 点击关闭
    MouseArea {
        anchors.fill: card
        onClicked: {
            fadeOut.start()
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

    // 入场动画：淡入
    NumberAnimation {
        id: fadeIn
        target: root
        property: "opacity"
        from: 0; to: 1
        duration: 220
        easing.type: Easing.OutCubic
    }

    // 离场动画：淡出
    NumberAnimation {
        id: fadeOut
        target: root
        property: "opacity"
        from: 1; to: 0
        duration: 300
        easing.type: Easing.InCubic
        onStopped: {
            root.close()
        }
    }

    // 4s 自动关闭
    Timer {
        interval: 4000
        onTriggered: fadeOut.start()
    }

    Component.onCompleted: {
        // 立即淡入
        fadeIn.start()
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
        delete engine;
        return;
    }

    // 显示窗口
    m_window->show();
    m_window->raise();
    m_window->requestActivate();

    // 4s 后关闭并删除
    QTimer::singleShot(4000, this, [this]() {
        if (m_window) {
            m_window->close();
            m_window->deleteLater();
            m_window = nullptr;
        }
        this->deleteLater();
    });
}

ToastWindow::~ToastWindow()
{
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
