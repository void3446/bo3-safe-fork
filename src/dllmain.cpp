
#include "stdafx.hpp"

#pragma comment(linker, "/export:D3D11CoreCreateDevice=C:\\Windows\\System32\\d3d11.D3D11CoreCreateDevice")
#pragma comment(linker, "/export:D3D11CoreCreateLayeredDevice=C:\\Windows\\System32\\d3d11.D3D11CoreCreateLayeredDevice")
#pragma comment(linker, "/export:D3D11CoreGetLayeredDeviceSize=C:\\Windows\\System32\\d3d11.D3D11CoreGetLayeredDeviceSize")
#pragma comment(linker, "/export:D3D11CoreRegisterLayers=C:\\Windows\\System32\\d3d11.D3D11CoreRegisterLayers")
#pragma comment(linker, "/export:D3D11CreateDevice=C:\\Windows\\System32\\d3d11.D3D11CreateDevice")
#pragma comment(linker, "/export:D3D11CreateDeviceAndSwapChain=C:\\Windows\\System32\\d3d11.D3D11CreateDeviceAndSwapChain")
#pragma comment(linker, "/export:D3D11CreateDeviceForType=C:\\Windows\\System32\\d3d11.D3D11CreateDeviceForType")
#pragma comment(linker, "/export:D3D11On12CreateDevice=C:\\Windows\\System32\\d3d11.D3D11On12CreateDevice")
#pragma comment(linker, "/export:D3D11On12CreateDeviceAndSwapChain=C:\\Windows\\System32\\d3d11.D3D11On12CreateDeviceAndSwapChain")

bool patched = false;
HMODULE current_module = nullptr;

DWORD WINAPI background_init(LPVOID lpParam)
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

    PROCESS_MEMORY_COUNTERS_EX pmc;

    while (!patched)
    {
        if (GetProcessMemoryInfo(GetCurrentProcess(), (PROCESS_MEMORY_COUNTERS*)&pmc, sizeof(pmc)))
        {
            if (pmc.PrivateUsage > settings.core.memory_threshold_bytes)
            {
                const size_t initial_hook_count = hook::hook_count;
                debug::log("memory threshold reached: private usage=%llu", pmc.PrivateUsage);

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
            }
        }

        Sleep(500);
    }

    return 0;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
    if (ul_reason_for_call == DLL_PROCESS_ATTACH)
    {
        DisableThreadLibraryCalls(hModule);
        current_module = hModule;

        HANDLE hThread = CreateThread(nullptr, 0, (LPTHREAD_START_ROUTINE)background_init, nullptr, 0, nullptr);

        if (hThread)
        {
            CloseHandle(hThread);
        }
    }
    return TRUE;
}
