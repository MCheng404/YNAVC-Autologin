#pragma once
/**
 * @brief Toast 通知窗口（C++ QWindow，右下角悬浮，QML 样式渲染）
 *
 * 通过 QQuickWindow + QQuickRenderControl 在 QWindow 中嵌入 QML 场景，
 * 实现与主题系统一致的 Toast 通知。
 */

#include <QWindow>
#include <QQuickWindow>
#include <QPointer>
#include <QTimer>
#include <QQmlEngine>
#include <QQmlComponent>
#include <QQuickItem>
#include <QPoint>
#include <QSize>

class ToastWindow : public QObject
{
    Q_OBJECT

public:
    // type: "info" | "success" | "error"
    // position: 0=右下角, 1=左下角, 2=顶部居中
    static void show(const QString &title, const QString &message,
                     const QString &type, int position,
                     QWindow *transientParent = nullptr);

private:
    ToastWindow(const QString &title, const QString &message,
                const QString &type, int position, QWindow *transientParent);
    ~ToastWindow();

    void positionWindow();
    void setupFadeOut();

    QPointer<QQuickWindow> m_window;   // 顶层 QML 窗口（create() 返回，由本对象负责释放）
    QQuickItem  *m_root    = nullptr;
    QTimer      *m_timer   = nullptr;

    // 堆叠管理 / 销毁守卫
    bool m_disposed = false;
    int  m_position = 0;
    int  m_slot     = 0;
};
