# ReactPhysics3D

このフォルダに ReactPhysics3D の `include/` と `lib/` を配置してください。

## 手順

1. [Releases](https://github.com/DanielChappuis/reactphysics3d/releases) からソースを取得
2. CMake でビルドするか、公式のプリビルドがあれば利用
3. 以下にコピー:
   - `include/` … ヘッダー一式（`fetch_reactphysics3d.ps1` で取得済みなら不要）
   - `lib/x64/Debug/` および `lib/x64/Release/` … `reactphysics3d.lib`

## 自動取得（PowerShell）

```powershell
.\scripts\fetch_reactphysics3d.ps1
```

スクリプトはソースを取得し、`include` を展開します。`lib` はビルド後に手動で配置してください。
