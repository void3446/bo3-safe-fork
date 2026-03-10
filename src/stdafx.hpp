#pragma once

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <TlHelp32.h>  

#include <intrin.h>
#include <cstring>      
#include <cstdlib>
#include <string>

#include <cstdint>
#include <type_traits>
#include <functional>
#include <psapi.h>

#pragma comment(lib, "Psapi.lib")
#pragma comment(lib, "Shlwapi.lib")
#pragma comment(lib, "dsound.lib")

#include "Core/Engine/engine.hpp"
#include "Core/Engine/crc/crc.hpp"
#include "Core/Config/config.hpp"
#include "Core/Debug/debug.hpp"
#include "Utility/build/hook/hook.hpp"

#include "Core/Hook/hook.hpp"
