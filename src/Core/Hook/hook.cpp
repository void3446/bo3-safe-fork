
#include "hook.hpp"

namespace inventory
{
    inline LONG quantity_hits = 0;

    int64_t __fastcall live_inventory_get_item_quantity(int, int)
    {
        const auto value = config::current().inventory.item_quantity;
        const LONG hit = InterlockedIncrement(&quantity_hits);
        if (hit <= 32)
        {
            debug::log("live_inventory_get_item_quantity hit #%ld -> %d", hit, value);
        }

        return value;
    }

    int __fastcall bg_unlockables_get_num_item_attachments(unsigned int a1, unsigned int a2) {return 255; }
    bool __fastcall bg_unlocked_get_challenge_unlocked_for_index() { return true; }
    bool __fastcall bg_emblem_is_entitlement_background_granted() { return true; }
    bool __fastcall bg_unlockables_item_option_locked() { return false; }
    bool __fastcall bg_unlockables_is_item_attachment_locked() { return false; }
    bool __fastcall bg_unlockables_is_attachment_slot_locked() { return false; }
    bool __fastcall bg_unlockables_is_item_purchased() { return true; }
    bool __fastcall bg_unlockables_character_customization_item_locked() { return false; }
    bool __fastcall bg_unlockables_emblem_or_backing_locked_by_challenge() { return false; }

    __declspec(noinline) void runtime()
    {
        const auto& settings = config::current().inventory;

        if (settings.override_item_quantity)
        {
            const bool created = hook::create(reinterpret_cast<void*>(BASE_OFFSET(0x1E09030)), reinterpret_cast<void*>(live_inventory_get_item_quantity));
            debug::log("hook create inventory quantity: target=%p result=%d", reinterpret_cast<void*>(BASE_OFFSET(0x1E09030)), created ? 1 : 0);
        }

        if (settings.unlock_item_attachments)
        {
            debug::log("hook create inventory attachments group");
            hook::create(reinterpret_cast<void*>(BASE_OFFSET(0x26A78D0)), reinterpret_cast<void*>(bg_unlockables_get_num_item_attachments));
            hook::create(reinterpret_cast<void*>(BASE_OFFSET(0x26A88D0)), reinterpret_cast<void*>(bg_unlockables_is_item_attachment_locked));
            hook::create(reinterpret_cast<void*>(BASE_OFFSET(0x26A86D0)), reinterpret_cast<void*>(bg_unlockables_is_attachment_slot_locked));
        }

        if (settings.unlock_challenge_items)
        {
            debug::log("hook create challenge unlock group");
            hook::create(reinterpret_cast<void*>(BASE_OFFSET(0x26AF5F0)), reinterpret_cast<void*>(bg_unlocked_get_challenge_unlocked_for_index));
        }

        if (settings.unlock_entitlement_backgrounds)
        {
            debug::log("hook create entitlement backgrounds");
            hook::create(reinterpret_cast<void*>(BASE_OFFSET(0x2667520)), reinterpret_cast<void*>(bg_emblem_is_entitlement_background_granted));
        }

        if (settings.unlock_item_options)
        {
            debug::log("hook create item options");
            hook::create(reinterpret_cast<void*>(BASE_OFFSET(0x26AA6C0)), reinterpret_cast<void*>(bg_unlockables_item_option_locked));
        }

        if (settings.unlock_item_purchases)
        {
            debug::log("hook create item purchases");
            hook::create(reinterpret_cast<void*>(BASE_OFFSET(0x26A9620)), reinterpret_cast<void*>(bg_unlockables_is_item_purchased));
        }

        if (settings.unlock_character_customization)
        {
            debug::log("hook create character customization");
            hook::create(reinterpret_cast<void*>(BASE_OFFSET(0x26A2030)), reinterpret_cast<void*>(bg_unlockables_character_customization_item_locked));
        }

        if (settings.unlock_emblems_from_challenges)
        {
            debug::log("hook create emblem challenge unlocks");
            hook::create(reinterpret_cast<void*>(BASE_OFFSET(0x26A3AE0)), reinterpret_cast<void*>(bg_unlockables_emblem_or_backing_locked_by_challenge));
        }
    }
}

namespace instant_message
{
    char __fastcall ignore_friend_message(__int64 a1, unsigned int a2, __int64 a3) { return true;  }

    __declspec(noinline) void runtime()
    {
        debug::log("hook create instant_message ignore_friend_message");
        void* targets[] =
        {
            reinterpret_cast<void*>(BASE_OFFSET(0x1E7E6B0))  // ignore_friend_message
        };

        void* detours[] =
        {
            reinterpret_cast<void*>(ignore_friend_message)
        };

        for (int i = 0; i < 1; ++i)
        {
            hook::create(targets[i], detours[i]);
        }
    }
}

namespace out_of_band
{
    __int64 __fastcall CL_HandleVoiceTypePacket(__int64 a1, int a2) { return true; }

    __declspec(noinline) void runtime()
    {
        debug::log("hook create out_of_band CL_HandleVoiceTypePacket");
        hook::create(reinterpret_cast<void*>(BASE_OFFSET(0x13E3AF0)), reinterpret_cast<void*>(CL_HandleVoiceTypePacket));
    }
}

namespace presence
{
    void __fastcall Live_PresenceParty(__int64 a1, __int64 a2)
    {
        return;
    }

    __declspec(noinline) void runtime()
    {
        debug::log("hook create presence Live_PresenceParty target=%p", reinterpret_cast<void*>(BASE_OFFSET(0x1E91820)));
        void* targets[] =
        {
            reinterpret_cast<void*>(BASE_OFFSET(0x1E91820)), // Live_PresenceParty
        };

        void* detours[] =
        {
            reinterpret_cast<void*>(Live_PresenceParty),
        };

        for (int i = 0; i < 1; ++i)
        {
            hook::create(targets[i], detours[i]);
        }
    }
}
