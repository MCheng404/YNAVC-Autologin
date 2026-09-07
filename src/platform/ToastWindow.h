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

    // 由 App 在创建 ThemeViewModel 后注入。
    // 首个通知可能早于任何 QML 窗口创建（托盘应用主窗口也常不可见），
    // 此时无法从窗口/QML 上下文反查 themeVM，故在此直接持有。
    static void setThemeViewModel(QObject *vm);

private:
    ToastWindow(const QString &title, const QString &message,
                const QString &type, int position, QWindow *transientParent);
    ~ToastWindow();

    void positionWindow();

    QPointer<QQuickWindow> m_window;   // 顶层 QML 窗口（create() 返回，由本对象负责释放）
    QQuickItem  *m_root    = nullptr;
    QTimer      *m_timer   = nullptr;

    // 堆叠管理 / 销毁守卫
    bool m_disposed = false;
    int  m_position = 0;
    int  m_slot     = 0;

    static QObject *s_themeVM;   // 由 App 注入的主题 ViewModel
};
