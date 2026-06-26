Locale [Re]mulator Launcher
========================

Launcher by: https://github.com/DTM9025/LELauncher

Emulator by: https://github.com/xupefei/Locale-Emulator

[Re]mulator by: https://github.com/InWILL/Locale_Remulator

TL;DR - same Launcher, using [Re]mulator instead of Emulator, with userspace hooks instead of NT calls implemented w/ Detours.

Now you can run x64 apps *as well* as x86 apps with it :D

## Configuration

This launcher requires all DLLs (`LRHookx64.dll`, `LRHookx32.dll`, `LRInjector64.dll`, `LRInjector32.dll`) to be located in the same directory. 
In addition, it requires `le.config` to be properly configured to run the desired executable with the appropriate locale 
settings. You can edit `le.config` with any text editor. The fields that you can edit are in the `le.config` Profile 
and are as follows:

* `Parameter`: The relative path from the directory of `LELauncher.exe` to the target executable you want to run.
* `Location`: The locale you want to simulate. These are the same as the ones used in Locale Emulator. Common values include `ja-JP` and `zh-CN`. Available location codes can be found [here](https://learn.microsoft.com/en-us/openspecs/windows_protocols/ms-lcid/a9eac961-e77d-41a6-90a5-ce1a8b0cdb9c).
* `Timezone`: The timezone you want to simulate. These are the same as the ones in Locale Emulator. Examples are `Tokyo Standard Time` and `China Standard Time`.
* `RunAsAdmin`: Whether to run the target executable as Admin or not. Can be set `true` or `false`.
* `RedirectRegistry`: Whether to fake language-related keys in Registry. Can be set `true` or `false`. **Recommended to be true**.
* `IsAdvancedRedirection`: Whether to fake system UI language. Can be set `true` or `false`.
* `RunWithSuspend`: Whether to run the target executable with a suspended process. Can be set `true` or `false`. **Recommended to be false**.
