#pragma once

#include <QVariant>
#include <QString>
#include <QNetworkReply>
#include <QList>
#include <QPair>
#include <QByteArray>

namespace Platform {

/**
 * @brief 校园网自助服务（Self）后端客户端 —— 在线设备管理
 *
 * 无状态设计：每个静态方法内部走完「GET 登录页 → POST 登录 → 目标操作」全流程。
 * QNetworkAccessManager 在调用线程内局部创建（可能被 worker 线程调用，
 * 禁止跨线程复用 QNAM，参考 AuthEngine::authenticate/logoutSession 的写法）。
 *
 * 已实测确认的接口（BASE = http://172.30.255.2:15000/Self）：
 *  - GET  /Self/login/?302=LI                  登录页，Set-Cookie: JSESSIONID
 *  - POST /Self/login/verify;jsessionid=<JSID>  登录提交（蜜罐字段 foo/bar 留空，绝不提交 code）
 *  - GET  /Self/dashboard/getOnlineList         设备列表 JSON 数组
 *  - GET  /Self/dashboard/tooffline?sessionid=  踢设备（不存在的 sid 返回 HTTP 500）
 */
class SelfServiceClient {
public:
    /** 拉取本账号在线设备。
     *  成功返回列表（每项 QVariantMap：sessionId/ip/mac/terminalType/loginTime/useTime），
     *  失败返回空列表并把原因写入 *errorOut。 */
    static QVariantList fetchDevices(const QString &account, const QString &password,
                                     QString *errorOut = nullptr);

    /** 踢指定会话下线。成功返回 true，失败返回 false 并写入 *errorOut。 */
    static bool kickDevice(const QString &account, const QString &password,
                           const QString &sessionId, QString *errorOut = nullptr);

private:
    struct Response {
        bool                         timeout     = false;
        QNetworkReply::NetworkError  error       = QNetworkReply::NoError;
        QString                      errorString;
        int                          status      = 0;
        QByteArray                   body;
        QString                      finalUrl;   // 跟随重定向后的最终 URL
    };

    static QString md5(const QString &plain);
    static QString normalizeMac(const QString &mac);
    static QString extractJsessionId(QNetworkAccessManager &nam, const QUrl &forUrl);
    static Response syncGet(QNetworkAccessManager &nam, const QUrl &url, int timeoutMs,
                            const QList<QPair<QByteArray, QByteArray>> &headers);
    static Response syncPost(QNetworkAccessManager &nam, const QUrl &url,
                             const QByteArray &body, int timeoutMs,
                             const QList<QPair<QByteArray, QByteArray>> &headers);
    static bool doLogin(QNetworkAccessManager &nam, const QString &account,
                        const QString &password, QString *jsidOut, QString *errorOut);

    static constexpr const char *kBase = "http://172.30.255.2:15000/Self";
};

} // namespace Platform
