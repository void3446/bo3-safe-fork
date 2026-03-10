
#pragma once

static inline uintptr_t module_base() noexcept
{
    static uintptr_t base = reinterpret_cast<uintptr_t>(GetModuleHandle(nullptr));

    return base;
}

#define BASE_OFFSET(address) (module_base() + (uintptr_t)(address))