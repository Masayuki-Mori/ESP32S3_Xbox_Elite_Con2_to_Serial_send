# ESP32S3 with Xbox Elite Wireless Controller Series 2 

## 特徴
ESP32S3とXbox Elite Wireless Controller Series 2をBluetoothで接続します。
- 各種ボタン・スティック・トリガーの入力をリアルタイム（7-15 ms）で取得します。
- ESP32S3側からコントローラーを振動させることができます。
- ホワイトリストを使用しているので、コントローラー以外の望ましく無いBTデバイスの影響を排除できます。

### 1. 接続
- **対象デバイス**: Xbox Elite Wireless Controller Series 2のみ。
- **使用ライブラリ**: [h2zero/NimBLE-Arduino]（Ver2.3.4で動作確認済み）
- **サービス**: HIDサービス（`1812`）とバッテリーサービス（`180f`）
- **Bluetooth情報**:LE 1M PHYで接続 

[h2zero/NimBLE-Arduino]:<https://github.com/h2zero/NimBLE-Arduino>

### 2. データ構造
### Analog inputs
| [Phys. Name] | Variable Name | Data type | Value |
|-------------|------------|---------|---------|
| Left stick (Horizontal) | `Xpad.LHori` | `uint16_t` | 0～65535 (neutral:32768) |
| Left stick (Vertical) | `Xpad.LVert` | `uint16_t` | 0～65535 (neutral:32768) |
| Right stick (Horizontal) | `Xpad.RHori` | `uint16_t` | 0～65535 (neutral:32768) |
| Right stick (Vertical) | `Xpad.RVert` | `uint16_t` | 0～65535 (neutral:32768) |
| Left trigger（LT） | `Xpad.LT` | `uint16_t` | 0～1023 (neutral:0) |
| Right trigger（RT） | `Xpad.RT` | `uint16_t` | 0～1023 (neutral:0) |

[Phys. Name]:<https://support.xbox.com/en-US/help/hardware-network/controller/get-to-know-elite-series-2/>

### Digital bottons

| [Phys. Name] | Variable Name | Data type | Value |
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

| [Phys. Name] | Variable Name | Data type | Value |
|-------------|------------|---------|-----------|
| Up | `Xpad.Up` | `uint8_t:1` | 0,1 |
| Down | `Xpad.Down` | `uint8_t:1` | 0,1 |
| Left | `Xpad.Left` | `uint8_t:1` | 0,1 |
| Right | `Xpad.Right` | `uint8_t:1` | 0,1 |

### Elite Series 2 limited

| [Phys. Name] | Variable Name | Data type | Value |
|-------------|------------|---------|----|
| P1 paddle | `Xpad.P1` | `uint8_t:1` | 0,1 |
| P2 paddle | `Xpad.P2` | `uint8_t:1` | 0,1 |
| P3 paddle | `Xpad.P3` | `uint8_t:1` | 0,1 |
| P4 paddle | `Xpad.P4` | `uint8_t:1` | 0,1 |
| Left trigger lock | `Xpad.LT_lock` | `uint8_t:2` | 0～2 |
| Right trigger lock | `Xpad.RT_lock` | `uint8_t:2` | 0～2 |
| Profile (button) | `Xpad.Profile` | `uint8_t` | 0～3 |

### `Xpad.D_pad` law value and directions

| `Xpad.D_pad` law value | Direction | `Up` | `Right` | `Down` | `Left` |
|-----------|------|------|---------|--------|--------|
| 0 | Neutral | 0 | 0 | 0 | 0 |
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

### PlatformIO設定
#### `platformio.ini` 必要設定例
```ini
[env:seeed_xiao_esp32s3]
platform = espressif32
board = seeed_xiao_esp32s3
framework = arduino
lib_deps = h2zero/NimBLE-Arduino
upload_speed = 921600
monitor_speed = 250000
board_build.f_cpu = 240000000L
build_flags = 
        -D ARDUINO_USB_CDC_ON_BOOT=1
        -D ARDUINO_USB_MODE=1
upload_port = /dev/tty.usbmodem101
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

## 動作フロー
1. 起動時：NimBLE initialize, Scan start
2. ターゲットデバイス発見：Connect to server device automatically 
3. 接続成功：Subscribe HID/Batter Services Notification
4. ループ処理：
   - ゲームパッド入力監視
   - RB/LBボタン制御
   - シリアル/HID出力
   - デバッグ情報表示

## Xbox Elite Wireless Controller Series 2 UUID list

> [!IMPORTANT]
> - UUID 0x180f(Battery Service) always show value 0x64(100).So we couldn't know battery level.
> - UUID 00000005-5f60-4c4f-9c83-a7953298d40d also show Pad inputs,bat slow reflesh rate(15ms).

| Service UUID | Characteristic UUID | Handle | Can Read | Can Write | Can notify |
|--------------|---------------------|--------|----------|-----------|------------|
| 0x1800 | 0x2a00 | 3 |  :heavy_check_mark: |  |  |
|        | 0x2a01 | 5 |  :heavy_check_mark: |  |  |
|        | 0x2a04 | 7 |  :heavy_check_mark: |  |  |
| 0x1801 |  |  | |  |  |
| 0x180a | 0x2a29 | 11 |  :heavy_check_mark: |  |  |
|        | 0x2a50 | 13 |  :heavy_check_mark: |  |  |
|        | 0x2a26 | 15 |  :heavy_check_mark: |  |  |
|        | 0x2a25 | 17 |  :heavy_check_mark: |  |  |
| 0x180f(Battery Service) | 0x2a19 | 20 |  :heavy_check_mark: |  | :heavy_check_mark:  |
| 0x1812(HID Service) | 0x2a4a | 24 |  :heavy_check_mark: |  |  |
|        | 0x2a4c | 26 |  |  |  |
|        | 0x2a4b | 28 | :heavy_check_mark: |  |  |
|        | 0x2a4d(Pad inputs) | 30 | :heavy_check_mark: |  | :heavy_check_mark: |
|        | 0x2a4d(Vibration) | 34 | :heavy_check_mark: | :heavy_check_mark: |  |
|        | 0x2a4d | 37 | :heavy_check_mark: |  | :heavy_check_mark: |
|        | 0x2a4d | 41 | :heavy_check_mark: |  | :heavy_check_mark: |
| 00000001-5f60-4c4f-9c83-a7953298d40d | 00000002-5f60-4c4f-9c83-a7953298d40d | 46 |  :heavy_check_mark: |  |  |
|  | 00000003-5f60-4c4f-9c83-a7953298d40d | 48 |  :heavy_check_mark: |  |  |
|  | 00000004-5f60-4c4f-9c83-a7953298d40d | 50 |   | :heavy_check_mark: |  |
|  | 00000005-5f60-4c4f-9c83-a7953298d40d | 52 |  :heavy_check_mark: |  | :heavy_check_mark: |
|  | 00000006-5f60-4c4f-9c83-a7953298d40d | 55 |  :heavy_check_mark: | :heavy_check_mark: |  |
|  | 00000008-5f60-4c4f-9c83-a7953298d40d | 57 |  :heavy_check_mark: | :heavy_check_mark: |  |
