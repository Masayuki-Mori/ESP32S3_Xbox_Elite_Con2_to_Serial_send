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
### Analog inputs

| Phys. Name | Variable Name | Data type | Value |
|-------------|------------|---------|---------|
| Left stick (Horizontal) | `Xpad.LHori` | `uint16_t` | 0～65535 (neutral:32768) |
| Left stick (Vertical) | `Xpad.LVert` | `uint16_t` | 0～65535 (neutral:32768) |
| Right stick (Horizontal) | `Xpad.RHori` | `uint16_t` | 0～65535 (neutral:32768) |
| Right stick (Vertical) | `Xpad.RVert` | `uint16_t` | 0～65535 (neutral:32768) |
| Left trigger（LT） | `Xpad.LT` | `uint16_t` | 0～1023 (neutral:0) |
| Right trigger（RT） | `Xpad.RT` | `uint16_t` | 0～1023 (neutral:0) |

### Digital bottons

| Phys. Name | Variable Name | Data type | Value |
|-------------|------------|---------|-----------|
| A button | `Xpad.A` | `uint8_t:1` | 0,1 |
| B button | `Xpad.B` | `uint8_t:1` | 0,1 |
| X button | `Xpad.X` | `uint8_t:1` | 0,1 |
| Y button | `Xpad.Y` | `uint8_t:1` | 0,1 |
| Left bumper | `Xpad.LB` | `uint8_t:1` | 0,1 |
| Right bumper | `Xpad.RB` | `uint8_t:1` | 0,1 |
| View button | `Xpad.View` | `uint8_t:1` | 0,1 |
| Menu button | `Xpad.Menu` | `uint8_t:1` | 0,1 |
| Xbox button | `Xpad.Xbox` | `uint8_t:1` | 0,1 |
| Left stick click | `Xpad.LS` | `uint8_t:1` | 0,1 |
| Right stick click | `Xpad.RS` | `uint8_t:1` | 0,1 |

### Directional pad (D-pad)

| Phys. Name | Variable Name | Data type | Value |
|-------------|------------|---------|-----------|
| Up | `Xpad.Up` | `uint8_t:1` | 0,1 |
| Down | `Xpad.Down` | `uint8_t:1` | 0,1 |
| Left | `Xpad.Left` | `uint8_t:1` | 0,1 |
| Right | `Xpad.Right` | `uint8_t:1` | 0,1 |

### Elite Series 2 limited

| Phys. Name | Variable Name | Data type | Value |
|-------------|------------|---------|----|
| P1 paddle | `Xpad.P1` | `uint8_t:1` | 0,1 |
| P2 paddle | `Xpad.P2` | `uint8_t:1` | 0,1 |
| P3 paddle | `Xpad.P3` | `uint8_t:1` | 0,1 |
| P4 paddle | `Xpad.P4` | `uint8_t:1` | 0,1 |
| Left trigger lock | `Xpad.LT_lock` | `uint8_t:2` | 0～2 |
| Right trigger lock | `Xpad.RT_lock` | `uint8_t:2` | 0～2 |
| Profile (button) | `Xpad.Profile` | `uint8_t` | 0～3 |

### tenkey値と方向パッド変換表

| `D-pad`Law value | Direction | `Up` | `Right` | `Down` | `Left` |
|-----------|------|------|---------|--------|--------|
| 0 | None | 0 | 0 | 0 | 0 |
| 1 | Up | 1 | 0 | 0 | 0 |
| 2 | Upper right | 1 | 1 | 0 | 0 |
| 3 | Right | 0 | 1 | 0 | 0 |
| 4 | Down right | 0 | 1 | 1 | 0 |
| 5 | Down | 0 | 0 | 1 | 0 |
| 6 | Down left | 0 | 0 | 1 | 1 |
| 7 | Left | 0 | 0 | 0 | 1 |
| 8 | Upper left | 1 | 0 | 0 | 1 |

### 3. 特殊機能

#### RB/LBボタンによる自動制御
- **RBボタン**: 押下→離す で前進コマンド送信（`f_up=20, r_up=20`）
- **LBボタン**: 押下→離す で後退コマンド送信（`f_up=-20, r_up=-20`）

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
