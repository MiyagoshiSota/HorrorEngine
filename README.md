## チラズアート系ホラー体験をデータ駆動で構築するための3Dゲームエンジン

HorrorEngine は、 チラズアート作品に代表される 「日常作業 × 怪異の崩壊」 を効率的に構築するための Task / WorkFlow / Work / Day 構造を備えたゲームエンジンです。

日常動作（作業）を Task として定義し、 それらを WorkFlow（作業手順）、Work（業務）、 そして Day（1日のストーリー単位） として整理するだけで、

異変のない日常から、怪異が侵入するホラー体験へと崩壊していく進行を容易に構築できます。

GUIにより、開発者はシーン上にオブジェクトを置き、 エディタから Task → WorkFlow → Work を組み合わせるだけでゲームを作れます。

詳しくは[Wiki](https://github.com/MiyagoshiSota/HorrorEngine/wiki)を見てね。
技術メモとかも書いてます。

---

## セットアップ（初回ビルド）

**前提**: Windows、Visual Studio 2022（C++ ワークロード）、CMake（ReactPhysics3D 用）。  
推奨: **Developer PowerShell for VS 2022** で実行（`msbuild` が PATH に入る）。

### 一括実行（推奨）

```powershell
git clone https://github.com/MiyagoshiSota/HorrorEngine.git
cd HorrorEngine
.\scripts\setup.ps1
```

`setup.ps1` が NuGet 復元 → ReactPhysics3D 取得・ビルド → DirectXTex ビルド → SoLoud ビルド → 本プロジェクトのビルド を順に実行します。  
（`setup.ps1` は vswhere で MSBuild を探すため、通常の PowerShell からでも実行できます。手動の手順では **Developer PowerShell for VS 2022** 推奨。）

---

### 手順ごとに実行する場合

いずれも **リポジトリルート** をカレントにし、**Developer PowerShell for VS 2022** で実行。

```powershell
# 0. clone と移動
git clone https://github.com/MiyagoshiSota/HorrorEngine.git
cd HorrorEngine
```

```powershell
# 1. NuGet パッケージの復元（Assimp）
msbuild HorrorEngine.sln -t:restore -p:RestorePackagesConfig=true -v:m
# 上で packages が作られない場合:
#   nuget restore HorrorEngine.sln
# （nuget.exe は https://www.nuget.org/downloads から取得）
```

```powershell
# 2. ReactPhysics3D（include 取得 + ビルドして lib 配置）
.\scripts\fetch_reactphysics3d.ps1
.\scripts\build_reactphysics3d.ps1
```

```powershell
# 3. DirectXTex（lib を external にコピー）
msbuild external\DirectXTex-main\DirectXTex\DirectXTex_Desktop_2022_Win10.vcxproj -p:Configuration=Debug -p:Platform=x64 -v:m
msbuild external\DirectXTex-main\DirectXTex\DirectXTex_Desktop_2022_Win10.vcxproj -p:Configuration=Release -p:Platform=x64 -v:m
New-Item -ItemType Directory -Force -Path external\directxtex\lib\x64\Debug, external\directxtex\lib\x64\Release | Out-Null
Copy-Item external\DirectXTex-main\DirectXTex\Bin\Desktop_2022_Win10\x64\Debug\DirectXTex.lib external\directxtex\lib\x64\Debug\
Copy-Item external\DirectXTex-main\DirectXTex\Bin\Desktop_2022_Win10\x64\Release\DirectXTex.lib external\directxtex\lib\x64\Release\
```

```powershell
# 4. SoLoud（lib を external にコピー）
New-Item -ItemType Directory -Force -Path external\soloud\lib\x64\Debug, external\soloud\lib\x64\Release | Out-Null
msbuild external\soloud20200207\build\vs2022\SoloudStatic.vcxproj -p:Configuration=Debug -p:Platform=x64 -v:m
Copy-Item external\soloud20200207\build\vs2022\x64\Debug\soloud_static.lib external\soloud\lib\x64\Debug\
msbuild external\soloud20200207\build\vs2022\SoloudStatic.vcxproj -p:Configuration=Release -p:Platform=x64 -v:m
Copy-Item external\soloud20200207\build\vs2022\x64\Release\soloud_static.lib external\soloud\lib\x64\Release\
```

```powershell
# 5. ビルド
msbuild HorrorEngine.sln -p:Configuration=Debug -p:Platform=x64 -v:m
```

実行ファイルは `Game\x64\Debug\Game.exe` などに出力されます。詳細は [docs/DEPENDENCIES.md](docs/DEPENDENCIES.md) を参照。
