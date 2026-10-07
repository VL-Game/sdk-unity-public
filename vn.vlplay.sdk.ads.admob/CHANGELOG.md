# Changelog — vn.vlplay.sdk.ads.admob

## 0.1.0 — 2026-10-07 (with core 1.6.0)

- First AdMob provider module: the official Google Mobile Ads Unity Plugin **11.3.0**
  (`play-services-ads` 25.4.0, `Google-Mobile-Ads-SDK ~> 13.6`) as the mediation engine behind
  `VLPlayAds` for rewarded and interstitial ads.
- Pinned to 11.3.0 because it is the last plugin that declares Unity 2019.4; 11.4.0 and later
  declare 2022.3, above the core's 2021.3.
- Each show is bound to its own ad object, so a reward, revenue or dismiss can never reach another
  show; a late callback after the close still reaches its own. No settle window.
- The reward ticket goes on the ad as SSV custom data with the player as SSV user id, only for
  `ssv:true` placements.
- Google's platform-specific error codes map to the same public codes as the native AdMob
  providers (Android `AdRequest.ERROR_CODE_*`, iOS `GADErrorCode`).
- Test devices from the CMS go on the global request configuration (keeping whatever else the
  game set there) before Google starts, and again at every init.
- `[assembly: AlwaysLinkAssembly]` from the start: nothing references the module's assembly, and
  UnityLinker would drop it from every IL2CPP build (found on the MAX module, 2026-09-28).
- Google's callbacks run on Google's thread (`RaiseAdEventsOnUnityMainThread` is obsolete in 11.3.0
  and costs impression-level revenue); the module locks its state and hands the revenue to the
  backend at once.
- A closed ad is kept until the next show on its unit: on Android the reward can arrive after the
  close, and it still reaches its own show, once.
- Refuses Google's experimental Next-Gen Android SDK (verified only on the standard SDK).
- Google's UMP consent flow runs before Google Mobile Ads only when the build turns on
  `VLPlaySettings.admobConsentEnabled` (off by default: the games ship in Vietnam). With it on, no ad
  is requested until UMP allows it, and `VLPlayAds.ShowPrivacyOptions()` opens Google's privacy form.
- Google Mobile Ads starts once per process (`AdmobInitLatch`): every caller hears success or failure
  exactly once, even when Google finishes on its own thread while a caller joins.
- Needs VLPlay core **1.6.0** or later, which also brings the AdMob App ID checks and the iOS
  network name `GADMAdapterGoogleAdMobAds` → `admob`.
