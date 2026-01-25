## 初めに

### 結果

2048x2048 PNGテクスチャ（107枚）のロード処理において、`WriteToSubresource`による同期的なデータ転送がボトルネック（全体の95-97%、平均950ms）であることを特定した。

**Phase 1（アップロードヒープ方式への移行）**を実施した結果：

| 指標 | 改善前 | 改善後 | 改善効果 |
|-----|--------|--------|---------|
| Data Transfer | 950 ms | 3.07 ms | **309倍高速化** |
| Total | 980 ms | 35.0 ms | **28倍高速化** |

目標値（Data Transfer平均 4.5ms以下）を達成したため、Phase 1の実装をもって本課題は**解決**とする。

[比較動画](https://drive.google.com/drive/u/0/folders/1L2QQS9V5o03Q3e2XrsJ02g1W4wUArgKH)

### テスト環境

**GPU情報**:
- GPU名: AMD Radeon RX 6700 XT
- ドライバーのバージョン: 32.0.21030.2001
- 専用 GPU メモリ: 12.0 GB
- 共有 GPU メモリ: 7.9 GB
- GPU メモリ: 19.9 GB


**PCIe情報**:
- PCIeバージョン: PCIe 4.0
- PCIeレーン数: x16
- PCIe接続状態: PCIe x16 4.0 @ x16 4.0（フル帯域幅で動作中）
- Resizable BAR: Enabled


**GPUメモリ情報**:
- メモリタイプ: GDDR6 (Samsung)
- 帯域幅: 384.0 GB/s
- バス幅: 192 bit


**CPU/System Memory情報**:
- メモリタイプ: DDR4-3200
- 構成: デュアルチャネル（InterleaveDataDepth: 2）
- 総容量: 16 GB（8 GB × 2 DIMM）
- DIMM情報: Crucial Technology CT8G4DFRA32A.C8FR
- 理論帯域幅: 約51.2 GB/s（デュアルチャネル、25.6 GB/s × 2）


## 計測

### 目的
テクスチャ読み込み処理の各ステップで時間がかかっている箇所を特定し、ボトルネックを明確にする。

### 計測項目

1. **ファイルI/O + デコード時間**
   - `LoadFromWICFile` / `LoadFromTGAFile` の実行時間
   - ファイル読み込みと画像デコード（PNG/JPEG等の展開）を含む
   - DirectXTexライブラリが内部で行う処理

2. **GPUリソース作成時間**
   - `CreateCommittedResource` の実行時間
   - GPUメモリの確保とリソースの初期化

3. **データ転送時間**
   - `WriteToSubresource` の実行時間
   - CPUメモリからGPUメモリへのデータ転送
   - `D3D12_CPU_PAGE_PROPERTY_WRITE_BACK` を使用している場合、この処理が同期的に実行される

4. **その他処理時間**
   - メタデータ処理、SRV設定など、上記以外の処理時間

### 計測方法

各処理の前後で `std::chrono::high_resolution_clock` を使用して時間を計測し、詳細なログを出力する。

#### ログ出力フォーマット
```
[Texture Load] {ファイルパス} - {幅}x{高さ}
  File I/O + Decode: {時間} ms
  GPU Resource Creation: {時間} ms
  Data Transfer: {時間} ms
  Other: {時間} ms
  Total: {合計時間} ms
```

### 実装

`Texture2D::InternalLoad`(EngineCore\src\Renderer\Texture\Texture2D.cpp) 内で各ステップの時間を計測し、詳細なログを出力する。
入力に使用するTextureは、japanese_street_at_night(Game\Assets\japanese_street_at_night/textures)であり、107テクスチャ、全て2048x2048である。

## ボトルネック特定

### 分析結果

計測ログ（107テクスチャ、全て2048x2048）を分析した結果、以下の統計が得られた。

#### 各ステップの時間分布

| ステップ | 最小値 | 最大値 | 平均値 | 全体に占める割合 |
|---------|--------|--------|--------|-----------------|
| File I/O + Decode | 6.349 ms | 47.372 ms | 約23 ms | **約2-3%** |
| GPU Resource Creation | 2.642 ms | 8.262 ms | 約3.5 ms | **約0.3-0.5%** |
| Data Transfer | 717.279 ms | 1104.1 ms | 約950 ms | **約95-97%** |
| Other | 0 ms | 0 ms | 0 ms | 0% |
| **Total** | **741.743 ms** | **1113.5 ms** | **約980 ms** | **100%** |

以下に、そのままドキュメントやレポートとして使用できるMarkdown形式で出力します。

---

### ボトルネックの特定

**現状の課題:**
- **主要ボトルネック**: Data Transfer（WriteToSubresource）
- **処理時間**: 700 - 1100 ms（全体の95-97%）
- **データ量**: 16 MB (2048x2048 RGBA8)

データ転送にかかる時間は、論理的に以下の2要素の合計で構成される。

**Data Transfer Total Time = CPU Copy + GPU Copy** 

現状（WriteToSubresource）では各工程がブラックボックス化されているため、ハードウェア仕様に基づく理論値を算出し、現状との乖離を検証する。

#### 1. CPU Copy の理論値

**前提環境**: DDR4-3200 Dual Channel

* **メモリ帯域幅（理論最大値）**:
    - 3200 MT/s×8 Byte×2 ch=51.2 GB/s

* **16MBコピーの所要時間**:
    - 物理最小値: 16÷51,200≈0.31 ms
    - 実効予測値: 0.5 ～ 1.5 ms
    - ※シングルスレッドによる memcpy の実効効率やキャッシュミスを考慮した現実的な範囲

#### 2. GPU Copy の理論値

**前提環境**: AMD Radeon RX 6700 XT / PCIe 4.0 x16 / ReBAR ON

* **PCIe帯域幅（実効値）**:
    - 転送レート: 16 GT/s×16 lanes=256 Gbps
    - 実効スループット: ≈31.5 GB/s (128b/130b Encoding)

* **16MB転送の所要時間**:
    - 物理最小値: 16÷31,500≈0.51 ms
    - 実効予測値: 0.6 ～ 0.7 ms
    - ※プロトコルオーバーヘッド約20%を考慮。

#### 結論：理論と現実の比較

理論上の合計時間​ = 0.5∼1.5 (CPU)　+　0.6∼0.7 (GPU)　≈　1.1∼2.2 ms​

* **理論値 (目安)**: **約 1.1 - 2.2 ms**
* **実測値 (現状)**: **700 - 1100 ms**
* **乖離**: **約 500〜900倍 の遅延**

本来数ミリ秒で完了すべき処理に1秒近く要していることから、データ転送がボトルネックであると断定できる。

## 仮説立案

この章で立てた仮説をもとに実装のためのロードマップを数フェーズに分け作成し、1フェーズ作成ごとに検証を行い今回の目的値との比較を行う。 
目的値まで到達した際に解決とし、以降のフェーズの実施は行わないものとする。

### 根本原因の仮説

**仮説1: `D3D12_CPU_PAGE_PROPERTY_WRITE_BACK` の非効率性**

現在の実装では、`D3D12_CPU_PAGE_PROPERTY_WRITE_BACK` と `D3D12_MEMORY_POOL_L0` を使用してリソースを作成している。このメモリタイプは以下の問題を抱えている：

- **CPUから直接書き込み可能だが、GPUアクセスが遅い**: `WRITE_BACK` はCPUからアクセス可能なメモリだが、GPUからアクセスする際にキャッシュコヒーレンシの問題やメモリマッピングのオーバーヘッドが発生する可能性がある。
- **`WriteToSubresource` の内部処理**: このAPIは内部で以下の処理を同期的に実行している可能性がある：
  - CPUキャッシュのフラッシュ
  - メモリマッピングの変更
  - GPUコマンドキューの待機（暗黙的な同期）
  - ページフォルト処理（仮想メモリの物理メモリへのマッピング）

現状の問題コード

**ファイル**: `EngineCore/src/Renderer/Texture/Texture2D.cpp`

```cpp
// === 2. GPUリソース作成時間の計測 ===
const auto resourceCreateStartTime = std::chrono::high_resolution_clock::now();

// リソースを確保 (フォーマットは画像に合わせる)
auto resDesc = CD3DX12_RESOURCE_DESC::Tex2D(
    meta.format,
    meta.width,
    meta.height,
    static_cast<UINT16>(meta.arraySize),
    static_cast<UINT16>(meta.mipLevels)
);

// 問題箇所1: WRITE_BACKメモリを使用
auto texHeapProp = CD3DX12_HEAP_PROPERTIES(D3D12_CPU_PAGE_PROPERTY_WRITE_BACK, D3D12_MEMORY_POOL_L0);

try
{
    ThrowIfFailed(g_Engine->Device()->CreateCommittedResource(
        &texHeapProp,
        D3D12_HEAP_FLAG_NONE,
        &resDesc,
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, // 最初からシェーダーリソースとして使用
        nullptr,
        IID_PPV_ARGS(&m_resource)
    ));
}
catch (const std::exception&)
{
    return false;
}

// === 3. データ転送時間の計測 ===
const auto transferStartTime = std::chrono::high_resolution_clock::now();

// 問題箇所2: WriteToSubresourceによる同期的なデータ転送（700-1100msかかる）
try
{
    ThrowIfFailed(m_resource->WriteToSubresource(
        0,
        nullptr, // 全領域
        img->pixels,
        static_cast<UINT>(img->rowPitch),
        static_cast<UINT>(img->slicePitch)
    ));
}
catch (const std::exception&)
{
    return false;
}
```

**問題点**:
- 150行目: `D3D12_CPU_PAGE_PROPERTY_WRITE_BACK` を使用したヒーププロパティの作成
- 178-184行目: `WriteToSubresource` による同期的なデータ転送（ボトルネックの主要因）

**仮説2: 非同期転送の未使用**

DirectX 12のベストプラクティスでは、テクスチャデータの転送は以下の手順で行うべき：
1. `D3D12_HEAP_TYPE_UPLOAD` ヒープにステージングバッファを作成
2. CPUからステージングバッファにデータをコピー（`Map`/`Unmap`）
3. コマンドリストで `CopyTextureRegion` を使用してステージングバッファからデフォルトヒープのテクスチャにコピー
4. 非同期で実行されるため、CPUはブロックされない

現在の実装は同期的な `WriteToSubresource` を使用しており、GPUの処理完了を待機している可能性がある。

#### 現状の問題コード

**ファイル**: `EngineCore/src/Renderer/Texture/Texture2D.cpp`

```cpp
// === 3. データ転送時間の計測 ===
const auto transferStartTime = std::chrono::high_resolution_clock::now();

// 問題箇所: WriteToSubresourceによる同期的なデータ転送
// コマンドリストを使用せず、直接GPUメモリに書き込むため、CPUがブロックされる
try
{
    ThrowIfFailed(m_resource->WriteToSubresource(
        0,
        nullptr, // 全領域
        img->pixels,
        static_cast<UINT>(img->rowPitch),
        static_cast<UINT>(img->slicePitch)
    ));
    // この時点で、GPUへの転送が完了するまでCPUが待機している
}
catch (const std::exception&)
{
    return false;
}

const auto transferEndTime = std::chrono::high_resolution_clock::now();
```

**問題点**:
- 178-184行目: `WriteToSubresource` は同期的に実行され、GPUの処理完了を待機する
- コマンドリストを使用した非同期転送（`CopyTextureRegion`）を使用していない
- 複数のテクスチャをロードする際、順次実行されるため、並列処理の恩恵を受けられない

**仮説3: リソース状態の不適切な管理**

現在、リソースは `D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE` 状態で作成されているが、`WriteToSubresource` を使用する場合、適切なリソース状態への遷移が必要な可能性がある。状態遷移が暗黙的に発生し、その待機時間が含まれている可能性がある。

#### 現状の問題コード

**ファイル**: `EngineCore/src/Renderer/Texture/Texture2D.cpp`

```cpp
// === 2. GPUリソース作成時間の計測 ===
const auto resourceCreateStartTime = std::chrono::high_resolution_clock::now();

// リソースを確保 (フォーマットは画像に合わせる)
auto resDesc = CD3DX12_RESOURCE_DESC::Tex2D(
    meta.format,
    meta.width,
    meta.height,
    static_cast<UINT16>(meta.arraySize),
    static_cast<UINT16>(meta.mipLevels)
);

auto texHeapProp = CD3DX12_HEAP_PROPERTIES(D3D12_CPU_PAGE_PROPERTY_WRITE_BACK, D3D12_MEMORY_POOL_L0);

try
{
    // 問題箇所: リソースをPIXEL_SHADER_RESOURCE状態で作成
    // WriteToSubresourceを使用する場合、COPY_DEST状態であるべき
    // 暗黙的な状態遷移が発生し、その待機時間が含まれている可能性がある
    ThrowIfFailed(g_Engine->Device()->CreateCommittedResource(
        &texHeapProp,
        D3D12_HEAP_FLAG_NONE,
        &resDesc,
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, // 問題: 書き込み用の状態ではない
        nullptr,
        IID_PPV_ARGS(&m_resource)
    ));
}
catch (const std::exception&)
{
    return false;
}

// その後、WriteToSubresourceを呼び出すが、リソース状態の遷移が暗黙的に発生
// この状態遷移の待機時間が、転送時間に含まれている可能性がある
```

**問題点**:
- 158行目: リソースを `D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE` 状態で作成している
- `WriteToSubresource` を使用する場合、`D3D12_RESOURCE_STATE_COPY_DEST` 状態であるべき
- 暗黙的な状態遷移が発生し、その待機時間が転送時間に含まれている可能性がある
- リソース状態遷移をコマンドリストで明示的に管理していない

**仮説4: メモリレイアウトの非効率性**

`WriteToSubresource` は、CPUメモリのレイアウト（行ピッチ）とGPUメモリのレイアウトが異なる場合、内部で再配置処理が発生する。2048x2048のテクスチャでは、この処理が重い可能性がある。

#### 現状の問題コード

**ファイル**: `EngineCore/src/Renderer/Texture/Texture2D.cpp`

```cpp
// === 3. データ転送時間の計測 ===
const auto transferStartTime = std::chrono::high_resolution_clock::now();

// 問題箇所: WriteToSubresourceにrowPitchとslicePitchを直接渡している
// CPUメモリのレイアウト（DirectXTexのScratchImage）とGPUメモリのレイアウトが異なる場合、
// 内部で再配置処理（メモリコピーとレイアウト変換）が発生する
try
{
    ThrowIfFailed(m_resource->WriteToSubresource(
        0,
        nullptr, // 全領域
        img->pixels,                    // ソースデータ（CPUメモリ）
        static_cast<UINT>(img->rowPitch),   // 行ピッチ（CPUメモリのレイアウト）
        static_cast<UINT>(img->slicePitch)  // スライスピッチ（CPUメモリのレイアウト）
    ));
    // WriteToSubresource内部で以下の処理が発生している可能性がある:
    // 1. CPUメモリのレイアウトをGPUメモリのレイアウトに変換
    // 2. 行単位でのメモリコピーと再配置
    // 3. 2048x2048のテクスチャでは、この処理が非常に重い
}
catch (const std::exception&)
{
    return false;
}
```

**問題点**:
- 182-183行目: `img->rowPitch` と `img->slicePitch` を直接使用している
- DirectXTexの `ScratchImage` のメモリレイアウトとGPUメモリのレイアウトが異なる場合、内部で再配置処理が発生
- 2048x2048のテクスチャ（約16MB）では、この再配置処理が非常に重い（数百ms〜1秒以上）
- メモリレイアウトを事前に最適化していない

### 検証方法

1. **仮説1の検証**: `D3D12_HEAP_TYPE_DEFAULT` + アップロードヒープ方式に変更し、パフォーマンスを計測
2. **仮説2の検証**: コマンドリストを使用した非同期転送方式に変更し、パフォーマンスを計測
3. **仮説3の検証**: リソース状態を明示的に管理し、状態遷移の時間を計測
4. **仮説4の検証**: メモリレイアウトを最適化し、再配置処理を回避

## 改善実装

### 改善ロードマップ

#### Phase 1: アップロードヒープ方式への移行

**目標**: `WriteToSubresource` を廃止し、DirectX 12の標準的なアップロード方式に変更

**実装内容**:
1. リソース作成: テクスチャリソースを D3D12_HEAP_TYPE_DEFAULT（GPU専用）かつ D3D12_RESOURCE_STATE_COPY_DEST 状態で作成。
1. レイアウト計算: device->GetCopyableFootprints() を使用し、転送に必要なサイズとアライメント（256バイト境界など）を計算。
1. ステージング確保: 計算したサイズ分の D3D12_HEAP_TYPE_UPLOAD バッファを確保。
1. CPU書き込み: アライメントを考慮しながら、画像データをステージングバッファに Map / memcpy / Unmap。
1. GPUコピー: コマンドリストで CopyTextureRegion を発行。
1. バリア: COPY_DEST → PIXEL_SHADER_RESOURCE へのリソースバリアを発行。

**期待される効果**: CPUはメモリコピー（memcpy）の時間しか拘束されず、PCIe転送待ちが発生しないため、転送時間が 1ms以下 に短縮される。

**実装ファイル**:
- `EngineCore/src/Renderer/Texture/Texture2D.cpp`
- `EngineCore/src/Renderer/Texture/Texture2D.h`（必要に応じて）

---

#### Phase 2: 非同期転送の実装

**目標**: 複数のテクスチャを並列でロードし、CPUのブロック時間を削減

**実装内容**:
1. テクスチャローダーに非同期転送キューを実装
2. 複数のテクスチャの転送コマンドをバッチで実行
3. 転送完了を非同期で待機（ゲームループをブロックしない）

**期待される効果**: 107テクスチャのロード時間を大幅に短縮（シーケンシャル実行の約1/10-1/20）

**実装ファイル**:
- `EngineCore/src/Renderer/Texture/Texture2D.cpp`
- `EngineCore/src/Renderer/Texture/TextureResourceManager.cpp`（必要に応じて）
- `EngineCore/src/Renderer/Engine.cpp`（コマンドリスト管理）

---

#### Phase 3: リソース状態の最適化

**目標**: リソース状態遷移を明示的に管理し、不要な待機を削減

**実装内容**:
1. テクスチャ作成時は `D3D12_RESOURCE_STATE_COPY_DEST` で作成
2. 転送完了後、`D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE` に遷移
3. 状態遷移をコマンドリストで明示的に管理

**期待される効果**: 状態遷移のオーバーヘッドを削減（数ms程度の改善）

**実装ファイル**:
- `EngineCore/src/Renderer/Texture/Texture2D.cpp`

---

#### Phase 4: メモリレイアウトの最適化

**目標**: メモリコピーの効率を最大化

**実装内容**:
1. テクスチャデータの行ピッチを最適化
2. 必要に応じて、データを事前に再配置してから転送
3. `CopyTextureRegion` のパラメータを最適化

**期待される効果**: メモリコピーのオーバーヘッドを削減（数ms程度の改善）

**実装ファイル**:
- `EngineCore/src/Renderer/Texture/Texture2D.cpp`

---

### 実装の優先順位

1. **Phase 1（高）**: 最も大きな効果が期待される。他の最適化の基盤となる。
2. **Phase 2（中）**: 大量のテクスチャをロードする際のユーザー体験を大幅に改善。
3. **Phase 3（低）**: Phase 1の実装後に、さらなる最適化が必要な場合に実施。
4. **Phase 4（低）**: 微細な最適化。Phase 1-3で十分な効果が得られた場合は省略可能。

## 検証

### 実装
`Texture2D::InternalLoad`(EngineCore\src\Renderer\Texture\Texture2D.cpp) 内で各ステップの時間を計測し、詳細なログを出力する。
入力に使用するTextureは、japanese_street_at_night(Game\Assets\japanese_street_at_night/textures)であり、107テクスチャ、全て2048x2048である。

### 定量評価における目標値の設定

[## 計測]と同条件の上で以下の目標値を設定する。目標値は、[### ボトルネックの特定]で計算した理論値に実用的なマージンを加えた値とする。

#### 目標値の算出根拠

**理論値（[### ボトルネックの特定]より）**:
- CPU Copy: 0.5 ～ 1.5 ms（実効予測値）
- GPU Copy: 0.6 ～ 0.7 ms（実効予測値）
- Data Transfer Total（理論上の合計）: 1.1 ～ 2.2 ms

**マージン設定方針**:
- 理論値の上限に約2倍のマージンを加える（実装のオーバーヘッド、システム負荷変動、測定誤差を考慮）
- 実用的な目標として、理論値の2倍程度を上限とする

#### 設定目標値

- **CPU Copy（メモリコピー）**: **3.0 ms以下**
  - 理論値上限（1.5 ms）× 2倍 = 3.0 ms

- **GPU Copy（PCIe転送）**: **1.5 ms以下**
  - 理論値上限（0.7 ms）× 2倍 ≈ 1.5 ms

- **データ転送時間（Data Transfer Total）**: **4.5 ms以下**
  - 理論値上限（2.2 ms）× 2倍 ≈ 4.4 ms → 丸めて4.5 ms
  - CPU Copy + GPU Copy を含む

よって目標値を、

**本環境において2048x2048のpngテクスチャの
データ転送時間合計平均が4.5 ms以下に到達することとする。**

### Phase 1 評価結果

Phase1GPUTimestamp.txtの計測結果（107テクスチャ、全て2048x2048）を分析し、目標値との比較を行う。
#### 定量評価

**各ステップの統計（107テクスチャの計測結果）**

| ステップ | 最小値 | 最大値 | 平均値 | 全体に占める割合 |
|---------|--------|--------|--------|-----------------|
| File I/O + Decode | 7.500 ms | 106.133 ms | 約23.310 ms | **約40-90%** |
| GPU Resource Creation | 2.917 ms | 17.228 ms | 約3.688 ms | **約4-43%** |
| Data Transfer(Total) | 2.221 ms | 5.366 ms | 約3.072 ms | **約2-18%** |
| > CPU Copy | 0.979 ms | 3.349 ms | 約1.512 ms | **約1-12%** |
| > GPU Copy | 1.182 ms | 2.897 ms | 約1.560 ms | **約1-9%** |
| Other | 0.000 ms | 0.001 ms | 約0.000 ms | **約0-0%** |
| **Total** | **17.442 ms** | **117.673 ms** | **約34.962 ms** | **100%** |

#### 目標値との比較

**CPU Copy（メモリコピー）**
- **実測平均値**: 約1.51 ms
- **目標値**: 3.0 ms以下
- **評価**: ✅ **目標値を達成**。理論値（0.5～1.5 ms）の範囲内に収まっており、メモリコピー処理は最適化されている。

**GPU Copy（PCIe転送 - GPU Timestamp計測）**
- **実測平均値**: 約1.56 ms
- **目標値**: 1.5 ms以下
- **評価**: ⚠️ **ほぼ達成**。理論値（0.6～0.7 ms）の約2倍程度。
- **理論値との乖離の原因**:
    - GPU Copy の計測には、CommandQueue::ExecuteCommandLists ～ Signal ～ イベントセット ～ WaitForSingleObject までのホスト側同期待機時間も含まれているため、厳密には純粋なPCIe転送のみの時間より若干長くなっていると考えられる。
    - また、GPUのコマンド発行から実際のリソース転送・完了までに、スケジューリングやディスパッチ待機などのオーバーヘッドが加算されるため、理論値より高めの値となるのは妥当である。

**Data Transfer (Total)**
- **実測平均値**: 約3.07 ms
- **目標値**: 4.5 ms以下
- **評価**: ✅ **目標値を達成**。
- **内訳**: CPU Copy（1.51 ms）+ GPU Copy（1.56 ms）= 3.07 ms

#### 改善前との比較

**改善前（WriteToSubresource方式）**:
- Data Transfer: 717.279 ms ～ 1104.1 ms（平均約950 ms）
- Total: 741.743 ms ～ 1113.5 ms（平均約980 ms）

**改善後（Phase 1: アップロードヒープ方式）**:
- Data Transfer (Total): 2.221 ms ～ 5.366 ms（平均約3.07 ms）
- Total: 17.442 ms ～ 117.673 ms（平均約35.0 ms）

**改善効果**:
- **Data Transfer**: 約**309倍**の高速化（950 ms → 3.07 ms）
- **Total**: 約**28倍**の高速化（980 ms → 35.0 ms）


