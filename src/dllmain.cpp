
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

                if (settings.inventory.any_enabled())
                {
                    inventory::runtime();
                }

                if (settings.core.enable_instant_message)
                {
                    instant_message::runtime();
                }

                if (settings.core.enable_out_of_band)
                {
                    out_of_band::runtime();
                }

                if (settings.core.enable_presence)
                {
                    presence::runtime();
                }

                if (hook::hook_count > initial_hook_count)
                {
                    if (settings.core.enable_crc)
                    {
                        crc::runtime();
                    }
                    else
                    {
                        hook::enable_all();
                    }
                }

                patched = true;
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
