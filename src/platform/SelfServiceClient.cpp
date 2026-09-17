#include "SelfServiceClient.h"

#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QNetworkCookieJar>
#include <QNetworkCookie>
#include <QEventLoop>
#include <QTimer>
#include <QUrl>
#include <QUrlQuery>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>
#include <QCryptographicHash>

namespace Platform {

QString SelfServiceClient::md5(const QString &plain)
{
    return QString::fromLatin1(
        QCryptographicHash::hash(plain.toUtf8(), QCryptographicHash::Md5).toHex());
}

QString SelfServiceClient::normalizeMac(const QString &mac)
{
    QString out;
    out.reserve(mac.length());
    for (const QChar &c : mac) {
        if (c == QLatin1Char(':') || c == QLatin1Char('-'))
            continue;
        out.append(c.toUpper());
    }
    return out;
}

QString SelfServiceClient::extractJsessionId(QNetworkAccessManager &nam, const QUrl &forUrl)
{
    QNetworkCookieJar *jar = nam.cookieJar();
    if (!jar)
        return QString();
    for (const QNetworkCookie &c : jar->cookiesForUrl(forUrl)) {
        if (c.name().compare(QByteArrayLiteral("JSESSIONID"), Qt::CaseInsensitive) == 0)
            return QString::fromLatin1(c.value());
    }
    return QString();
}

SelfServiceClient::Response SelfServiceClient::syncGet(
    QNetworkAccessManager &nam, const QUrl &url, int timeoutMs,
    const QList<QPair<QByteArray, QByteArray>> &headers)
{
    Response r;

    QNetworkRequest req(url);
    // 跟随重定向：登录成功会 302 → dashboard，最终 URL 含 "dashboard" 作为成功判定依据
    req.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                     QNetworkRequest::NoLessSafeRedirectPolicy);
    for (const auto &h : headers)
        req.setRawHeader(h.first, h.second);

    QEventLoop loop;
    QTimer timer;
    timer.setSingleShot(true);
    bool timedOut = false;
    QObject::connect(&timer, &QTimer::timeout, &loop, [&loop, &timedOut]() {
        timedOut = true;
        loop.quit();
    });

    QNetworkReply *reply = nam.get(req);
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    timer.start(timeoutMs);
    loop.exec();

    if (timedOut) {
        reply->abort();           // 中断仍在进行的请求
        r.timeout = true;
    } else {
        timer.stop();             // 防止已触发的单发定时器在 loop 退出后悬空触发
    }

    r.error       = reply->error();
    r.errorString = reply->errorString();
    r.status      = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    r.body        = reply->readAll();
    r.finalUrl    = reply->url().toString();
    reply->deleteLater();
    return r;
}

SelfServiceClient::Response SelfServiceClient::syncPost(
    QNetworkAccessManager &nam, const QUrl &url, const QByteArray &body, int timeoutMs,
    const QList<QPair<QByteArray, QByteArray>> &headers)
{
    Response r;

    QNetworkRequest req(url);
    req.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                     QNetworkRequest::NoLessSafeRedirectPolicy);
    for (const auto &h : headers)
        req.setRawHeader(h.first, h.second);

    QEventLoop loop;
    QTimer timer;
    timer.setSingleShot(true);
    bool timedOut = false;
    QObject::connect(&timer, &QTimer::timeout, &loop, [&loop, &timedOut]() {
        timedOut = true;
        loop.quit();
    });

    QNetworkReply *reply = nam.post(req, body);
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    timer.start(timeoutMs);
    loop.exec();

    if (timedOut) {
        reply->abort();
        r.timeout = true;
    } else {
        timer.stop();
    }

    r.error       = reply->error();
    r.errorString = reply->errorString();
    r.status      = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    r.body        = reply->readAll();
    r.finalUrl    = reply->url().toString();
    reply->deleteLater();
    return r;
}

bool SelfServiceClient::doLogin(QNetworkAccessManager &nam, const QString &account,
                                const QString &password, QString *jsidOut, QString *errorOut)
{
    const QString loginPage = QString::fromLatin1(kBase) + QStringLiteral("/login/?302=LI");

    // 1) GET 登录页：建立会话并取 JSESSIONID（Set-Cookie）
    Response page = syncGet(nam, QUrl(loginPage), 5000,
                            {{ QByteArrayLiteral("Referer"), loginPage.toUtf8() }});
    if (page.timeout) {
        if (errorOut) *errorOut = QStringLiteral("获取登录页超时(5s)");
        return false;
    }
    if (page.error != QNetworkReply::NoError) {
        if (errorOut) *errorOut = QStringLiteral("获取登录页失败: ") + page.errorString;
        return false;
    }

    const QString jsid = extractJsessionId(nam, QUrl(loginPage));
    if (jsid.isEmpty()) {
        if (errorOut) *errorOut = QStringLiteral("登录页未返回 JSESSIONID Cookie");
        return false;
    }

    // 2) POST 登录提交（蜜罐字段 foo/bar 留空，绝不提交 code/checkcode）
    const QUrl postUrl(QString::fromLatin1(kBase)
                       + QStringLiteral("/login/verify;jsessionid=") + jsid);
    QUrlQuery q;
    q.addQueryItem(QStringLiteral("account"), account);
    q.addQueryItem(QStringLiteral("password"), md5(password));
    q.addQueryItem(QStringLiteral("foo"), QString());
    q.addQueryItem(QStringLiteral("bar"), QString());
    const QByteArray postBody = q.query(QUrl::FullyEncoded).toUtf8();

    Response resp = syncPost(nam, postUrl, postBody, 8000,
        {{ QByteArrayLiteral("Referer"), loginPage.toUtf8() },
          { QByteArrayLiteral("Content-Type"),
            QByteArrayLiteral("application/x-www-form-urlencoded") }});
    if (resp.timeout) {
        if (errorOut) *errorOut = QStringLiteral("登录提交超时(8s)");
        return false;
    }
    if (resp.error != QNetworkReply::NoError) {
        if (errorOut) *errorOut = QStringLiteral("登录提交失败: ") + resp.errorString;
        return false;
    }

    // 3) 登录成功判定：跟随重定向后最终 URL 含 "dashboard"
    //    （实测登录成功 → 302 跳转到 dashboard；失败则停留在登录页 / 返回错误页）
    const bool ok = resp.finalUrl.contains(QStringLiteral("dashboard"), Qt::CaseInsensitive);
    if (!ok) {
        if (errorOut) *errorOut = QStringLiteral("登录被拒绝（账号或密码错误）");
        return false;
    }

    if (jsidOut) *jsidOut = jsid;
    return true;
}

QVariantList SelfServiceClient::fetchDevices(const QString &account, const QString &password,
                                             QString *errorOut)
{
    QNetworkAccessManager nam;
    // cookie 以 nam 为父，生命周期覆盖本次调用
    QNetworkCookieJar *jar = new QNetworkCookieJar(&nam);
    nam.setCookieJar(jar);

    QString jsid;
    if (!doLogin(nam, account, password, &jsid, errorOut))
        return {};

    const QUrl url(QString::fromLatin1(kBase)
                   + QStringLiteral("/dashboard/getOnlineList;jsessionid=") + jsid);
    Response resp = syncGet(nam, url, 8000,
        {{ QByteArrayLiteral("Referer"),
            QString::fromLatin1(kBase).toUtf8() + "/dashboard/" }});
    if (resp.timeout) {
        if (errorOut) *errorOut = QStringLiteral("获取设备列表超时(8s)");
        return {};
    }
    if (resp.error != QNetworkReply::NoError) {
        if (errorOut) *errorOut = QStringLiteral("获取设备列表失败: ") + resp.errorString;
        return {};
    }
    if (resp.status != 200) {
        if (errorOut) *errorOut = QStringLiteral("获取设备列表 HTTP ") + QString::number(resp.status);
        return {};
    }

    QJsonParseError jerr;
    const QJsonDocument doc = QJsonDocument::fromJson(resp.body, &jerr);
    if (jerr.error != QJsonParseError::NoError || !doc.isArray()) {
        if (errorOut) *errorOut = QStringLiteral("设备列表 JSON 解析失败: ") + jerr.errorString();
        return {};
    }

    QVariantList list;
    list.reserve(doc.array().size());
    for (const QJsonValue &v : doc.array()) {
        if (!v.isObject())
            continue;
        const QJsonObject o = v.toObject();
        QVariantMap m;
        m[QStringLiteral("sessionId")]    = o.value(QLatin1String("sessionId")).toVariant();
        m[QStringLiteral("ip")]           = o.value(QLatin1String("ip")).toVariant();
        m[QStringLiteral("mac")]          = o.value(QLatin1String("mac")).toVariant();
        m[QStringLiteral("terminalType")] = o.value(QLatin1String("terminalType")).toVariant();
        m[QStringLiteral("loginTime")]    = o.value(QLatin1String("loginTime")).toVariant();
        m[QStringLiteral("useTime")]      = o.value(QLatin1String("useTime")).toVariant();
        list.append(m);
    }
    return list;
}

bool SelfServiceClient::kickDevice(const QString &account, const QString &password,
                                   const QString &sessionId, QString *errorOut)
{
    QNetworkAccessManager nam;
    QNetworkCookieJar *jar = new QNetworkCookieJar(&nam);
    nam.setCookieJar(jar);

    QString jsid;
    if (!doLogin(nam, account, password, &jsid, errorOut))
        return false;

    QUrl url(QString::fromLatin1(kBase)
             + QStringLiteral("/dashboard/tooffline;jsessionid=") + jsid);
    QUrlQuery q;
    q.addQueryItem(QStringLiteral("sessionid"), sessionId);
    url.setQuery(q);

    Response resp = syncGet(nam, url, 8000,
        {{ QByteArrayLiteral("Referer"),
            QString::fromLatin1(kBase).toUtf8() + "/dashboard/" }});
    if (resp.timeout) {
        if (errorOut) *errorOut = QStringLiteral("踢设备超时(8s)");
        return false;
    }
    if (resp.error != QNetworkReply::NoError) {
        if (errorOut) *errorOut = QStringLiteral("踢设备失败: ") + resp.errorString;
        return false;
    }
    // 实测：传不存在的 sid 返回 HTTP 500；正常踢除应非 500（多为 200/302）
    if (resp.status == 500) {
        if (errorOut) *errorOut = QStringLiteral("踢设备失败(HTTP 500，可能会话不存在)");
        return false;
    }
    return true;
}

} // namespace Platform
