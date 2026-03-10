#include "config.hpp"

namespace
{
    config::settings g_settings{};
    std::string g_config_path;

    constexpr const char* default_config_contents =
        "; BO3 Safe Fork configuration\r\n"
        "; This file lives next to d3d11.dll in your BO3 install folder.\r\n"
        "; Missing or malformed values fall back to the safe defaults below.\r\n"
        "; Restart BO3 after changing this file.\r\n"
        "\r\n"
        "[core]\r\n"
        "; show_startup_popup: shows the module startup popup from the original project.\r\n"
        "show_startup_popup=false\r\n"
        "; enable_debug_logging: writes bo3_patch.log next to the DLL for local diagnostics.\r\n"
        "; Leave this false in normal use.\r\n"
        "enable_debug_logging=false\r\n"
        "; enable_crc: preserves the original debug-register hook activation path.\r\n"
        "enable_crc=true\r\n"
        "; enable_presence: keeps the handshake-related Live_PresenceParty patch enabled.\r\n"
        "enable_presence=true\r\n"
        "; enable_instant_message: keeps the original instant message filter hook enabled.\r\n"
        "enable_instant_message=true\r\n"
        "; enable_out_of_band: keeps the original out-of-band voice packet filter hook enabled.\r\n"
        "enable_out_of_band=true\r\n"
        "; memory_threshold_bytes: wait until BO3 reaches this private memory size before installing hooks.\r\n"
        "memory_threshold_bytes=3145728000\r\n"
        "\r\n"
        "[inventory]\r\n"
        "; override_item_quantity: enables the original quantity override hook.\r\n"
        "; Leave this false for legit-safe play.\r\n"
        "override_item_quantity=false\r\n"
        "; item_quantity: value returned by the quantity override hook when enabled.\r\n"
        "item_quantity=999\r\n"
        "; unlock_item_attachments: unlocks attachment count and attachment lock checks.\r\n"
        "unlock_item_attachments=false\r\n"
        "; unlock_challenge_items: unlocks challenge-based item checks.\r\n"
        "unlock_challenge_items=false\r\n"
        "; unlock_entitlement_backgrounds: unlocks entitlement background checks.\r\n"
        "unlock_entitlement_backgrounds=false\r\n"
        "; unlock_item_options: unlocks item option lock checks.\r\n"
        "unlock_item_options=false\r\n"
        "; unlock_item_purchases: bypasses purchase checks.\r\n"
        "unlock_item_purchases=false\r\n"
        "; unlock_character_customization: unlocks character customization checks.\r\n"
        "unlock_character_customization=false\r\n"
        "; unlock_emblems_from_challenges: unlocks emblem/backing challenge checks.\r\n"
        "unlock_emblems_from_challenges=false\r\n";

    bool read_bool(const char* section, const char* key, bool default_value) noexcept
    {
        return GetPrivateProfileIntA(section, key, default_value ? 1 : 0, g_config_path.c_str()) != 0;
    }

    int read_int(const char* section, const char* key, int default_value) noexcept
    {
        return GetPrivateProfileIntA(section, key, default_value, g_config_path.c_str());
    }

    uint64_t read_u64(const char* section, const char* key, uint64_t default_value) noexcept
    {
        char buffer[64]{};
        char default_buffer[64]{};
        _ui64toa_s(default_value, default_buffer, _countof(default_buffer), 10);

        GetPrivateProfileStringA(section, key, default_buffer, buffer, static_cast<DWORD>(_countof(buffer)), g_config_path.c_str());

        char* end = nullptr;
        const auto value = _strtoui64(buffer, &end, 10);

        return (end && end != buffer && *end == '\0') ? value : default_value;
    }

    void write_default_config_if_missing() noexcept
    {
        DWORD attributes = GetFileAttributesA(g_config_path.c_str());
        if (attributes != INVALID_FILE_ATTRIBUTES)
        {
            return;
        }

        HANDLE file = CreateFileA(
            g_config_path.c_str(),
            GENERIC_WRITE,
            FILE_SHARE_READ,
            nullptr,
            CREATE_NEW,
            FILE_ATTRIBUTE_NORMAL,
            nullptr);

        if (file == INVALID_HANDLE_VALUE)
        {
            return;
        }

        DWORD bytes_written = 0;
        WriteFile(
            file,
            default_config_contents,
            static_cast<DWORD>(strlen(default_config_contents)),
            &bytes_written,
            nullptr);

        CloseHandle(file);
    }

    void load_settings() noexcept
    {
        g_settings.core.show_startup_popup = read_bool("core", "show_startup_popup", false);
        g_settings.core.enable_debug_logging = read_bool("core", "enable_debug_logging", false);
        g_settings.core.enable_crc = read_bool("core", "enable_crc", true);
        g_settings.core.enable_presence = read_bool("core", "enable_presence", true);
        g_settings.core.enable_instant_message = read_bool("core", "enable_instant_message", true);
        g_settings.core.enable_out_of_band = read_bool("core", "enable_out_of_band", true);
        g_settings.core.memory_threshold_bytes = read_u64("core", "memory_threshold_bytes", 3145728000ULL);

        g_settings.inventory.override_item_quantity = read_bool("inventory", "override_item_quantity", false);
        g_settings.inventory.item_quantity = read_int("inventory", "item_quantity", 999);
        g_settings.inventory.unlock_item_attachments = read_bool("inventory", "unlock_item_attachments", false);
        g_settings.inventory.unlock_challenge_items = read_bool("inventory", "unlock_challenge_items", false);
        g_settings.inventory.unlock_entitlement_backgrounds = read_bool("inventory", "unlock_entitlement_backgrounds", false);
        g_settings.inventory.unlock_item_options = read_bool("inventory", "unlock_item_options", false);
        g_settings.inventory.unlock_item_purchases = read_bool("inventory", "unlock_item_purchases", false);
        g_settings.inventory.unlock_character_customization = read_bool("inventory", "unlock_character_customization", false);
        g_settings.inventory.unlock_emblems_from_challenges = read_bool("inventory", "unlock_emblems_from_challenges", false);
    }
}

namespace config
{
    void initialize(HMODULE module_handle)
    {
        char module_path[MAX_PATH]{};
        if (GetModuleFileNameA(module_handle, module_path, static_cast<DWORD>(_countof(module_path))) == 0)
        {
            g_config_path = "bo3_patch.ini";
            load_settings();
            return;
        }

        std::string path_value(module_path);
        const size_t separator = path_value.find_last_of("\\/");
        if (separator == std::string::npos)
        {
            g_config_path = "bo3_patch.ini";
        }
        else
        {
            g_config_path = path_value.substr(0, separator + 1) + "bo3_patch.ini";
        }

        write_default_config_if_missing();
        load_settings();
    }

    const settings& current() noexcept
    {
        return g_settings;
    }

    const char* path() noexcept
    {
        return g_config_path.c_str();
    }
}
