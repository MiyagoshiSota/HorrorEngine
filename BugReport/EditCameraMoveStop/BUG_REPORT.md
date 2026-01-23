# バグレポート: カメラ移動時のカクつき問題

## 問題概要

エディタモードでカメラをWASDキーで移動中、移動がカクカクして途中で止まる問題が発生していた。

https://drive.google.com/drive/folders/1_pkmDbEdoI6AQmeR5JOHLHIR7dAMh5B7?usp=drive_link

## 発生環境

- **発生ブランチ**: master 

## 再現手順

1. エディタモードで起動
2. WASDキーを押し続けて移動

## 原因分析

### 根本原因

`EngineCore/src/Core/App.cpp` の `main_loop()` 関数において、**Windowsメッセージがある間はゲームループが回らない実装**になっていた。

**修正前の実装（問題のあるバージョンのイメージ）:**
```cpp
if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE) == TRUE) {
    // メッセージ処理
    TranslateMessage(&msg);
    DispatchMessage(&msg);
}
else {
    // ゲームループ（InputDevice::Update()など）
}
```

**問題点:**
- `PeekMessage` が `TRUE` を返す限り「メッセージ処理側」だけが実行され、  
  `else` 側のゲームループ（`InputDevice::Update()` / `Scene->Update()` など）が**一切回らない**

### 詳細な動作

1. 毎フレームのループ先頭で `PeekMessage` が `TRUE` を返し続ける
2. その間、`else` ブロック（ゲームループ）が一切実行されない
3. 入力更新・シーン更新が止まり、結果としてカメラ移動がガクガクになる

### ログによる確認

`MoveSecondMsg02.txt` のログでは、イベントが発生するたびにそのイベントの処理だけが走り、  
ゲームループ側の処理がスキップされていることが確認できる。

## 修正内容

### 修正後の実装（現在の実装）

`EngineCore/src/Core/App.cpp` の `main_loop()` 関数を、  
**「毎フレーム必ずゲームループを回しつつ、メッセージキューは空になるまで処理する」** 形に修正した。

```cpp
void main_loop() {
    MSG msg = {};

    while (WM_QUIT != msg.message)
    {
        // 経過時間計測
        auto currentTime = std::chrono::steady_clock::now();
        std::chrono::duration<float> deltaTime = currentTime - g_lastFrameTime;
        g_lastFrameTime = currentTime;

        while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            if (msg.message == WM_QUIT)
            {
                break;
            }

            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        // ゲームループ
        auto delta_time = deltaTime.count();
        InputDevice::GetInstance().Update(delta_time);

        if (g_scene_type == scene_type::play_mode)
        {
            g_Scene->Update(delta_time);
            WorkManager::GetInstance().Update(delta_time);
        }
        else if (g_scene_type == scene_type::editor_mode)
        {
            g_Scene->EditorUpdate(deltaTime.count());
        }

        g_Engine->BeginRender();
        g_Scene->Draw();
        g_Engine->EndRender();
        g_Engine->MoveToNextFrame();
    }
}
```

## 修正後の動作確認

- [X] エディタモードでのカメラ移動がカクつかず、キー入力に対して滑らかに追従することを確認
