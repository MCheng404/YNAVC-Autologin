#include "AuthEngine.h"
#include "ConnectivityChecker.h"
#include "Logger.h"
#include "Settings.h"

#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QUrl>
#include <QUrlQuery>
#include <QEventLoop>
#include <QTimer>
#include <QDateTime>
#include <QThread>

AuthEngine::AuthEngine(ConnectivityChecker *checker, Logger *logger,
                        Settings *settings, QObject *parent)
    : QObject(parent)
    , m_checker(checker)
    , m_logger(logger)
    , m_settings(settings)
    // m_nam 不在此处创建：authenticate() 可能被工作线程调用，
    // QNetworkAccessManager 必须在实际调用线程中创建和使用。
{
}

bool AuthEngine::isBusy() const
{
    return m_busy.load();
}

bool AuthEngine::isInBlackout() const
{
    QTime now = QTime::currentTime();
    int curMinutes = now.hour() * 60 + now.minute();
    // 黑名单时段：03:55-05:05（分钟数 235-305）
    return (curMinutes >= 235 && curMinutes <= 305);
}

void AuthEngine::resetFailCount()
{
    m_failCount = 0;
}

int AuthEngine::getExponentialBackoff(int fails) const
{
    int cappedFails = qMin(fails, 6);
    int waitSeconds = qMin(300, 5 * (1 << cappedFails));
    return waitSeconds;
}

QString AuthEngine::cleanMac(const QString &mac)
{
    QString result;
    result.reserve(mac.length());
    for (const QChar &c : mac) {
        if (c != QLatin1Char(':') && c != QLatin1Char('-')) {
            result.append(c.toUpper());
        }
    }
    return result;
}

QString AuthEngine::buildPostData(const QString &username, const QString &password,
                                   const QString &ip, const QString &mac) const
{
    QString macClean = cleanMac(mac);
    // 与原版 autologin.c 完全一致的 POST 格式
    return QStringLiteral("DDDDD=%1&upass=%2&0MKKey=123456&wlanuserip=%3&mac=%4")
        .arg(username, password, ip, macClean);
}

AuthEngine::AuthResult AuthEngine::authenticate(const QString &mac, const QString &ip)
{
    // 原子锁防抖：防止并发调用
    bool expected = false;
    if (!m_busy.compare_exchange_strong(expected, true)) {
        emit statusMessage(QStringLiteral("认证进行中，忽略重复调用"));
        return AuthResult::Busy;
    }

    // 3秒时间窗口防抖
    qint64 nowMs = QDateTime::currentMSecsSinceEpoch();
    qint64 elapsed = nowMs - m_lastAuthMs.load();
    if (elapsed < 3000) {
        m_busy = false;
        emit statusMessage(QStringLiteral("点击防抖：距离上次认证仅 %1ms，跳过").arg(elapsed));
        return AuthResult::Busy;
    }

    emit statusMessage(QStringLiteral("认证登录: IP=%1 MAC=%2").arg(ip, mac));

    QString username = m_settings->username();
    QString password = m_settings->password();

    if (username.isEmpty()) {
        m_busy = false;
        emit statusMessage(QStringLiteral("用户名为空，跳过认证"));
        return AuthResult::Failed;
    }

    // 自动补 @unicom 后缀（与原版 autologin.c 一致）
    if (!username.contains(QLatin1Char('@'))) {
        username += QStringLiteral("@unicom");
    }

    // 先访问 F.htm 页面
    // 在当前调用线程创建局部 QNAM（避免跨线程使用主线程的 m_nam）
    QNetworkAccessManager nam;
    {
        QEventLoop loop;
        QNetworkRequest request(QUrl(QStringLiteral("http://172.30.255.2/F.htm")));
        QTimer::singleShot(2000, &loop, &QEventLoop::quit);
        QNetworkReply *reply = nam.get(request);
        connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
        loop.exec();
        reply->deleteLater();
    }

    // 等待 800ms（与原版一致）
    QThread::msleep(800);

    // 构建 POST 数据
    QString postData = buildPostData(username, password, ip, mac);

    // 执行 HTTP POST 认证
    QEventLoop loop;
    QNetworkRequest request(QUrl(QStringLiteral("http://172.30.255.2/")));
    request.setHeader(QNetworkRequest::ContentTypeHeader,
                       QStringLiteral("application/x-www-form-urlencoded"));
    request.setRawHeader("Referer", "http://172.30.255.2/");

    QTimer::singleShot(5000, &loop, &QEventLoop::quit);
    QNetworkReply *reply = nam.post(request, postData.toUtf8());
    connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();

    AuthResult result = AuthResult::Failed;
    QByteArray responseData = reply->readAll();
    int httpStatus = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    reply->deleteLater();

    if (httpStatus > 0 && !responseData.isEmpty()) {
        QString responseStr = QString::fromUtf8(responseData);

        // 成功关键词检测（与原版一致）
        if (responseStr.contains(QStringLiteral("成功")) ||
            responseStr.contains(QStringLiteral("信息页")) ||
            responseStr.contains(QStringLiteral("success")) ||
            responseStr.contains(QStringLiteral("logout")) ||
            responseStr.contains(QStringLiteral("已经"))) {

            emit statusMessage(QStringLiteral("认证成功! MAC=%1").arg(cleanMac(mac)));
            result = AuthResult::Success;
            m_failCount = 0;
            m_lastAuthMs = QDateTime::currentMSecsSinceEpoch();
            m_busy = false;
            emit authCompleted(result);
            return result;
        }

        if (responseStr.contains(QStringLiteral("Msg=02"))) {
            emit statusMessage(QStringLiteral("已在线(缓存)"));
            result = AuthResult::AlreadyOnline;
            m_failCount = 0;
            m_lastAuthMs = QDateTime::currentMSecsSinceEpoch();
            m_busy = false;
            emit authCompleted(result);
            return result;
        }

        // 响应未匹配关键词 → 二次校验
        emit statusMessage(QStringLiteral("响应未匹配，二次校验连通性"));
        QThread::msleep(1000);

        if (m_checker && m_checker->check()) {
            emit statusMessage(QStringLiteral("二次校验通过，认证成功!"));
            result = AuthResult::Success;
            m_failCount = 0;
            m_lastAuthMs = QDateTime::currentMSecsSinceEpoch();
            m_busy = false;
            emit authCompleted(result);
            return result;
        }
    } else {
        // POST 本身失败，也做二次校验
        emit statusMessage(QStringLiteral("POST 未收到响应，二次校验连通性"));
        QThread::msleep(1000);

        if (m_checker && m_checker->check()) {
            emit statusMessage(QStringLiteral("二次校验通过，网络已通!"));
            result = AuthResult::Success;
            m_failCount = 0;
            m_lastAuthMs = QDateTime::currentMSecsSinceEpoch();
            m_busy = false;
            emit authCompleted(result);
            return result;
        }
    }

    // 认证失败
    m_failCount++;
    emit statusMessage(QStringLiteral("认证失败"));
    m_lastAuthMs = QDateTime::currentMSecsSinceEpoch();
    m_busy = false;
    emit authCompleted(result);
    return result;
}

bool AuthEngine::logoutSession()
{
    emit statusMessage(QStringLiteral("清理本机会话：GET http://172.30.255.2/F.htm"));

    // 在当前调用线程创建局部 QNAM（可能被 worker 线程调用，禁止跨线程复用 QNAM）
    QNetworkAccessManager nam;
    QEventLoop loop;
    QNetworkRequest request(QUrl(QStringLiteral("http://172.30.255.2/F.htm")));
    QTimer::singleShot(3000, &loop, &QEventLoop::quit);
    QNetworkReply *reply = nam.get(request);
    connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();

    // 注意：finished 在连接失败/超时时同样会触发，所以不能用它判定"收到响应"，
    // 必须看 error() —— 只有 NoError 才算真正拿到了网关的响应。
    const bool ok = (reply->error() == QNetworkReply::NoError);
    const QString errStr = reply->errorString();
    reply->deleteLater();

    if (ok)
        emit statusMessage(QStringLiteral("本机会话已清理"));
    else
        emit statusMessage(QStringLiteral("本机会话清理失败：%1").arg(errStr));
    return ok;
}
