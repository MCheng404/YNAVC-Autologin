#pragma once

/**
 * @file Win32Defs.h
 * @brief Win32/WinRT COM 类型前向声明
 *
 * 仅在本目录（platform/）中使用，上层代码零 #include <windows.h>
 * 基本类型通过 <windef.h> 获取（轻量级，不会拉入完整 Windows API）
 * WinRT 自定义类型（vtable/IID/函数指针）在此文件中手动定义
 */

#ifdef Q_OS_WIN

// 包含基本 Win32 类型（DWORD, HWND, HMODULE, HRESULT, GUID 等）
// <windef.h> 是轻量级头文件，不像 <windows.h> 那样拉入整个 Windows API
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windef.h>

// WinRT HSTRING（不包含在 windef.h 中）
typedef void* HSTRING;

// IInspectable（WinRT 根接口，不包含在标准 Windows SDK 头文件中）
struct IInspectable;
struct IInspectableVtbl {
    HRESULT (STDMETHODCALLTYPE *QueryInterface)(IInspectable*, const GUID&, void**);
    ULONG   (STDMETHODCALLTYPE *AddRef)(IInspectable*);
    ULONG   (STDMETHODCALLTYPE *Release)(IInspectable*);
    HRESULT (STDMETHODCALLTYPE *GetIids)(IInspectable*, ULONG*, GUID**);
    HRESULT (STDMETHODCALLTYPE *GetRuntimeClassName)(IInspectable*, HSTRING*);
    HRESULT (STDMETHODCALLTYPE *GetTrustLevel)(IInspectable*, INT32*);
};
struct IInspectable { const IInspectableVtbl *lpVtbl; };

// INetworkOperatorTetheringManagerStatics2 IID
// {5B235412-35F0-49E7-9B08-16D278FBAA42}
static const GUID IID_INetworkOperatorTetheringManagerStatics = {
    0x5B235412, 0x35F0, 0x49E7, {0x9B, 0x08, 0x16, 0xD2, 0x78, 0xFB, 0xAA, 0x42}
};

// NetworkInformation statics IID
// {5074F851-950D-4165-9C15-365619481EEA}
static const GUID IID_INetworkInformationStatics = {
    0x5074F851, 0x950D, 0x4165, {0x9C, 0x15, 0x36, 0x56, 0x19, 0x48, 0x1E, 0xEA}
};

// WinRT 函数指针类型
typedef HRESULT (STDMETHODCALLTYPE *PFN_RoGetActivationFactory)(HSTRING, const GUID&, void**);
typedef HRESULT (STDMETHODCALLTYPE *PFN_WindowsCreateString)(PCWSTR, UINT32, HSTRING*);
typedef HRESULT (STDMETHODCALLTYPE *PFN_WindowsDeleteString)(HSTRING);

// WinRT 初始化/反初始化函数指针类型
// 必须在线程调用 RoGetActivationFactory 之前先 RoInitialize，否则返回 RO_E_NOTINITIALIZED
typedef HRESULT (STDMETHODCALLTYPE *PFN_RoInitialize)(int initType);
typedef void (STDMETHODCALLTYPE *PFN_RoUninitialize)(void);

// RoInitialize 的多线程套间类型参数（RO_INIT_TYPE 枚举，值为 1）
#ifndef RO_INIT_MULTITHREADED
#define RO_INIT_MULTITHREADED 1
#endif

// INetworkOperatorTetheringManagerStatics2 vtable
struct ITetheringManagerStatics;
struct ITetheringManagerStaticsVtbl {
    HRESULT (STDMETHODCALLTYPE *QueryInterface)(ITetheringManagerStatics*, const GUID&, void**);
    ULONG   (STDMETHODCALLTYPE *AddRef)(ITetheringManagerStatics*);
    ULONG   (STDMETHODCALLTYPE *Release)(ITetheringManagerStatics*);
    HRESULT (STDMETHODCALLTYPE *GetIids)(ITetheringManagerStatics*, ULONG*, GUID**);
    HRESULT (STDMETHODCALLTYPE *GetRuntimeClassName)(ITetheringManagerStatics*, HSTRING*);
    HRESULT (STDMETHODCALLTYPE *GetTrustLevel)(ITetheringManagerStatics*, INT32*);
    HRESULT (STDMETHODCALLTYPE *GetTetheringCapabilityFromConnectionProfile)(ITetheringManagerStatics*, IInspectable*, INT32*);
    HRESULT (STDMETHODCALLTYPE *CreateFromConnectionProfile)(ITetheringManagerStatics*, IInspectable*, IInspectable**);
};
struct ITetheringManagerStatics { const ITetheringManagerStaticsVtbl *lpVtbl; };

// INetworkOperatorTetheringManager vtable
struct ITetheringManager;
struct ITetheringManagerVtbl {
    HRESULT (STDMETHODCALLTYPE *QueryInterface)(ITetheringManager*, const GUID&, void**);
    ULONG   (STDMETHODCALLTYPE *AddRef)(ITetheringManager*);
    ULONG   (STDMETHODCALLTYPE *Release)(ITetheringManager*);
    HRESULT (STDMETHODCALLTYPE *GetIids)(ITetheringManager*, ULONG*, GUID**);
    HRESULT (STDMETHODCALLTYPE *GetRuntimeClassName)(ITetheringManager*, HSTRING*);
    HRESULT (STDMETHODCALLTYPE *GetTrustLevel)(ITetheringManager*, INT32*);
    HRESULT (STDMETHODCALLTYPE *get_MaxClientCount)(ITetheringManager*, UINT32*);
    HRESULT (STDMETHODCALLTYPE *get_ClientCount)(ITetheringManager*, UINT32*);
    HRESULT (STDMETHODCALLTYPE *get_TetheringOperationalState)(ITetheringManager*, INT32*);
    HRESULT (STDMETHODCALLTYPE *GetCurrentAccessPointConfiguration)(ITetheringManager*, IInspectable**);
    HRESULT (STDMETHODCALLTYPE *ConfigureAccessPointAsync)(ITetheringManager*, IInspectable*, IInspectable**);
    HRESULT (STDMETHODCALLTYPE *StartTetheringAsync)(ITetheringManager*, IInspectable**);
    HRESULT (STDMETHODCALLTYPE *StopTetheringAsync)(ITetheringManager*, IInspectable**);
};
struct ITetheringManager { const ITetheringManagerVtbl *lpVtbl; };

// INetworkInformationStatics vtable
struct INetworkInformationStatics;
struct INetworkInformationStaticsVtbl {
    HRESULT (STDMETHODCALLTYPE *QueryInterface)(INetworkInformationStatics*, const GUID&, void**);
    ULONG   (STDMETHODCALLTYPE *AddRef)(INetworkInformationStatics*);
    ULONG   (STDMETHODCALLTYPE *Release)(INetworkInformationStatics*);
    HRESULT (STDMETHODCALLTYPE *GetIids)(INetworkInformationStatics*, ULONG*, GUID**);
    HRESULT (STDMETHODCALLTYPE *GetRuntimeClassName)(INetworkInformationStatics*, HSTRING*);
    HRESULT (STDMETHODCALLTYPE *GetTrustLevel)(INetworkInformationStatics*, INT32*);
    HRESULT (STDMETHODCALLTYPE *GetConnectionProfiles)(INetworkInformationStatics*, IInspectable**);
    HRESULT (STDMETHODCALLTYPE *GetInternetConnectionProfile)(INetworkInformationStatics*, IInspectable**);
};
struct INetworkInformationStatics { const INetworkInformationStaticsVtbl *lpVtbl; };

// IAsyncInfo vtable
struct IAsyncInfo;
struct IAsyncInfoVtbl {
    HRESULT (STDMETHODCALLTYPE *QueryInterface)(IAsyncInfo*, const GUID&, void**);
    ULONG   (STDMETHODCALLTYPE *AddRef)(IAsyncInfo*);
    ULONG   (STDMETHODCALLTYPE *Release)(IAsyncInfo*);
    HRESULT (STDMETHODCALLTYPE *get_Id)(IAsyncInfo*, UINT32*);
    HRESULT (STDMETHODCALLTYPE *get_Status)(IAsyncInfo*, INT32*);
    HRESULT (STDMETHODCALLTYPE *get_ErrorCode)(IAsyncInfo*, HRESULT*);
    HRESULT (STDMETHODCALLTYPE *Cancel)(IAsyncInfo*);
    HRESULT (STDMETHODCALLTYPE *Close)(IAsyncInfo*);
};
struct IAsyncInfo { const IAsyncInfoVtbl *lpVtbl; };

#endif // Q_OS_WIN
