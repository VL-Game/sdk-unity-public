# VLPlay SDK for Unity (`vn.vlplay.sdk`)

Unity wrapper for the VLPlay native SDKs — auth, IAP, ads (IAA), FAB, anti-addiction (Decree 147), push. Auth, IAP, FAB, anti-addiction and push are a **thin C# bridge** over the released native binaries ([vlgame-sdk-android](https://github.com/VL-Game/vlgame-sdk-android) AAR + [vlgame-sdk-ios](https://github.com/VL-Game/vlgame-sdk-ios) xcframework). IAA is the deliberate exception from 1.6.0: the selected ad network's official Unity plugin runs mediation, while the VLPlay C# core owns reward tickets, CMS gates, reconciliation and ad events; the native ads manager stays off in every Unity build.

> **Unity 1.6.3 (2026-10-31), native 1.9.8.** AdMob module **0.1.1** (unchanged) uses Google Mobile Ads Unity plugin **11.3.0**. `VLPlayAccount.BindSocial("apple")` returns `-3` before any screen opens (signing in with Apple is unchanged); a guest upgrade needs a verified email or phone and is otherwise refused with `1100`; the iOS Podfile hook removes only complete duplicated static-framework arguments, so names such as `MTGSDKBanner` stay intact (re-export the iOS project). Android Release IL2CPP/R8/High and unsigned iOS Release/linkage are verified on Unity 6000.0.85f1. Each game still needs its own device, OAuth, IAP and server-reward acceptance. Read [the upgrade checklist](https://sdk-docs.vlplay.vn/docs/integration/unity/upgrade-1.6.0) before replacing a 1.4.4 or 1.5.x package.

> **Previous release: Unity 1.6.2 (2026-10-31), native 1.9.6** — minified Android builds keep `SdkConfig`; a Minify-on Release build passes R8 on Unity 6000.0.85f1; the iOS Podfile hook raises every Pod below iOS 15.0 for Xcode 27.

> **Binary packaging revision, 2026-10-08:** partner core **1.6.0** and AdMob **0.1.0** ship precompiled C# DLLs, not implementation `.cs`, `.asmdef` or debug symbols. Native pin and public API are unchanged. Older source packages, including **1.5.1**, are withdrawn from partner distribution. Source stays in this private repository. DLLs can still be decompiled; this is not obfuscation or a credential-protection mechanism.

> **Previous release: `v1.5.1` (2026-09-26)** — no native change (still **v1.9.2**). ⚠️ **Behaviour a game sees on Android:** `SignIn()` called right after `Init()` now resumes the stored session instead of showing the login form, and `SetVerboseLogging(false)` right after `Init()` now sticks; `SignOut()` returns before the session is gone — leave gameplay on `OnSignOut`, not on the line after `SignOut()`. In the Editor, ads are set up in the `VLPlaySettings` inspector (provider dropdown, only the fields that provider needs, an **Ads build check** section). Upgrading from 1.4.x: `OnSignIn` waits for the identity-verification popup since `v1.5.0` — see the CHANGELOG. **Distribution is the UPM tarball** — partners can install from the public git host or ask VLPlay for the versioned tarball; this source repository stays private. Docs: [sdk-docs.vlplay.vn → Integration → Unity](https://sdk-docs.vlplay.vn/docs/integration/unity/installation).
>
> **Changed in `1.6.0`: ads run only through the ad network's official Unity plugin.** The native SDK's own ads manager is off in every Unity build; with `adsEnabled` on, the build needs the network's official Unity plugin plus the VLPlay provider module (`vn.vlplay.sdk.ads.admob`, released with 1.6.0; `vn.vlplay.sdk.ads.max` is an internal preview that is not published). `adsRuntimeOwner`, the AdMob App ID fields, `admobMediationNetworks` and **VLPlay ▸ Sync Ads Dependencies** are gone. A 1.5.x game with ads on must install plugin + module (or turn ads off) to move up — an AdMob game uses `vn.vlplay.sdk.ads.admob`. See the CHANGELOG and [Ads (IAA)](#ads-iaa) below.
>
> **Partner baseline:** ship core SDK only. Leave `adsEnabled` off and do not call `VLPlayAds` until VLPlay tells you to turn ads on.
>
> 🔴 **`v1.2.5` is mandatory for any iOS game that switches ads on.** On native ≤ 1.4.4 the whole app froze the moment it touched ads — typically the `VLPlayAds.SetUserId(...)` this package tells you to make after sign-in. The native AdMob provider's `setUserId` called itself, as a tail call, so it never crashed: it spun forever on the calling thread, which on iOS *is* the Unity main thread. Last frame stays on screen, input dead, **no crash log**. Only with CMS `ads.provider = admob`; Android was never affected. No C# changed here — the fix is the binary.
>
> 🆕 **`v1.2.4` made the iOS export ship what the SDK needs.** Push Notifications capability + `VLPlay.entitlements` (without them iOS issued no device token, so `OnDeepLink` could never fire on iOS at all), a guard that refuses a `GoogleService-Info.plist` belonging to another bundle id, and a warning when `fbAppId` is empty — iOS resolves URL schemes at install time, so no CMS value repairs either of those after the build.
>
> 🆕 **`v1.2.1` stops two things that looked like SDK bugs.** `Init()` now sets `Application.runInBackground = true`, so the game no longer freezes behind the SDK's HUD or its progress spinner — untick `VLPlaySettings.runInBackground` to own that flag yourself. And `VLPlayEvents.OnDeepLink` finally carries a tapped push notification into C#: both natives had been parsing the destination and finding nobody listening, so the player tapped a notification and the game opened wherever they left off.
>
> ⚠️ **Breaking in `v1.2.0`: `VLPlayAccount.RedeemGiftcode` is gone.** Native 1.4.3 deleted the giftcode API on both platforms, so the wrapper was binding a ghost — compile-broken on iOS, a silent `ErrNativeCall` on Android. Redeem outside the SDK. See the [CHANGELOG](CHANGELOG.md).
>
> ⚠️ **Upgrade from `v1.0.1`** — it vendored native `v1.3.5`, and four defects are live in every build still running it: on iPad, touching outside the SDK's popup froze the whole app; every iOS purchase reported a revenue of zero to AppsFlyer; the IAP catalogue and purchase history came back empty in any minified build; and push-tap handling could crash the app at launch. See the [CHANGELOG](CHANGELOG.md) for what each one looked like from the player's side.
>
> Released ahead of the combined on-device certification pass, following the native SDKs' cut-then-verify precedent — the minify proof and the device matrix run against this artifact, and anything they find lands as `v1.2.x`. **Deep-link delivery has not been exercised on a device yet**: it needs a real push on both platforms.

## Requirements

- Unity **2021.3 LTS** or newer. **Release builds verified on 6000.0 LTS** — that is the one the demo project and the release matrix use. The floor is what the package *declares*: the C# is pinned to the 2021.3 language level (the type-check harness compiles at `LangVersion 9.0`) and the Editor APIs used are all 2019.3-or-older, and package import/compilation was checked on 2021.3.45f2. Mobile Release builds were measured on 6000.0.73f1 up to 1.6.1 and on 6000.0.85f1 from 1.6.2; verify your game's actual Editor version separately.
- [External Dependency Manager for Unity (EDM4U)](https://github.com/googlesamples/unity-jar-resolver) — resolves the Android transitive deps and the iOS CocoaPods.
- iOS: CocoaPods, deployment target **≥ 15.0** (the native pod requires it — a lower target fails `pod install` with *"required a higher minimum deployment target"*).
- Android: minSdk ≥ 23, and **Application Entry Point = Activity** (see constraints below). 23 is the vendored AAR's own floor (`minSdkVersion="23"` in its manifest — it was 21 up to native `1.5.1`), so a project still set to 21 now **fails the manifest merge at build time**. With ads on, the ad network's official plugin can raise it: the AppLovin MAX plugin 8.6.6 (AppLovin Android 13.6.4) needs **24**, and the build refuses a lower Minimum API Level.

## Install

**1. EDM4U.** Add it via the OpenUPM registry — put this in `Packages/manifest.json`:

```json
"scopedRegistries": [
  {
    "name": "package.openupm.com",
    "url": "https://package.openupm.com",
    "scopes": ["com.google.external-dependency-manager"]
  }
],
"dependencies": {
  "com.google.external-dependency-manager": "1.2.187"
}
```

> Google's old "Game Package Registry" (`unityregistry-pa.googleapis.com`) no longer serves EDM4U — it returns 404, and Unity reports *"Package [com.google.external-dependency-manager] cannot be found"*. Use OpenUPM.

**2. This package — online from its public git URL.** Binary releases are
published, file for file, to the public repository `VL-Game/sdk-unity-public` (one folder per
package, one tag `<package>/<version>` per release; `tools/publish-public-upm.py` does it). Unity
needs Git on the `PATH`:

```
Window ▸ Package Manager ▸ + ▸ Add package from git URL…
https://github.com/VL-Game/sdk-unity-public.git?path=/vn.vlplay.sdk#vn.vlplay.sdk/1.6.3
```

Or ask your VLPlay contact for `vn.vlplay.sdk-1.6.3.tgz`, drop it in the project, and point
`manifest.json` at it:

```json
"dependencies": {
  "vn.vlplay.sdk": "file:../ThirdParty/vn.vlplay.sdk-1.6.3.tgz"
}
```

The tarball is an optional CI input, not the partner handoff default. Build partner artifacts with
`tools/build-binary-packages.mjs`, not `npm pack` from this private source tree. The public publisher
rejects C# source/debug symbols. For source-to-DLL upgrades remove/re-add the old UPM package to
refresh its lock hash; keep the existing settings asset and `.meta`. The Editor migration repairs
its script reference without replacing credentials. If needed run **VLPlay > Migrate Settings to
Binary SDK**. Remove old `VLPlay.Sdk` entries from game asmdef `references`; the DLL is automatically
referenced (or use `precompiledReferences` for a game asmdef with `overrideReferences: true`).

<details>
<summary>Internal only — installing from git</summary>

This repo is private, so a partner cannot use this. For VLPlay developers, always
pin the tag; an untagged URL resolves to whatever `HEAD` happens to be and the
build stops being reproducible:

```
Window ▸ Package Manager ▸ + ▸ Add package from git URL…
https://github.com/VL-Game/vlgame-sdk-unity.git#v1.6.3
```

</details>

**3. Settings.** `Assets ▸ Create ▸ VLPlay ▸ Settings`, fill the per-game credentials from the VLPlay CMS/Ops, and save it at `Assets/Resources/VLPlaySettings.asset`.

## Quick start

```csharp
using VLPlay;

VLPlaySDK.Init();                                  // once, at boot
VLPlayEvents.OnSignIn += user => Debug.Log(user);  // events fire on the Unity main thread
VLPlayEvents.OnAntiAddictionKick += reason => ReturnToLobby(); // REQUIRED (Decree 147)
VLPlaySDK.SignIn();                                // opens the native login popup
```

## Layout

| Path | What |
|---|---|
| `Runtime/` | C# facade (`VLPlaySDK`, `VLPlayEvents`, `VLPlaySettings`) + per-platform bridges |
| `Runtime/Plugins/Android/VLPlaySDK.aar` | Native Android SDK (core v1.9.8) — stable filename, swap for per-game variants |
| `Runtime/Plugins/iOS/VLPlayUnityBridge.mm` | extern-C bridge compiled into the Unity-iPhone target |
| `NativeLibs~/iOS/LocalPods/VLPlaySDK/` | Vendored binary pod: `VLPlaySDK.xcframework` (core v1.9.8) + podspec — injected into the EDM4U Podfile by the build post-processor (U1) |
| `Editor/VLPlaySDKDependencies.xml` | EDM4U manifest (Android maven deps + iOS pods) |

## Ads (IAA)

> `1.6.0` (released 2026-10-07). `1.5.1` and earlier run ads through the native SDK.

In a Unity game the ad network's **official Unity plugin** runs mediation, behind the **VLPlay provider module** for that network. The VLPlay SDK keeps what talks to VLPlay's backend: reward tickets (SSV Option A), reward confirmation (`OnRewardConfirmed`), `/ads/events`, and the gates the CMS drives (kill-switch, placements, caps, age). The native SDK's own ads manager is switched off in every Unity build (`sdk_ads_host_owned` / `VLPLAY_ADS_HOST_OWNED`, written and read back by the post-processors).

| Network (`adsProvider`) | Official Unity plugin | VLPlay provider module |
|---|---|---|
| `admob` (default) | Google Mobile Ads Unity plugin **11.3.0** | `vn.vlplay.sdk.ads.admob` 0.1.1 — paired with 1.6.1, 1.6.2 and 1.6.3 (`Providers~/`, its own tarball, public git host) |
| `max` | AppLovin MAX Unity plugin **8.6.6** | `vn.vlplay.sdk.ads.max` 0.1.0 — **internal preview, not published** (no AppLovin account, never run on a device); the catalog lists no released module for partner installation |

**Turn ads on (MAX — internal only until the module is released)**

1. Import the official plugin at the module's pin (`AppLovin-MAX-Unity-Plugin-8.6.6-…unitypackage`). The module `vn.vlplay.sdk.ads.max` comes in step 3.
2. AppLovin ▸ Integration Manager: the SDK key, and every mediated-network adapter the MAX dashboard enables (the plugin writes their dependencies and iOS SKAdNetwork ids).
3. `VLPlaySettings`: `adsEnabled` on, **Ad network** = AppLovin MAX — picking it installs `vn.vlplay.sdk.ads.max` (below); Player Settings ▸ Android ▸ Minimum API Level **24**. The setup checklist at the top of the inspector shows each of these and fixes what the Editor can.
4. Ad units and placements come from the CMS `ads` block; the CMS `ads.provider` must be `max` too.

**Turn ads on (AdMob)**

1. Import the official plugin at the module's pin (`GoogleMobileAds-v11.3.0.unitypackage`; the OpenUPM package `com.google.ads.mobile` 11.3.0 carries the same assemblies but no build has used it yet). The module `vn.vlplay.sdk.ads.admob` comes in step 3. 11.3.0 is the last plugin that still supports Unity 2021.3 (11.4.0 and later declare 2022.3).
2. Assets ▸ Google Mobile Ads ▸ Settings: the AdMob App ID of the Android app and of the iOS app — each platform has its own. The plugin writes them into the build (`APPLICATION_ID`, `GADApplicationIdentifier`) along with Google's SKAdNetwork ids; mediated networks come as Google's own adapter packages.
3. `VLPlaySettings`: `adsEnabled` on, **Ad network** = AdMob (Google) — picking it installs `vn.vlplay.sdk.ads.admob` (below). No Android floor beyond the core's 23. **Ask for consent (Google UMP)** stays off unless the game ships in the EEA/UK.
4. Ad units and placements come from the CMS `ads` block; the CMS `ads.provider` must be `admob` too. A rewarded placement with `ssv:true` sends the reward ticket as the ad's SSV custom data (the ad unit's SSV callback must point at VLPlay's backend for the game's environment).

**Picking a network installs its module.** The **Ad network** dropdown lists every network with a released provider module (`VLPlayAdsCatalog.Vendor.ModuleVersion`). Picking one removes the other networks' modules and adds its own through Package Manager (`Client.AddAndRemove`, one resolve), from where the core came from: `Providers~/` of a repo checkout (`file:` folder), `vn.vlplay.sdk.ads.<network>-<version>.tgz` next to a core tarball, or `?path=/vn.vlplay.sdk.ads.<network>#vn.vlplay.sdk.ads.<network>/<version>` on the public git host. Only once the network's official plugin is in the project — a module without its plugin stops the project compiling; the checklist then offers **Install**. Leftover modules get **Remove**. The old network's plugin is removed by hand. The rules are `VLPlayAdsSetupView.ModuleSource` / `PlanModules`; switching networks end to end is in the docs (Installation §6, *Switching to another ad network*).

**The build refuses:** `adsEnabled` on without the plugin, without the module, with another network's plugin or module, with the plugin at another version than the module's pin, with an empty MAX SDK key, with an AdMob App ID that is missing, is an ad unit id, or is the same on both platforms, with Google's experimental Next-Gen Android SDK selected in the AdMob plugin settings, or below the module's Android floor — and `adsEnabled` off with an ad plugin or a module still in the project. `Assets/VLPlay/Editor/VLPlayAdsDependencies.xml` left by 1.5.x is refused too (the checklist deletes it): it would pull the ad SDK a second time.

Reward integrity is unchanged: for a game with a server (`ssv:true`), grant only on `OnRewardConfirmed`, never on `OnUserRewarded`. Consent: AdMob runs Google's UMP consent flow only when `VLPlaySettings.admobConsentEnabled` is on — off by default, as VLPlay games ship in Vietnam; turn it on for a game distributed in the EEA/UK and publish the consent messages in AdMob ▸ Privacy & messaging.

## Known constraints (inherited from the native binaries)

- **Android: Application Entry Point must be `Activity`, not `GameActivity`** (*Player Settings ▸ Android ▸ Publishing Settings*). The native SDK's login/identity/upgrade screens are classic `Activity`s and its input handling assumes the classic entry point.
- **iOS: still don't ship the Firebase Unity SDK or the AppsFlyer Unity plugin — but the reason changed at native 1.5.0.** Since the vendored **1.5.0** binary is a **static** framework baking **zero** vendor classes (measured with `nm -gU`; on ≤1.4.5 it baked 59 AppsFlyer + 20 Firebase/GoogleUtilities classes, and two Mach-O images could carry two implementations of the same class with a green build). The vendor code now arrives once, via CocoaPods, from this package's own dependency list. What survives every version is the **ownership** rule: the SDK owns the `[AppsFlyerLib shared]` singleton (CMS dev key, `start`, the `isStopped` kill-switch) and configures `FIRApp`/`GIDSignIn` — a Unity plugin that initialises its own copy races ours regardless of how the classes got linked. Send your own events with `VLPlaySDK.LogEvent`.

  Externalising the two out of the xcframework is native-side work (tracked as `NAT-IOS-EMBEDS-SHARED-DEPS`); until it lands, keep the Firebase Unity SDK and the AppsFlyer Unity plugin out of a project that ships this package on iOS.
- **Don't install the AppsFlyer Unity plugin** — beyond the duplicate-class point above, on **both** platforms the native SDK owns the AppsFlyer instance: it calls `init` / `start` / `stop` itself (with the dev key from the CMS, and honouring the CMS kill-switch). A second owner calling `initSDK`/`startSDK`/`stopSDK` races it — last writer wins, sessions can double or tracking can be stopped from under the game. Use `VLPlaySDK.LogEvent` instead (the CMS afEvents remap/whitelist applies).
- **Ads need the network's official Unity plugin** (1.6.0+, see [Ads (IAA)](#ads-iaa)) — and only that one: two ad plugins in one build are refused. On released 1.5.x, do **not** install it: there the native SDK runs ads and would initialise the same vendor SDK.

## Troubleshooting

**iOS: app builds fine but dies at launch —** `dyld: Library not loaded: @rpath/FBSDKCoreKit.framework/FBSDKCoreKit`

EDM4U emits `use_frameworks! :linkage => :static` by default, but FBSDK and OneSignal ship as prebuilt *dynamic* xcframeworks. CocoaPods can't re-link those statically, so under static linkage it skips the `[CP] Embed Pods Frameworks` phase and nothing copies them into the `.app` — the build still succeeds, then dyld fails at launch. The iOS build post-processor rewrites the Podfile to plain `use_frameworks!` for you (look for the `[VLPlay]` warning in the build log). If you generate the Podfile yourself, keep the linkage **dynamic**.

**iOS: `pod install` fails with "required a higher minimum deployment target" —** set *Player Settings ▸ iOS ▸ Target minimum iOS Version* to **15.0** or higher.

**Android: `Package [com.google.external-dependency-manager] cannot be found` —** the Google registry is dead; install EDM4U from OpenUPM (see Install).

## Updating the native binaries

```
Android: cp vlgame-lib-sdk/sdk/android-sdk/core/<ver>/VLPlaySDK-core-<ver>.aar Runtime/Plugins/Android/VLPlaySDK.aar
iOS:     unzip vlgame-lib-sdk/sdk/ios-sdk/core/<ver>/VLPlaySDK-core-<ver>.xcframework.zip
         → NativeLibs~/iOS/LocalPods/VLPlaySDK/VLPlaySDK.xcframework  (keep this exact name)
then bump s.version in NativeLibs~/iOS/LocalPods/VLPlaySDK/VLPlaySDK.podspec
```

The iOS UI resources need no separate step: since native v1.1.6 the xcframework nests
`VLPlaySDKResource.bundle` inside `VLPlaySDK.framework` and the SDK resolves it via
`bundleForClass`. (Pins before that shipped an `sdk-resources/` side-car here instead.)

Verify on the binary rather than the source tree — `javap -classpath <aar>/classes.jar` for the
Android API surface, `nm -U` + the shipped `Headers/` for the iOS selectors.

Unity **1.6.3** pins **native v1.9.8** (iOS `ff5c80b` / Android `c82c7af`). Previous Unity 1.6.2 vendors native 1.9.6. Native 1.9.8 handles first-top-up error `1100` by asking the player to link a verified email or phone. A charged receipt uses `reconcile-receipt`; it does not need account linking. Keep purchases pending until server delivery is confirmed; do not grant items from a local failure or ask the player to buy again.
