#pragma once

#include "../../stdafx.hpp"

namespace debug
{
    void initialize(HMODULE module_handle, bool enabled) noexcept;
    void log(const char* format, ...) noexcept;
    bool enabled() noexcept;
}
