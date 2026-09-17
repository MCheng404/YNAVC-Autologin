#include "NetworkAdapter.h"

#include <QStringList>

#ifdef Q_OS_WIN
// winsock2 必须在 iphlpapi（间接包含 windows.h）之前
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#else
#include <cstring>
#endif

namespace Platform {

bool NetworkAdapter::isAirplaneMode()
{
#ifdef Q_OS_WIN
    ULONG bufSize = 0;
    GetAdaptersAddresses(AF_INET, GAA_FLAG_INCLUDE_PREFIX, nullptr, nullptr, &bufSize);
    if (bufSize == 0) return true;

    auto *adapters = reinterpret_cast<PIP_ADAPTER_ADDRESSES>(malloc(bufSize));
    if (!adapters) return true;

    if (GetAdaptersAddresses(AF_INET, GAA_FLAG_INCLUDE_PREFIX, nullptr, adapters, &bufSize) != ERROR_SUCCESS) {
        free(adapters);
        return true;
    }

    bool foundActive = false;
    for (auto *a = adapters; a; a = a->Next) {
        if (a->IfType == IF_TYPE_SOFTWARE_LOOPBACK) continue;
        if (a->OperStatus == IfOperStatusUp && !(a->Flags & IP_ADAPTER_NO_MULTICAST)) {
            foundActive = true;
            break;
        }
    }
    free(adapters);
    return !foundActive;
#else
    return false;
#endif
}

bool NetworkAdapter::hasWlanAdapter()
{
#ifdef Q_OS_WIN
    ULONG bufSize = 0;
    GetAdaptersAddresses(AF_UNSPEC, GAA_FLAG_INCLUDE_PREFIX, nullptr, nullptr, &bufSize);
    if (bufSize == 0) return false;

    auto *adapters = reinterpret_cast<PIP_ADAPTER_ADDRESSES>(malloc(bufSize));
    if (!adapters) return false;

    if (GetAdaptersAddresses(AF_UNSPEC, GAA_FLAG_INCLUDE_PREFIX, nullptr, adapters, &bufSize) != ERROR_SUCCESS) {
        free(adapters);
        return false;
    }

    bool hasWlan = false;
    for (auto *a = adapters; a; a = a->Next) {
        if (wcsstr(a->Description, L"WLAN") ||
            wcsstr(a->Description, L"Wi-Fi") ||
            wcsstr(a->Description, L"Wireless") ||
            wcsstr(a->Description, L"802.11")) {
            hasWlan = true;
            break;
        }
    }
    free(adapters);
    return hasWlan;
#else
    return false;
#endif
}

QString NetworkAdapter::getMacAddress()
{
#ifdef Q_OS_WIN
    ULONG bufSize = 0;
    GetAdaptersInfo(nullptr, &bufSize);
    if (bufSize == 0) return {};

    auto *adapter = reinterpret_cast<PIP_ADAPTER_INFO>(malloc(bufSize));
    if (!adapter) return {};

    if (GetAdaptersInfo(adapter, &bufSize) != ERROR_SUCCESS) {
        free(adapter);
        return {};
    }

    QString mac = QStringLiteral("%1:%2:%3:%4:%5:%6")
        .arg(adapter->Address[0], 2, 16, QLatin1Char('0'))
        .arg(adapter->Address[1], 2, 16, QLatin1Char('0'))
        .arg(adapter->Address[2], 2, 16, QLatin1Char('0'))
        .arg(adapter->Address[3], 2, 16, QLatin1Char('0'))
        .arg(adapter->Address[4], 2, 16, QLatin1Char('0'))
        .arg(adapter->Address[5], 2, 16, QLatin1Char('0'));

    free(adapter);
    return mac.toUpper();
#else
    return {};
#endif
}

QString NetworkAdapter::getLocalIp(const QString &gateway)
{
    Q_UNUSED(gateway)
#ifdef Q_OS_WIN
    SOCKET s = socket(AF_INET, SOCK_DGRAM, 0);
    if (s == INVALID_SOCKET) return {};

    struct sockaddr_in addr = {};
    addr.sin_family      = AF_INET;
    addr.sin_port        = htons(80);
    addr.sin_addr.s_addr = inet_addr(gateway.toUtf8().constData());

    QString localIp;
    if (connect(s, reinterpret_cast<struct sockaddr*>(&addr), sizeof(addr)) == 0) {
        struct sockaddr_in local = {};
        int slen = sizeof(local);
        getsockname(s, reinterpret_cast<struct sockaddr*>(&local), &slen);
        char buf[INET_ADDRSTRLEN] = {};
        inet_ntop(AF_INET, &local.sin_addr, buf, sizeof(buf));
        localIp = QString::fromUtf8(buf);
    }

    closesocket(s);
    return localIp;
#else
    return {};
#endif
}

bool NetworkAdapter::isUsableIp(const QString &ip)
{
    // 空串不可用
    if (ip.isEmpty()) return false;
    // 全 0 地址（未分配）不可用
    if (ip == QStringLiteral("0.0.0.0")) return false;

    // 纯 QString 解析，不引入新依赖
    QStringList parts = ip.split(QLatin1Char('.'));
    if (parts.size() != 4) return false; // 非标准 IPv4，保守判不可用

    bool ok0 = false, ok1 = false;
    int b0 = parts.at(0).toInt(&ok0);
    int b1 = parts.at(1).toInt(&ok1);
    if (!ok0 || !ok1) return false;

    // APIPA：169.254.x.x（DHCP 未就绪）
    if (b0 == 169 && b1 == 254) return false;
    // 回环地址：127.x.x.x
    if (b0 == 127) return false;

    return true;
}

} // namespace Platform
