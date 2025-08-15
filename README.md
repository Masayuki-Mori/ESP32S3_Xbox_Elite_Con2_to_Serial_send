# ESP32S3 with Xbox Elite Wireless Controller Series 2 

## 概要
ESP32S3とXbox Elite Wireless Controller Series 2をBluetoothで接続します。
- 各種ボタン・スティック・トリガーの入力をリアルタイム（7-15 ms）で取得します。
- ESP32S3側からコントローラーを振動させることができます。
- ホワイトリストを使用しているので、コントローラー以外の望ましく無いBTデバイスの影響を排除できます。

### 1. 接続
- **対象デバイス**: Xbox Elite Wireless Controller Series 2のみ。
- **プロトコル**: NimBLE（ESP32用軽量BLEライブラリ）
- **サービス**: HIDサービス（`1812`）とバッテリーサービス（`180f`）

### 2. データ構造
```c
struct Xpad {
    // アナログスティック（左右）
    uint16_t LHori :0(left) - 65535(right)
    uint16_t LVert :0(up) - 65535(down)
    uint16_t RHori :0(left) - 65535(right)
    uint16_t RVert :0(up) - 65535(down)
    // アナログトリガー
    uint16_t LT :0 - 1023(pushed)
    uint16_t RT :0 - 1023(pushed)
    // ボタン（ビットフィールド）
    bool A : 0 - 1 (pushed)
    bool B : 0 - 1 (pushed)
    bool X : 0 - 1 (pushed)
    bool Y : 0 - 1 (pushed)
    bool LB : 0 - 1 (pushed)
    bool RB : 0 - 1 (pushed)
    bool LS : 0 - 1 (pushed)
    bool RS : 0 - 1 (pushed)
    bool View : 0 - 1 (pushed)
    bool Menu : 0 - 1 (pushed)
    bool Xbox : 0 - 1 (pushed)
    // 方向パッド
    bool Up : 0 - 1 (pushed)
    bool Down : 0 - 1 (pushed)
    bool Left : 0 - 1 (pushed)
    bool Right : 0 - 1 (pushed)
    // Eliteコントローラー専用
    uint8_t Profile:0 - 3(Rotary)
    bool P1 : 0 - 1 (pushed)
    bool P2 : 0 - 1 (pushed)
    bool P3 : 0 - 1 (pushed)
    bool P4 : 0 - 1 (pushed)
    uint8_t tLT_dep : 0 - 2 ()
    uint8_t tRT_dep : 0 - 2 ()
};
```

### 3. 特殊機能

#### RB/LBボタンによる自動制御
- **RBボタン**: 押下→離す で前進コマンド送信（`f_up=20, r_up=20`）
- **LBボタン**: 押下→離す で後退コマンド送信（`f_up=-20, r_up=-20`）

#### シリアル通信プロトコル
```c
// 送信データ構造（10バイト）
struct DataPacket {
    char start[2];        // 'U', 'P'
    int16_t lift_mm_front;// 前方リフト量
    int16_t lift_mm_rear; // 後方リフト量
    uint16_t sumo_angle;  // 相撲角度？
    char end[2];          // 'E', 'D'
};
```

## 注意事項

### ハードウェア要件
1. **ESP32**: NimBLE対応が必要
2. **PCA9865**: I2Cサーボドライバ（アドレス: `0x40`）
3. **シリアル接続**: Serial2（GPIO3=RX, GPIO2=TX, 115200bps）
4. **I2C**: Wire（SDA/SCLピンは環境依存）

### PlatformIO設定
#### `platformio.ini` 必要設定例
```ini
[env:esp32]
platform = espressif32
board = esp32dev
framework = arduino
lib_deps = 
    h2zero/NimBLE-Arduino@^1.4.0
    Wire
monitor_speed = 115200
build_flags = 
    -DCONFIG_BT_NIMBLE_ROLE_CENTRAL_DISABLED=0
    -DCONFIG_BT_NIMBLE_ROLE_PERIPHERAL_DISABLED=0
```

### 重要な注意点

#### 1. MACアドレスの変更
```c
// 使用するゲームパッドのMACアドレスに変更必要
static NimBLEAddress targetDeviceAddress("98:7a:14:40:27:b3",false);
```

### トラブルシューティング

#### 接続できない場合
1. ゲームパッドのペアリングモード確認
2. MACアドレスの正確性確認

#### PCA9865が応答しない
1. I2C配線確認（SDA/SCL + 電源）
2. プルアップ抵抗（4.7kΩ）の確認
3. アドレス競合の確認（`i2c_scanner`使用推奨）

## 動作フロー
1. 起動時：NimBLE初期化、スキャン開始
2. ターゲットデバイス発見：自動接続試行
3. 接続成功：HID/バッテリー通知登録
4. ループ処理：
   - ゲームパッド入力監視
   - RB/LBボタン制御
   - シリアル/HID出力
   - デバッグ情報表示

## Xbox Elite Series 2コントローラー - ボタンと読み取り値対応表

### アナログ入力（16bit値）

| 物理的な名称 | 構造体メンバ | データ型 | 値の範囲 | 説明 |
|-------------|------------|---------|---------|------|
| 左スティック（水平） | `Xpad.LHori` | `uint16_t` | 0～65535 | 左:0、中央:32768、右:65535 |
| 左スティック（垂直） | `Xpad.LVert` | `uint16_t` | 0～65535 | 上:0、中央:32768、下:65535 |
| 右スティック（水平） | `Xpad.RHori` | `uint16_t` | 0～65535 | 左:0、中央:32768、右:65535 |
| 右スティック（垂直） | `Xpad.RVert` | `uint16_t` | 0～65535 | 上:0、中央:32768、下:65535 |
| 左トリガー（LT） | `Xpad.LT` | `uint16_t` | 0～65535 | 未押下:0、最大:65535 |
| 右トリガー（RT） | `Xpad.RT` | `uint16_t` | 0～65535 | 未押下:0、最大:65535 |

### デジタルボタン（1bit値）

| 物理的な名称 | 構造体メンバ | データ型 | 押下時の値 | 説明 |
|-------------|------------|---------|-----------|------|
| Aボタン | `Xpad.A` | `uint8_t:1` | `1` | 右側の下ボタン |
| Bボタン | `Xpad.B` | `uint8_t:1` | `1` | 右側の右ボタン |
| Xボタン | `Xpad.X` | `uint8_t:1` | `1` | 右側の左ボタン |
| Yボタン | `Xpad.Y` | `uint8_t:1` | `1` | 右側の上ボタン |
| 左バンパー | `Xpad.LB` | `uint8_t:1` | `1` | 左肩上部ボタン |
| 右バンパー | `Xpad.RB` | `uint8_t:1` | `1` | 右肩上部ボタン |
| Viewボタン | `Xpad.View` | `uint8_t:1` | `1` | 左側小ボタン（旧Back） |
| Menuボタン | `Xpad.Menu` | `uint8_t:1` | `1` | 右側小ボタン（旧Start） |
| Xboxボタン | `Xpad.Xbox` | `uint8_t:1` | `1` | 中央のXboxロゴボタン |
| 左スティック押し込み | `Xpad.LS` | `uint8_t:1` | `1` | 左スティックのクリック |
| 右スティック押し込み | `Xpad.RS` | `uint8_t:1` | `1` | 右スティックのクリック |

### 方向パッド（D-Pad）

| 物理的な名称 | 構造体メンバ | データ型 | 押下時の値 | 説明 |
|-------------|------------|---------|-----------|------|
| 上方向 | `Xpad.Up` | `uint8_t:1` | `1` | D-Pad上方向 |
| 下方向 | `Xpad.Down` | `uint8_t:1` | `1` | D-Pad下方向 |
| 左方向 | `Xpad.Left` | `uint8_t:1` | `1` | D-Pad左方向 |
| 右方向 | `Xpad.Right` | `uint8_t:1` | `1` | D-Pad右方向 |

### 特殊な入力処理

| 入力 | 構造体メンバ | データ型 | 値 | 説明 |
|------|------------|---------|----|----- |
| D-Pad生値 | `Xpad.tenkey` | `uint8_t` | 1-8 | switch文で個別方向に変換 |
| 接続状態 | `Xpad.Connect` | `uint8_t` | 0/1 | 接続ボタンの状態 |
| プロファイル | `Xpad.Profile` | `uint8_t` | 0-3 | アクティブなプロファイル番号 |

### Elite Series 2 専用機能

| 物理的な名称 | 構造体メンバ | データ型 | 値 | 説明 |
|-------------|------------|---------|----|----- |
| パドル1 | `Xpad.P1` | `uint8_t:1` | `1` | 背面パドル1（左上） |
| パドル2 | `Xpad.P2` | `uint8_t:1` | `1` | 背面パドル2（左下） |
| パドル3 | `Xpad.P3` | `uint8_t:1` | `1` | 背面パドル3（右上） |
| パドル4 | `Xpad.P4` | `uint8_t:1` | `1` | 背面パドル4（右下） |
| 左トリガー深度 | `Xpad.tLT_dep` | `uint8_t:2` | 0-3 | トリガーストップ位置 |
| 右トリガー深度 | `Xpad.tRT_dep` | `uint8_t:2` | 0-3 | トリガーストップ位置 |

### tenkey値と方向パッド変換表

| `tenkey`値 | 方向 | `Up` | `Right` | `Down` | `Left` |
|-----------|------|------|---------|--------|--------|
| 0 | なし | 0 | 0 | 0 | 0 |
| 1 | 上 | 1 | 0 | 0 | 0 |
| 2 | 右上 | 1 | 1 | 0 | 0 |
| 3 | 右 | 0 | 1 | 0 | 0 |
| 4 | 右下 | 0 | 1 | 1 | 0 |
| 5 | 下 | 0 | 0 | 1 | 0 |
| 6 | 左下 | 0 | 0 | 1 | 1 |
| 7 | 左 | 0 | 0 | 0 | 1 |
| 8 | 左上 | 1 | 0 | 0 | 1 |
