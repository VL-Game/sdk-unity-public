# VLPlay Unity SDK Binary Distribution

Core **1.6.3** (2026-10-31), native **1.9.8**, AdMob provider **0.1.1**. Core **1.6.2** (native 1.9.6)
stays available at tag `vn.vlplay.sdk/1.6.2`. C# implementation is shipped as
platform-selected precompiled DLLs. No SDK C# source, assembly definitions or debug symbols.
Native iOS bridge glue is compiled by Unity/Xcode. DLLs can be decompiled; never embed credentials.

Install core in Unity Package Manager through this public Git URL:

```text
https://github.com/VL-Game/sdk-unity-public.git?path=/vn.vlplay.sdk#vn.vlplay.sdk/1.6.3
```

For AdMob, import Google's official Unity plugin 11.3.0 after core, then add:

```text
https://github.com/VL-Game/sdk-unity-public.git?path=/vn.vlplay.sdk.ads.admob#vn.vlplay.sdk.ads.admob/0.1.1
```

Git is required on the development machine. Firebase/game credentials are supplied separately.
Keep one EDM4U installation. See [installation](https://sdk-docs.vlplay.vn/docs/integration/unity/installation)
and [upgrade](https://sdk-docs.vlplay.vn/docs/integration/unity/upgrade-1.6.0).

Source packages (including 1.5.1 and the initial 1.6.0 source packaging) were withdrawn on
2026-10-08. Remove/re-add previously installed VLPlay packages to refresh their lock hashes;
preserve your settings asset and its .meta. The binary Editor migrates its script reference;
the fallback menu is **VLPlay > Migrate Settings to Binary SDK**. Remove source-asmdef references
from your game's assembly definitions; managed plug-ins are referenced automatically by default.
This is a clean binary-only history; implementation history is private. Previously downloaded
copies cannot be recalled. Use is governed by each package's proprietary LICENSE.md.
