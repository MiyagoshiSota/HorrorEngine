# Player 概念の設計

「絶対的なプレイヤー」を表すシングルトンと、その編集用ウィンドウ（PlayerWindow）の設計・現状をまとめる。

---

## 1. 設計の目的

- **絶対 1 体のプレイヤー**: シーン内で「プレイヤー」は常に 1 体だけ存在し、WorkFlow の Reward（例: アイテム付与）や Trigger（例: アイテム保持中クリック）が参照する対象を一意に決められるようにする。
- **アイテムリスト（インベントリ）の一元管理**: 入手・所持判定・消費を `Player` 経由で行い、Reward/Trigger が「誰に」を意識せずに済むようにする。
- **絶対 GameObject としてのスポーン**: プレイヤー用の GameObject を「Player がスポーンする」か「既存の GO を Player に登録する」かのどちらかで決め、シーン内の「どの GO がプレイヤーか」を明確にする。
- **Camera は持たない**: カメラは Scene が保持する現状のままとする。

---

## 2. 現状の実装

### 2.1 クラス構成

| クラス | 役割 |
|--------|------|
| **Player** | シングルトン。インベントリと「プレイヤー用 GameObject」の参照を保持。 |
| **PLayerController** | Component。ある GameObject に付与し、WASD で Rigidbody を動かす。Player クラスとは別。 |

- `Player` と `PLayerController` は独立している。  
- 「プレイヤーとして動かす GameObject」に `PLayerController` を付ける運用は可能だが、`Player::GetPlayerGameObject()` が指す GO と一致させるかはシーン側の責務。

### 2.2 Player の API（現状）

**取得**

- `Player::GetInstance()` … シングルトン参照。

**プレイヤー GameObject（絶対 1 体）**

- `Spawn()` … 新規 `GameObject` を生成し、名前 `"Player"` を付けて `g_Scene->AddGameObject(go)` し、内部参照に保持。既に保持していれば何もしない（`false` を返す）。
- `SetPlayerGameObject(std::shared_ptr<GameObject> go)` … 外部で生成した GO をプレイヤーとして登録。
- `GetPlayerGameObject()` … 保持しているプレイヤー用 GO。いなければ `nullptr`。
- `HasPlayerGameObject()` … プレイヤー GO を保持しているか。

**アイテムリスト（インベントリ）**

- `AddItem(const std::string& itemId)` … アイテム ID（文字列）を 1 件追加。
- `HasItem(const std::string& itemId)` … 所持しているか。
- `RemoveItem(const std::string& itemId)` … 最初の 1 件を削除。見つかれば `true`。
- `ClearInventory()` … 全削除。
- `GetInventory()` … `const std::vector<std::string>&` で中身を返す。

### 2.3 データ（現状）

- `m_playerGameObject` … `std::shared_ptr<GameObject>`。プレイヤーとして扱う 1 体の GO。
- `m_inventory` … `std::vector<std::string>`。アイテム ID のリスト（重複可。順序は追加順）。

### 2.4 スポーンまわり

- `Spawn()` 内で `GameObject` を `make_shared` で生成しているだけなので、**Component は一切付かない**（Rigidbody / PLayerController 等はシーン側やローダーで付与する想定）。
- プレイヤー GO をシーン JSON で読み込み、その `shared_ptr` を `SetPlayerGameObject(go)` に渡す運用も可能。

### 2.5 他システムとの関係（現状）

- **Work / WorkFlow / Trigger / Reward**: まだ `Player` を参照していない。  
  今後、例: `AddItemReward` が `Player::GetInstance().AddItem(id)` を呼ぶ、`HoldingItemCondition` が `Player::GetInstance().HasItem(id)` を参照する、といった形で利用する想定。
- **Scene**: `Player::Spawn()` が `g_Scene->AddGameObject(go)` でシーンに追加するだけ。Scene 側は「Player が誰か」を知らない。
- **Camera**: Scene が保持。Player は持たない。

---

## 3. PlayerWindow で編集したい要素（予定）

Editor 用の **PlayerWindow**（`IDrawWindow` 継承）を設け、次のような項目を編集できるようにする想定。

### 3.1 プレイヤー GameObject

- **現在のプレイヤー GO の表示**: `GetPlayerGameObject()` の名前、または「未設定」。
- **設定方法の選択**:
  - **Spawn**: 「スポーン」ボタンで `Spawn()` を実行。既にいれば無効または「既に存在します」表示。
  - **シーン内から選択**: シーンの `GetGameObjects()` 一覧から 1 つ選び、それを `SetPlayerGameObject(selected)` でプレイヤーに設定。
- **解除**: プレイヤー GO を null にする操作（必要なら `ClearPlayerGameObject()` のような API を Player に追加）。

### 3.2 アイテムリスト（インベントリ）

- **一覧表示**: `GetInventory()` の内容をリスト表示（ID の文字列）。
- **追加**: テキスト入力 + 「追加」で `AddItem(id)`。
- **削除**: 一覧の各行に「削除」ボタンで `RemoveItem(id)`（またはインデックスで 1 件削除）。
- **全クリア**: `ClearInventory()` に対応するボタン。
- （将来）**重複可否・最大数**などのルールを Player 側で持つ場合は、その設定項目もここに並べる想定。

### 3.3 その他（将来候補）

- プレイヤー GO の Transform の簡易表示（読み取り専用 or 編集可能にするかは要検討）。
- 「現在持っているアイテム」を 1 つに限定する「手持ち」概念を導入する場合、その表示・切り替え。

---

## 4. 設計上の注意点・未決定事項

- **アイテム ID の仕様**: 現状は `std::string` のまま。ID と表示名のマスタを持つか、数値 ID に変えるかは未定。
- **インベントリの永続化**: セーブ/ロードでインベントリを出すか、Player のシリアライズをどこで行うかは未定。
- **プレイヤー GO の永続化**: シーン JSON に「この GO を Player として扱う」フラグを持たせるか、起動時に名前で検索して `SetPlayerGameObject` するかは未定。
- **Editor / Play の切り替え**: Player の参照やインベントリを、モード切り替え時にリセットするか、Editor で触った内容を Play に引き継ぐかはポリシー次第。

---

## 5. ファイル配置（現状）

| パス | 説明 |
|------|------|
| `EngineCore/src/Scene/Character/Player/Player.h` | Player クラス宣言。 |
| `EngineCore/src/Scene/Character/Player/Player.cpp` | Spawn / インベントリ実装。 |
| `EngineCore/src/Scene/Character/Player/PlayerController.h` | PLayerController（Component）。 |

PlayerWindow は `EngineCore/src/GUI/Windows/DrawPlayerWindow.h`（および必要なら `.cpp`）に追加し、`LayoutPresetManager` 等で他のウィンドウと同様に登録する想定。

---

## 6. Play Mode Camera（専用設定）

PlayMode 時のカメラの動きは、**Player ではなく専用の設定オブジェクト**で持つ。PlayerWindow から編集し、DefaultScene::Update（PlayMode）で適用する。

### 6.1 設計の目的

- **Player の単一責務**: Player は「誰がプレイヤーか」「何を持っているか」に専念し、カメラ挙動は別レイヤーにする。
- **変更の局所化**: カメラモードやパラメータの追加は設定クラスのみで完結する。
- **参照の一意性**: Editor の PlayerWindow と PlayMode の DefaultScene::Update の両方が、同じ 1 つの設定を読む。

### 6.2 設定の保持

- **クラス名**: `PlayModeCameraConfig`
- **保持場所**: シングルトン（`PlayModeCameraConfig::GetInstance()`）。Editor / Play のどちらからも参照できる。
- **適用箇所**: `DefaultScene::Update(float deltaTime)` 内（PlayMode 時のみ）。設定のモードに応じて `m_Camera` の位置・注視点を更新するか、従来どおり `m_Camera->Update(deltaTime)`（入力駆動）を呼ぶ。

### 6.3 モードとパラメータ

| モード | 説明 | パラメータ例 |
|--------|------|----------------|
| **Free** | 入力（WASD・右ドラッグ・中ドラッグ・ホイール）でカメラのみ移動。従来の Editor 風。 | なし |
| **FirstPerson** | カメラ＝プレイヤー位置＋目のオフセット。注視点＝目の位置＋プレイヤー前方方向。 | 目のオフセット (float3) |
| **Follow** | カメラ＝プレイヤー後方オフセット（距離・高さ）。注視点＝プレイヤー付近。スムージング可。 | 距離・高さ・注視点の高さ・スムージング係数 |

- **プレイヤー未設定時**: モードが FirstPerson / Follow でもプレイヤー GO がなければ、そのフレームはカメラを更新しない（または Free 相当にフォールバック）。

### 6.4 PlayerWindow との関係

- PlayerWindow に **「Play Mode Camera」** セクションを追加する。
- モード選択（コンボボックス）と、各モード用のパラメータ（DragFloat3 / DragFloat）を並べる。
- 編集対象は `PlayModeCameraConfig::GetInstance()` のフィールドのみ。

### 6.5 ファイル配置

| パス | 説明 |
|------|------|
| `EngineCore/src/Scene/Camera/PlayModeCameraConfig.h` | PlayMode 用カメラ設定（モード enum・パラメータ・シングルトン）。 |
| `EngineCore/src/Scene/Camera/PlayModeCameraConfig.cpp` | `GetInstance()` の実装。 |
| `DefaultScene::Update` | 設定を読み、`ApplyPlayModeCamera(deltaTime)` でモードに応じて `m_Camera` を更新。 |
| `DrawPlayerWindow` | 「Play Mode Camera」セクションでモード選択とパラメータを編集。 |
