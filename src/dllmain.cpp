#include "stdafx.hpp"

#include <d3dcommon.h>
#include <dxgi.h>

struct ID3D11Device;
struct ID3D11DeviceContext;

#pragma comment(linker, "/export:D3D11CoreCreateDevice=C:\\Windows\\System32\\d3d11.D3D11CoreCreateDevice")
#pragma comment(linker, "/export:D3D11CoreCreateLayeredDevice=C:\\Windows\\System32\\d3d11.D3D11CoreCreateLayeredDevice")
#pragma comment(linker, "/export:D3D11CoreGetLayeredDeviceSize=C:\\Windows\\System32\\d3d11.D3D11CoreGetLayeredDeviceSize")
#pragma comment(linker, "/export:D3D11CoreRegisterLayers=C:\\Windows\\System32\\d3d11.D3D11CoreRegisterLayers")
#pragma comment(linker, "/export:D3D11CreateDeviceForType=C:\\Windows\\System32\\d3d11.D3D11CreateDeviceForType")
#pragma comment(linker, "/export:D3D11On12CreateDevice=C:\\Windows\\System32\\d3d11.D3D11On12CreateDevice")
#pragma comment(linker, "/export:D3D11On12CreateDeviceAndSwapChain=C:\\Windows\\System32\\d3d11.D3D11On12CreateDeviceAndSwapChain")

namespace
{
    bool patched = false;
    HMODULE current_module = nullptr;
    INIT_ONCE g_init_once = INIT_ONCE_STATIC_INIT;
    INIT_ONCE g_worker_once = INIT_ONCE_STATIC_INIT;
    INIT_ONCE g_d3d11_module_once = INIT_ONCE_STATIC_INIT;
    HMODULE g_system_d3d11 = nullptr;

    using d3d11_create_device_t = HRESULT(WINAPI*)(
        IDXGIAdapter*,
        D3D_DRIVER_TYPE,
        HMODULE,
        UINT,
        const D3D_FEATURE_LEVEL*,
        UINT,
        UINT,
        ID3D11Device**,
        D3D_FEATURE_LEVEL*,
        ID3D11DeviceContext**);

    using d3d11_create_device_and_swap_chain_t = HRESULT(WINAPI*)(
        IDXGIAdapter*,
        D3D_DRIVER_TYPE,
        HMODULE,
        UINT,
        const D3D_FEATURE_LEVEL*,
        UINT,
        UINT,
        const DXGI_SWAP_CHAIN_DESC*,
        IDXGISwapChain**,
        ID3D11Device**,
        D3D_FEATURE_LEVEL*,
        ID3D11DeviceContext**);

    DWORD WINAPI background_init(LPVOID)
    {
        config::initialize(current_module);
        const auto& settings = config::current();
        debug::initialize(current_module, settings.core.enable_debug_logging);
        debug::log("config loaded from %s", config::path());
        debug::log(
            "core settings: crc=%d presence=%d im=%d oob=%d quantity=%d threshold=%llu",
            settings.core.enable_crc ? 1 : 0,
            settings.core.enable_presence ? 1 : 0,
            settings.core.enable_instant_message ? 1 : 0,
            settings.core.enable_out_of_band ? 1 : 0,
            settings.inventory.override_item_quantity ? 1 : 0,
            settings.core.memory_threshold_bytes);

        if (settings.core.show_startup_popup)
        {
            MessageBoxA(NULL, "System Modules Initializing.", "Module Notification", MB_OK | MB_ICONINFORMATION | MB_SYSTEMMODAL);
        }

        PROCESS_MEMORY_COUNTERS_EX pmc{};
        while (!patched)
        {
            if (GetProcessMemoryInfo(GetCurrentProcess(), reinterpret_cast<PROCESS_MEMORY_COUNTERS*>(&pmc), sizeof(pmc)))
            {
                debug::log(
                    "background init memory check: private usage=%llu, configured threshold=%llu",
                    pmc.PrivateUsage,
                    settings.core.memory_threshold_bytes);

                if (pmc.PrivateUsage >= settings.core.memory_threshold_bytes)
                {
                    const size_t initial_hook_count = hook::hook_count;

                    if (settings.inventory.any_enabled())
                    {
                        debug::log("initializing inventory hooks");
                        inventory::runtime();
                    }

                    if (settings.core.enable_instant_message)
                    {
                        debug::log("initializing instant_message hooks");
                        instant_message::runtime();
                    }

                    if (settings.core.enable_out_of_band)
                    {
                        debug::log("initializing out_of_band hooks");
                        out_of_band::runtime();
                    }

                    if (settings.core.enable_presence)
                    {
                        debug::log("initializing presence hooks");
                        presence::runtime();
                    }

                    if (hook::hook_count > initial_hook_count)
                    {
                        if (settings.core.enable_crc)
                        {
                            debug::log("starting crc runtime with %zu registered hooks", hook::hook_count);
                            crc::runtime();
                        }
                        else
                        {
                            debug::log("crc disabled; enabling %zu hooks directly", hook::hook_count);
                            hook::enable_all();
                        }
                    }
                    else
                    {
                        debug::log("no hooks registered");
                    }

                    patched = true;
                    debug::log("background initialization complete");
                    break;
                }
            }

            Sleep(250);
        }

        return 0;
    }

    BOOL CALLBACK start_background_worker(PINIT_ONCE, PVOID, PVOID*)
    {
        HANDLE thread = CreateThread(nullptr, 0, background_init, nullptr, 0, nullptr);
        if (!thread)
        {
            return FALSE;
        }

        CloseHandle(thread);
        return TRUE;
    }

    BOOL CALLBACK initialize_patch_once(PINIT_ONCE, PVOID, PVOID*)
    {
        return InitOnceExecuteOnce(&g_worker_once, start_background_worker, nullptr, nullptr);
    }

    void ensure_initialized() noexcept
    {
        InitOnceExecuteOnce(&g_init_once, initialize_patch_once, nullptr, nullptr);
    }

    BOOL CALLBACK load_system_d3d11(PINIT_ONCE, PVOID, PVOID*)
    {
        char system_directory[MAX_PATH]{};
        if (GetSystemDirectoryA(system_directory, static_cast<UINT>(_countof(system_directory))) == 0)
        {
            return FALSE;
        }

        std::string module_path(system_directory);
        module_path += "\\d3d11.dll";

        g_system_d3d11 = LoadLibraryA(module_path.c_str());
        return g_system_d3d11 != nullptr;
    }

    HMODULE get_system_d3d11() noexcept
    {
        InitOnceExecuteOnce(&g_d3d11_module_once, load_system_d3d11, nullptr, nullptr);
        return g_system_d3d11;
    }

    template <typename T>
    T get_d3d11_proc(const char* proc_name) noexcept
    {
        HMODULE module = get_system_d3d11();
        if (!module)
        {
            return nullptr;
        }

        return reinterpret_cast<T>(GetProcAddress(module, proc_name));
    }
}

extern "C" __declspec(dllexport) HRESULT WINAPI D3D11CreateDevice(
    IDXGIAdapter* pAdapter,
    D3D_DRIVER_TYPE DriverType,
    HMODULE Software,
    UINT Flags,
    const D3D_FEATURE_LEVEL* pFeatureLevels,
    UINT FeatureLevels,
    UINT SDKVersion,
    ID3D11Device** ppDevice,
    D3D_FEATURE_LEVEL* pFeatureLevel,
    ID3D11DeviceContext** ppImmediateContext)
{
    ensure_initialized();

    const auto proc = get_d3d11_proc<d3d11_create_device_t>("D3D11CreateDevice");
    if (!proc)
    {
        return E_FAIL;
    }

    return proc(
        pAdapter,
        DriverType,
        Software,
        Flags,
        pFeatureLevels,
        FeatureLevels,
        SDKVersion,
        ppDevice,
        pFeatureLevel,
        ppImmediateContext);
}

extern "C" __declspec(dllexport) HRESULT WINAPI D3D11CreateDeviceAndSwapChain(
    IDXGIAdapter* pAdapter,
    D3D_DRIVER_TYPE DriverType,
    HMODULE Software,
    UINT Flags,
    const D3D_FEATURE_LEVEL* pFeatureLevels,
    UINT FeatureLevels,
    UINT SDKVersion,
    const DXGI_SWAP_CHAIN_DESC* pSwapChainDesc,
    IDXGISwapChain** ppSwapChain,
    ID3D11Device** ppDevice,
    D3D_FEATURE_LEVEL* pFeatureLevel,
    ID3D11DeviceContext** ppImmediateContext)
{
    ensure_initialized();

    const auto proc = get_d3d11_proc<d3d11_create_device_and_swap_chain_t>("D3D11CreateDeviceAndSwapChain");
    if (!proc)
    {
        return E_FAIL;
    }

    return proc(
        pAdapter,
        DriverType,
        Software,
        Flags,
        pFeatureLevels,
        FeatureLevels,
        SDKVersion,
        pSwapChainDesc,
        ppSwapChain,
        ppDevice,
        pFeatureLevel,
        ppImmediateContext);
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID)
{
    if (ul_reason_for_call == DLL_PROCESS_ATTACH)
    {
        DisableThreadLibraryCalls(hModule);
        current_module = hModule;
    }

    return TRUE;
}
