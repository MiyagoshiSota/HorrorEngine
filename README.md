## チラズアート系ホラー体験をデータ駆動で構築するための3Dゲームエンジン

HorrorEngine は、 チラズアート作品に代表される 「日常作業 × 怪異の崩壊」 を効率的に構築するための Task / WorkFlow / Work / Day 構造を備えたゲームエンジンです。

日常動作（作業）を Task として定義し、 それらを WorkFlow（作業手順）、Work（業務）、 そして Day（1日のストーリー単位） として整理するだけで、

異変のない日常から、怪異が侵入するホラー体験へと崩壊していく進行を容易に構築できます。

GUIにより、開発者はシーン上にオブジェクトを置き、 エディタから Task → WorkFlow → Work を組み合わせるだけでゲームを作れます。

詳しくは[Wiki](https://github.com/MiyagoshiSota/HorrorEngine/wiki)を見てね。
技術メモとかも書いてます。

---

## セットアップ（初回ビルド）

外部ライブラリは **`external/`** に集約されています。詳細は [docs/DEPENDENCIES.md](docs/DEPENDENCIES.md) を参照してください。

1. **clone**
   ```bash
   git clone https://github.com/MiyagoshiSota/HorrorEngine.git
   cd HorrorEngine
   ```

2. **NuGet パッケージの復元**  
   - ソリューション右クリック → **「NuGet パッケージの復元」**  
   - `packages/` に Assimp が入ります。

3. **ReactPhysics3D**  
   - `.\scripts\fetch_reactphysics3d.ps1` を実行 → `external\reactphysics3d\include` にヘッダーが入ります。  
   - [ReactPhysics3D](https://github.com/DanielChappuis/reactphysics3d) をビルドし、`reactphysics3d.lib` を `external\reactphysics3d\lib` に配置してください。

4. **DirectXTex の lib**  
   - `DirectXTex-main\DirectXTex` の vcxproj でビルドし、`DirectXTex.lib` を `external\directxtex\lib\x64\Debug` および `x64\Release` にコピー。  
   - 手順: `external\directxtex\lib\README.md` を参照。

5. **SoLoud の lib**  
   - `soloud20200207\build\vs2022\SoloudStatic.vcxproj` でビルドし、`soloud_static.lib` を `external\soloud\lib\x64\Debug` および `x64\Release` にコピー。  
   - 手順: `external\soloud\lib\README.md` を参照。

6. **ビルド**  
   - `HorrorEngine.sln` を開き、Game をスタートアップにしてビルド・実行。
