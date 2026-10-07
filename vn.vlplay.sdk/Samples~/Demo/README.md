# VLPlay SDK — Demo console sample

IMGUI console exercising the whole shipped wrapper surface: sign-in / sign-out,
anti-addiction (warn / kick / status), IAP V3 (catalog / buy / history / restore),
ads, account operations, and the config passthroughs (language, orientation,
feature flags). Every `VLPlayEvents` callback is appended to the on-screen log —
including `OnNotice`, which is the SDK's only way to show the player a validation
message or a backend error since native 1.3.3, and which a game that does not
subscribe silently swallows. Use the console to device-verify your integration.

## Run it

1. Import this sample from Package Manager (VLPlay SDK ▸ Samples ▸ Demo).
2. Create your settings asset: **Assets ▸ Create ▸ VLPlay ▸ Settings**, save it as
   `Assets/Resources/VLPlaySettings.asset`, fill **your** game's `clientId` /
   `apiKey` / `clientSecret` (issued by VLPlay Ops — never another game's) and
   the per-platform Firebase config paths.
3. New empty scene → empty GameObject → add **VLPlayDemoConsole** → Play.
   - In the Editor you get the Play-Mode mock (no network).
   - On device the real native SDK runs: Login opens the native popup.
4. IAP tiles need a signed-in account **and** store-configured products
   (Google License Tester / Apple sandbox). Run **Catalog** first — **Buy**
   targets the first CMS package.

## Notes

- `AA warn` / `AA kick` go through the SDK's debug entry points so the real
  native chain runs (Android needs `SetVerboseLogging(true)` in release builds).
  Strip debug/verbose calls before shipping.
- `Config` logs the raw CMS config JSON, which carries per-game third-party
  keys — do not copy that logging into a shipped game.
- A dismissed store sheet reports `Cancelled` on **both** platforms — no charge,
  so don't show an error for it. (Earlier revisions of this file claimed iOS
  could only report `Failed`; that was wrong when it was written.)
- `UpdateGameInfo` is called from the sign-in handler, not from a tile. That is
  where a real game calls it — as soon as the character is known — because it is
  what puts `roleId` / `serverGameId` on every purchase. Without it the store
  still charges and your game server gets a delivery callback it cannot route.
