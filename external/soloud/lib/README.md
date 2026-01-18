# SoLoud ライブラリ

`soloud_static.lib` をここに配置してください。

1. `soloud20200207/build/vs2022/SoloudStatic.vcxproj` をビルド
2. 出力の `soloud_static.lib` を以下にコピー:
   - `lib/x64/Debug/`
   - `lib/x64/Release/`

既に `soloud20200207/build/vs2022/x64/Debug` 等でビルド済みの場合は、そのパスを vcxproj の `AdditionalLibraryDirectories` に残す運用でも構いません。
