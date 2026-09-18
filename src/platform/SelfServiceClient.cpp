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
#include <QRegularExpression>
#include <QDebug>
#include <QRandomGenerator>
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

    // 统一失败出口：写 errorOut 的同时落一条日志，便于在 debug.log 里定位
    auto fail = [&](const QString &msg) -> bool {
        if (errorOut) *errorOut = msg;
        qWarning() << "[SelfService] 登录失败:" << msg;
        return false;
    };

    // 1) GET 登录页：建立会话并取 JSESSIONID（Set-Cookie）
    Response page = syncGet(nam, QUrl(loginPage), 5000,
                            {{ QByteArrayLiteral("Referer"), loginPage.toUtf8() }});
    if (page.timeout)
        return fail(QStringLiteral("获取登录页超时(5s)"));
    if (page.error != QNetworkReply::NoError)
        return fail(QStringLiteral("获取登录页失败: ") + page.errorString);

    const QString jsid = extractJsessionId(nam, QUrl(loginPage));
    if (jsid.isEmpty())
        return fail(QStringLiteral("登录页未返回 JSESSIONID Cookie"));

    // 1.5) 从登录页 HTML 中提取 checkcode
    //      经 HAR 实测确认：浏览器提交的就是登录页 HTML 里这个值。
    //      页面加载后 JS 会请求 login/randomCode 换一张验证码图片，
    //      但**不会改动** checkcode 字段，所以这里取 HTML 值即可。
    QString checkcode;
    {
        const QRegularExpression re(
            QStringLiteral("name=\"checkcode\"\\s+value=\"([^\"]*)\""));
        const QRegularExpressionMatch m = re.match(QString::fromUtf8(page.body));
        if (m.hasMatch())
            checkcode = m.captured(1);
    }
    if (checkcode.isEmpty())
        qWarning() << "[SelfService] 未能从登录页提取 checkcode，页面长度 ="
                   << page.body.size();

    // 1.7) ★★ 必须先请求一次 randomCode，否则服务端不认这次登录 ★★
    //      这是页面加载后 JS 一定会做的事：
    //        $("#codeimage").attr('src', window.ctx + 'login/randomCode?t=' + Math.random());
    //      实测确认：跳过这一步直接 POST，服务端会**静默拒绝**
    //      （302 回带 jsessionid 的登录页、响应体为空、无任何提示）；
    //      补上这一步后立刻 302 → /Self/dashboard。
    //      怀疑服务端用该请求把「当前验证码」标记为有效，缺失则视为非法流程。
    {
        const QString codeUrl = QString::fromLatin1(kBase)
            + QStringLiteral("/login/randomCode?t=0.")
            + QString::number(QRandomGenerator::global()->bounded(1000000));
        Response img = syncGet(nam, QUrl(codeUrl), 5000,
                               {{ QByteArrayLiteral("Referer"), loginPage.toUtf8() }});
        qInfo() << "[SelfService] randomCode 已请求: HTTP" << img.status
                << img.body.size() << "字节";
    }

    // 2) POST 登录提交
    //    字段与顺序严格照搬 HAR 里抓到的**成功样本**：
    //      foo=&bar=&checkcode=<HTML值>&account=<账号>&password=<MD5>&code=
    //
    //    四个踩过的坑（勿改，每一条都是实测血泪）：
    //    a) URL **必须**带 ";jsessionid=xxx"（表单 action 的原样）。HAR 里显示的
    //       /Self/login/verify 是 Chrome 从显示 URL 中剥离 ";jsessionid=" 的假象。
    //    b) **必须**提交 checkcode（值取自登录页 HTML），否则服务端静默拒绝。
    //    c) **必须先请求一次 randomCode**（见上面的 1.7 步），否则同样静默拒绝。
    //    d) 不要提交 submit —— 该按钮被 JS 置为 disabled，不会进入提交数据。
    // 1.6) 账号归一化：自助服务只认「纯账号」
    //      项目里 Settings::username() 是**认证用**账号，可能带 @unicom 这类后缀
    //      （认证接口需要），但自助服务登录必须用纯账号 ——
    //      HAR 实测确认：浏览器提交的是 "13211688719"，不是 "13211688719@unicom"。
    //      带着后缀会被服务端静默拒绝（302 回登录页、无任何提示）。
    QString loginAccount = account;
    {
        const int at = loginAccount.indexOf(QLatin1Char('@'));
        if (at > 0)
            loginAccount = loginAccount.left(at);
    }
    if (loginAccount != account)
        qInfo() << "[SelfService] 账号已归一化:" << account << "→" << loginAccount;

    const QUrl postUrl(QString::fromLatin1(kBase)
                       + QStringLiteral("/login/verify;jsessionid=") + jsid);
    QUrlQuery q;
    q.addQueryItem(QStringLiteral("foo"), QString());
    q.addQueryItem(QStringLiteral("bar"), QString());
    q.addQueryItem(QStringLiteral("checkcode"), checkcode);
    q.addQueryItem(QStringLiteral("account"), loginAccount);
    q.addQueryItem(QStringLiteral("password"), md5(password));
    q.addQueryItem(QStringLiteral("code"), QString());
    const QByteArray postBody = q.query(QUrl::FullyEncoded).toUtf8();

    qInfo() << "[SelfService] 提交登录 account=" << account
            << " checkcode=" << checkcode
            << " JSESSIONID=" << jsid.left(8);

    Response resp = syncPost(nam, postUrl, postBody, 8000,
        {{ QByteArrayLiteral("Referer"), loginPage.toUtf8() },
          { QByteArrayLiteral("Content-Type"),
            QByteArrayLiteral("application/x-www-form-urlencoded") },
          // HAR 实测：浏览器提交时会带 Origin；部分框架用它做 CSRF/来源校验，
          // 缺失时可能被静默拒绝。这里取 kBase 的 scheme://host:port 部分。
          { QByteArrayLiteral("Origin"),
            QString::fromLatin1(kBase).section(QLatin1Char('/'), 0, 2).toUtf8() }});
    if (resp.timeout)
        return fail(QStringLiteral("登录提交超时(8s)"));
    if (resp.error != QNetworkReply::NoError)
        return fail(QStringLiteral("登录提交失败: ") + resp.errorString);

    // 3) 登录成功判定：跟随重定向后最终 URL 含 "dashboard"
    //    （实测登录成功 → 302 跳转到 dashboard；失败则停留在登录页 / 返回错误页）
    const bool ok = resp.finalUrl.contains(QStringLiteral("dashboard"), Qt::CaseInsensitive);
    if (!ok) {
        // 顺带把服务端渲染的提示抠出来（例如"验证码错误！"），否则无从判断失败原因
        QString tip;
        {
            const QRegularExpression re(QStringLiteral("\\)\\)\\('([^']{1,60})'\\)"));
            const QRegularExpressionMatch m =
                re.match(QString::fromUtf8(resp.body));
            if (m.hasMatch())
                tip = m.captured(1);
        }
        return fail(QStringLiteral("登录被拒绝: HTTP=%1 finalUrl=%2%3")
                        .arg(resp.status)
                        .arg(resp.finalUrl)
                        .arg(tip.isEmpty() ? QString()
                                           : QStringLiteral("  服务端提示=") + tip));
    }

    qInfo() << "[SelfService] 登录成功";
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
