# VLPlay Ads — Google AdMob provider (`vn.vlplay.sdk.ads.admob`)

> **0.1.1 (core 1.6.1):** fixes the Editor runtime DLL location while preserving its GUID. Google plugin remains 11.3.0. Android IL2CPP/R8/High and unsigned iOS Release/linkage are verified on Unity6000.0.73f1.

> **0.1.0, released with core 1.6.0 on 2026-10-07.** Device scope so far: Google test ads shown on an
> iPhone through this module (06/10); rewarded/interstitial close, next load, resume and
> Android are on the QA checklist in the handoff, and the rewarded-ad server confirmation
> (SSV, `OnRewardConfirmed`) has not been exercised end to end — a game with a server runs
> that acceptance on staging before it grants on it (handoff §6).

Registers the **official Google Mobile Ads Unity Plugin** as the one mediation engine
behind `VLPlayAds`. Since VLPlay core 1.6.0 that is the only way a Unity game runs ads:
the native SDK's own ads manager is off in every Unity build. Game code does not change:
it keeps calling `VLPlayAds` / `VLPlayEvents`. Policy (placements, caps, cooldowns,
kill-switch, reward tickets, reconcile, `/ads/events`) stays in the VLPlay core.

## What a build needs

| Piece | Where it comes from |
|---|---|
| VLPlay core `vn.vlplay.sdk` | **1.6.1 or later** (`compatibleCore >=1.6.1 <2.0.0`; 1.6.0 introduced the provider seam) |
| Official Google Mobile Ads Unity Plugin | **exactly** `officialPluginVersion` of `VLPlayAdsProvider.json` (11.3.0) |
| AdMob App ID — Android **and** iOS | Assets ▸ Google Mobile Ads ▸ Settings; each platform has its own app and App ID |
| Player Settings ▸ Android ▸ Minimum API Level | **23 or higher** — the core's own floor; this module adds none |
| Player Settings ▸ iOS ▸ Target minimum iOS Version | **15.0 or higher** (the VLPlaySDK pod's floor; Google-Mobile-Ads-SDK 13.x needs only 12.0) |
| `Assets/Plugins/Android` | must exist before External Dependency Manager resolves — the setup checklist creates it |
| Mediated-network adapters | Google's own adapter packages for each network the AdMob mediation group uses |
| `VLPlaySettings.adsEnabled` | on |
| `VLPlaySettings.adsProvider` + CMS `ads.provider` | `admob` |
| Placements, ad unit ids, test devices | CMS `ads` block, at runtime |

The build refuses anything else: no module, two modules, a module for another provider or
core, a plugin at another version, a bootstrap type that did not compile (usually: the Google
plugin is missing), an AdMob App ID that is empty, that is an ad unit id (it has a `/` where an
App ID has a `~`), or that is the same on both platforms, an iOS target below 15.0, an Android
Gradle project without `play-services-ads` and the core's dependencies, or a generated
`sdkconfig.xml` / `VLPlaySDK-Info.plist` that does not switch the native ads manager off.

A wrong App ID is refused because it does not fail on its own: the plugin accepts it, the build
succeeds, the app launches, and on the platform that got it wrong the Google SDK never starts —
no crash, no ad. An empty iOS App ID is refused before the export; the plugin itself notices only
in a post-process, after the Xcode project was written.

### Why plugin 11.3.0

11.3.0 is the last Google Mobile Ads Unity plugin that declares Unity 2019.4; 11.4.0 and 11.5.0
declare 2022.3, above the 2021.3 the VLPlay core supports (measured on OpenUPM, 2026-09-28). The
Android SDK is the same in all three (`play-services-ads` 25.4.0); iOS asks for
`Google-Mobile-Ads-SDK ~> 13.6` (11.5.0: `~> 13.9`), and CocoaPods resolves the newest 13.x either
way.

## How it differs from the MAX module

Google hands every impression its own ad object and raises that object's callbacks only. A
reward, a revenue or a dismiss therefore always belongs to the show that owns the object — the
module binds each show to its ad and needs no settle window. A revenue or reward Google sends
after the close still reaches its own show (the closed ad is released only when the next show on
the unit starts). A load may run while the unit's ad is on screen; the new fill is reported when
that ad ends.

The SSV ticket goes on the loaded ad as `ServerSideVerificationOptions` (custom data = the
ticket, user id = the player) right before show, only for a placement with `ssv:true`. AdMob
tickets carry no signature: Google signs its own callback, and VLPlay's backend verifies it.

Google's error codes differ by platform (Android `AdRequest.ERROR_CODE_*`, iOS `GADErrorCode`);
the module maps each to the public codes the native AdMob providers answer.

## Install

Order matters: the module's assembly compiles against the plugin's `GoogleMobileAds*.dll`, and a
project that has the module but not the plugin does not compile far enough to import anything.

1. Create `Assets/Plugins/Android` if it does not exist (External Dependency Manager cannot create it).
2. Import the official plugin `GoogleMobileAds-v11.3.0.unitypackage`
   (SHA-256 `88a8591cdd350bb530cc73668356320dc9ff3e2134eb492ca6effe159739ae69`) from
   <https://github.com/googleads/googleads-mobile-unity/releases/tag/v11.3.0>. It carries External
   Dependency Manager 1.2.188; a project that already has EDM4U as a UPM package leaves that
   folder out of the import.
3. Add the module to `Packages/manifest.json`, next to `vn.vlplay.sdk` 1.6.1+ — from the same place
   the core came from: the `vn.vlplay.sdk.ads.admob-0.1.1.tgz` tarball next to the core tarball
   (`"vn.vlplay.sdk.ads.admob": "file:../ThirdParty/vn.vlplay.sdk.ads.admob-0.1.1.tgz"`), the public
   git host (`https://github.com/VL-Game/sdk-unity-public.git?path=/vn.vlplay.sdk.ads.admob#vn.vlplay.sdk.ads.admob/0.1.1`),
   or, for a VLPlay checkout, a `file:` path to this folder. Picking **AdMob (Google)** in the
   `VLPlaySettings` inspector writes that line for you.
4. Assets ▸ Google Mobile Ads ▸ Settings: the Android and the iOS App ID.
5. Open the `VLPlaySettings` asset and clear every red line of the setup checklist.

## Layout

- `Runtime/AdmobAdsRuntime.cs` — the engine: holders per format + ad unit, no double load, each
  show bound to its ad object, Google codes → the nine public codes. No Google types — driven by
  `tools/csharp-harness~/module-tests` in the core repo.
- `Runtime/GoogleMobileAdsApi.cs` — the only file naming Google types.
- `Runtime/AdmobProviderBootstrap.cs` — registers the engine at `SubsystemRegistration`.
- `VLPlayAdsProvider.json` — what the core's build guard checks.

Real-Unity check: `tools/verify-admob-unity.sh <GoogleMobileAds-v11.3.0.unitypackage>` in the core
repo (`RELEASE=1` adds an IL2CPP + R8 APK and an Xcode Release app).
