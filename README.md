# Xbox Elite Wireless Controller Series 2 制御システム - 動作説明と注意事項

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
    uint16_t LHori, LVert, RHori, RVert;
    // トリガー
    uint16_t LT, RT;
    // ボタン（ビットフィールド）
    uint8_t A:1, B:1, X:1, Y:1, LB:1, RB:1;
    // 方向パッド
    uint8_t Up:1, Down:1, Left:1, Right:1;
    // その他
    uint8_t tenkey, Connect, Profile;
    // など
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
