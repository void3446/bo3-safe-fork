
#pragma once

#include "../../../stdafx.hpp"

namespace hook 
{
    struct hook_t 
    {
        void* target{};
        void* detour{};
        void* original{};

        bool enabled{};
    };

    constexpr size_t max_hooks = 128;
    inline hook_t hooks[max_hooks];
    inline size_t hook_count{};

    inline void write_jump(void* src, void* dst) 
    {
        DWORD old;
        VirtualProtect(src, 14, PAGE_EXECUTE_READWRITE, &old);
        unsigned char* p = (unsigned char*)src;
        p[0] = 0x49; p[1] = 0xBB;
        *reinterpret_cast<void**>(p + 2) = dst;
        p[10] = 0x41; p[11] = 0xFF; p[12] = 0xE3; p[13] = 0x90;
        VirtualProtect(src, 14, old, &old);
    }

    inline bool create(void* target, void* detour, void** trampoline = nullptr)
    {
        if (!target || !detour || hook_count >= max_hooks) return false;

        unsigned char orig[14];
        memcpy(orig, target, 14);

        void* tramp = VirtualAlloc(nullptr, 28, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
        if (!tramp) return false;

        memcpy(tramp, orig, 14);
        write_jump((unsigned char*)tramp + 14, (unsigned char*)target + 14);

        hooks[hook_count] = { target, detour, tramp, false };
        if (trampoline) *trampoline = tramp;
        hook_count++;

        return true;
    }

    inline bool enable(void* target) 
    {
        for (size_t i = 0; i < hook_count; i++)

            if (hooks[i].target == target && !hooks[i].enabled) 
            {
                write_jump(hooks[i].target, hooks[i].detour);
                hooks[i].enabled = true;
                return true;
            }

        return false;
    }

    inline bool disable(void* target)
    {
        for (size_t i = 0; i < hook_count; i++)

            if (hooks[i].target == target && hooks[i].enabled) {

                DWORD old;
                VirtualProtect(hooks[i].target, 14, PAGE_EXECUTE_READWRITE, &old);
                memcpy(hooks[i].target, hooks[i].original, 14);
                VirtualProtect(hooks[i].target, 14, old, &old);

                hooks[i].enabled = false;

                return true;
            }
        return false;
    }

    inline bool remove(void* target) 
    {
        for (size_t i = 0; i < hook_count; i++)

            if (hooks[i].target == target) 
            {
                if (hooks[i].enabled) disable(target);

                VirtualFree(hooks[i].original, 0, MEM_RELEASE);

                hooks[i] = hooks[hook_count - 1];
                hook_count--;

                return true;
            }
        return false;
    }

    inline void enable_all()
    {
        for (size_t i = 0; i < hook_count; ++i)

            if (!hooks[i].enabled) 
            {
                write_jump(hooks[i].target, hooks[i].detour);

                hooks[i].enabled = true;
            }
    }

    inline void disable_all()
    {
        for (size_t i = 0; i < hook_count; ++i)

            if (hooks[i].enabled)
            {
                DWORD old;

                VirtualProtect(hooks[i].target, 14, PAGE_EXECUTE_READWRITE, &old);
                memcpy(hooks[i].target, hooks[i].original, 14);
                VirtualProtect(hooks[i].target, 14, old, &old);

                hooks[i].enabled = false;
            }
    }


#define add_hook(target, detour, trampoline_ptr) \
do { create(target, detour, trampoline_ptr); enable(target); } while(0)

}