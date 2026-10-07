# Changelog

## [1.6.0] — 2026-10-07

### Binary distribution revision — 2026-10-08

- Partner core1.6.0 and AdMob0.1.0 packages contain platform-selected precompiled C# DLLs,
  no C# implementation source, assembly definitions or debug symbols. Native1.9.5 and public APIs unchanged.
- Preserve source-based VLPlaySettings assets through an Editor script-reference migration;
  the asset GUID and credentials stay unchanged. Runtime DLLs are distinct for Editor/Android/iOS.
- Public source package history and older downloadable source tarballs (including1.5.1) are
  withdrawn with Mark's approval. Re-add a previously installed source UPM package so its lock
  hash resolves the new binary distribution. Source remains private; DLLs can be decompiled.
- Public publisher now refuses source/debug payloads. `build-binary-packages.mjs` is the partner
  packaging entry; ordinary `npm pack` of the private development tree is not a partner artifact.
- Pass the package's linker preservation file explicitly to UnityLinker so IL2CPP High
  retains the core bridges and optional AdMob module, including in a minimal host scene.

`package.json` and `VLPlayAdsProviderManifest.CoreVersion` already say **1.6.0**: the provider seam
below is the reason, and a module has to be able to name the first core that has it. The released
1.5.1 has no seam (no friend assemblies, no module guard), so the MAX module now declares
`compatibleCore >=1.6.0 <2.0.0` — it used to say `>=1.5.1`, which let it claim a core it cannot
compile against. A minor, not a patch, so a 1.5.x hotfix cut from `main` never has to pretend to
carry the seam.

### Updated — native 1.9.5 and iOS purchase callbacks

- Vendors the released core Android AAR and signed iOS XCFramework **1.9.5**
  (Android `1fc7eac`, iOS `5a006bd`). 1.9.4 brought session-owned IAP/recovery, restore
  verification and watchdog fixes, the OneLink cold-start relay and saved-link attribution;
  1.9.5 adds an English/Vietnamese row for error `1080` (the phone has no Zalo, so the OTP
  could not be sent), stops the Android legacy Support tab showing a placeholder hotline
  (it shows the game's own hotline, its call icon opens the dialer, both hidden when there is
  none), and points the iOS legacy "request account deletion" link at the Support Centre
  instead of a 404 or the game's fan page. No public API, dependency pin or minimum OS
  version changed.
- iOS `Purchase` now settles from its own native completion. A busy sale or a session
  ending during the guest-upgrade offer returns `Failed` with the native error code
  instead of leaving the callback pending. A refused same-SKU retry cannot receive the
  original sale's success or cancellation. Notification metadata still supplies the
  transaction id and store cancellation; each request replies once and removes its observer.
- iOS catalog `isActive` now matches Android's parser: JSON booleans and `true`/`false`
  strings are honored; other shapes use the default `true`, including empty strings,
  numeric zero and objects. Objects no longer cause an Objective-C exception.
- Backend changed-role/server pending-order reuse needs BE #426 deployed separately.

### Changed — ads run only through the ad network's official Unity plugin (BREAKING)

Decided 27/09: in a Unity game the ad network's official Unity plugin runs mediation, behind the
VLPlay provider module for that network. The VLPlay SDK keeps what talks to VLPlay's backend —
reward tickets, reward confirmation, `/ads/events`, the gates the CMS drives. The native SDK's own
ads manager no longer runs in a Unity build at all.

- **Every Unity build switches the native ads manager off** — `sdk_ads_host_owned = true` in
  `res/raw/sdkconfig.xml`, `VLPLAY_ADS_HOST_OWNED` = `<true/>` in `VLPlaySDK-Info.plist` — with ads
  off too, so nothing the CMS says can start it. Both post-processors read the file back and stop the
  build if the switch is missing.
- **`VLPlaySettings.adsEnabled` is the build's master switch.** On: the build needs the official
  plugin of `adsProvider` at the module's pin and exactly one provider module for it (the checks
  below). Off: no ad plugin and no provider module may be in the project — an ads-free build carries
  no ad engine. At runtime, off keeps the ads core from starting anything whatever the CMS says:
  `VLPlayAds.IsEnabled` is false and every call answers `DISABLED`, naming the setting.
- **Removed:** `adsRuntimeOwner`; `admobAppIdAndroid`, `admobAppIdIos`, `admobMediationNetworks`
  (App IDs and adapters live in the official plugin's own settings); the menu **VLPlay ▸ Sync Ads
  Dependencies** and the file it generated; VLPlay writing `APPLICATION_ID`,
  `GADApplicationIdentifier`, `SKAdNetworkItems` or `AdNetworkIdentifiers` (the official plugin's
  post-processor writes what its network needs); the wrapper's bindings to the native ads calls (the
  native backend bridge — tickets, reconcile, events — stays). An asset saved by 1.5.x keeps the
  removed keys in its YAML; Unity ignores them.
- **`Assets/VLPlay/Editor/VLPlayAdsDependencies.xml` left by 1.5.x is refused at build** — it would
  make EDM4U pull the ad SDK a second time. The setup checklist has **Delete the file**.
- **`adsProvider` defaults to `admob`** — the network released with 1.6.0. `max` keeps its row and its
  plugin checks, but `vn.vlplay.sdk.ads.max` stays an internal preview VLPlay does not publish (no
  AppLovin account, never run on a device), so the catalog lists no released module for it and a
  partner cannot install it from the checklist until one is released — the same path as LevelPlay.
- **Build guards recognize LevelPlay** by its type — `Unity.Services.LevelPlay.LevelPlay`
  (`com.unity.services.levelplay` 8.7.0 and later) or the global `IronSource` (8.0–8.10, and the
  older `.unitypackage`) — or by an `Assets/LevelPlay` / `Assets/IronSource` folder; any one of these
  is enough. LevelPlay still cannot ship until its VLPlay provider module exists, but it can no longer
  hide in an ads-off or MAX build. This includes a game that runs LevelPlay ads of its own outside
  VLPlay: with `adsEnabled` off its build is refused too.
- **Demo sample:** **Ads cfg** injects a test config for the build's `adsProvider`. AppLovin publishes
  no test ad units, so the MAX config carries placeholder unit ids to replace with the game's own.

**Who must act.** A game on 1.5.x with `adsEnabled` on (ads through the native SDK) cannot move to
1.6.0 as it is: install the network's official plugin and the VLPlay provider module, or turn
`adsEnabled` off. An AdMob game moves up with `vn.vlplay.sdk.ads.admob` (below); LevelPlay has no
module yet, so a LevelPlay game stays on 1.5.x until one ships. A game with ads off only deletes the
1.5 dependency file if it has one.

### Added — a OneLink can open the installed game (`VLPlaySettings.oneLinkDomains`)

Until now a Unity game could only receive a **deferred** OneLink (tap → install → open), which the
native SDK resolves on its own. A tap on a game that was already installed opened the browser or the
store: the build declared no link domain, and on iOS nothing handed the tap to AppsFlyer.

- **`VLPlaySettings.oneLinkDomains`** — the game's OneLink domains, comma-separated:
  `thuonglam.onelink.me/5e8b`, a branded domain, or a whole OneLink pasted from the dashboard (only
  the domain and the template id are kept). Drawn above the platform tabs with the Facebook App ID,
  since both builds use it. Empty: nothing below happens.
- **iOS** — `applinks:<domain>` (one per host) goes into the entitlements in the SAME
  `ProjectCapabilityManager` session as Push and Sign in with Apple; a second manager on the pbxproj
  would overwrite the first. `VLPlayUnityAppController` now overrides
  `application:continueUserActivity:restorationHandler:`: the SDK first, then Unity's own
  implementation (guarded by `instancesRespondToSelector:`), so `Application.deepLinkActivated` keeps
  firing.
- **Android** — one `<intent-filter android:autoVerify="true">` per domain on the launcher Activity
  (`https`, the host, the template id as `pathPrefix`). One filter per domain because the `<data>`
  of one filter combine as a cross product. The Activity becomes `singleTask` when it has no launch
  mode or `standard` (Unity's own manifest already says `singleTask`). Idempotent across exports.
  Nothing is forwarded from C#: `UnityPlayerActivity.onNewIntent` already calls `setIntent`, which is
  what AppsFlyer reads for a tap on the running game, and the native SDK holds the launch intent of a
  cold start itself (native 1.9.4).
- **An entry that cannot be a link domain stops the build** (`VLPlayOneLinkConfig.Problem`): a typo
  fails nowhere later — the link simply opens the browser.
- **A launcher `activity-alias` is handled** (Unity's own manifest declares an Activity, some games'
  declare an alias over it): the VIEW filters go on the alias, the launchable component, and
  `singleTask` on its target Activity, since an alias cannot carry a launch mode. The target is
  resolved the way Android reads manifest names — `.Game`, `Game` and `com.game.Game` name one class,
  and an AGP 8 manifest without a `package` matches a relative name by its tail — and an alias whose
  target is missing or removed by the merger (`tools:node="remove"`) stops the build by name. A
  same-host VIEW filter counts as already declared only when it is a complete App Link
  (`autoVerify`, DEFAULT + BROWSABLE, `https`); an incomplete one gets the canonical filter added.
- Outside the build, and each one silent when missing: Associated Domains on the App ID (Apple
  Developer portal) plus a regenerated profile; the iOS Team ID / bundle id and the **Play App
  Signing** SHA-256 in the OneLink template; `deep_link_value` on the link for a route.
- **Needs native 1.9.4 for the cold start** (the game not running when the link is tapped): up to
  1.9.3 the native SDK handed that tap to AppsFlyer before AppsFlyer had its dev key. This package now vendors
  1.9.5; device and AppsFlyer dashboard checks remain separate.

Verified: 974 core + 149 editor assertions (Unity 6000.0.73f1's own manifest as the fixture);
`.mm` strict-compiled against the vendored headers, both slices, ARC and MRC; a real Unity 6000.0.73f1
export — the APK's merged manifest carries both filters on the launcher Activity, the iOS entitlements
both `applinks:` entries, an invalid entry is refused, an empty field changes nothing — and the
exported Xcode project builds for a device with the override in `UnityFramework`. Not run on a
device: no tap on a real OneLink yet.

### Fixed — iOS ignored `useStaging`, and the Apple button could not sign in

Both on every Unity iOS build up to 1.5.1. Android was not affected by either.

- **`useStaging` now reaches iOS.** The post-processor has always written it as `DIRECT_VERSION`
  in `VLPlaySDK-Info.plist`, but the native SDK does not read that key to pick a gateway — it
  starts on production and waits for the host to set `isSandbox` before `+handleApplication:`,
  which a Unity game has no code to do. So every iOS build talked to production whatever the
  setting said, while the Android bridge honoured it. A game that exists only on staging got
  `403 {"statusCode":10001,"msg":"Permission denied"}` on every sign-in route (the console shows
  `<VLPlaySDK> 10001 on signup/guest: Permission denied`), and `/sdk/config` still answered 200
  with the server's defaults, so nothing looked misconfigured. The bridge's app controller now
  reads `DIRECT_VERSION` from the same plist the SDK reads and sets `isSandbox` before anything
  else runs at launch, and logs one line that survives Release:
  `[VLPlay] gateway: staging (VLPlaySDK-Info.plist DIRECT_VERSION = false)`.
  **If your iOS build has been testing "staging", it was testing production.**
- **Sign in with Apple capability.** The SDK's login popup always shows the Apple button on iOS
  (App Store guideline 4.8 requires it next to Google and Facebook), but the post-processor added
  only Push Notifications to the entitlements. The tap failed inside AuthenticationServices —
  `AKAuthenticationError Code=-7026`, then `ASAuthorizationError Code=1000` — and the player saw
  "Apple sign-in failed", with a green build and often a provisioning profile that did carry the
  capability. `Unity-iPhone/VLPlay.entitlements` now carries `com.apple.developer.applesignin`
  too. **The App ID must have Sign in with Apple enabled** in the Apple Developer portal; with
  automatic signing Xcode does it for you, with a manual profile that lacks it the archive fails.
- Tests: `tools/csharp-harness~/ios-bridge-tests` compiles the shipping `.mm` (pod-absent, with and
  without ARC) and runs the decision against plists in the shape the post-processor writes, plus
  the launch-order wiring; macOS only, it says SKIPPED on Linux. The Editor tests run the
  post-processor's own `AddCapabilities` against a recording `ProjectCapabilityManager`.
  Measured on a real Unity 6000.0.73f1 iOS export of this package, signed Release build:
  the app's entitlements carry `com.apple.developer.applesignin`, `UnityFramework` carries the
  gateway log line, and VLPlaySDK is linked once.

### Fixed — iOS: a static pod LIBRARY was linked into both Unity targets

Every Unity iOS build up to 1.5.1 whose pods include a vendored static library. The post-install
hook strips a duplicated static pod from `Pods-Unity-iPhone`, but it read `-framework` flags only;
a pod that vendors `lib<name>.a` is linked with `-l"<name>"` and went into both binaries.
`IronSourceAdQualitySDK` is one — Unity Ads 4.x depends on it, so any game with Unity Ads (for
example AdMob's Unity Ads mediation adapter, `GoogleMobileAdsMediationUnity`) carried it twice.
Measured on `topdown_shooter` (Unity Ads 4.19.0 through the AdMob adapter, Release device build):
the app executable defined 264 Objective-C classes, 263 of them also in `UnityFramework` — every
`ISAdQuality*`, `SML*` and `UADQ*` class — and the console printed
`objc[…]: Class ISAdQualitySdkManager is implemented in both …/ZombieWar and …/UnityFramework`
for each. Two copies of an SDK that swizzles and looks classes up by name; whichever the runtime
registered first answers.

- The hook now also strips `-l"<name>"` from `Pods-Unity-iPhone` when the same library is linked
  into `Pods-UnityFramework` and its `lib<name>.a` under `Pods/` is an ar archive (thin or
  universal — the same detector as frameworks). System libraries on the same line (`-l"z"`,
  `-l"c++"`, `-l"sqlite3"`) have no file under `Pods/` and stay. The match ends at the name, so
  `-l"FooExtra"` survives a strip of `-l"Foo"`.
- The export audit (`PodStaticLinkageProblem`) checks libraries by the same rule, so a duplicate
  the hook could not repair fails the build instead of shipping.
- `tools/verify-ios-app-linkage.sh` now fails on **any** Objective-C class defined in two images,
  read from `__objc_classlist` (not `__objc_classrefs`, which lists every class an image merely
  uses). Its two prefix checks (VLPlay, AppsFlyer) passed the build above.
- Measured: re-running `pod install` with the new hook over a copy of that export left 0
  `-l"IronSourceAdQuality"` in the app target (4 configurations had it) and kept it in
  `UnityFramework`; the unsigned Release device build then defined 1 class in the app executable
  (`PodsDummy_Pods_Unity_iPhone`), 0 duplicates, the executable went from 2.35 MB to 68 KB,
  28 frameworks still embedded. The guard is red on the old build (263 duplicates) and green on
  the new one.
- Tests: the Editor tests run the generated hook under a real `ruby` over a Pods tree with a thin
  and a universal duplicated static library, an app-only library whose name extends a stripped
  one, system libraries, and an extension target; and the audit over a duplicated library and a
  system library on both targets. Four injected faults (no `-l` strip, audit ignoring libraries,
  no name boundary, libraries not checked for being static) each turned the expected assertion red.

### Fixed — the VLPlaySettings inspector drew broken rows

`EditorGUILayout.PropertyField` also draws a field's decorators, so the settings asset's own
`[Header]`s landed inside the guided sections ("Credentials (from VLPlay CMS/Ops)", "Android", a second
"Ads (IAA)") and pushed **Browse…** onto a line of its own. Guided fields now draw their control
directly (bool / string, `BeginProperty` kept); the `[Header]`s stay for the plain multi-asset list.
Section title, fold arrow and badge share one hand-laid row (the arrow no longer hangs outside the
box); the label column is a fixed 170–220 px instead of 45% of the window; buttons at the end of a
row have one width, so those fields end on one line; **Show** reveals only its own secret; help text
sits under the control it explains; the ads section's two setup paragraphs are one info box with the
manager button on the right; a build-check refusal is an error box without the Console's
`[VLPlay] ` prefix (`VLPlayAdsSetupView.BuildVerdict`); badges read OK / WARN / ERROR — a red "FIX"
block looked like a button that did nothing. Labels the docs quote are unchanged.

### Added — picking an ad network installs its provider module

No more hand-editing `Packages/manifest.json` with a path to the module. The **Ad network** dropdown
lists every network VLPlay releases a module for (`VLPlayAdsCatalog.Vendor.ModuleVersion`: MAX and
AdMob 0.1.0; LevelPlay none), installed or not. Picking one runs `Client.AddAndRemove` once: the
other networks' modules out, the picked one's module in, at the version released with this core,
from where the core itself came from:

- a folder (`file:../vlgame-sdk-unity`, VLPlay's own games) → `Providers~/vn.vlplay.sdk.ads.<network>`
  in that checkout;
- a tarball (`file:../ThirdParty/vn.vlplay.sdk-1.6.0.tgz`) → `vn.vlplay.sdk.ads.<network>-<version>.tgz`
  in the same folder, which VLPlay sends with the SDK;
- the public git host (`…/sdk-unity-public.git?path=/vn.vlplay.sdk#…`) → the same repository,
  `?path=/vn.vlplay.sdk.ads.<network>#vn.vlplay.sdk.ads.<network>/<version>`.

A module is only added once the network's official plugin is in the project — without it the module,
and so the whole project, stops compiling. The setup checklist carries the same plan as buttons:
**Install** on *Ads provider module*, **Remove** on the new *Other provider module* line, and
**Remove** on *Ads are off in this build* when a module is left in an ads-free project. An unknown
`adsProvider` changes nothing — a typo must not strip the working module. Removing the old network's
plugin stays a manual step. Rules in `VLPlayAdsSetupView.ModuleSource` / `PlanModules` (30 new
assertions; each rule seen red by injection); `verify-csharp.sh` pins the catalog's module versions
to `Providers~/*/package.json` and the inspector's wiring. Measured in Unity 6000.0.73f1: a repo-folder
project with the MAX module and both plugins switched to AdMob (manifest rewritten, Package Manager
`Success`, reopened with 0 compiler errors and `VLPlay.Sdk.Ads.Admob.dll` built); a tarball project
added `file:../ThirdParty/vn.vlplay.sdk.ads.admob-0.1.0.tgz` (locked as `local-tarball`). Compiles on
Unity 2021.3.45f2.

### Added — public git host for released packages

`tools/publish-public-upm.py <package.tgz> --repo <clone of VL-Game/sdk-unity-public> [--push]`
publishes a packed tarball to the public repository: its `package/` content into the package's
folder, tag `<package>/<version>`. It refuses a package or version it does not publish, dev-only
content at the package root, anything the secret scan finds (the SDK Hub's patterns plus optional
extra ones, every file and every file inside `.aar`/`.jar`/`.zip`, self-tested against planted keys
each run; the Android demo mode's `demo_` mock tokens are listed, not failed), and an existing tag.
After staging, the index must equal the tarball byte for byte — a global `*~` ignore would otherwise
drop `NativeLibs~/`. `vn.vlplay.sdk` 1.5.1 is published (`vn.vlplay.sdk/1.5.1` → `cd4212f`); installing
it from the git URL was measured on Unity 6000.0.73f1 and 2021.3.45f2 (Unity runs
`git fetch --depth=1` on the tag, so the repository's history never slows an install). The provider
modules go up with the 1.6.0 cut, next to the core.

### Added — AdMob provider module `vn.vlplay.sdk.ads.admob` 0.1.0 (released with 1.6.0)

Built 28/09 while MAX waits for an AppLovin account: Google publishes test ad units, so AdMob can be
run on a device without one.

- **The module** (`Providers~/vn.vlplay.sdk.ads.admob`): the official Google Mobile Ads Unity Plugin
  **11.3.0** as the mediation engine for rewarded and interstitial. Pinned to 11.3.0 because it is the
  last plugin that declares Unity 2019.4 (11.4.0 and 11.5.0 declare 2022.3, above this core's 2021.3);
  the Android SDK is the same (`play-services-ads` 25.4.0). Google hands every impression its own ad
  object, so each show is bound to its ad: a reward, a revenue or a dismiss can never reach another
  show, and one that arrives after the close still reaches its own — no settle window as with MAX.
  The reward ticket goes on the ad as SSV custom data with the player as SSV user id. Google's error
  codes differ by platform and map to the codes the native AdMob providers answer.
- **AdMob App ID check.** The plugin writes the App IDs from its own settings (Assets ▸ Google Mobile
  Ads ▸ Settings); the build now refuses, for the platform being built, an App ID that is empty, that
  is an ad unit id, or that is the same on both platforms. A wrong App ID builds and launches and the
  Google SDK never starts — no crash, no ad. The iOS one is refused before the Xcode export (the plugin
  itself notices only in a post-process). The setup checklist has one line per platform and a button
  that opens the plugin's settings; the inspector has **Open Google Mobile Ads settings**.
- **Plugin version pin for AdMob** read from the plugin's own assembly version (GoogleMobileAds.dll
  11.3.0.0), as the plugin reports itself.
- **iOS network name fixed:** AdMob's own fills on iOS report the adapter class
  `GADMAdapterGoogleAdMobAds` (measured in the GoogleMobileAds 13.6.0 binary), which no row mapped —
  every AdMob-served impression on iOS would have counted as `unknown`. Added, with
  `GADMAdapterCustomEvents` → `custom_event`. The native iOS table has the same gap.
- **AdMob: Google's Next-Gen Android SDK is refused.** Plugin 11.3.0 can switch Android to Google's
  experimental Next-Gen SDK (Android API 24, different mediation exclusions). The module is verified
  only on the standard SDK, so a build with Next-Gen selected stops; the checklist line **AdMob
  Android SDK architecture** opens the plugin settings.
- **AdMob callbacks on Google's thread; revenue reported at once.** The module no longer sets
  `MobileAds.RaiseAdEventsOnUnityMainThread` (obsolete in 11.3.0; Google says routing paid events
  through Unity's update loop loses impression-level revenue). The revenue event goes to the
  backend batch from Google's thread; every event the game sees still arrives on Unity's main thread.
- **A reward Google sends after the close still counts** (on Android it can: googleads-mobile-unity
  #1689). The closed ad is kept until the next show on its unit, so its late reward reaches its own
  show once — for a game with `ssv:false` that is the credit.
- **AdMob consent (Google UMP) is opt-in:** `VLPlaySettings.admobConsentEnabled` — **Ask for
  consent (Google UMP)** in the AdMob part of **Ads (IAA)** — is **off by default**, because VLPlay
  games ship in Vietnam, where GDPR does not apply: Google Mobile Ads starts at once and no consent
  form is shown. On (a game distributed in the EEA/UK): the module runs Google's UMP update and
  consent form before Google Mobile Ads starts and requests no ad until UMP allows it; consent kept
  from an earlier session starts it at once. New `VLPlayAds.IsPrivacyOptionsRequired` /
  `VLPlayAds.ShowPrivacyOptions()` for the privacy button Google asks for — always `false` / a no-op
  with consent off. A start that UMP does not allow is retried every 10 s.
- **Google Mobile Ads starts once per process, and each caller hears the outcome exactly once.**
  Google's callbacks now run on Google's thread (Android: the Android UI thread), and a caller joining
  while Google finished could hear "initialized" twice; the decision is taken under one lock.
- **Demo sample:** the AdMob test config picks Google's test units for the platform it runs on (the
  Android units do not load on iOS).
- `tools/verify-admob-unity.sh <GoogleMobileAds-v11.3.0.unitypackage>`: the real-Unity check for this
  module, as `verify-provider-unity.sh` is for MAX.

### Added — setup checklist and build checks a partner cannot miss

Nothing here is ads-specific; every Unity game gets it.

- **Setup checklist at the top of the `VLPlaySettings` inspector.** Each line is judged by the same
  rule the build applies, and each problem the Editor can fix has a button:
  - *Android dependencies*: EDM4U installed, `Assets/Plugins/Android` present, every dependency the SDK
    declares actually in `mainTemplate.gradle` or `Plugins/Android` — **Create folder and resolve** /
    **Resolve now** runs EDM4U's Force Resolve;
  - *Android minimum API level* against the SDK's 23 (24 on a MAX build) — **Set to N**;
  - *iOS minimum version* against the VLPlaySDK pod's 15.0 — **Set to 15.0**;
  - with ads on: the provider module against this core, the official plugin against the module's pin
    (**Open download page** goes to the vendor's own release), a second network's plugin, the MAX SDK
    key (**Open Integration Manager**), and the native ads manager switch; with ads off: an ad plugin
    or module still in the project; always: the 1.5 dependency file (**Delete the file**).
  The answers refresh when the project changes (a resolve, an import, an Integration Manager save).
- **The Android build stops when the SDK's dependencies are not in the Gradle project.** Checked on
  the generated project, after EDM4U is done: an app built without them installs and then crashes on
  the first SDK screen. The usual cause is the one EDM4U cannot fix itself — `Assets/Plugins/Android`
  did not exist when it ran, so it fell back to its own bundled Gradle, which does not run on the JDK
  17 that Unity 6 ships; the only trace was a line in the Editor log. The message names what is
  missing and the button that fixes it.
- **The iOS build stops when Target minimum iOS Version is below 15.0** (the VLPlaySDK pod's own
  deployment target). The export used to pass and `pod install` failed afterwards, in Xcode, with a
  CocoaPods message that never mentions Player Settings.
- **Ads:** the ad network dropdown offers only networks whose module is installed.
- **The rest of the `VLPlaySettings` inspector is reorganised.** A header names the package version,
  the backend host the build talks to (`gw-s` / `gw`) and a summary of what the sections below report. Sections fold
  and carry a badge with what they hold: Setup checklist, Connection (environment as a Staging /
  Production switch, the three credentials — API key and client secret masked, **Show** reveals),
  Platforms (Android / iOS tabs: Firebase config paths with **Browse…**, which stores a
  project-relative path; Facebook App ID; iOS push as Development / Production), Ads (IAA), Runtime.
  Each field says what the build or the SDK will do with its value — an empty client ID (the build
  stops), a pasted API key in the client ID, a trailing space, a Firebase path to a file that does
  not exist (the post-processors only log that and ship without it), verbose logging on with
  Production. Nothing is refused; the asset format is unchanged, and several different assets
  selected at once still get the plain field list. The decisions are in `VLPlaySettingsView`
  (harness-tested); `verify-csharp.sh` now pins that the inspector draws every `VLPlaySettings`
  field.

### Fixed — ads core and MAX module (27/09 audit and reviews)

- **A failed load is retried.** NO_FILL, TIMEOUT and NETWORK_ERROR are retried up to 6 times, backing
  off 5 s → 64 s with up to 1 s of jitter, and `OnAdLoadFailed` / `ad_load_failed` fire only when the
  budget is spent — the native rule (`RetryPolicy.java`). An official vendor plugin does not retry
  on its own, so one no-fill at launch used to leave the placement empty until the game preloaded
  again. A retry due when ads are switched off or the provider changes is dropped, not answered.
- **A provider that throws no longer ends ads for the session.** A throw at initialize is retried
  after 10 s (not on every `IsReady` poll); a throw at preload answers `NOT_READY` instead of leaving
  the game waiting; in the MAX module, a throwing `Load` no longer leaves the ad unit marked
  "loading" for good.
- **Editor, Enter Play Mode Options (domain reload off):** a new Play session resets the ads core, so
  an ad on screen when Play stopped no longer holds its ad unit into the next session ("still on
  screen" for every show there).
- **The real credit no longer rides on the reward callback** (review 27/09). The plugin core used to
  start the reconcile — the only path to `OnRewardConfirmed` — from the network's reward callback,
  on the nonce of whichever show that callback reached. MAX names the ad unit in a callback, never
  the impression, and AppLovin documents no order between the reward and the hide (rewards after
  the hide, and rewards that never come, are both reported), so a late reward reconciled the NEXT
  show's nonce — and, reconcile being once per nonce, could swallow that show's own confirmation.
  Now each show reconciles its OWN nonce from its OWN dismiss, and the backend decides (SSV reaches
  it server-to-server): `OnUserRewarded` is advisory only and never picks a nonce. Consequences:
  `OnRewardConfirmed` arrives after `OnAdDismissed`; a rewarded ad whose network never fires the
  reward callback is still confirmed when SSV credited it; a skipped ad is polled to the end (the
  native poll: 10 checks over about 3.5 minutes) and ends unconfirmed.
- **MAX module: a late reward's advisory no longer lands on the next show.** A show that ends still
  owed its reward or revenue holds its unit for up to 5 s — the next show there waits (`IsReady`
  false, a fill that lands meanwhile is reported when the window closes); a reward never reaches a
  show that has not been displayed; each show gets at most one reward and one revenue callback; what
  arrives after the window is dropped with a log line. This keeps `OnUserRewarded` — the credit
  itself for ssv:false games — and the `ad_rewarded` row right; it does not decide any SSV credit.
  One limit is left: a reward MAX delivers more than 5 s after the hide AND after the next ad was
  displayed cannot be told apart from that ad's own advisory. The runtime contract
  (`IVLPlayAdsRuntime`) no longer claims more than a provider can know.
- **MAX module: `MaxSdk.ShowRewardedAd` / `ShowInterstitial` throwing no longer wedges the ad unit.**
  The module had already marked the unit "on screen", so `IsReady` stayed false and a deferred load
  waited for a dismiss that never came, for the rest of the session. The mark is rolled back before
  the exception reaches the core (which answers the game `SHOW_FAILED`).
- **A new player's first rewarded show is no longer refused because the old player's show is still
  waiting for its ticket** (review 27/09). `SetUserId` with another id, sign-in / sign-out and
  session expiry — and the CMS kill-switch or a provider change — now drop the waits along with the
  tickets; each wait carries its own token, so the old (stale) answer, when it lands, fails the old
  show without ending the new player's wait.
- **Sign-out and session expiry now clear the player the ad provider reports under** (review 27/09,
  second round). They used to drop only the reward tickets; MAX keeps its user id for the process,
  so every ad after a sign-out went on being reported as the player who left — in MAX's `{USER_ID}`
  and its user-level reporting (the credit itself is not affected: the backend ticket decides it).
  The SDK's own sign-in does not clear it: the game binds the player with `VLPlayAds.SetUserId`,
  often before the SDK's cold-launch sign-in event lands. A new Play session (Editor, domain reload
  off) also starts with no player, and a provider is now handed the empty id too (runtime contract).
- **Editor, domain reload off: work queued in the last Play session no longer runs in the next.** The
  main-thread dispatcher's queue and timers are statics; a MAX settle window still open when Play
  stopped fired in the next session and asked MAX for one more load on the unit that session was
  using. The dispatcher now drops that work when a Play session starts, and a settle window of a
  module instance that is no longer the registered engine does nothing.
- **`OnAdLoadFailed` fires only for a `Preload` the game called** — as on the native path, where only
  the game's own preload carries a listener. The SDK's own preloads — `PreloadAll` (also run at init
  and when the kill-switch comes back), the next ad after a dismiss, the one a not-ready show starts,
  and the retries of those — are logged instead, whether refused before the vendor (ads switched
  off, the hourly cap the closing interstitial just reached, age, no unit) or failed after the retry
  budget. The first symptom (review 27/09): a CMS kill-switch landing while an ad was on screen raised
  `OnAdLoadFailed(DISABLED)` when it closed. The `ad_load_failed` row is still reported whoever asked;
  a game preload's retries still answer the game; an ask made before a kill-switch or provider change
  is not answered after it. `OnAdLoaded` still fires for every fill.

### Added — provider modules (first: AppLovin MAX)

Plan `IAA-OFFICIAL-UNITY-PLUGIN-FULL-PLAN-2026-09-27`, release R1. Written while the native path was
still the default (an `adsRuntimeOwner` field chose between the two); the owner is gone since — see
*Changed* above — and the checks below apply to every build with `adsEnabled` on.

- **Provider modules can plug in.** The provider SPI (`IVLPlayAdsRuntime`, its sink and show
  listener, `VLPlayAdsRuntime.Register`) stays internal; `VLPlay.Sdk` lets exactly three assemblies
  in — `VLPlay.Sdk.Ads.Max`, `VLPlay.Sdk.Ads.Admob`, `VLPlay.Sdk.Ads.LevelPlay`. Any other assembly
  that tries fails to compile (CS0122).
- **Provider manifest** (`VLPlayAdsProvider.json` at a module's package root): provider, module
  version, the one assembly it may live in, bootstrap type, compatible core range, provider SPI
  generation, and an exact official-plugin pin (never a range, never "latest").
- **Two more build layers for a plugin-owned build.** Besides the official plugin, the build now
  requires exactly one provider module for `adsProvider`, compiled into its assembly, for this core
  and SPI, with the official plugin at the module's pin, and — for MAX — a non-empty SDK key in the
  AppLovin Integration Manager (read without creating its settings asset). After writing
  `res/raw/sdkconfig.xml` / `VLPlaySDK-Info.plist`, both post-processors read the file back and
  refuse a build whose "ads are host-owned" switch is missing.
- **First provider module: `vn.vlplay.sdk.ads.max` 0.1.0** under `Providers~/` (its own UPM package,
  not in the core tarball) — the official AppLovin MAX Unity Plugin **8.6.6** as the engine behind
  `VLPlayAds` for rewarded and interstitial. 8.6.6 bundles AppLovin Android 13.6.4, whose own
  manifest declares `minSdkVersion` **24**, so a plugin-owned MAX build needs Player Settings ▸
  Android ▸ Minimum API Level 24 (the core alone still supports 23). The module declares that
  floor as `androidMinSdk` in its manifest and the build refuses an Android project below it — a
  Unity export does not run Gradle, so without the check the project exports clean and fails in
  the partner's manifest merge. Every MAX call the module makes is identical between 8.6.3 and
  8.6.6.
- **Runtime config the module needs.** `ads.testDeviceIds` reaches the engine before it initialises
  (MAX ignores test devices set later); `IsReady` asks about the placement's unit as the CMS has it
  now (the SPI takes the placement, not its id); a `preload:true` placement in a format this build
  does not serve is logged by name instead of being skipped in silence.
- **Inspector.** The ads section shows readiness (core + SPI, module, installed plugin against its
  pin, host-owned switch, MAX SDK key) and opens AppLovin ▸ Integration Manager; its build check
  previews the module layers too.

Verified: `csharp-tests` 744 → 848 assertions, new `csharp-module-tests` 54 (the real core and the
real MAX module logic as two assemblies over a fake plugin), 0 failures; 17 injected faults in the
guards, the core and the module each failed the test written for them, and 5 structural pins each
failed when their wire was cut. `tools/verify-provider-unity.sh` with Unity 6000.0.73f1 and the
official MAX plugin 8.6.6: module compiles against `MaxSdk.Scripts`, the exported Gradle project
declares `minSdk 24`, Android export writes `sdk_ads_host_owned = true`, iOS export writes
`VLPLAY_ADS_HOST_OWNED = true` with `AppLovinSDK 13.6.4` in the pod lock, and the builds with no
module, an empty SDK key, an Android Minimum API Level of 23, or the native owner next to the
plugin are refused. Not run: a device, a full Gradle/Xcode build, Unity 2021.3.

**Correction (27/09 audit):** this entry first also said the exported Gradle project declared
`com.applovin:applovin-sdk:13.6.4`. It did not — that string was only the list EDM4U *meant* to fetch.
The tool's project never had `Assets/Plugins/Android`, EDM4U fell back to its own Gradle, which
crashed on JDK 17, and the export carried none of the SDK's 25 Android dependencies and no AppLovin
SDK; the tool read only `sdkconfig.xml` and went green. Both are fixed above: the tool sets the
project up the way a partner must and checks the Gradle project, and the build itself now refuses
such an export.

### Fixed

- **The `VLPlaySettings` inspector no longer falls back to plain fields.** Unity drops a custom editor
  that does not declare `[CanEditMultipleObjects]` whenever the Inspector holds more than one target —
  and a selection kept across a package upgrade holds the same asset twice. Right after moving to
  1.5.1 the settings asset then showed the old generic inspector (`adsProvider` as a text field)
  under *"Multi-object editing not supported."*. The inspector now declares it; the same asset
  selected twice still gets the guided ads section, and several different settings assets get plain
  fields with a note, since the ads preview can only speak for one asset. On 1.5.1: click another
  asset, then `VLPlaySettings` again.

Verified: in a real Unity 6000.0.73f1 project, `Editor.CreateEditor` on one asset, two assets and the
same asset twice returns `VLPlaySettingsEditor` for all three; with 1.5.1 the last two return
`GenericInspector`. `csharp-tests` 735 → 744 assertions, 0 failures; removing the attribute fails a
new structural pin in `verify-csharp.sh`, and breaking the one-asset count or the guided rule fails
the assertions written for them.

## [1.5.1] — 2026-09-26

No native change: the vendored SDK is still **v1.9.2** (iOS `a5e99b4` / Android `cfae7d8`), byte for
byte the binaries 1.5.0 shipped. Upgrading from 1.4.x, read 1.5.0 below first — `OnSignIn` moved
behind the identity-verification popup there.

### Added

- **Ads setup in the `VLPlaySettings` inspector.** `adsProvider` is a dropdown of the providers this
  package ships (AdMob, AppLovin MAX), and picking one opens only what that provider needs: AdMob
  shows its two App ID fields and a toggle per mediation network with an adapter here (Unity Ads,
  Liftoff Monetize / Vungle); MAX says its SDK key and ad units come from the CMS, and that this
  package ships no MAX mediation adapters yet, so a MAX build serves AppLovin demand only. The MAX
  note distinguishes VLPlay's native-owned path from AppLovin's official Unity-plugin flow: the
  former must not install that plugin, while the preview plugin-owned path uses **AppLovin >
  Integration Manager** for its SDK key, adapters, iOS SKAdNetwork ids and — with the Google
  adapter — the AdMob App IDs. `adsRuntimeOwner` is hidden while empty — a game picks
  one provider and the native SDK runs it; a stored value (a leftover `unity-plugin`, a typo) is
  shown as a dropdown with a warning so it can be cleared. A stored network list with duplicate or
  blank entries — which no toggle can show, and which the build refuses — gets a **Clean up** button. Underneath, **Ads build check** shows the ads-only rules Android and iOS enforce —
  the build guard's own messages — next to a **Sync Ads Dependencies** button. Plugin-owned builds
  are refused unless exactly the official plugin matching `adsProvider` is present. A stored value
  the package does not know (a typo'd `"Max"`) stays selected and is flagged
  instead of being shown as AdMob. Storage is unchanged: the asset still saves the same strings, so
  existing projects open as they were. The inspector lives in its own editor assembly with no
  platform-module reference, so it also shows on a machine with one build module installed.

Verified headless: `csharp-tests` 661 → 735 assertions, 0 failures (the decisions behind the
inspector, including the build check matching the guard's own message); four injected faults —
swapped App ID pairing, typo shown as a known provider, a toggle deleting an unknown entry, the
vendor-plugin probe dropped — each fail the assertion written for it, and so do five on the
plugin-provider guard (missing plugin accepted, second engine accepted, unknown provider accepted —
that one by crashing the suite with a NullReferenceException rather than a named assertion,
inspector preview skipping the check, preprocessor call removed — the last caught by the structural
pin in `verify-csharp.sh`) — and forcing the owner field always shown or always hidden fails
the two assertions on each side, and so does a clean-up check that never fires. A structural pin in
`verify-csharp.sh` checks that every field the inspector keeps out of its generic draw is drawn by
its ads section — hiding `verboseLogging` there without drawing it fails the pin. Compiled in a real Unity 6000.0.73f1 project
(`VLPlay.Sdk.Editor.Inspector.dll`, 0 errors). **Not opened in the Editor UI.**

### Changed

- **The build no longer points partners at the unreleased plugin-owned ads path.** When a vendor's
  own Unity plugin (MAX Unity Plugin, Google Mobile Ads Unity) sits next to the native ads path, the
  refusal now says only to remove it — `VLPlay ▸ Sync Ads Dependencies` already adds the vendor SDK
  the build needs. It used to offer `adsRuntimeOwner = "unity-plugin"` as the other fix, but that
  path has no shipped engine yet: the build passed and the game ran with no ads. An unknown
  `adsRuntimeOwner` is now told to clear the field instead of being steered to the preview value,
  and the `adsEnabled` / `adsProvider` tooltips stop calling the native path "legacy".

### Fixed

- **Android: `SignIn()` right after `Init()` resumes a stored session instead of showing the
  login form.** `Init()` queues the native init on the Android UI thread and returns, and the game's
  client id and api key are only read inside `initStartSDK`. `SignIn()` went straight to native from
  Unity's thread, so a boot that called it in the same frame sent its refresh-token request before
  the credentials were set: the resume could not succeed and the player was asked to sign in again.
  `SignIn` and `SignOut` now wait behind the init on the same thread, as the four IAP calls have
  since 1.5.0.
- **Android: `SetVerboseLogging(false)` right after `Init()` now sticks.** With `verboseLogging`
  ticked in the settings asset, the init runnable turned verbose logging on *after* the game's call
  had turned it off, so request bodies and tokens kept reaching logcat. The call now waits behind the
  init too. iOS was never affected: its init runs synchronously inside `Init()`.
- **Android: `DebugForceAntiAddiction()` right after `SetVerboseLogging(true)` fires again.** The
  native debug hooks only run with verbose logging on, and the documented QA recipe calls the two in
  one handler. Once `SetVerboseLogging` waited behind the init, the debug call — still sent straight
  from Unity's thread — reached native first and did nothing. It now waits in the same queue.
- **Android: `SignOut()` returns before the session is gone.** It now runs on the UI thread, so
  `IsSignedIn` and `CurrentUser` still report the old session until the call has run. iOS clears them
  at once. Leave gameplay on `OnSignOut`, not on the line after `SignOut()`.
- `VLPlaySDK.IsSignedIn` documents what it means: a session exists on the device, possibly only read
  back from storage and not resumed. Gate gameplay on `OnSignIn`, and call `SignIn()` on every launch.

Verified headless: `android-bridge-tests` 28 → 43 assertions, 0 failures; putting `signIn` and
`setVerboseLogging` back on Unity's thread fails exactly 4 of them, with the recorded call order
showing `signIn` before `initApplication` and the settings' `setVerboseLogging(true)` after the
game's `false`; sending the debug call straight to native again fails the 4 debug assertions, the
recorded order showing it ahead of `initApplication`. **Not device-verified.**

## [1.5.0] — 2026-09-23

### Changed

- Native pin **v1.8.3 → v1.9.2** (iOS `a5e99b4` / Android `cfae7d8`). What a Unity game notices:
  - **`OnSignIn` waits for the identity-verification gate.** With identity verification on, it fires
    once the player is through the gate: not needed, already verified, or the popup completed. A
    player who taps **Đăng xuất** on the popup produces `OnSignOut` with **no** `OnSignIn` before it,
    so that handler can run while the game is still on its sign-in scene. If the gate cannot decide
    within 10 seconds, `OnSignIn` fires anyway. Up to 1.4.8 `OnSignIn` came first and the popup
    opened over a game already in play.
  - **Ads on the native path** (`adsRuntimeOwner` empty — every game today): one show per ad unit at
    a time (`OnAdShowFailed` with `SHOW_FAILED` while the previous ad on that unit has not been
    dismissed); the reward ticket is spent when the ad is handed to the network and never reused
    after a kill-switch, a provider change or a switch to another player; ad-event `revenue` / `ts`
    bounded to 2^53−1, and one bad event no longer rejects the whole `/ads/events` batch. iOS: an
    expired token on a GET is now refreshed (the reward poll could report a credited reward as not
    confirmed), and a `null` in the ads config can no longer crash the app on every launch.
  - Native 1.9.0's host-owned switch and backend bridge are what the plugin-owned preview below runs
    on; a native-owned build writes and does exactly what it did before.
- Verified on the vendored artifacts, not the copy step: the re-vendor tool's checks 7/7 (both
  slices report 1.9.2, the AAR bakes 1.9.2 and carries no vendor bytes, nested bundle 119 png /
  0 tiff, 0 `.meta` in the pod dir, podspec valid); the bridge's 30 Android names are in the AAR's
  `classes.jar` (411 classes) and covered by a `-keep`, its 33 iOS names are in the framework binary,
  and the host-owned switch is declared and present on both. Three public iOS headers differ from
  the previously vendored 1.9.0 — `Constants.h` (the version), `Global.h` (one added method the
  SDK's own identity popup calls) and `VLPlayAds.h` (comments) — and the bridge compiles clean
  against them under strict clang on both slices, pod-present and pod-absent, with a fabricated
  selector as the control that must fail. The classes whose parsing the `.mm` mirrors
  (`StorePackage`, `TransactionItem`, `AntiAddictionStatus`, `UserModel`) are byte-identical to
  1.9.0's, so the differential parser results carry over.

### Added (internal preview — nothing changes for a shipping game)

- Ads runtime seam for the IAA Unity-first plan: an engine-free ads core, a one-engine provider
  registry, a `schemaVersion` 1/2 config parser (v2 is a **draft** pending the contract decision) and a
  vendor-free fake runtime. Reached only when `VLPlaySettings.adsRuntimeOwner = "unity-plugin"`; the
  default (empty) keeps `VLPlayAds` on the native path exactly as before. Public API and events unchanged.
  The plugin path applies the same policies as the native ads manager: age gate (`ads.minAge` ×
  anti-addiction age group → `AGE_RESTRICTED`), interstitial cold-start window and frequency caps
  (persisted in PlayerPrefs), rewarded + interstitial only, `NOT_READY` until the provider initialised.
  It re-reads the ads config when it changes, so a CMS kill-switch lands without a restart — both
  `ads.enabled=false` and a removed `ads` block, as native; an unreadable config keeps the last one.
  Turning ads back on re-preloads the `preload:true` placements: native tears its provider down and
  starts it again, an official Unity plugin keeps its one initialisation, so the core asks it for the
  fills instead. A load that lands while ads are off raises nothing and reports nothing, and so does
  any init or load callback of a provider instance that has since been replaced (a second Play
  session with domain reload off): an old instance still subscribed to the vendor's callbacks can no
  longer mark the new one ready or report the same load a second time.
- Plugin-owned builds write the native "ads are host-owned" switch into the build output —
  `sdk_ads_host_owned = true` in `res/raw/sdkconfig.xml`, `VLPLAY_ADS_HOST_OWNED` = `<true/>` in
  `VLPlaySDK-Info.plist` — from one shared decision. Native-owned builds write exactly what they did before.
- Build guard: `adsRuntimeOwner = "unity-plugin"` is refused until the vendored native carries the
  "ads are host-owned" switch, and an unknown owner value is refused. `tools/verify-native-bindings.sh`
  checks that declaration against the binaries in both directions. The vendored native is now
  **1.9.0**, which carries the switch, so the plugin owner is accepted by that guard. `adsEnabled` is
  the build master switch on this path too: a plugin-owned build with it off is refused (an ads-free
  build ships no vendor plugin — clear `adsRuntimeOwner` and remove the plugin), and at runtime such a
  settings asset runs the native ads path with a warning naming the owner it ignored, so the Editor
  does not quietly rehearse a path the build will refuse.
- Build guard (INV-ADS-1, the half a declaration cannot cover): a build is refused when a vendor's
  own Unity plugin is in the project while `adsRuntimeOwner` is still empty. The native SDK ships no
  vendor classes but probes the classpath for them, so that combination means the native ads manager
  and the plugin both initialise the same vendor SDK as soon as Ops enables ads — double preloads,
  double reward tickets, double `/ads/events`, all invisible until it is live. Detection is data on
  the vendor rows (install folder + entry type, so a UPM-installed plugin with no `Assets/` folder is
  still found) and the refusal names both ways out.
- The native backend bridge (native 1.9.0) is bound on all three bridges, internal to the wrapper:
  `AdsGetConfigSnapshot` (the server's `ads` block verbatim — a v2 `unitIds` object survives the
  native cache), `AdsRequestRewardTicket`, `AdsReportEvent` and `AdsCheckRewardStatus`. Android binds
  the two new native listeners (`RewardTicketListener`, `RewardStatusListener`) through per-call JNI
  proxies; iOS replies through the request-id channel (`adsRewardTicket` / `adsRewardStatus`), and a
  pod-absent build still answers so no callback hangs. The plugin-owned path now reads its ads config
  from that snapshot instead of the SDK's re-emitted config JSON, and a `DebugApplyLocalConfig` on that
  path survives until the server block actually changes. No public API change.
- The plugin-owned core now runs the whole reward and reporting lifecycle the native ads
  manager runs, through that bridge (wave U1 of the plan): SSV Option A reward tickets
  fetched at preload, cached per placement until 30 s before expiry, re-fetched inline at
  show when stale, refused as `SHOW_FAILED` when the backend cannot issue one or the ticket
  would exceed MAX's 8 KB custom-data limit (a truncated `nonce:signature` never verifies);
  the nonce is spent the moment the show is handed to the provider — not at the provider's shown
  callback, so a second show of the placement can never ride it — and the next preload issues a
  new one. One show per ad unit is on the provider at a time: a show while an earlier ad on the
  same ad unit and format has not failed or been dismissed is refused as `SHOW_FAILED`, whatever the
  provider's `IsReady` still says — keyed by unit, not placement, because a vendor plugin holds one
  show per unit and two placements may share one. One
  ticket request per placement is out at a time: a show landing while the preload's request is
  out waits on it instead of asking again, and a second show on that placement meanwhile is
  refused as `SHOW_FAILED` (one nonce pays out once). When the ticket lands the show re-runs
  every gate against the config as it is then, so a CMS kill-switch, a provider change or a
  removed placement during the fetch answers the game (`DISABLED`, `INVALID_PLACEMENT`) instead
  of showing; a provider change drops cached tickets, as native, and an answer landing after a
  kill-switch never refills the cache. A ticket belongs to the player whose session asked for it:
  `SetUserId` with another id and the SDK's own sign-in, sign-out and session expiry drop every
  cached ticket and every request still out, and a show waiting on one is refused as `SHOW_FAILED`
  instead of playing on the previous player's nonce; an ad already on screen keeps its unit. After
  the network's advisory reward the core runs the native reconcile poll for that nonce, at
  most once per nonce, and `OnRewardConfirmed` fires only when the server confirms — with the
  server's amount, currency and nonce, never the network's. Every provider callback reports
  its `/ads/events` row into the SDK's own batch (`ad_load`, `ad_load_failed`, `ad_impression`
  with ILRD revenue/currency/precision, `ad_click`, `ad_rewarded`, `ad_reward_confirmed`,
  `ad_dismissed`), whitelisted to the BE ingest keys, with the winning network canonicalised
  through the same vocabulary as native (`VLPlayAdNetworks`, a merge of the Android and iOS
  tables). The record of a show is scoped to that show — the provider reports each show to a
  listener handed over with it — so a preload landing while an ad is on screen cannot retag its
  click, reward or dismiss, and a reward or revenue callback that lands after the next show of the
  placement began still reconciles its own nonce and names its own network. The advisory
  `OnUserRewarded` carries the reward terms the placement had when that show was handed over — as the
  native providers keep `activePlacement` — not those of a config refresh that landed before the
  reward. Impression revenue the
  backend would refuse (NaN, infinite, negative — MAX reports -1 when it has none — or past 2^53-1)
  is left out of the event instead of failing the batch. Dismiss re-preloads the placement
  (next-ad-ready, as native). A dedicated `IVLPlayAdsBackend` seam keeps the core engine-free;
  the harness drives it with a fake backend. Provider modules (wave U2) report load and init through
  the sink and every show's callbacks — shown, click, impression revenue, reward, dismiss — to that
  show's listener. Each `Initialize` must report `OnInitialized` on its own sink even when the vendor
  SDK is already running from an earlier instance, and a `Preload` that finds a fill already loaded
  must report `OnLoaded` again; the interface documents both.

### Fixed

- **Android: the shop no longer comes back empty at launch.** `Init()` queues the native init on the
  Android UI thread and returns at once, but `GetProductCatalog`, `Purchase`, `GetPurchaseHistory`
  and `RestorePurchases` went straight to native from Unity's thread. A game that opened its shop
  right after `Init()` reached the SDK before it had a context: the catalogue and the history came
  back as error `-1` *"SDK chưa được khởi tạo"*, a purchase or restore ran against an SDK with no
  client id yet. Seen on a partner's device logs, every launch. All four now wait their turn behind
  the init on the same thread, so they always see an initialised SDK; one that cannot reach native
  at all still answers once, with `ErrNativeCall`. Every Unity release up to 1.4.8 has this.
- iOS was checked and is not affected: the SDK boots inside `didFinishLaunching`, and Unity loads the
  first scene only after that returns.

## [1.4.8] — 2026-09-21

### Changed

- Native pin **v1.8.2 → v1.8.3**. This is the release that matters for an Android game shipping an
  English locale: until now `values-en` was not a drifted translation but a **different error table
  pasted over the right one** — of the 67 four-digit codes that had an English row, 62 carried the
  wrong meaning (`1004` read *"Account not found"*, the sentence that belongs to `1008`) and a
  further 18 had no English row at all, so the device fell back to Vietnamese. The SDK looks the
  message up by the four-digit `statusCode` on the common request path, so this reached almost every
  API error. iOS was never affected; v1.8.3 ports its reviewed English column into Android verbatim.
- No C# change, no Podfile change, no binding change. Verified rather than assumed: all 24 public
  iOS headers are byte-identical between 1.8.2 and 1.8.3 except the version `#define`, the AAR keeps
  401 classes with `proguard.txt` and the merged manifest byte-identical, and the bridge's 28 Android
  + 29 iOS bound names are all still present and keep-covered.

### Fixed

- The UPM tarball no longer carries internal `tools/` or `.github/` files (the published `1.4.7`
  carried 19 such files: `tools/` 17, `tools.meta`, `.github/workflows/verify.yml`). `tools/` has no trailing `~`, so Unity imported all
  of it into the consuming project. Measured by diffing the two published tarballs: **528 → 509 files, 19 removed and 0 added**,
  `NativeLibs~` unchanged at 398. Because this changes the distributable, it could not reuse the already-published `1.4.7`.

## [1.4.7] — 2026-09-20

Native pin **1.8.1 → 1.8.2**. No C# change, no Podfile change, no new Editor behaviour.

### Read this before planning an upgrade

**A Unity game was not affected by either defect 1.8.2 fixes, and this release repairs nothing you
could reach through this package.** Both defects sit on purchase entry points the wrapper does not
bind:

| Native defect fixed in 1.8.2 | Entry point | Reachable from Unity? |
|---|---|---|
| iOS: the built-in shop and `buyPackage` read only `purchaseCode` from init-payment, and the transaction id was written under a key the legacy verifier does not read — so a player could be charged and `complete-payment` never run | `-openShop`, `-buyPackage:parentViewController:` | **No.** The bridge calls `purchasePackageWithProductId:`, which clears the scalar code and always takes the direct-verify path |
| Android: the `openPay` screen read only `purchaseCode`, so retrying a product with a pending order reached Google Play with an empty code and `complete-payment` was refused after the charge | `VLPlaySDKManager.openPay` | **No.** The bridge calls `purchasePackage`, which already read both shapes |

Take this release to keep the native pin current, or if your game also calls the native SDK directly
outside the wrapper. If neither applies, `1.4.6` is functionally equivalent for you.

### Verified

Vendored artifacts are the ones the release CI built for `core/v1.8.2`, not a local build: both iOS
slices report `1.8.2`, the AAR bakes `1.8.2` with 0 vendor bytes, the nested resource bundle holds
119 png / 0 tiff, 0 `.meta` in the pod directory, and `pod ipc spec` is valid. Bridge binding sweep:
28 Android names present in `classes.jar` and covered by a `-keep`, 29 iOS names present in the
framework binary. Gates: 7 C# profiles, 219 runtime assertions, 53 Editor assertions, package check.
**Not device-verified.**

## [1.4.6] — 2026-09-20

Native pin unchanged (**1.8.1**). Corrects the scope of the 1.4.5 fix and narrows it
away from extension targets.

### Fixed

- **A universal static framework was still linked twice.** 1.4.5 said static pods were
  unlinked *by binary type rather than by name*; that held only for a thin `!<arch>`
  archive. A pod shipping universal slices — the ordinary shape for an xcframework
  covering several architectures — has a fat (`cafebabe`) wrapper, read as dynamic, and
  stayed linked into both Unity targets. The audit that runs after the hook carried the
  same thin-only test, so the net 1.4.5 described did not exist for this case. Both now
  read the fat header and check every slice, 32- and 64-bit.
  **1.4.5 lost no behaviour**: `VLPlaySDK` and `AppsFlyerLib` each carry one thin device
  slice and were still repaired. What was missing was cover for a third, universal-only
  static pod.
- **The repair no longer touches extension targets.** It was scoped as "every target
  except `UnityFramework`", which sweeps in a notification-service or widget extension
  the game added. An extension is a separate process and needs its own copy, so removing
  the link there breaks it instead of repairing a duplicate. The hook and the audit now
  speak about the Unity app/host pair only.

Both implementations of "is this static" are pinned against the same fixtures, so a
drift between them fails the build. 53 Editor assertions (from 43), negative-tested in
both directions. **Not device-verified.**

## [1.4.5] — 2026-09-19

Vendored native is unchanged (**1.8.1**). Two packaging defects, one of which is in
the 1.4.4 you may already have installed.

### Fixed

- **1.4.4 shipped 400 compiled `.class` files.** Its tarball holds 928 entries where
  1.4.3 holds 528: an AAR had been unzipped inside the build checkout to inspect it,
  and `npm pack` swept the result in. A build is not affected — Unity does not compile
  them — but the package is nearly twice the file count and it exposes the Android
  SDK's internal bytecode, which decompiles. Every release from 0.1.0 through 1.4.3 is
  clean; only 1.4.4 is affected. **If you are on 1.4.4, upgrade and delete the old
  tarball.** `tools/verify-package.sh` now fails on any compiled class and on any
  unexpected entry at the package root, so this cannot ship again.

### Hardening

- **Static pods duplicated across Unity targets are now unlinked by binary type, not
  by name.** The post-install hook used to carry a two-name list (`VLPlaySDK`,
  `AppsFlyerLib`); the audit that runs afterwards already identified static archives by
  their `ar` magic. A third static pod entering the dependency closure would therefore
  have been caught by the audit and *failed your export*, with nothing you could do
  until VLPlay shipped a new package. The hook now reads the same magic bytes and
  repairs the general case itself. It strips a framework only when it is static, linked
  into `UnityFramework`, and linked into at least one other target — a static pod your
  game links into its app target alone is not duplicated and is left alone.
- **The audit remains, as the net.** A static framework duplicated across two targets
  with `UnityFramework` not among them still fails the export by name, because the hook
  cannot know which copy to keep.
- The importable demo clears its player-bound IAP and notice state on both
  `OnSignOut` and `OnSessionExpired`, demonstrating the lifecycle ownership a game
  needs before routing back to its login screen.

## [1.4.4] — 2026-09-16

### Fixed

- **iOS: `AppsFlyerLib` was linked into both `Unity-iPhone` and `UnityFramework`.** The
  post-processor's single-target rule only covered `VLPlaySDK`; `AppsFlyerFramework`'s default
  subspec vendors a *static* xcframework too, so every Unity build carried two copies and the
  runtime logged `objc: Class AFSDK… is implemented in both …` at launch. The rule is now a list
  (`VLPlaySDK`, `AppsFlyerLib`) — note it strips by *framework* name, which for AppsFlyer differs
  from the pod name. Measured: `AppsFlyerLib` is the only static dependency in the closure;
  FBSDK*, OneSignal* and every source pod are dynamic under `use_frameworks!`.
  `tools/verify-ios-app-linkage.sh` now fails on a duplicated AppsFlyer carrier as well.
  Partners: upgrade the package and re-export; no Podfile change. Native iOS partners are
  unaffected (single target). Switching AppsFlyer to its `/Dynamic` subspec is a separate,
  coordinated migration (native podspec + this package + docs + partner Podfiles together).

## [1.4.3] — 2026-09-16

Vendored native **1.7.2 → 1.8.1**. The C# facade, bridges and dependency pins are unchanged;
the update is entirely inside the released AAR and xcframework.

### Native fixes a Unity game inherits

- **Android keeps the FAB account button after a silent session restore.** Unity registers the
  SDK after its only host Activity has already resumed, so native 1.7.2 could miss that lifecycle
  event and had no Activity to attach the floating ball to. Native 1.8.1 seeds the current host
  Activity during bootstrap.
- **An upgraded guest with an unusable restore token no longer reaches a dead end.** The first
  guest tap still refuses by masked account name; the same control then offers *"Play with a new
  account"*. Only that armed second tap sends `system.forceNewGuest: true`. Closing the popup or a
  successful authentication clears the offer; an offline/5xx/429 failure keeps it armed.
- **iOS Facebook sign-in works when ATT is denied or restricted.** Native chooses Facebook Limited
  Login, obtains the backend nonce first, and resolves the Limited credential to the same account
  as classic login. Account binding remains classic-only because its backend route accepts a Graph
  token, not a Limited JWT.
- Session and guest hardening from native 1.7.3–1.8.1 also rides this bump: restored guest truth
  comes from the backend account, password changes on another device stop causing an endless silent
  login loop, and guest signup is process-wide single-flight.

### Compatibility audit

- Android: all 28 names the bridge binds are present and covered by the shipped keeps (400 SDK
  classes); the bound public surface is additive only. `AndroidManifest.xml` and `proguard.txt` are
  byte-identical to native 1.7.2.
- iOS: public headers are byte-identical to native 1.7.2 except the version literal; all 29 runtime
  names still resolve. Both slices report 1.8.1 and retain the nested resource bundle.
- Native dependency declarations did not move, so EDM4U and the binary podspec remain unchanged.

Verified headlessly: 7 C# type-check profiles with warnings as errors, runtime + Editor assertions,
strict Objective-C++ bridge compile against both new iOS slices and pod-absent mode, native binding
guards, byte-identical parser/DTO classes versus 1.7.2, and package verification. **Not
device-verified:** the acceptance case is restart a signed-in Unity game on Android and confirm the
FAB account button remains available.

## [1.4.2] — 2026-09-10

Native pin unchanged (**1.7.2**). One Android build fix.

### Facebook login on Android now reaches the callback on every game, not only the demo

- `VLPlayPostProcessAndroid` writes `res/values/vlplay-facebook.xml` with
  `fb_login_protocol_scheme = fb<fbAppId>` into the generated `unityLibrary` module. Facebook's OAuth
  callback on Android comes back through an `<intent-filter android:scheme="@string/fb_login_protocol_scheme">`
  declared in the native AAR's manifest, and the AAR ships that string at the **demo app's** App ID.
  A native partner overrides it in its own `res/values`; a Unity game owns no such file, and up to
  1.4.1 nothing wrote it — so on any game whose Facebook App ID differed from the demo's, the consent
  screen completed and the callback was dropped silently (no error, no log). The App ID and client
  token themselves stay CMS-driven at runtime; the scheme is the one value Android resolves at install
  time, which is why it lives in `VLPlaySettings.fbAppId` — the same field the iOS post-processor has
  written into `CFBundleURLSchemes` since 1.2.4.
- Empty `fbAppId` → nothing is written and the build logs a warning (parity with the iOS step). It
  was already required for Facebook on iOS; it is now required for Facebook on Android too.
- Pinned by 8 new `editor-tests` assertions that run the shipping writer over a real `res/` dir
  (scheme value, resource name, non-translatable, well-formed XML with exactly one string, empty and
  null ids write nothing, idempotent re-run). Both negative tests red on the assertion named for
  them (dropping the `fb` prefix → 1 failure; dropping the empty-id guard → 2 failures).

Verified headlessly: 7 type-check profiles, 219 runtime + 37 editor assertions, package verifier.
**Not verified:** no Unity Editor build and no device ran this cut — a Facebook login on a
non-demo Android game is the acceptance test.

## [1.4.1] — 2026-09-09

Vendored native **1.7.1 → 1.7.2**. A fix release; the one thing that changes *here* is that a
purchase can now say which character it is for.

### A purchase carries its delivery target — and is refused early without one

- `VLPlayPurchaseRequest` gains `roleId` and `serverGameId`. They travel with the sale; a value here
  wins over the one set through `VLPlaySDK.UpdateGameInfo`, empty falls back to it. Both bridges call
  the natives' new entry (Android 9-arg `purchasePackage`, iOS 10-arg
  `purchasePackageWithProductId:…roleID:serverGameID:…`).
- Native 1.7.2 refuses a sale whose target resolves empty **before** init-payment — nothing is
  charged, no store sheet opens. The callback gets `outcome = Failed`, `errorCode =
  VLPlaySDK.ErrMissingDeliveryTarget` (**-4**, new public constant) and the backend's own sentence
  (*"Vui lòng chọn nhân vật trước khi thanh toán."*). The backend has enforced the same rule since
  2026-09-07, so a game that never calls `UpdateGameInfo` was already failing there — it now fails
  one round-trip earlier and with a message that names the fix.
- **iOS wiring detail that matters:** the refusal is the one purchase outcome the SDK does NOT
  announce through its payment notification, so the `.mm` now passes a `purchaseCompletion` block
  that handles exactly that arm and lets every other outcome keep arriving through the notification
  observer. Without it the game's callback would hang forever on a refused sale.
- **The Editor mock applies the same rule** (`VLPlayRuntimePolicy.MissingDeliveryTarget`, pinned by
  8 assertions): in Play Mode, `Purchase()` without a prior `UpdateGameInfo` (or a `roleId` on the
  request) is refused with -4 — so the mistake shows on the developer's machine, not on a device.
  `UpdateGameInfo` is therefore now *stored* by the mock (it used to be echoed only).

### Native fixes a Unity game inherits with no C# change

- `10001` shows the backend's `msg` instead of the placeholder *"Dữ liệu không hợp lệ"*, and every
  `10001` is logged ungated (`10001 on <path>: <msg>`).
- Android FAB HUD: closing the HUD in the same frame as a tab tap (sign-out, session expiry, a fast
  close) no longer throws `NullPointerException: ViewGroup.removeAllViews()` at
  `VLPlayFABHudController.installContentForTab`.

### Verified

7 type-check profiles; 219 + 29 assertions; `verify-native-bindings` 28 Android names present and
kept (387 classes), 29 iOS names present; strict-clang trio against the **new** headers (pod-present
both slices + pod-absent, `-Werror` objc-method-access / undeclared-selector / protocol /
deprecated-declarations), with a renamed selector failing the same check so it is not vacuous;
package verifier green. **Not verified, and not claimed:** no Unity Editor build and no device ran
this cut.

## [1.4.0] — 2026-09-03

Vendored native moves **1.5.1 → 1.7.1** — eight native releases that had not reached a single Unity
game — and this package gains its first AdMob mediation adapters. See the native changelogs for what
each release carried; the entries below are what changes *here*.

### Native bump: what a Unity game inherits

- **OneLink deep links actually reach the game**, including the deferred case (install → first open).
  No C# was needed: `AndroidBridge` already claimed the listener before `initStartSDK`, and the `.mm`
  parks the payload until `Init()` opens the channel.
- **Flat `utm_*` attribution on every account-creating call**, so a partner can be paid for an install.
- **The AppsFlyer switch is tri-state** (`UNKNOWN` allows the once-per-install postback rather than
  losing it). **Retention cohorts are no longer sent by the SDK** — ownership of that is a decision
  that has not landed (ads-hub D3), so do not start firing `af_retention_d*` on the strength of this.
- **StoreKit refuses to open when init-payment returns no `appAccountToken`**, and payment-blocked
  surfaces behind code `1096`.
- Android guest-upgrade no longer holds a revoked bearer.

### Build-time pins that moved with it — check your project

- **`minSdk` is now 23** (measured in the vendored AAR's manifest). A project still on 21 **fails the
  Android manifest merge**.
- `com.android.billingclient:billing` **7.0.0 → 9.1.0**; `com.applovin:applovin-sdk` **13.5.1 → 13.6.2**.
  Both are prebuilt into the AAR, so a stale pin still resolves green and only fails when a player taps buy.


### Fixed

- **The AdMob manifest override now follows the base a build actually resolves.**
  Google Mobile Ads declares `android.adservices.AD_SERVICES_CONFIG` in
  `play-services-ads-lite:23.6.0` and dropped it in `24.0.0`. The default,
  no-adapter build still resolves 23.6.0, so it still collides with the copy
  Firebase Analytics contributes and still needs the `tools:replace` override;
  a build selecting a mediation adapter resolves 25.4.0, where there is no
  collision and no `@xml/gma_ad_services_config` resource to point at. The
  override was briefly removed outright, which would have failed the manifest
  merge for every game running plain AdMob. `VLPlayAdsCatalog` now answers it
  from the resolved base, and the Editor suite runs the manifest writer against
  both cases.
- Release packages no longer include local `bin/` and `obj/` output from the
  editor post-processor harness.

### Added

- **A build can declare AdMob mediation adapters, and the tooling ships them.**
  `VLPlaySettings.admobMediationNetworks` lists the mediation networks a build
  carries an adapter for. The generated EDM4U manifest gains one dependency per
  declared network on both platforms, plus a `<repositories>` block for networks
  that are not on Maven Central, and the iOS post-processor writes the merged,
  de-duplicated union of every declared network's `SKAdNetworkItems` instead of
  the single vendor id it wrote before. Removing a network takes its package,
  pod, repository and SKAdNetwork ids with it.

  **The list ships empty and an empty list is a supported end state**: with no
  networks declared the generated manifest is byte-for-byte the one this package
  has always emitted — pinned in the harness against the file a shipped game has
  on disk. Adapters are compiled in, so this is a build-time declaration, never a
  CMS switch: AdMob picks the winning network server-side, so no remote list
  could enable, disable, or predict one.

  Under any provider other than `admob` the list is **ignored, with a warning**
  from **VLPlay ▸ Sync Ads Dependencies** — not refused. Refusing would block a
  legitimate swap back to MAX over a field that no longer applies; ignoring it
  silently would let the settings claim this build carries adapters it does not.

- **The adapter catalog gains its first two rows: `unity` (Unity Ads, bidding)
  and `vungle` (Liftoff Monetize — formerly Vungle, waterfall).** Unreleased,
  like the mechanism above — nothing here reaches a partner until a release
  note says so, and mediation is not production-ready until the device/console
  gates (Ad Inspector per network, SSV chain on third-party fill, consent/UMP)
  are run. Coordinates were read off Google's integration guides on 2026-09-02,
  never from memory: Android `com.google.ads.mediation:unity:4.20.0.1` /
  `com.google.ads.mediation:vungle:7.7.7.0` (both resolve from Google's Maven,
  no extra repository — and unity's row also emits
  `com.unity3d.ads:unity-ads:4.20.0`, because that adapter's POM declares no
  dependency on its own network SDK while vungle's does; a new generic
  `Adapter.AndroidExtraPackages` field, since an adapter without its SDK
  resolves green and then never fills), iOS pods `GoogleMobileAdsMediationUnity` /
  `GoogleMobileAdsMediationVungle` (emitted unpinned; each pins
  `Google-Mobile-Ads-SDK ~> 13.3` and pulls its network SDK transitively), and
  each row carries its vendor-approved attribution snapshots: Unity's 76
  `SKAdNetworkItems` ids, and Liftoff's two published arrays kept separate —
  161 `SKAdNetworkItems` ids (six of which Liftoff itself spells
  `.adattributionkit`, mirrored verbatim) and the six `AdNetworkIdentifiers`
  ids for iOS 17.4+ AdAttributionKit. They are two lists, not one used twice.
  Not because iOS would reject the other ids — Apple documents `.skadnetwork` and
  `.adattributionkit` ids as compatible across both frameworks — but because
  which ids belong in that array is Liftoff's to declare, and feeding the 161 to
  both keys would have this package declaring 155 networks Liftoff never asked a
  publisher to. `AdNetworkIdentifiers`
  is the RAW plist key — Xcode's inspector shows a friendlier label with spaces,
  and writing that label yields a key iOS never reads while the build stays
  green, so `verify-csharp.sh` now refuses any attribution key that is not
  space-free. The snapshots are reviewed source data, never downloaded during a
  build. The id stays `vungle`, never `liftoff`: the native SDKs and the
  backend's canonical network list spell it that way, and reporting joins on it.

  Selecting any adapter also moves the emitted AdMob base to
  `play-services-ads:25.4.0`, the version both adapter POMs are built against —
  Gradle would resolve it anyway (highest-wins), so declaring the co-move keeps
  the manifest telling the truth; an empty selection still emits the
  byte-identical `23.6.0` baseline. The requirement is declared on each
  `Adapter.AndroidBasePackage`, not on the vendor: a future incompatible pair
  fails generation/build with both canonical IDs named instead of relying on
  Gradle's highest-wins selection. The static compatibility of the vendored AAR
  against GMA 25.4.0 was measured (every referenced class and member resolves),
  with on-device behaviour still an open gate. Adapters remain build-time: the
  CMS cannot install one into a shipped binary.

### Changed

- **Vendored native bumped `1.5.1` → `1.7.0`, and the two build-time pins that
  must move with it were moved.** Eight native releases had not reached a single
  Unity game: OneLink deferred routing (which needs no C# — `AndroidBridge` claims
  the listener before `initStartSDK` and the `.mm` parks the payload), the flat
  `utm_*` attribution that lets a partner be paid for an install, StoreKit refusing
  to open when init-payment returns no `appAccountToken`, the payment-blocked
  message behind code 1096, and the Android guest-upgrade fix that had been holding
  a revoked bearer. Measured on the artifacts: both slices report `1.7.0`, the AAR
  bakes it with 0 vendor bytes, and the name sweep resolves **28 Android + 29 iOS**
  bound names with a fabricated-name control at 0 — no native rename rode the bump.
  - `com.android.billingclient:billing` **`7.0.0` → `9.1.0`** in the EDM4U manifest.
    Native 1.7.0 compiles against 9.1.0; the AAR is prebuilt, so resolving the old
    line still **builds green** and only fails when a player taps buy (8.x changed
    `queryProductDetailsAsync`). This pin is lockstep, not a preference.
  - `com.applovin:applovin-sdk` **`13.5.1` → `13.6.2`**, mirroring the native
    `compileOnly` pin again. 13.5.1 existed to hold the old minSdk 21 floor; that
    reason expired when the SDK's own floor became 23.
  - **`minSdk` is now 23** (measured in the vendored AAR's manifest; it was 21 up to
    native `1.5.1`). A project still set to 21 fails the manifest merge. README and
    Unity docs are updated in the matching release work; publish them with the wrapper
    release, not before a package vendoring this native is available.

- **Unhandled notices reach the SDK fallback again.** The wrapper necessarily claims
  both native notice callbacks to expose `VLPlayEvents.OnNotice` and
  `OnNoticeCleared`; when the game subscribes to neither, it now hands notices back
  to the native renderer, so a validation error is never invisible. Decided **per event**,
  the way iOS decides per delegate selector: subscribing to `OnNotice` takes over drawing,
  subscribing to `OnNoticeCleared` only tells the game when a notice ends while the SDK
  keeps drawing it. In that mixed case the clear reaches **both** the game and the SDK's
  renderer — whoever drew a notice is who clears it. Routing it to the game alone is what
  iOS native does today, and it strands a sticky banner on screen for the rest of the
  session; dropping the notice instead would hide every validation error. The rule is a
  pure function (`VLPlayRuntimePolicy.RouteNotice` / `RouteNoticeCleared`) rather than
  branches in the dispatcher: all four subscription states plus the drew-it/clears-it
  invariant are asserted headlessly, and each arm is negative-tested.

- **The ads build guard compares the generated manifest with what the settings
  declare, instead of inferring a vendor from strings inside it.** The old check
  searched the file for two hardcoded vendor coordinates. That could not survive
  adapters landing in the same document: Google's adapter artifacts match neither
  vendor string, so it would have gone on answering "admob" and passing every
  build — passing because it was blind, not because it was right, which is the
  worse of the two ways it could have broken. The new check names the exact
  dependency that is missing or unexpected, and still catches the case it was
  originally written for: a manifest left stale after someone changed the
  settings and forgot to re-run **VLPlay ▸ Sync Ads Dependencies**. The documented
  MAX version override (for its minSdk note) stays allowed; AdMob and mediation
  adapter versions remain exact so incompatible drift is refused.

  `VLPlayBuildGuards.AdsDependencyProvider` is gone and `AdsBuildProblem` now
  takes the manifest text plus the declared network list. Internal build tooling
  — no game-facing API changed.

- **The ads vendor table moved from `Editor/VLPlayAdsVendors.cs` to
  `Runtime/Internal/VLPlayAdsCatalog.cs`** and gained the adapter rows. It had to
  move to be reachable at all: the build guard lives in a deliberately
  module-independent assembly that references only `VLPlay.Sdk`, so from
  `Editor/` the table was invisible to it — which is why that guard had been
  reduced to string matching in the first place. Same reasoning as
  `VLPlayBuildGuards`; being engine-free is also what lets the emitted document
  be asserted without a Unity Editor.

### Verified

`tools/verify-csharp.sh`: all 7 define profiles type-check, with **211 runtime
assertions + 29 Editor assertions / 0 failures**. The Editor suite executes the
shipping plist writer and the shipping Android manifest writer against faithful stubs; the runtime suite covers both adapter
rows, the no-adapter byte-for-byte baseline, dependency rendering, conflicting-base
rejection, attribution-key shapes and the provider-id structural pins.
`tools/verify-native-bindings.sh` resolves all **28 Android + 29 iOS** names, the
strict-clang bridge check passes pod-present device + simulator and pod-absent, and
`tools/verify-package.sh` confirms the packed AAR, xcframework and `NativeLibs~`
survive. Scratch resolution also completed with **85 Gradle artifacts / 0 conflicts**
and all **8 CocoaPods** dependencies resolved.

Unity 6000.0.73f1 also resolved the generated adapter selection through EDM4U in
an isolated project. Android produced a Release IL2CPP APK with High stripping;
iOS exported, installed all pods, wrote 163 `SKAdNetworkItems` plus 6
`AdNetworkIdentifiers`, and built Release no-sign in Xcode with VLPlay linked once.

⚠️ **Not verified:** Unity 2021.3 and the physical-device/console gates for GMA
25.4.0 + the two third-party adapters (Ad Inspector, third-party fill, SSV
confirmation and consent/UMP).

## [1.3.2] — 2026-08-13

### Fixed

- **iOS static `VLPlaySDK` now links exactly once.** CocoaPods still retains
  every generated target so its Embed Pods Frameworks phase remains intact, but
  the post-processor removes `-framework VLPlaySDK` from every aggregate except
  `Pods-UnityFramework`. Linking a static SDK into both `Unity-iPhone` and
  `UnityFramework` creates two independent Objective-C singleton graphs: SDK
  screens can render while actions that cross those graphs silently do nothing.
- **The release tarball cannot package local tarballs or Unity project assets.**
  `.npmignore` excludes `*.tgz`, `Assets/`, and generated `Assets.meta`; the
  package verifier now rejects either if they reappear.

### Verified

`real-game-demo/unity/topdown_shooter` was exported with Unity 6000.0.73f1 and
built Release for `iphoneos` without signing. Its final `ZombieWar.app` has one
VLPlay carrier (`UnityFramework`, 108 classes), 27 embedded frameworks, and
238 PNGs in `VLPlaySDKResource.bundle`, verified by
`tools/verify-ios-app-linkage.sh`.

## [1.3.1] — 2026-08-12

**Fixes the iOS build that 1.3.0 broke.** 1.3.0 was the first release whose
Podfile made partners resolve the SDK's whole dependency closure themselves —
and CocoaPods derives a pod target's deployment target from *that pod's own
podspec* (floor 8.0), not from `platform :ios, '15.0'`. `ZMJTipView` declares
iOS 8.0, below the libarclite floor Xcode 14.3+ removed, so the Pods project
failed to build. It never surfaced in-house because every VLPlay-authored
Podfile already carries a `post_install` pinning all pods to 15.0.

### Fixed

- **Native pin 1.5.0 → 1.5.1: `ZMJTipView` is gone.** The native SDK replaced it
  with ~60 lines on its own base view controller, so the dependency list is now
  **seven** pods (podspec and `Editor/VLPlaySDKDependencies.xml` both updated).
- **The iOS build post-processor pins the SDK's pod closure to iOS 15.0.** Even
  without ZMJTipView, `GTMSessionFetcher` (10.0), `PromisesObjC` (9.0) and
  `OneSignalXCFramework` (11.0) still declare targets below 15. The hook is
  written into the EDM4U Podfile before `pod install` and touches only the
  SDK's own closure.
  - It merges into a game's existing `post_install` **whatever the block
    variable is named**. Appending a second `post_install` would not merge:
    CocoaPods keeps only the last one and silently drops the other, so a game
    that wrote `post_install do |pi|` would have lost one hook with no warning.

## [1.3.0] — 2026-08-12

**Native pin 1.4.5 → 1.5.0 — the U-6 static split.** The vendored iOS framework
is now a **static** archive that bakes **zero** vendor classes: AppsFlyer,
FirebaseCore, GoogleSignIn and ZMJTipView arrive through CocoaPods like
everything else, so a game can finally ship its own Firebase or AppsFlyer
without two implementations of the same classes in one app. Android is a
version sync — the AAR never baked vendor classes.

### Changed

- **The binary podspec declares all eight dependencies** (was four): the
  FBSDK trio + OneSignal as before, now joined by `AppsFlyerFramework ~> 6.18`,
  `FirebaseCore ~> 12.12`, `GoogleSignIn ~> 9.1`, `ZMJTipView ~> 0.1`.
  `Editor/VLPlaySDKDependencies.xml` mirrors the same list. A Unity game
  changes **nothing** — EDM4U and the post-processor resolve it all — but the
  first iOS build after upgrading downloads more pods.
- **`-ObjC` now reaches UnityFramework** via the podspec's
  `user_target_xcconfig`. A static library's Objective-C categories are
  discarded without it; this SDK has 28, including the networking layer's —
  the failure would be a green build whose first sign-in dies with
  `unrecognized selector`.
- **`VLPlaySDKResource.bundle` is copied by CocoaPods** (`s.resources`) —
  a static framework's wrapper never reaches the app bundle, so the resources
  must travel on their own. `NSBundle+Additions` already resolves the
  main-bundle location at runtime.

### Verified

Re-vendor tool 7/7 · bindings 28/28 Android + 26/26 iOS measured against the
**static** binary · `.mm` clang trio against the 1.5.0 headers
(`-Werror=objc-method-access/-protocol/-deprecated-declarations`, pod-present
both slices + pod-absent) · C# 7/7 profiles, 85 assertions · `pod ipc spec`
valid with the new fields.

⚠️ **Not device-verified** — and this release changes the iOS link, so the next
iOS device pass matters more than usual: sign-in (the categories), any SDK
screen (the bundle), and Google login (GoogleSignIn now from pods).

## [1.2.5] — 2026-08-12

**Native pin moves for the first time since `v1.2.0`: 1.4.4 → 1.4.5.** No C# in
this package changed, and none needed to — the entire fix is inside one native
method body. It is here because a Unity game on iOS could not get it any other
way.

### Fixed

- **The whole iOS app froze the moment the game touched ads.** Typically on the
  `VLPlayAds.SetUserId(...)` that our own documentation tells you to make after
  sign-in — and this package exposes that call, which is why the wrapper had to
  move for anyone to escape it. The native AdMob provider's `setUserId` re-entered
  itself; the compiler emitted a tail call, so the stack never grew and it did not
  crash — it **spun**, forever, on whichever thread called it. On iOS a Unity
  game's script thread *is* the app's main thread, so the whole game stopped on
  its last drawn frame: input dead, no exception, **no crash log to collect**, and
  the floating ball still painted on top. Background work carried on, so analytics
  kept reporting after the freeze and the app looked healthy from the log.

  It only bites when the game's CMS `ads.provider` is `admob` and the provider has
  initialised — a game on `max` never saw it — so flipping a game from MAX to
  AdMob reads as though the flip broke it. **Android was never affected**: it is a
  different implementation, and Unity's script thread is not the main thread there,
  so the same class of native stall is invisible on that platform.

  There is no client-side workaround worth taking. Skipping `SetUserId` avoids the
  freeze and gives up the server-side reward attribution it exists for.

### Verified

Measured on the artifacts, not inferred from the release notes. On the binary this
package shipped at `1.2.4`, `-[VLPlayAdmobProvider setUserId:]` is a **single
instruction** branching into itself; on the one vendored here it tail-calls
`setSsvUserId:`, with **zero** self-sends left in the method on either slice.

The public surface is unchanged, which is why no bridge code moved: across all 24
iOS public headers the only difference is the version string in `Constants.h`, and
every Android class the bridge binds has byte-identical `javap` output (369 classes
in both). Bindings 28/28 Android (present *and* `-keep`-covered) + 26/26 iOS; C#
7/7 profiles, 85 assertions; the `.mm` type-checks against the **new** headers with
`-Werror=objc-method-access` in all three modes, and a preprocessor control proves
the pod-present run really compiled the SDK branch rather than falling through to
the fallback.

⚠️ **Not device-verified** — cut-then-verify, per the native SDKs' precedent.

## [1.2.4] — 2026-08-11

Same native pin as `v1.2.0`–`v1.2.3` (**v1.4.4**) — no binaries changed. **iOS
build pipeline only.** Everything here was found by reading an iOS device log from
a real game build: the app ran, signed in and reported analytics, while three
separate features were dead because of what the export did not contain.

### Added

- **The iOS export now turns on the Push Notifications capability** and writes
  `Unity-iPhone/VLPlay.entitlements` with `aps-environment`. Nothing did this
  before, so *every* Unity iOS build shipped without it: iOS issued no device
  token, OneSignal logged `'Push Notifications' capability missing!`, and the deep
  links that arrive on a notification tap — `VLPlayEvents.OnDeepLink`, added in
  `v1.2.1` — could never fire on iOS at all. New setting
  `VLPlaySettings.iosPushProductionEntitlement` picks the value: off (default) is
  `development`, on is `production` for TestFlight and the App Store. A value the
  provisioning profile does not match fails the Xcode archive, so a wrong one is
  loud rather than silent. It is unrelated to `useStaging`, which picks the VLPlay
  backend, not Apple's push environment.

- **The build now refuses a `GoogleService-Info.plist` issued for a different
  bundle id.** That file is not only Firebase's: the post-processor lifts
  `REVERSED_CLIENT_ID` out of it and writes it as the app's Google URL scheme, so
  the wrong file makes the build advertise another app's OAuth client. Google
  Sign-In then fails *after* the player has consented, leaving a Firebase bundle-id
  warning as the only clue. The check runs before the Xcode export exists, names
  both identifiers, and stays quiet when it has no evidence (no `BUNDLE_ID` key, an
  unparseable file, no plist configured) — it must never block a build on a guess.

### Fixed

- **The embed-frameworks check rejected healthy iOS exports.** It asserted one file
  by name, `Pods-UnityFramework-frameworks.sh`, but CocoaPods deliberately gives a
  *framework* target no embed phase — a framework does not embed its own
  dependencies, the host app does — so it writes `Pods-Unity-iPhone-frameworks.sh`
  against the app target instead. Every export on `1.2.0`–`1.2.3` failed, and the
  advice in the message ("delete `Pods/` and re-export") regenerated the same
  correct layout and failed again. The check now looks at every generated script,
  accepts any one that carries the frameworks and is actually run by a target, and
  keeps failing the real case it exists for: under static linkage CocoaPods
  generates no embed script at all and the app dies at launch.

### Changed

- **An empty `fbAppId` now says so at build time.** No `fb<APPID>` URL scheme is
  written, and iOS resolves URL schemes at install time, so no CMS value can repair
  it afterwards: the player taps Facebook, consents, and nothing comes back. The
  warning spells that out and is safe to ignore for a game without Facebook login.
  `FacebookAppID`/`FacebookClientToken` still stay out of `Info.plist` on purpose —
  those are per-game values the SDK applies at runtime from `/sdk/config`.

## [1.2.3] — 2026-08-10

Same native pin as `v1.2.0`–`v1.2.2` (**v1.4.4**) — no binaries changed. **Editor
and sample only: no device behaviour changes.** Worth taking anyway, because the
Editor mock is where a game's grant code gets written.

### Fixed

- **The Editor mock raised `OnRewardConfirmed` for every rewarded show.** On a
  device that event comes only from backend SSV, so a Mode B (`ssv:false`)
  placement never raises it — the mock was teaching games to grant in a place
  production will never reach. It now confirms only for a rewarded placement
  injected with `ssv:true`, matching devices.

- **A bad debug config failed silently.** Malformed JSON was swallowed, and
  well-formed JSON of the wrong shape did not even raise — either way ads stayed
  enabled and nothing confirmed, which is indistinguishable from correct Mode B.
  Both now warn, and the applied-config line reports how many `ssv:true` rewarded
  placements were recognised.

### Added

- **The sample ships both reward modes.** A **Mode** tile switches what the next
  "Ads cfg" injection sets up and shows which is armed. Without this the fix
  above left Mode A — the default for a game with a server — impossible to
  rehearse in the Editor at all, since ads there can only be enabled through the
  debug inject and the only config shipped was `ssv:false`.

  On a device `ssv:true` is **not** a shortcut: the SDK asks the backend for a
  reward ticket at preload, so the placement must exist in the CMS for that game
  and the backend must be reachable. A show that refuses without a ticket is the
  design working, not a sample defect.

## [1.2.2] — 2026-08-10

Same native pin as `v1.2.0` and `v1.2.1` (**v1.4.4**, iOS `f885cc5` / Android
`e6e792e`) — no binaries changed. One settings field became two, because one of
them could only ever be right for one of your two builds.

### Changed

- **`admobAppId` is now `admobAppIdAndroid` + `admobAppIdIos`.** An AdMob App ID
  is per platform — your Android app and your iOS app are two separate apps in
  the AdMob console, with two different ids — but a single field was written
  into both the Android manifest (`APPLICATION_ID`) and the iOS `Info.plist`
  (`GADApplicationIdentifier`). The documented workaround was to remember to
  switch the value before each build: a manual step in the middle of an
  automated pipeline, guarding a failure that does not announce itself. A
  **missing** App ID crashes at launch and you find out immediately. The **wrong
  platform's** App ID does not crash — ads simply never initialise, and Google
  counts the traffic as invalid. Each post-processor now reads its own field, so
  neither build can pick up the other's id.

  **Your existing value is carried over** into `admobAppIdAndroid`
  automatically; nothing is lost. Fill `admobAppIdIos` before your next iOS
  build — until you do, that build is refused with a message naming the field,
  rather than silently shipping the Android id. Android builds are unaffected.

  Only games that ship ads are touched at all. If `adsEnabled` is off, or your
  provider is MAX, there is nothing to do.

### Fixed

- **The build guard judges each target on its own App ID.** It previously read
  the one shared field, so filling it for Android also satisfied an iOS build.
- **Five documentation samples subscribed to events with lambdas** while our own
  troubleshooting page tells you to use named methods — a lambda cannot be
  unsubscribed, and with domain reload disabled the stale handler survives into
  the next Play and every event fires twice. Corrected on the site.

## [1.2.1] — 2026-08-09

Same native pin as `v1.2.0` (**v1.4.4**, iOS `f885cc5` / Android `e6e792e`) — no
binaries changed. Two things a game can see, and two corrections to what we told you.

### Added

- **`VLPlayEvents.OnDeepLink` — a tapped push notification now reaches the game.**
  Both native SDKs have owned this end for a long time: iOS holds
  `UNUserNotificationCenter`'s delegate and broadcasts `VLPlayDeepLinkNotification`,
  Android holds the OneSignal click listener and routes through
  `VLPlayDeepLinkRouter`. Nothing on the Unity side listened, so a player tapping a
  notification opened the game and landed wherever they left off — the notification
  might as well not have carried a destination. The wrapper now binds both ends and
  hands over a `VLPlayDeepLink` (`action`, `path`, `GetParam(...)`, `rawUrl`).

  The SDK still does not navigate for you: the link can land mid-cutscene or on a
  screen the game must not leave, so the route is the game's call.

  **A link that arrives before you subscribe is held, not dropped.** That is the whole
  point — a tap on a *dead* app is handled by the native SDK during launch, before your
  first scene has run a line of code, and both natives discard the event when nobody is
  registered. Two buffers close that window: the iOS bridge parks the link until
  `VLPlaySDK.Init()` opens the channel, and `OnDeepLink` holds it until the first
  handler attaches — which means your handler can fire inside your own `+=` line. Only
  the newest link is kept, and only the first subscriber gets it.

  Nothing arrives in the Editor; there is no native SDK there to route from.

### Changed

- **`Init()` now sets `Application.runInBackground = true`, and the game can take it
  back.** Every Unity game froze behind the SDK's HUD, progress spinner and OTP popup
  until it set that flag itself, and nothing we shipped said so — not the docs, not the
  reference demo. The cause is a deliberate native decision meeting a Unity default:
  those surfaces are Android `Dialog`s (the SDK is loaded by arbitrary host Activities
  and cannot assume a `FragmentActivity`), a `Dialog` takes window focus **without**
  pausing the Activity, and Unity stops the player loop on focus loss. No hang, no
  crash — the frame simply stops, and it reads as the SDK deadlocking the game.

  Opt out with `VLPlaySettings.runInBackground` (ticked by default); the SDK then never
  touches the flag. **A project with no settings asset counts as opted in** — it has not
  opted out of anything, and freezing is the worse default. Pressing home still pauses
  through `onPause()`, so this costs no battery.

- **The reward events now describe both crediting modes.** The XML docs said to grant
  **only** on `OnRewardConfirmed`. That is Mode A (the game has a server; `ssv:true`),
  where the ledger is already written when the event arrives and you reconcile rather
  than add. It is wrong for Mode B (no game server, currency purely local, `ssv:false`),
  where the reward path never reaches the backend, confirmation never fires, and
  `OnUserRewarded` **is** the crediting event — deliberately spoofable, which is the
  trade a Mode B title accepts. `SetUserId` guidance was corrected the same way: native
  1.4.4 does raise `OnSignIn` after a cached auto-login, so the extra bootstrap bind is
  only needed on older natives.

### Fixed

- **Documented Android floor was 22; the AAR's merged manifest says 21.** The constraint
  had been copied rather than measured, and it was turning away Android 5.0 titles the
  SDK actually supports.

- **`tools/verify-native-bindings.sh` was blind to 16 of the 26 iOS names it claimed to
  cover.** KVC keys passed through a helper — the helper body reads `valueForKey:key`, a
  variable, while the literal lives at the call site — so 4 `VIDUser` fields and all 12
  `AntiAddictionStatus` ones went unchecked while the script reported "10 names, all
  present". A missing KVC key throws `NSUnknownKeyException`, so that class of drift is a
  crash rather than a silent skip. Nothing was actually broken on 1.4.4 or 1.4.1; the gap
  was in what could be proven.

## [1.2.0] — 2026-08-05

**Re-vendored the native binaries to `v1.4.4`** (iOS `f885cc5` / Android `e6e792e`),
two releases on from the `v1.4.1` that `v1.1.0` shipped. What a Unity game gets out
of that jump:

- **A login now survives a force-close.** The native session-resume path was
  probabilistic: the credential store kept a `Cipher` in an instance field on a
  process-wide singleton, so a concurrent read could land between `init()` and
  `doFinal()` and the write was lost — silently, the setter swallowed the exception.
  A first login on a fresh device is the noisiest moment there is, which is why it
  read as "only saves from the second login onwards". `signIn()` is now
  refresh-token-first on both platforms: a live session goes straight into the game
  **without the popup flashing**. `OnSignIn` still fires on that path.
- **Signing out actually signs you out.** Making `signIn()` refresh-first exposed
  that no credential-clearing path ever erased the refresh token, so a logout left
  it on disk and the next launch resumed the session the player had just ended.
  Fixed native-side, with an invariant test that fails if a future key is written
  and not cleared.
- **The FAB no longer decodes a full-resolution network image into a 56dp circle** —
  the Google Play "unoptimised bitmap" flag, fixed by sampling to the drawn size.
- iOS is **StoreKit-only** and Android **Google-Play-only** from 1.4.3 (the WebView
  payment bridge and the wallet deep-links are gone; Apple rejected the former under
  Guideline 3.1.1). Nothing the wrapper binds went with them — `Purchase`,
  `GetProductCatalog`, `GetPurchaseHistory` and `RestorePurchases` are untouched.

**BREAKING — `VLPlayAccount.RedeemGiftcode` is removed.** Native 1.4.3 deleted the
giftcode API outright on both platforms (Android `redeemGiftcode` +
`$GiftcodeRedeemListener`, iOS `redeemGiftcode:completion:` — verified absent from
the 1.4.3/1.4.4 binaries), so the wrapper had nothing left to bind: on iOS the
`.mm` stopped compiling against the shipped headers, and on Android the call
resolved `ErrNativeCall` at runtime. A game that redeems codes does it outside the
SDK now. This alone makes the next release a **minor** (v1.2.0).

Also removed the `.mm`'s `#import <VLPlaySDK/VLPlayBridge.h>` — that header left
the native public headers in 1.4.3 (StoreKit-only cut);
`VLPlaySDKPaymentDidFinishNotification` is declared in `VLPlaySDKManager.h`, which
was already imported.

Repairs from an independent audit of `v1.1.0`. Nothing here is released yet.

**`VLPlaySDK.UpdateGameInfo(serverGameId, roleId, roleName)` — new, and the reason
this batch exists.** The `roleId` and `serverGameId` the SDK puts on every purchase
come from a native store that only `updateGameInfo` writes; both natives have had
that call public for a long time and **the wrapper never exposed it**. A Unity game
therefore shipped every purchase with an empty `roleId` — the store charged, the
receipt verified, the backend answered 200, and the delivery callback reached the
game server with nothing to route by (`1036`, ~35 minutes of retries, then ops).
Nothing failed loudly anywhere in that chain. `SetFABContext` was not a substitute:
it feeds a different store, and on the vendored iOS binary nothing reads it back.
The demo sample and the facade's own example also used the key `serverId`, which
the native support tab never reads — the key it reads is `serverGameId`.

**Account operations now resolve exactly once, even when the call never reaches
native.** `BindSocial`, `UpgradeGuest` and `DeactivateAccount` (and `RedeemGiftcode`,
since removed above)
dispatch through the Android UI thread; if there was no activity, or the hop threw,
or the native call threw, the failure was logged and the game's callback was never
invoked — leaving a spinner up forever against a documented promise that every API
"resolves exactly once". All three paths now resolve with `ErrNativeCall`, and the
proxy carries a one-shot guard so a late native reply cannot deliver on top of it.

**The in-flight proxy set is locked.** Adds ran on the caller's thread and removes
on the dispatcher; the facade documents every API as safe to call at any time and
never checked the thread, so a game starting an account op from a worker mutated a
`HashSet` concurrently.

**A player build on an unsupported platform no longer runs the Editor mock.** It
used to, which meant a standalone build of a mobile game reported a signed-in user
and *successful purchases*. New `UnsupportedBridge` fails every call instead.

**Builds now fail before export on configurations that cannot work**, rather than warning: iOS
without a resolvable package, without the vendored pod, or without a Podfile (the
SDK would simply be absent); either player target without settings/`clientId`, with ads settings
out of sync with the generated EDM4U vendor dependency, or with a generated AdMob dependency and
no app id — a case whose own code comment already said "MobileAds will crash at launch".

**The manifest patcher checks each SDK activity separately.** It used to treat
`LoginActivity` as a stand-in for all three, so a game that declared only that one
never got `AccountInfoActivity` or `payment.Payment`, and hit
`ActivityNotFoundException` later, far from the cause.

**The demo sample handles `OnNotice` / `OnNoticeCleared`.** Since native 1.3.3 the
SDK draws no toasts of its own; a game that does not render notices shows the player
nothing at all. The sample subscribed to 14 of 16 events while its README claimed
"every callback", and the two missing ones were exactly these.

**Corrected in the sample's README:** a dismissed store sheet reports `Cancelled` on
both platforms. The claim that iOS could only report `Failed` was wrong when it was
written.

**A player build with no `VLPlaySettings` asset at all now fails.** The previous pass
refused an *empty* `clientId` on Android; the more common case — the asset was never
created — still warned and carried on, on both platforms. On Android that `return`
also skipped the three activity declarations, so the app authenticated against
nothing *and* threw `ActivityNotFoundException` on the first login tap; on iOS it
produced an Xcode project with no `VLPlaySDK-Info.plist` in it. iOS also never
refused an empty `clientId` the way Android did. Both platforms now ask the same
question through one guard and give the same answer, naming the file to create.

**The manifest patcher reads the manifest as XML.** `xml.Contains(name)` calls an
activity "declared" when the name only appears in a comment, and when the host wrote
`tools:node="remove"` — which means the merged manifest will *not* have it. Either
one skipped the injection and produced the `ActivityNotFoundException` above.
`activity-alias` now counts, a longer name that merely contains ours does not, and a
manifest that will not parse says so instead of guessing.

**The per-call runnable is released when the UI-thread hop itself throws.** The
release rides inside the runnable, so a hop that never ran left it in a static set
for the life of the process. The game's callback was already covered (`FailLocally`
is one-shot); this is the collection, not the contract.

**Corrected in the README:** shipping the Firebase Unity SDK or the AppsFlyer Unity
plugin alongside this package on iOS was described as a *link-time* clash. It is not
— measured on the `v1.4.4` binary this release vendors, the xcframework exports
**72 AppsFlyer classes** (`6.18.0`) and **20 Firebase/GoogleUtilities classes**
(it was 73 and 21 on the `v1.4.1` the claim was first written against), and because each
copy lives in its own Mach-O image the link succeeds and the build goes green. The
collision is at runtime, where the Objective-C loader keeps one of the two and says
which one is undefined. Tracked native-side as `NAT-IOS-EMBEDS-SHARED-DEPS`. The
same entry now also states that the native SDK *owns* the AppsFlyer instance on both
platforms — a second owner calling `initSDK`/`startSDK`/`stopSDK` races it.

**Repo checks now run in CI** (`.github/workflows/verify.yml`), covering the three
failure classes this repo has actually shipped: a name bound at runtime with no
ProGuard keep, a sample calling an API the wrapper no longer has, and `NativeLibs~`
being dropped from the tarball. `tools/verify-csharp.sh` type-checks **seven** define
profiles with the sample included and warnings as errors — one per bridge, plus two
that compile `Editor/` against a stub `UnityEditor`, which nothing did before: Unity
puts those scripts in their own assemblies, so a wrong name in the code that decides
whether a build ships credentials was only ever found by opening the project. It then
runs **34 assertions** over the three build-time decisions above, because a type-check
cannot see a wrong branch. `tools/verify-package.sh` audits the packed tarball.

**The binding checker now asks whether a class still exists, not only whether it is
kept.** It verified `-keep` coverage in the AAR's `proguard.txt` and stopped there,
and the keeps are wildcards: `-keep interface …VLPlaySDKManager$* { *; }` went on
matching `$GiftcodeRedeemListener` for as long as the name was asked about — which
is to say, after native deleted the interface. So the re-vendor tool's binding check
passed while the Android runtime was certain to throw `ClassNotFoundException` on
the first call, and the wrapper's `try/catch` would have turned that into one log
line. Coverage is not existence; each derived name is now also looked up in
`classes.jar`, with its own fabricated-name control. Run against the pre-removal
bridge and the 1.4.4 AAR, the coverage half still reports all 27 names green while
the new half names `$GiftcodeRedeemListener` and fails the run.

**Those refusals now happen before Unity writes anything.** They used to run in the
post-processors, which meant the build went red *after* the Xcode project or the
Gradle project had already been written — and that export is the deliverable. A
partner who saw the red Unity build, shrugged, and opened the generated
`Unity-iPhone.xcworkspace` got a **green** Xcode build carrying no
`VLPlaySDK-Info.plist`: exactly the outcome the guard exists to prevent, reached by
walking around it. `VLPlayPreprocessBuild` rejects the configuration up front and
leaves nothing behind; the post-processors keep only the checks that genuinely need
the generated path (Podfile, vendored pod, manifest).

**The AdMob check reads the generated dependency manifest, not the settings asset.**
`VLPlay ▸ Sync Ads Dependencies` is a menu item and says so — "not automatic on
build on purpose" — so `VLPlaySettings` and the vendor EDM4U actually resolves can
disagree in both directions. Ads turned off in settings while a stale
`VLPlayAdsDependencies.xml` still ships AdMob is the dangerous one: the plist key is
skipped and MobileAds aborts at launch. Turning ads on without re-syncing is not a
launch crash, but the requested ads integration is absent because no vendor is in
the build; letting that configuration pass would silently ship ads-off. Both
directions are therefore refused and say which side to sync. A typo in
`adsProvider` — previously an unknown vendor that silently produced no dependency
at all — is also refused rather than shipped as ads-off.

**The guard lives in its own module-independent assembly.** Put it in `Editor/` and
it belongs to `VLPlay.Sdk.Editor`, whose asmdef carries a precompiled reference to
`UnityEditor.iOS.Extensions.Xcode.dll`. On a studio machine with only Android Build
Support installed, that assembly does not compile while
`VLPlay.Sdk.Editor.Android` still does — so the one check standing between a partner
and a credential-less build drops out of every Android build, quietly. No machine
with both modules can observe that, which is every machine this repo is tested on,
so `tools/verify-csharp.sh` asserts it structurally instead: the assembly governing
`VLPlayPreprocessBuild.cs` must carry no precompiled references and reference nothing
but `VLPlay.Sdk`, with `VLPlay.Sdk.Editor` as the control that proves the check can
still tell the two apart.

## [1.1.0] — 2026-08-01

**Re-vendored the native binaries to `v1.4.1`** (iOS `924b007` / Android `f0149a8`), up from `v1.3.5` — the wrapper skipped 1.3.6, 1.3.7, 1.3.8, 1.3.9 and 1.4.0. **Four defects that are live in every build running `v1.0.1`** are fixed by this bump alone, with no wrapper code change:

- **iPad: touching anywhere outside the SDK's own popup froze the whole app.** The SDK's overlay window compared the hit-test result against two objects, but UIKit wraps the root view controller in three more layers on iPad, so the window matched neither and swallowed every touch that belonged to the game underneath. The game and the SDK popup both rendered and neither responded; only the floating ball worked, because it lived inside the window doing the swallowing. Present in **every released artifact up to 1.3.9** — including the one `v1.0.1` ships. Fixed in native 1.4.0 by `VLPlaySDKPassthroughWindow`.
- **iOS purchases reported a revenue of zero to AppsFlyer.** No `af_revenue` anywhere in the iOS SDK before 1.4.1, so every iOS transaction arrived worth nothing.
- **The IAP catalogue and purchase history came back empty in any minified build.** The wrapper reads `StorePackage`, `StorePackage$Item` and `TransactionItem` one getter at a time over JNI, and nothing in the AAR's `proguard.txt` covered `sdk.vlplay.vn.payment` until native 1.3.8 — R8 renamed them, every call threw, and the wrapper's `try/catch` turned it into a single log line. Same failure class as the ads keeps (1.3.3) and the anti-addiction keeps (1.3.5), and the reason `tools/verify-native-bindings.sh` was red on this repo's tip: it now passes.
- **Push-tap handling could crash a partner's app at launch.** Games were told to register the OneSignal click listener themselves, which throws if OneSignal is not yet initialised. From native 1.4.1 the SDK owns that hook; a host only listens for the routed deep link.

Behaviour a Unity game can observe, inherited from the same bump: **sessions now resume on cold launch by default** (`isSaveAccessToken` flipped `NO` → `YES` in native 1.3.7 — a game that never set it silently lost auto-login on every launch before), the SDK's keyboard mirror bar is gone (1.4.0), a cold-launch purchase replay no longer swallows an already-paid transaction (1.3.9), and the payment notification now carries **`deliveryPending`** — backend code 1036, meaning the player HAS been charged and the receipt IS verified while delivery to the game server is still retrying. It is a **success** state; the `.mm` already surfaces it on `VLPlayPurchaseResult`.

**Zero wrapper-API impact, proven on the artifacts rather than the source tree:**

- Of the **27 classes and interfaces this wrapper binds by name**, 26 are `javap`-identical 1.3.5 ↔ 1.4.1 and the 27th (`VLPlaySDKManager`) only **gained** members — nothing removed, nothing renamed. A fabricated class name fails to resolve in the same run, so the comparison is not vacuous.
- The iOS public `Headers/` differ in four files, all additive: the version literal, a `trackCustomEventAccepted:params:` reporting whether an event survived the CMS whitelist, a purchase overload carrying `deliveryPending`, and rewritten documentation on the orientation and `isSaveAccessToken` properties. No selector moved — proven by the strict-clang trio (pod-present device **and** simulator against the **new** headers, plus pod-absent) running clean with `-Werror=objc-method-access -Werror=protocol -Werror=undeclared-selector -Werror=deprecated-declarations`.
- `StorePackage`, `StorePackage$Item`, `TransactionItem` and `AntiAddictionStatus` are **byte-identical `.class` files** across the two versions, so the parsers the `.mm` mirrors did not change and the differential harnesses' verdict still holds (control: `VLPlaySDKManager.class` does differ).
- Artifact checks: both slices self-report `1.4.1`, the AAR bakes `1.4.1` and stays vanilla (0 vendor bytes), 25 consumer keeps, the nested `VLPlaySDKResource.bundle` carries 119 png / 0 tiff **in both slices**, 0 `.meta` in the pod dir, `pod ipc spec` valid, and the vendored AAR is **SHA-256 identical** to the published `vlgame-lib-sdk` artifact. The AAR still declares only its own four activities, so the build post-processor's injection of `LoginActivity` / `AccountInfoActivity` / `payment.Payment` remains necessary and does not duplicate.

### `VLPlayOrientation.FollowApp` — the SDK follows the game's orientation, and the wrapper can finally say so

Native gained `ORIENTATION_FOLLOW_APP` in **1.3.6**, made it the **default** (`sdkUiOrientation = ORIENTATION_FOLLOW_APP`) and deprecated `ORIENTATION_FOLLOW_SYSTEM` in the same release. The wrapper had no matching value, which cost two things on every version since:

- **`GetOrientation()` reported the wrong value on a fresh install.** Native held `3`; `FromNative` had no case for it and fell through to `FollowSystem`.
- **`SetOrientation(FollowSystem)` was a downgrade dressed as the default.** It is the C# default *value* of the enum, and the facade's own documentation called it the default *behaviour* — but on Android it maps to `SCREEN_ORIENTATION_UNSPECIFIED`, the constant native had just deprecated. A game asking for "the default" moved the SDK off it.

`VLPlayOrientation.FollowApp` is now the documented default: **the SDK's screens adopt the orientations the game supports** — a landscape-only game keeps the SDK landscape, and the SDK never unlocks an orientation the game does not declare. Android maps it to `SCREEN_ORIENTATION_BEHIND`, iOS to `UIInterfaceOrientationMaskAll`, which the 1.4.1 header defines as "follow the app" for exactly this reason. Nothing needs to call `SetOrientation` to get this; it is what the native SDK already does.

- **Appended, never renumbered** — the enum's ordinals are the iOS bridge's wire value. `FollowSystem` stays at `0` and keeps working; it is now documented as legacy.
- **Platform drift, documented on the getter:** iOS cannot distinguish the two (both are `MaskAll`), so it reports `FollowApp` for either. The getter reports what the SDK actually does rather than echoing the setter.
- Verified: 4-profile C# compile (`UNITY_ANDROID`, `UNITY_IOS`, and both under `UNITY_EDITOR`) clean with warnings-as-errors, with a negative control that fails on an invented enum member; the strict-clang trio re-run clean after the `.mm` change.

## [1.0.1] — 2026-07-27

**Anti-addiction status works in minified builds.** Re-vendored the native binaries to **`v1.3.5`** (iOS `bc5b91a` / Android `aad630e`), whose only change is two ProGuard keeps in `consumer-rules.pro` for `sdk.vlplay.vn.antiaddiction.AntiAddictionManager` and `AntiAddictionStatus` — classes this wrapper has bound **by name over JNI** since U2-4 (2026-07-16) and which nothing kept.

R8 runs in the host app and cannot see a JNI binding, so with Player Settings ▸ Publishing ▸ **Minify** enabled the class was renamed and `AndroidBridge.GetAntiAddictionStatusJson()` threw `ClassNotFoundException` into its own `catch` — `VLPlaySDK.GetAntiAddictionStatus()` returned `null` forever and `DebugForceAntiAddiction()` was a no-op, silently, with only a `Debug.LogWarning`. Same failure class as the ads/FAB keeps added in native v1.3.3, but quieter: renamed rather than deleted, and enforcement was never affected — the native session limits and `OnAntiAddictionWarn` / `OnAntiAddictionKick` kept working throughout, since they route through `VLPlaySDKManager$AntiAddictionListener`, which was already kept.

- **Proven on a real minified build**, which is what was missing when the ads keeps shipped. With the keeps in, `AntiAddictionManager` and `AntiAddictionStatus` hold their original names in `classes.dex` and `getLastStatus` / `debugForceKickPopup` / `getRemainingMinutesToday` / `shouldKick` are all present — while `AntiAddictionConfig` and `HeartbeatRequestBuilder`, same package but not kept, lose their names in the very same R8 pass. That contrast is the proof; presence alone would not be, since an un-minified build shows everything.
- **Zero wrapper-API impact, proven on the artifacts:** all 24 classes and interfaces this wrapper binds by name are `javap`-identical 1.3.4↔1.3.5 (the 7 `AndroidJavaClass` targets + the 14 nested manager listeners + the 3 ads listeners), and the iOS public `Headers/` differ in exactly one line — the version literal in `Constants.h` — across all 25 headers, so no selector moved. Both slices self-report `1.3.5`, the AAR bakes `1.3.5` and stays vanilla (0 vendor bytes, 22 consumer keeps), nested bundle 119 png / 0 tiff, 0 `.meta` in the pod dir, `pod ipc spec` valid.
- **Distribution:** the docs' install page now leads with the channel a partner can actually use — ask VLPlay for the `.tgz`. The git URL and `vlgame-lib-sdk` both sit behind the private `VL-Game` org and are marked internal. The durable rail (`/sdk/download?type=unity-sdk`) is still gated on a backend change.

## [1.0.0] — 2026-07-26

**First release.** The full surface — auth + events (U2), IAP V3 (U3), ads/IAA (U4), account ops + FAB (U5), host notices, anti-addiction — over native **v1.3.4**, all verified against the shipped binaries (differential parser harnesses, strict-clang trios, full JNI/javap sweeps, 3-profile C# compiles). ⚠️ Released ahead of the combined on-device certification pass, consistent with how the native SDKs cut (v1.2.1/v1.3.2/v1.3.3/v1.3.4 precedent); anything user-facing found there lands as v1.0.x.

- **Re-vendored native binaries to `v1.3.4`** (iOS `d8cae82` / Android `4faa55d`). One native change: the FAB delete-account funnel now emits `sdk_delete_account_entry_click` and `sdk_delete_account_cancel` — analytics-only, native-internal. **Zero wrapper-API impact, proven on the artifacts:** every class and nested listener interface this wrapper binds by name over JNI is `javap`-identical 1.3.3↔1.3.4 (`VLPlaySDKManager`, `VLPlayFAB`, `VLPlayAds`, `AdManager`, `AdsConfig`, `AntiAddictionManager`, all 10 manager listeners + the 3 ads listeners), the iOS public headers differ only in the version literal (verified on the source range `8fe050a..d8cae82` — no `.h` touched but `Constants.h`), both slices self-report `1.3.4`, the AAR bakes `1.3.4` and stays vanilla (0 vendor bytes, all 20 consumer-proguard keeps intact), nested bundle 119 png / 0 tiff, `pod ipc spec` valid.

- **Notices reach the game — `VLPlayEvents.OnNotice` / `OnNoticeCleared`** (native v1.3.3). From v1.3.3 the native SDK stopped drawing its own iOS notification bar / Android Toasts and hands every short user-facing message (form validation, BE errors, operation results) to the host instead. **Before this the Unity wrapper had no passthrough at all, so a Unity game received nothing** — the notice was only written to the native log and the player saw nothing. `OnNotice(string message, bool sticky)` mirrors both native signatures 1:1; `sticky = true` means keep it up until `OnNoticeCleared` (iOS connection-lost / purchasing; Android never emits it today). Android goes through one `NoticeProxy` registered alongside the other listeners; iOS implements the two optional delegate methods in the `.mm` and ships them over the existing envelope as `{"message":…,"sticky":…}`. ⚠️ **Subscribe or the player sees nothing** — the same trap the native hosts have.
- **Re-vendored native binaries to `v1.3.3`** (iOS `8fe050a` / Android `df49e76`). Beyond the notices change: AdMob rewarded placements could not be shown **at all** before this (the reward-ticket parser demanded a signature the backend deliberately omits for AdMob), the ads config cache no longer drops unknown top-level blocks on cold launch, reward tickets now carry the platform so per-OS `platformOverrides` resolve server-side, and — the one that matters most here — **`consumer-rules.pro` now keeps the classes this wrapper binds by name over JNI** (`VLPlayAds`, the three ads listeners, `AdManager`, `AdsConfig`, `VLPlayFAB`). They have no Java caller inside the AAR, so R8 was **deleting** them: any Unity game with Player Settings ▸ Publishing ▸ Minify enabled lost ads and the floating button entirely, `ClassNotFoundException` swallowed into a `Debug.LogError` — no crash, nothing visible to the player. iOS additionally exposes `+[VLPlayAds debugApplyConfig:]` publicly, so `DebugApplyLocalConfig` no longer needs its runtime-resolution workaround on that platform.
  - **Verified on the artifacts:** both slices self-report `1.3.3`, the AAR bakes `1.3.3` and stays vanilla (0 vendor bytes), nested bundle 119 png / 0 tiff, 0 `.meta` in the pod dir, `pod ipc spec` valid. Every JNI name the bridge binds resolves in the new AAR, and `VLPlaySDKManager$NoticeListener` exposes exactly `onNotice(String, boolean)` + `onNoticeCleared()`. The strict-clang trio (pod-present device + simulator against the **new** headers, plus pod-absent) is clean with `-Werror=objc-method-access -Werror=protocol -Werror=undeclared-selector -Werror=deprecated-declarations` — that is what proves the new delegate selectors actually exist on the shipped headers rather than merely compiling.

- **Re-vendored native binaries to `v1.3.2`** (iOS `d7b108b` / Android `7405149`, up from v1.2.1 — the wrapper skipped 1.3.0 and 1.3.1). On top of the 1.3.0/1.3.1 auth-UI work described below, 1.3.2 adds the FAB's delete-account flow (Account → Hồ sơ → an in-HUD confirm screen → `deactivateAccount`), a version watermark on the HUD sheet, a landscape scroll fix on the Profile sub-tab, and — iOS only — Apple auto-login now resuming via the BE refresh token instead of the ~10-minute Apple identity token. **Zero wrapper-API impact, proven on the artifacts:** every bound Android class is byte-identical 1.3.1↔1.3.2 (`VLPlaySDKManager` 137 members, `VLPlayFAB` 16, `AntiAddictionManager` 17, `VLPlayAds` 12, `AdManager` 17, `AdsConfig` 24), every listener/callback interface is identical, and **no class was removed or renamed** — the six new classes are all anonymous inner classes of `VLPlayFABAccountProfile`, which the wrapper does not bind. The iOS public `Headers/` differ only in the version literal plus a `nullable` annotation + comment on `-handleSignInResponse:` (same selector, same ABI, and the `.mm` never calls it). Both slices self-report `1.3.2`, the AAR bakes `1.3.2` and stays vanilla (0 vendor bytes), nested bundle 119 png / 0 tiff. Note the FAB delete-account flow is reachable through the existing `ShowFloatingBall` passthrough — no new wrapper API.

<details><summary>Earlier note from the 1.3.1 re-vendor (superseded above, kept for the API-stability evidence)</summary>

- **Re-vendored native binaries to `v1.3.1`** (iOS `a0f8867` / Android `546f4ff`, up from v1.2.1 — the wrapper skipped 1.3.0). The two bumps are UI/config only: the auth-popup redesign aligned to the Web SDK + a version watermark on the login card (1.3.0), Android auth assets re-derived from the Web SDK + guest social icon → game controller (1.3.1, Android was a pure version-sync), and the Package-platform `&platform=ios|android` query param on the product-catalog call (1.3.0, internal — `GetProductCatalog`'s signature is unchanged). **Zero wrapper-API impact, proven on the artifacts:** every bound Android class's public surface + both `AndroidJavaProxy` listener interfaces are byte-identical 1.2.1↔1.3.1 (javap-diffed), and the iOS public `Headers/` differ only in the version literal + one doc-comment; the `.mm`'s KVC keys (`accessToken`/`lastStatus`/`signedIn`) are present in the 1.3.1 binary. Both slices self-report `1.3.1`, the AAR bakes `1.3.1` and stays vanilla (0 vendor bytes), nested bundle 119 png / 0 tiff.

</details>

- **Account ops — `VLPlayAccount`** (U5-1): `BindSocial(provider, cb)` (OAuth runs INSIDE the SDK — game integrates no Facebook/Google SDK; ⚠️ `apple` is **iOS-only**, the Android native rejects it) · `UpgradeGuest(username, password, email, cb)` (email `""` omits; a non-empty email runs the SDK's email-OTP dialog first) · `ShowIdentityVerification()` / `ShowGuestUpgrade()` (native popups, fire-and-forget) · `RedeemGiftcode(code, cb)` (success carries the BE body in `dataJson`) · `DeactivateAccount(cb)` (⚠️ destructive; `OnSignOut` follows on success — App Store 5.1.1(v) requires the path to exist). One terminal `VLPlayAccountResult` per call on the Unity main thread; flows that present native UI (OAuth handover, OTP) resolve at their real end. All four native listener interfaces share one shape, so Android uses a single proxy class; iOS routes completion blocks through the request-id envelope (`accountResult`). `deactivateAccountWithCompletion:` lives only in the iOS INTERNAL header — resolved at runtime (selector verified in the shipped binary).
- **FAB passthrough on `VLPlaySDK`** (U5-2): `ShowFloatingBall` / `DismissFloatingBall` / `ConfigureFAB(json)` (local override, merged under CMS; flat keys) / `SetFABContext(json)` (string values; ≥6 chars unlocks the support form). Bridged to the **canonical `VLPlayFAB` class on both platforms** — the manager-level FAB methods turn out to be deprecated shims on iOS (`__deprecated_msg` → `+[VLPlayFAB …]`), and mirroring the canonical home keeps `-Werror=deprecated-declarations` clean. Android account/FAB calls all run on the UI thread (the flows present dialogs/windows; Unity's script thread is NOT Android's main thread — it IS on iOS).
- Demo console gains a second TÀI KHOẢN row (Bind G / Upgrade / Identity / Guest UI) + (Giftcode / FAB toggle / Deact ⚠); `Samples~/Demo` regenerated (no hardcoded giftcode there).
- **Ads (IAA) surface — `VLPlayAds` + 7 events** (U4): provider-agnostic facade over the native ads module (AppLovin MAX / Google AdMob, chosen server-side via CMS `ads.provider`; no API names a vendor): `IsEnabled` / `IsInitialized` / `IsReady(placement)` / `Preload` / `PreloadAll` / `ShowRewarded` / `ShowInterstitial` / `SetUserId` + QA-only `DebugApplyLocalConfig(json)`. Lifecycle arrives as broadcast events on `VLPlayEvents` — `OnAdLoaded` / `OnAdLoadFailed` / `OnAdShown` / `OnAdDismissed` / `OnAdShowFailed` / `OnUserRewarded` / `OnRewardConfirmed` — each carrying the placementId; the native per-call listeners are bridged through one long-lived proxy/delegate per type. **INTEGRITY: grant ONLY on `OnRewardConfirmed`** (BE SSV); `OnUserRewarded` is the network's advisory signal. Everything is safe to call at any time — disabled/not-ready surfaces as a provider-neutral `VLPlayAdError` event, nothing throws into game code.
  - `SetUserId` is the integrator's job (the native SDK does not wire it) — call it after sign-in, before any rewarded show; buffered natively until the provider is ready.
  - `DebugApplyLocalConfig` injects a local `ads` config (same JSON as the CMS block) so the pipeline runs without CMS provisioning — the AdMob **public test units fill everywhere including emulators, no account**; ssv:true placements injected this way can never confirm (no BE ticket). Android routes through `AdsConfig.fromJson` + `AdManager.init/applyConfig` on the UI thread (the same path as the native sample's test card); iOS resolves `VLPlayAdsConfig`/`VLPlayAdManager` at runtime (not public headers) — both verified present in the vendored v1.2.1 binaries.
  - Editor mock: ads stay off until `DebugApplyLocalConfig`; preload→loaded, rewarded runs the full shown→advisory→confirmed→dismissed sequence on successive dispatcher ticks so the grant path is exercisable in Play Mode.
  - **Build plumbing (U4-3):** `VLPlaySettings` gains `adsProvider` (admob|max) + `admobAppId`; menu **VLPlay ▸ Sync Ads Dependencies** writes/deletes `Assets/VLPlay/Editor/VLPlayAdsDependencies.xml` (in Assets, not the package — git-URL installs live in the read-only PackageCache; EDM4U scans any Editor/ folder) pinning `play-services-ads:23.6.0`/`Google-Mobile-Ads-SDK` or `applovin-sdk:13.5.1`/`AppLovinSDK` (13.5.1 deliberate — 13.6.x raises manifest minSdk to 23/24). Post-processors inject the AdMob `APPLICATION_ID` manifest meta-data (Android) / `GADApplicationIdentifier` (iOS) — mandatory or MobileAds crashes at launch — plus the chosen vendor's own permanent `SKAdNetworkItems` id (the FULL merged network list stays a go-production task; lists rot).
  - Do NOT install AppLovin's MAX Unity Plugin or the AdMob Unity plugin alongside — the native SDK owns mediation.
- **Fix: the U3 purchase observer broke pod-present compilation** — `VLPlaySDKPaymentDidFinishNotification` is declared in `VLPlayBridge.h`, which the bridge never imported (only the umbrella imports it). Undetected because U3's differential harnesses run pod-absent and no real iOS export has been built since U3 landed; the next one would have failed loudly. Caught by re-running the strict-clang pod-present check against the shipped headers; import added.
- **`Samples~/Demo` is now a real importable sample** (U6-2): an IMGUI console covering the whole shipped surface — sign-in/out, anti-addiction (warn/kick/status), IAP V3 (catalog/buy/history/restore), passthroughs — with an env badge (STAGING/PRODUCTION from `VLPlaySettings.useStaging`), a signed-in user pill, and a live `VLPlayEvents` log panel. Import from Package Manager → create your `Resources/VLPlaySettings` asset → add `VLPlay.Demo.VLPlayDemoConsole` to an empty scene. Buy deliberately requires Catalog first (no product id is hardcoded in the sample). The full demo project (scene, build scripts, stg creds) stays in `vlgame-demos/vlplay-unity-demo`.
- **`tools/update-native-binaries.sh`** (U6-4): one-command re-vendor of a published native version from a `vlgame-lib-sdk` checkout — places the AAR + xcframework (GAP2-safe rename), bumps the binary podspec, then verifies on the artifacts: both slices' `Constants.h` self-report the version, the AAR bakes the version literal and stays vanilla (0 vendor bytes), the nested resource bundle is sane (pngs, 0 tiffs), no `.meta` leaked into the pod dir, `pod ipc spec` parses. It does not replace the §6 compile/javap/KVC harnesses — it prints what's still on you.
- **`VLPlaySDK.GetPurchaseHistory(page, size, callback)`** (U3-4) — one page of the player's IAP history (Apple + Google rows, server-filtered by the signed-in account) as `VLPlayTransaction[]`; empty history is a success with an empty array (BE 1049), not an error. Unblocked by the v1.2.1 re-vendor: the iOS method existed in the binary but was internal-only until `51d7082`.
  - iOS returns raw BE rows, so the .mm **mirrors Android's `TransactionItem` parser** (identifier drop rule, `transactionId` ← code/_id fallback, packageName → name → productId chain, `paymentStatus` ← legacy `status`, currency's absent-vs-empty default) — both parsers executed over the same 12 rows, diffed field-by-field, identical. One mirrored *behavior* is deliberately not the *intent*: a legacy numeric `createdAt` is coerced to its decimal string, because that is what org.json's `optString` actually does (Android's numeric-fallback branch is unreachable).
  - `createdAtEpochMillis` never crosses the bridge — computed in C# with **one parser for both platforms**, restricted to the exact zone-required input space Android's four `SimpleDateFormat` patterns accept (a zoneless timestamp yields 0 on both). Executed on 7 cases including a `+07:00` offset resolving to the same instant as its `Z` form.
  - ⚠️ Pagination metadata (`totalPages`/`page`/`limit`) is **Android-only** — the iOS SDK consumes the response envelope; iOS reports -1 for all three. Platform-neutral paging idiom: `transactions.Length == size` → probably more.
- **`VLPlaySDK.RestorePurchases(callback)`** (U3-4) — re-credits previously-purchased **non-consumables** (App Store 3.1.3(b) mandatory UX; same API on Android). One terminal callback: restored `{productId, transactionId}` items (possibly empty — nothing to restore is a success) or an error. BE re-verification is idempotent (1034), so no double-grant; per-item failures are skipped native-side (best-effort batch). Wire to an explicit "Restore purchases" button.
- **`VLPlaySDK.Purchase(storeProductId | VLPlayPurchaseRequest, callback)`** (U3-3) — native V3 store purchase (init-payment → billing sheet → complete-payment, all server-verified inside the native SDK). One terminal `VLPlayPurchaseResult` per call, on the Unity main thread: `outcome` (Success / Cancelled / Failed) + `transactionId` + `productId` + `deliveryPending` (BE 1036 — charged and verified, game-server delivery retrying: treat as success) + error code/message. The request object carries `isConsumable` (default true, the SDK's historical behaviour) and `extraData` (forwarded verbatim to the BE, reaches the game server's delivery callback).
  - **iOS reaches Android parity by observing the SDK's payment notification** (`VLPlaySDKPaymentDidFinishNotification`, public in `VLPlayBridge.h`) instead of the method's 2-arg completion — the notification carries transactionId/productId/deliveryPending, the completion none of them. Events are filtered by productId so a cold-launch replay of a *different* queued product can't resolve the wrong request; an unattributed failure (the native loses the productId on some error posts) still resolves rather than hanging the callback. The decision function was **executed** on 8 real notification shapes taken from the native source (foreign-product event ignored, lost-productId failure resolved, fallback messages) — not just compiled.
  - ⚠️ Platform drift, by native design: **on iOS a user-cancel arrives as `Failed`**, not `Cancelled` — the iOS payment notification has no cancel marker (Android's `onCancel` is structured). The bridge already maps a future native `"cancelled"` status.
- **`VLPlayEvents.OnPurchaseCompleted`** — success broadcast for ANY purchase, including webpay checkouts inside the SDK's own UI (FAB shop), which no `Purchase()` callback sees. No payload (neither native provides one). ⚠️ An IAP `Purchase()` raises **both** this and its own callback — the natives fire their global listener alongside the per-call one; don't grant twice.
- **`VLPlaySDK.GetProductCatalog(callback)`** (U3-2) — the game's purchasable packages, as `VLPlayStorePackage[]`. Callback runs on the Unity main thread, fires exactly once, and hands back either packages or a `VLPlayError`. Buy with `storeProductId`, never `packageId`. Note the catalog is the CMS's view, not the store's: a package whose product is missing or unapproved in App Store Connect / Play Console still appears here and only fails at purchase.
  - The platforms do not share a wire here — Android returns parsed `StorePackage` POJOs, iOS returns the BE's **raw dictionaries** — so the iOS bridge **mirrors Android's `StorePackage.from`** rather than serializing the wire as-is. Both parsers were run over the same 18 rows and their output diffed field-by-field; that test caught two real drifts (`org.json`'s `optString(key, default)` returns the default only when the key is **absent** — a key present with `""` reads as `""`, which is not the same as "empty means fall back"). A row with no `storeProductId`/`productId` is dropped on both platforms: it cannot be bought.
  - `description` is `""` and never null on either platform (Java hands over null; iOS has no null to give).
  - This is the wrapper's first request→response API. Replies are correlated by request id (`VLPlayPendingRequests`) because iOS answers through `UnitySendMessage`, a single global channel with nothing tying a reply to its caller; Android needs no id — each request gets its own proxy. U3-3/U3-4 reuse it.
- Re-vendored native binaries to **v1.2.1** (iOS `4cf2705` / Android `82cddd4`), up from v1.1.5. What the wrapper gains: **purchase history works on iOS** — `listSuccessfulTransactionsAtPage:size:completion:` existed in the v1.1.5 binary but was declared only in an internal header, so no consumer could call it (Android has had the equivalent public since day one); that asymmetry is what blocked `GetPurchaseHistory` (U3-4). Also two iOS config-cache fixes that a Unity game sees directly on cold launch — an `identityRequireIdCard=false` game no longer has the ID-card fields forced back on, and a game with a local FAB override no longer loses its ball on relaunch — plus the session-expire fix (a flaky network right after login no longer reports a logout) and the iOS 26 toast fix (on iOS 26 no SDK message was legible). The vendored v1.1.5 binary also self-reported its version as `1.0.0`; from v1.2.1 the version it reports is true.
- **iOS: the `sdk-resources/` side-car is gone.** Since native v1.1.6 the xcframework nests `VLPlaySDKResource.bundle` (nibs, pngs, Roboto, `ErrorCode`/`LocalizableString`, `SVProgressHUD.bundle`) inside `VLPlaySDK.framework`, and `NSBundle+Additions.m` resolves it via `bundleForClass` — which is searched **before** the main bundle, so the side-car copy could never win once it was nested. It was load-bearing only while this package vendored v1.1.5 (pre-nesting). Dropping it removes ~156 duplicate files from every iOS build; the podspec no longer declares `resource_bundles`, matching the `g01-ios-game` consumer pattern it mirrors.
- **Passthrough API: language, orientation, CMS gates** (U2-4). `SetLanguage`/`GetLanguage` (`VLPlayLanguage`), `SetOrientation`/`GetOrientation` (`VLPlayOrientation`), `IsFeatureEnabled` (`VLPlayFeature` constants), `IsConfigReady`, `GetConfigJson`. Two platform drifts are **native, not wrapper** — the API documents them rather than hiding them: (1) Android *persists* a language override, iOS keeps it in memory only ⇒ set it every launch; (2) iOS applies the orientation to effectively every SDK screen (incl. payment), Android only to login + account — its identity / anti-addiction / guest-upgrade screens keep following the device. `IsFeatureEnabled` resolves exactly six flags on both platforms and answers `false` for anything else — including real config keys that aren't routed through it (`identityRequireIdCard`, `min_age`); read those from `GetConfigJson`. `GetConfigJson` is the SDK's re-serialized config (the `/sdk/config` wire shape), so it is *not* byte-identical across platforms — read it defensively, and don't log it in a shipped build (it carries the per-game AppsFlyer dev key + OAuth client IDs).
- **`VLPlaySDK.DebugForceAntiAddiction(kick)` (QA only).** Without it the anti-addiction path is effectively untestable: a real kick needs a minor's account plus a tightened CMS cap. This routes to the native SDK's own debug helpers, so the **real** chain runs (native status → delegate/listener → popup → host callback) rather than faking a C# event. Android requires native **v1.1.5+** and verbose logging on — before v1.1.5 its helpers bailed on `BuildConfig.DEBUG`, which is `false` in every shipped AAR, so they were dead exactly where they were needed. iOS has no such gate. Strip the call before shipping.
- **Anti-addiction warn/kick now reach the game** (U2-3). `VLPlayEvents.OnAntiAddictionWarn` / `OnAntiAddictionKick` finally have a producer: Android registers an `AntiAddictionListener` proxy (`setAntiAddictionListener`), iOS implements the two `@optional` `VLPlaySDKManagerDelegate` methods. The native SDK fires these from its *status* chokepoint rather than its popup code, so the game is told to stop **even when the SDK has no Activity/ViewController to show its own popup**. ⚠️ **Breaking (pre-1.0):** `OnAntiAddictionKick` now carries `VLPlayAntiAddictionKick { reason, cooldownUntil }` instead of a bare reason string — `cooldownUntil` is the server's ISO-8601 instant and must never be derived on the client.
- Re-vendored native binaries to **v1.1.5** (iOS `af4b81f`, unchanged / Android `fe589d3`). v1.1.5 lands the Android **warn-cadence fix** (it was firing the anti-addiction warning on every status poll; now debounced once per session, matching iOS — so `OnAntiAddictionWarn` finally has the same cadence on both platforms) **and** makes the native AA debug hooks usable in a shipped AAR. `sdk-resources/` unchanged; the iOS xcframework rebuilt byte-identical (same core pin), so only the AAR moved.
- **iOS: force dynamic pod linkage** (`VLPlayPostProcessIOS`). EDM4U emits `use_frameworks! :linkage => :static`, but FBSDK 18 and OneSignal 5.x ship as prebuilt *dynamic* xcframeworks that CocoaPods cannot re-link statically: under static linkage it skips the `[CP] Embed Pods Frameworks` phase, so nothing copies them into the `.app`. The build linked cleanly and reported success, then the app died at launch with `dyld: Library not loaded: @rpath/FBSDKCoreKit.framework/FBSDKCoreKit`. The Podfile is now rewritten to plain `use_frameworks!` (order 41, before `pod install`), which restores the embed phase — verified: 17/17 frameworks embedded, 0 missing, 0 duplicate symbols.
- **U1 build plumbing verified end to end on real builds** (2026-07-11). Android: APK builds, `res/raw/sdkconfig.xml` resolves as `R.raw.sdkconfig` via `java.util.Properties`, Firebase string resources emitted from `google-services.json`, all EDM4U deps land in the dex (incl. the jcenter-era `balloon` + `swipelayout`, resolved via the declared jitpack fallback — the `TODO(U1)` on those is now closed). iOS: Xcode export + `pod install` + `xcodebuild -sdk iphoneos` link clean; the local `VLPlaySDK` pod, `VLPlaySDKResource.bundle`/`SVProgressHUD.bundle`, generated `VLPlaySDK-Info.plist` and `GoogleService-Info.plist` all ship in the app.
- **README:** EDM4U must now be installed from **OpenUPM** — Google's Game Package Registry (`unityregistry-pa.googleapis.com`) no longer serves it (404). Added a troubleshooting section (dyld launch crash, iOS deployment target ≥ 15.0, EDM4U-not-found) and the Android **Application Entry Point = Activity** constraint (GameActivity breaks the native SDK's classic-Activity screens).
- Re-vendored native binaries to **v1.1.3** (iOS `41bc3dc` / Android `6082e20`, = develop HEAD). Brings in: the public class rename `SDKManager` → `VLPlaySDKManager`, the IAA `VLPlayAds` ads module + AdMob mediation provider, the anti-addiction `warningMessage`/`cooldownUntil` server contract, and the iOS Firebase GUL AppDelegate-swizzler crash fix. `sdk-resources/` unchanged (0 resource files changed between the v1.1.1 and v1.1.3 pins). Swept `VLPlayUnityBridge.mm` `NSClassFromString` target `SDKManager` → `VLPlaySDKManager`.

## [0.1.0] — 2026-07-02

U0 scaffold.

- UPM package layout (`vn.vlplay.sdk`, Unity 2021.3+), Runtime/Editor asmdefs.
- C# facade: `VLPlaySDK` (Init/SignIn/SignOut/CurrentUser/LogEvent), `VLPlayEvents` (main-thread event surface incl. anti-addiction warn/kick), `VLPlaySettings` ScriptableObject.
- Bridge skeleton: `EditorBridge` mock (Play Mode works without devices), `AndroidBridge` (AndroidJavaClass → `VLPlaySDKManager`), `IosBridge` (P/Invoke → `VLPlayUnityBridge.mm`, NSClassFromString-guarded stub).
- Native binaries vendored at **v1.1.1 core**: `Runtime/Plugins/Android/VLPlaySDK.aar` + `NativeLibs~/iOS/LocalPods/VLPlaySDK/` (VLPlaySDK.xcframework + fresh `sdk-resources` from `vlgame-sdk-ios @ 6660c7f` + binary podspec mirroring the proven g01-ios-game consumer pattern).
- EDM4U manifest `Editor/VLPlaySDKDependencies.xml` (Android transitive deps mirrored from the canonical AAR consumer; iOS FBSDK 18 + OneSignal 5.5.1 only — statically-embedded deps excluded).
- `link.xml` IL2CPP stripping guard.

Pending U1: iOS local-pod injection post-processor, Android creds/`google-services` post-processor, listener proxies, real user/session plumbing.
