# 外部パッケージの管理方針

Github から clone した人がすぐビルドできるよう、依存関係の置き場所と取得方法を統一するための指針です。

---

## 1. 依存関係一覧（external/ 構成）

| ライブラリ | 用途 | 参照先 | 備考 |
|------------|------|--------|------|
| **imgui** | GUI | `external/imgui` | ソースを EngineCore がコンパイル。LICENSE 同梱 |
| **nlohmann/json** | JSON | `external/nlohmann_json` | ヘッダーのみ。`#include <nlohmann/json.hpp>` |
| **DirectXTex** | テクスチャ | `external/directxtex`（lib は `external/directxtex/lib`） | ビルド済み lib は `lib/README.md` の手順で配置 |
| **SoLoud** | 音声 | `external/soloud/include` と `external/soloud/lib` | `soloud_static.lib` は `lib/README.md` の手順で配置 |
| **ReactPhysics3D** | 物理 | `external/reactphysics3d/include` と `external/reactphysics3d/lib` | `scripts/fetch_reactphysics3d.ps1` で include 取得。lib はビルド後に配置 |
| **Assimp** | 3Dモデル | NuGet `packages/AssimpCpp.*`, `packages/assimp_native.*` | **NuGet の復元**が必須。`$(SolutionDir)packages\...` で参照 |

---

## 2. 推奨: 3 つの管理方法

### A. リポジトリ同梱（今のまま＋パス整理）

- **向いているもの**: ヘッダーのみ / ライブラリのソースをプロジェクトに含めてビルドしているもの  
  - imgui, nlohmann/json（json-develop）, DirectXTex, SoLoud
- **やること**:
  - 上記は **リポジトリ内の相対パスのみ** を参照する（`$(SolutionDir)DirectXTex-main/DirectXTex` など）
  - `C:\Program Files (x86)\nlohmann_json` や `C:\Users\...\DirectXTex-main` などの**絶対パスを削除**
  - nlohmann は **`json-develop/json-develop/include` のみ** を使う

### B. NuGet（Assimp）

- **向いているもの**: C++ 用 NuGet パッケージがあるもの（Assimp など）
- **やること**:
  - `packages.config` を維持
  - clone 後に **「ソリューションの NuGet パッケージの復元」** を実行（または `msbuild -t:restore`）
  - vcxproj のインクルード/ライブラリは **`$(SolutionDir)packages\...`** など相対化（絶対パス禁止）

### C. vcpkg（ReactPhysics3D など）

- **向いているもの**: システム-wide にインストールしているもの（ReactPhysics3D）の置き換え
- **やること**:
  - プロジェクトルートに `vcpkg.json` を追加し、`reactphysics3d` などを依存に記載
  - `vcpkg install` で取得。MSBuild 連携は `vcpkg integrate` または `VCPKG_ROOT` で
  - vcxproj の `AdditionalIncludeDirectories` / `AdditionalLibraryDirectories` を vcpkg の `installed/x64-windows/include` 等に合わせる

vcpkg で取得できる代表的な例:

- `reactphysics3d`
- `nlohmann-json`（json-develop をやめて vcpkg に寄せる場合）
- `assimp`（NuGet をやめる場合）
- `directxtex`, `imgui`, `soloud` など（リポジトリ同梱をやめる場合）

---

## 3. すべて内包する場合のフォルダ構成（推奨）

外部ライブラリを **`external/`** に集約し、`$(SolutionDir)external/...` で参照します。

```
HorrorGengine/
├── external/                    # 外部パッケージ一式（リポジトリに同梱）
│   ├── imgui/                   # ヘッダー＋ソース。EngineCore がコンパイルに含める
│   │   ├── LICENSE
│   │   ├── imgui.cpp, imgui.h, imgui_impl_dx12.*, ...
│   ├── nlohmann_json/           # ヘッダーのみ。include ルート = この直下
│   │   ├── LICENSE
│   │   └── nlohmann/
│   │       └── json.hpp
│   ├── directxtex/              # DirectXTex のソース（ヘッダー＋cpp）
│   │   ├── LICENSE
│   │   ├── DirectXTex.h, *.cpp, Shaders/, ...
│   │   └── lib/                 # ビルド済み DirectXTex.lib を配置
│   │       └── x64/{Debug,Release}/
│   │       └── README.md
│   ├── soloud/
│   │   ├── LICENSE
│   │   ├── include/             # soloud.h 等
│   │   └── lib/                 # soloud_static.lib を配置
│   │       └── x64/{Debug,Release}/
│   │       └── README.md
│   ├── reactphysics3d/          # 取得後: include/ と lib/ を配置
│   │   ├── README.md
│   │   ├── include/
│   │   └── lib/
│   └── (Assimp は NuGet のまま。external には置かない)
├── scripts/
│   └── fetch_reactphysics3d.ps1 # ReactPhysics3D の include 取得用
├── packages/                    # NuGet 復元で生成。.gitignore 済み（Assimp）
│   ├── AssimpCpp.5.0.1.6/
│   └── assimp_native.4.0.1/
├── EngineCore/
│   └── src/
└── Game/
```

### 参照パス（vcxproj の例）

| ライブラリ       | Include | Lib |
|------------------|---------|-----|
| imgui            | `$(SolutionDir)external\imgui` | （ソースをコンパイルに含める） |
| nlohmann_json    | `$(SolutionDir)external\nlohmann_json` | — |
| directxtex       | `$(SolutionDir)external\directxtex` | `$(SolutionDir)external\directxtex\lib\$(Platform)\$(Configuration)` |
| soloud           | `$(SolutionDir)external\soloud\include` | `$(SolutionDir)external\soloud\lib\$(Platform)\$(Configuration)` |
| reactphysics3d   | `$(SolutionDir)external\reactphysics3d\include` | `$(SolutionDir)external\reactphysics3d\lib\$(Platform)\$(Configuration)` |
| Assimp           | `$(SolutionDir)packages\AssimpCpp.5.0.1.6\build\native\include` 等 | `$(SolutionDir)packages\assimp_native.4.0.1\build\native\lib\...` |

### 初回セットアップで必要な作業

1. **NuGet の復元** … Assimp 用。`packages/` が生成される。
2. **ReactPhysics3D**  
   - `scripts\fetch_reactphysics3d.ps1` を実行 → `external\reactphysics3d\include` にヘッダーが入る。  
   - その後、[公式](https://github.com/DanielChappuis/reactphysics3d)でビルドし、`reactphysics3d.lib` を `external\reactphysics3d\lib` に配置。
3. **DirectXTex の lib**  
   - `DirectXTex-main\DirectXTex` の vcxproj でビルドし、`DirectXTex.lib` を `external\directxtex\lib\x64\Debug`（および Release）にコピー。  
   - 詳細は `external\directxtex\lib\README.md` を参照。
4. **SoLoud の lib**  
   - `soloud20200207\build\vs2022\SoloudStatic.vcxproj` でビルドし、`soloud_static.lib` を `external\soloud\lib\x64\Debug`（および Release）にコピー。  
   - 詳細は `external\soloud\lib\README.md` を参照。

同梱するものは **`$(SolutionDir)external\...`** の相対パスのみで参照。Assimp は **`$(SolutionDir)packages\`** を前提。

---

## 4. clone した人向け・やること一覧（external/ 構成）

**コマンドラインで完結する具体的な手順は [README のセットアップ](../README.md#セットアップ初回ビルド) を参照。**

1. **clone** → `git clone` で取得
2. **NuGet の復元** → `msbuild -t:restore` または `nuget restore`
3. **ReactPhysics3D** → `.\scripts\fetch_reactphysics3d.ps1` のあと `.\scripts\build_reactphysics3d.ps1`
4. **DirectXTex の lib** → `msbuild DirectXTex-main\DirectXTex\DirectXTex_Desktop_2022_Win10.vcxproj` でビルドし、`Bin\Desktop_2022_Win10\x64\{Debug,Release}\DirectXTex.lib` を `external\directxtex\lib\x64\{Debug,Release}\` にコピー
5. **SoLoud の lib** → `msbuild soloud20200207\build\vs2022\SoloudStatic.vcxproj` でビルドし、`soloud20200207\lib\soloud_static.lib` を `external\soloud\lib\x64\{Debug,Release}\` にコピー（Debug/Release で 2 回ビルド）
6. **ビルド** → `msbuild HorrorEngine.sln -p:Configuration=Debug -p:Platform=x64`

一括実行: `.\scripts\setup.ps1`（[README](../README.md) の「一括実行」参照）。

---

## 5. 設定ファイルで揃えること（external/ 構成で適用済み）

- **vcxproj**  
  - `AdditionalIncludeDirectories` / `AdditionalLibraryDirectories` は  
    `$(SolutionDir)external\...` および `$(SolutionDir)packages\...`（Assimp）のみ。絶対パスは使用していない。
- **.clangd**  
  - `-I` はプロジェクトルートからの相対パス（`external/...`, `EngineCore/src`, `packages/...`）。
- **.vscode/c_cpp_properties.json**  
  - `"${workspaceFolder}/external/..."` および `"${workspaceFolder}/packages/..."` のみ。

---

## 6. 今後の方針案

- **短期（clone しやすくするだけ）**  
  - 上記の「絶対パス削除」と「nlohmann / DirectXTex の参照先統一」  
  - NuGet 復元と ReactPhysics3D の導入手順を README に明記。

- **中期（vcpkg に寄せる）**  
  - ReactPhysics3D を vcpkg で導入。  
  - 必要に応じて Assimp / nlohmann / DirectXTex なども vcpkg に移行し、`vcpkg install` 一発で揃うようにする。

- **Git Submodule を使う場合**  
  - `DirectXTex-main`, `json-develop`, `soloud20200207`, `imgui` などを submodule にすると、バージョン固定しやすくなる。  
  - その場合は README に  
    `git clone --recursive` または `git submodule update --init --recursive` を記載。

---

## 7. まとめ

| 方法 | 対象例 | clone 後の追加作業 |
|------|--------|--------------------|
| **リポジトリ同梱** | imgui, json-develop, DirectXTex, SoLoud | なし（パスを相対化するだけ） |
| **NuGet** | Assimp |  NuGet の復元 1 回 |
| **vcpkg** | ReactPhysics3D（＋任意で Assimp 等） | `vcpkg install` と `integrate` |
| **Submodule** | 上記を submodule 化する場合 | `git submodule update --init --recursive` |

「Github から clone した人が開きやすく」するには、

1. **絶対パス・マシン固有パスをやめ、`$(SolutionDir)` / `${workspaceFolder}` / 相対パスに統一する**  
2. **NuGet 復元と ReactPhysics3D の導入手順を README に書く**  
3. **ReactPhysics3D を vcpkg または `external/` に置き、参照を相対化する**

の 3 つが重要です。
