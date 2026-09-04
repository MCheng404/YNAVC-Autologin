#include "HotspotManager.h"

#ifdef Q_OS_WIN
#include <objbase.h>
#endif

#include <QDebug>

#include "model/Logger.h"

namespace Platform {

HotspotManager::HotspotManager(QObject *parent)
    : QObject(parent)
{
}

void HotspotManager::setLogger(Logger *logger)
{
    m_logger = logger;
}

void HotspotManager::logWarn(const QString &msg)
{
    if (m_logger) m_logger->log(msg);
    else qWarning() << msg;
}

void HotspotManager::logInfo(const QString &msg)
{
    if (m_logger) m_logger->log(msg);
    else qDebug() << msg;
}

HotspotManager::~HotspotManager()
{
#ifdef Q_OS_WIN
    if (m_combase) {
        FreeLibrary(m_combase);
        m_combase = nullptr;
    }
#endif
}

bool HotspotManager::ensureLoaded()
{
    if (m_initialized) return true;

#ifdef Q_OS_WIN
    m_combase = LoadLibraryW(L"combase.dll");
    if (!m_combase) {
        qWarning() << "热点：combase.dll 加载失败";
        return false;
    }

    m_pRoGetActivationFactory = reinterpret_cast<PFN_RoGetActivationFactory>(
        GetProcAddress(m_combase, "RoGetActivationFactory"));
    m_pWindowsCreateString = reinterpret_cast<PFN_WindowsCreateString>(
        GetProcAddress(m_combase, "WindowsCreateString"));
    m_pWindowsDeleteString = reinterpret_cast<PFN_WindowsDeleteString>(
        GetProcAddress(m_combase, "WindowsDeleteString"));
    m_pRoInitialize = reinterpret_cast<PFN_RoInitialize>(
        GetProcAddress(m_combase, "RoInitialize"));
    m_pRoUninitialize = reinterpret_cast<PFN_RoUninitialize>(
        GetProcAddress(m_combase, "RoUninitialize"));

    if (!m_pRoGetActivationFactory || !m_pWindowsCreateString || !m_pWindowsDeleteString
        || !m_pRoInitialize || !m_pRoUninitialize) {
        logWarn("热点：WinRT 函数获取失败");
        FreeLibrary(m_combase);
        m_combase = nullptr;
        return false;
    }

    m_initialized = true;
    return true;
#else
    return false;
#endif
}

bool HotspotManager::isAvailable()
{
    return ensureLoaded();
}

int HotspotManager::waitAsyncComplete(IInspectable *pAsyncOp, int timeoutMs)
{
#ifdef Q_OS_WIN
    if (!pAsyncOp) return -1;

    auto *pInfo = reinterpret_cast<IAsyncInfo*>(pAsyncOp);
    INT32 status = 0;
    int waited = 0;

    while (waited < timeoutMs) {
        HRESULT hr = pInfo->lpVtbl->get_Status(pInfo, &status);
        if (FAILED(hr)) {
            qWarning() << "热点：get_Status 失败 hr=" << Qt::hex << hr;
            return -1;
        }
        // WinRT AsyncStatus: 0=Started, 1=Completed, 2=Canceled, 3=Error
        if (status == 1) return 0;   // Completed
        if (status == 2) return 2;   // Canceled
        if (status == 3) return 3;   // Error

        Sleep(250);
        waited += 250;
    }

    qDebug() << "热点：轮询超时，进行二次验证";
    return -2;  // Timeout
#else
    Q_UNUSED(pAsyncOp)
    Q_UNUSED(timeoutMs)
    return -1;
#endif
}

int HotspotManager::verifyHotspotOn()
{
#ifdef Q_OS_WIN
    if (!ensureLoaded()) return -1;

    // 工作线程无 Qt 事件循环/Win32 消息泵，必须使用 MTA：
    // 先 RoInitialize(MTA)，再 CoInitializeEx(MTA)，否则 WinRT 异步完成通知无法投递
    // 仅当本次为首次初始化（返回 S_OK）才记录，避免拆掉调用方（start）已有的套间
    bool roInit = (m_pRoInitialize(RO_INIT_MULTITHREADED) == S_OK);
    HRESULT hrCo = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    bool comInit = (hrCo == S_OK);

    HSTRING hs1 = nullptr, hs2 = nullptr;
    INetworkInformationStatics *pNI = nullptr;
    ITetheringManagerStatics *pTM = nullptr;
    IInspectable *pProf = nullptr;
    ITetheringManager *pMgr = nullptr;
    INT32 state = -1;

    m_pWindowsCreateString(
        L"Windows.Networking.Connectivity.NetworkInformation", 50, &hs1);
    HRESULT hr = m_pRoGetActivationFactory(hs1, IID_INetworkInformationStatics,
                                            reinterpret_cast<void**>(&pNI));
    m_pWindowsDeleteString(hs1);

    if (SUCCEEDED(hr)) {
        hr = pNI->lpVtbl->GetInternetConnectionProfile(pNI, &pProf);
    }
    if (SUCCEEDED(hr) && pProf) {
        m_pWindowsCreateString(
            L"Windows.Networking.NetworkOperators.NetworkOperatorTetheringManager",
            67, &hs2);
        hr = m_pRoGetActivationFactory(hs2, IID_INetworkOperatorTetheringManagerStatics,
                                        reinterpret_cast<void**>(&pTM));
        m_pWindowsDeleteString(hs2);
    }
    if (SUCCEEDED(hr) && pTM) {
        hr = pTM->lpVtbl->CreateFromConnectionProfile(pTM, pProf,
                                                        reinterpret_cast<IInspectable**>(&pMgr));
    }
    if (SUCCEEDED(hr) && pMgr) {
        pMgr->lpVtbl->get_TetheringOperationalState(pMgr, &state);
    }

    if (pMgr)        pMgr->lpVtbl->Release(pMgr);
    if (pTM)         pTM->lpVtbl->Release(pTM);
    if (pProf)       pProf->lpVtbl->Release(pProf);
    if (pNI)         pNI->lpVtbl->Release(pNI);

    if (comInit) CoUninitialize();
    if (roInit)  m_pRoUninitialize();
    return state;
#else
    return -1;
#endif
}

int HotspotManager::processTetheringResult(IInspectable *pAsyncOp)
{
#ifdef Q_OS_WIN
    if (!pAsyncOp) {
        // 无异步句柄，直接做二次验证
        int st = verifyHotspotOn();
        qDebug() << "热点二次验证状态:" << st << "(2=On)";
        return st;
    }

    int st = waitAsyncComplete(pAsyncOp, 15000);
    pAsyncOp->lpVtbl->Release(pAsyncOp);

    // 不管 IAsyncInfo 返回什么，都检查 TetheringOperationalState
    int finalState = verifyHotspotOn();
    logInfo(QString("热点最终状态: %1 (2=On)").arg(finalState));
    return finalState;
#else
    Q_UNUSED(pAsyncOp)
    return -1;
#endif
}

bool HotspotManager::start()
{
#ifdef Q_OS_WIN
    if (!ensureLoaded()) {
        logWarn("热点：combase.dll 加载失败");
        return false;
    }

    // 工作线程无 Qt 事件循环/Win32 消息泵，必须使用 MTA：
    // 先 RoInitialize(MTA)，再 CoInitializeEx(MTA)，否则 WinRT 异步完成通知无法投递
    // 仅当本次为首次初始化（返回 S_OK）才记录，避免拆掉 verifyHotspotOn 已有的套间
    bool roInit = (m_pRoInitialize(RO_INIT_MULTITHREADED) == S_OK);
    HRESULT hrCo = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    bool comInit = (hrCo == S_OK);

    HRESULT hr = S_OK;
    HSTRING hsNetInfo = nullptr, hsTethMgr = nullptr;
    INetworkInformationStatics *pNetInfo = nullptr;
    ITetheringManagerStatics  *pTethStatics = nullptr;
    IInspectable *pProfile = nullptr;
    ITetheringManager *pMgr = nullptr;
    bool started = false;
    // 3. 重试获取 profile + 开启热点（profile 为空 / StartTetheringAsync 失败各重试）
    const int kMaxAttempt = 3;

    // 1. 获取 NetworkInformation statics
    m_pWindowsCreateString(
        L"Windows.Networking.Connectivity.NetworkInformation", 50, &hsNetInfo);
    hr = m_pRoGetActivationFactory(hsNetInfo, IID_INetworkInformationStatics,
                                    reinterpret_cast<void**>(&pNetInfo));
    m_pWindowsDeleteString(hsNetInfo);
    if (FAILED(hr)) {
        logWarn("热点：NetworkInformation 激活失败");
        goto cleanup;
    }

    // 2. 获取 NetworkOperatorTetheringManager statics
    m_pWindowsCreateString(
        L"Windows.Networking.NetworkOperators.NetworkOperatorTetheringManager",
        67, &hsTethMgr);
    hr = m_pRoGetActivationFactory(hsTethMgr, IID_INetworkOperatorTetheringManagerStatics,
                                    reinterpret_cast<void**>(&pTethStatics));
    m_pWindowsDeleteString(hsTethMgr);
    if (FAILED(hr)) {
        logWarn("热点：TetheringManager 激活失败");
        goto cleanup;
    }

    // 3. 重试获取 profile + 开启热点（profile 为空 / StartTetheringAsync 失败各重试）
    for (int attempt = 0; attempt < kMaxAttempt && !started; ++attempt) {
        if (attempt > 0) {
            Sleep(1500);
            logWarn(QString("热点：第 %1 次重试获取连接配置文件").arg(attempt));
        }

        // 释放上一次遗留的 profile / manager
        if (pProfile) { pProfile->lpVtbl->Release(pProfile); pProfile = nullptr; }
        if (pMgr)     { pMgr->lpVtbl->Release(pMgr);         pMgr = nullptr; }

        // 3a. 获取 Internet 连接配置文件（profile 为空则重试）
        hr = pNetInfo->lpVtbl->GetInternetConnectionProfile(pNetInfo, &pProfile);
        if (FAILED(hr) || !pProfile) {
            logWarn("热点：GetInternetConnectionProfile 失败");
            continue;
        }

        // 3b. 由 profile 创建 TetheringManager
        hr = pTethStatics->lpVtbl->CreateFromConnectionProfile(
            pTethStatics, pProfile, reinterpret_cast<IInspectable**>(&pMgr));
        if (FAILED(hr) || !pMgr) {
            logWarn("热点：CreateFromConnectionProfile 失败");
            continue;
        }

        // 3c. 已开启则直接成功
        INT32 state = 0;
        pMgr->lpVtbl->get_TetheringOperationalState(pMgr, &state);
        if (state == 2) {
            logInfo("热点已在运行，无需再开启");
            started = true;
            break;
        }

        // 3d. 开启热点（失败则重试）
        IInspectable *pAsyncOp = nullptr;
        hr = pMgr->lpVtbl->StartTetheringAsync(pMgr, &pAsyncOp);
        if (FAILED(hr)) {
            logWarn("热点：StartTetheringAsync 失败");
            continue;
        }
        logInfo("热点：StartTetheringAsync 已调用，等待完成...");
        processTetheringResult(pAsyncOp);
        started = true;
    }

cleanup:
    if (pMgr)        pMgr->lpVtbl->Release(pMgr);
    if (pTethStatics) pTethStatics->lpVtbl->Release(pTethStatics);
    if (pProfile)    pProfile->lpVtbl->Release(pProfile);
    if (pNetInfo)    pNetInfo->lpVtbl->Release(pNetInfo);
    if (comInit) CoUninitialize();
    if (roInit)  m_pRoUninitialize();
    return started;
#else
    return false;
#endif
}

} // namespace Platform
