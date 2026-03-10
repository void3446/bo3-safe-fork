# BO3 Safe Fork

This fork keeps the third-party BO3 `d3d11.dll` startup fix but makes the inventory-related behavior configurable instead of hardwired.

Upstream reference:

- Original project: `https://github.com/pressanykeytocontinueproject/Hello-World-`
- This fork keeps the BO3 startup fix path and removes the always-on unlock behavior by default.

The default public-safe setup is:

- BO3 still autoloads `d3d11.dll` from the game folder.
- The handshake-related patch path stays enabled.
- Inventory unlock hooks stay off by default.
- The item quantity override remains optional through `bo3_patch.ini`.
- `BO3 Safe` starts T7Patch before launching the game.
- `BO3 Direct` launches the game without starting T7Patch.

Both launchers still use the custom `d3d11.dll`, because the DLL lives in the BO3 install folder and is loaded automatically by the game.

## Quick install

1. Make sure your BO3 install is already in the downgraded-compatible state required by the upstream fix.
2. Extract this package anywhere.
3. Run `powershell -NoProfile -ExecutionPolicy Bypass -File .\install.ps1`.
4. If prompted, confirm your BO3 folder path.
5. If you want the T7-backed launcher, provide the path to `t7patch_2.04.exe`.
6. Choose whether to enable the item quantity override.
7. Use the desktop launchers the installer creates:
   `BO3 Safe.cmd`
   `BO3 Direct.cmd`

## Config file

The installer writes `bo3_patch.ini` into your BO3 install folder.

Important options:

- `override_item_quantity`
  Turns on the upstream quantity override hook.
- `enable_debug_logging`
  Writes `bo3_patch.log` next to the DLL for local diagnostics. Leave this `false` in normal use.
- `item_quantity`
  The quantity used when that override is enabled.
- `unlock_item_attachments`
  Unlocks attachment-related checks.
- `unlock_challenge_items`
  Unlocks challenge-related checks.
- `unlock_entitlement_backgrounds`
  Unlocks entitlement background checks.
- `unlock_item_options`
  Unlocks item option checks.
- `unlock_item_purchases`
  Bypasses purchase checks.
- `unlock_character_customization`
  Unlocks character customization checks.
- `unlock_emblems_from_challenges`
  Unlocks emblem/backing challenge checks.

For legit-safe play, leave every unlock option set to `false`.

## Building

1. Install Visual Studio 2022 Build Tools with the C++ workload.
2. Run `powershell -NoProfile -ExecutionPolicy Bypass -File .\build_release.ps1`.
3. The staged release files will appear in the `release` folder.

## Notes

- The project still targets `d3d11.dll` as the output name because BO3 autoloads that filename.
- If `bo3_patch.ini` is missing, the DLL creates a safe default config next to itself on first load.
- T7Patch is optional for `BO3 Direct`, but recommended for safer public play.
- The current community-friendly config default is legit-safe. The item quantity override is still available if you want it.
