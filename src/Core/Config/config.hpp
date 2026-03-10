#pragma once

#include "../../stdafx.hpp"

namespace config
{
    struct core_settings
    {
        bool show_startup_popup{ false };
        bool enable_crc{ true };
        bool enable_presence{ true };
        bool enable_instant_message{ true };
        bool enable_out_of_band{ true };
        uint64_t memory_threshold_bytes{ 3145728000ULL };
    };

    struct inventory_settings
    {
        bool override_item_quantity{ false };
        int item_quantity{ 999 };
        bool unlock_item_attachments{ false };
        bool unlock_challenge_items{ false };
        bool unlock_entitlement_backgrounds{ false };
        bool unlock_item_options{ false };
        bool unlock_item_purchases{ false };
        bool unlock_character_customization{ false };
        bool unlock_emblems_from_challenges{ false };

        bool any_enabled() const noexcept
        {
            return override_item_quantity
                || unlock_item_attachments
                || unlock_challenge_items
                || unlock_entitlement_backgrounds
                || unlock_item_options
                || unlock_item_purchases
                || unlock_character_customization
                || unlock_emblems_from_challenges;
        }
    };

    struct settings
    {
        core_settings core;
        inventory_settings inventory;
    };

    void initialize(HMODULE module_handle);
    const settings& current() noexcept;
    const char* path() noexcept;
}
