#pragma once

#include <QString>
#include <optional>

namespace Platform {

/**
 * @brief 网卡检测工具：飞行模式/WLAN/MAC/IP
 *
 * 所有方法为静态方法，封装 Win32 GetAdaptersAddresses / GetAdaptersInfo
 */
class NetworkAdapter {
public:
    /** 检测是否处于飞行模式（无活跃非回环网卡） */
    static bool isAirplaneMode();

    /** 检测是否有 WLAN 适配器 */
    static bool hasWlanAdapter();

    /** 获取第一个非回环网卡的 MAC 地址（格式: XX:XX:XX:XX:XX:XX） */
    static QString getMacAddress();

    /** 获取连接到指定网关的本地 IP 地址 */
    static QString getLocalIp(const QString &gateway);

    /** IP 是否可用于校园网认证：
     *  - 空字符串 / "0.0.0.0"           → 不可用
     *  - 169.254.x.x（DHCP 未就绪时的 APIPA 自动私有地址）→ 不可用
     *  - 127.x.x.x（回环）              → 不可用
     *  其余视为可用。 */
    static bool isUsableIp(const QString &ip);
};

} // namespace Platform
