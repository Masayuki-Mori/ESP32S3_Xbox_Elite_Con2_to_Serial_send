// Reference: https://github.com/h2zero/NimBLE-Arduino/blob/master/examples/NimBLE_Client/NimBLE_Client.ino

#include <NimBLEDevice.h>
#include <Wire.h>

bool input_flag = false;
bool btnY_stat = false;
bool btnB_stat = false;
bool btnLB_stat = false;
bool btnRB_stat = false;

struct __attribute__((packed)){
  uint16_t LHori;
  uint16_t LVert;
  uint16_t RHori;
  uint16_t RVert;
  uint16_t LT;
  uint16_t RT;
  uint8_t tenkey;
  uint8_t A:1;
  uint8_t B:1;
  uint8_t :1;
  uint8_t X:1;
  uint8_t Y:1;
  uint8_t :1;
  uint8_t LB:1;
  uint8_t RB:1;
  uint8_t :2;
  uint8_t View:1;
  uint8_t Menu:1;
  uint8_t Xbox:1;
  uint8_t LS:1;
  uint8_t RS:1;
  uint8_t :1;
  uint8_t Connect;
  uint8_t Profile;
  uint8_t tLT_dep:2;
  uint8_t tRT_dep:2;
  uint8_t :4;
  uint8_t P1:1;
  uint8_t P2:1;
  uint8_t P3:1;
  uint8_t P4:1;
  uint8_t :4;
  uint8_t Up:1;
  uint8_t Down:1;
  uint8_t Left:1;
  uint8_t Right:1;
}Xpad;

union DataPacket {
    struct {
        char start[2];      // 'U', 'P'
        int16_t lift_mm_front;// 2バイト
        int16_t lift_mm_rear; // 2バイト 
        uint16_t sumo_angle; 
        char end[2];        // 'E', 'N'
    } data;
    uint8_t bytes[10];
};

static NimBLEUUID uuidServiceBattery("180f");
static NimBLEUUID uuidServiceHid("1812");
static NimBLEAddress targetDeviceAddress("98:7a:14:40:27:b3",false);

static const NimBLEAdvertisedDevice* advDevice;
static bool                          doConnect = false;
static NimBLEClient* pConnectedClient = nullptr;
class ClientCallbacks : public NimBLEClientCallbacks {
  void onConnect(NimBLEClient* pClient) {
    Serial.printf("Connected\n");
  }
  void onDisconnect(NimBLEClient* pClient, int reason) {
    pConnectedClient = nullptr;
    Serial.printf("%s Disconnected, reason = %d - Starting scan\n", pClient->getPeerAddress().toString().c_str(), reason);
    NimBLEDevice::getScan()->start(0);
  }
  void onPassKeyEntry(NimBLEConnInfo& connInfo) {
    NimBLEDevice::injectPassKey(connInfo, 0);
  }
  void onConfirmPasskey(NimBLEConnInfo& connInfo, uint32_t pass_key) {
    NimBLEDevice::injectConfirmPasskey(connInfo, true);
  }
  void onAuthenticationComplete(NimBLEConnInfo& connInfo) {
    if (!connInfo.isEncrypted()) {
      NimBLEDevice::getClientByHandle(connInfo.getConnHandle())->disconnect();
      return;
    }
  }
}clientCallbacks;

class ScanCallbacks : public NimBLEScanCallbacks {
  void onResult(const NimBLEAdvertisedDevice* advertisedDevice) override {
    Serial.printf("Found Our Service: %s\n", advertisedDevice->toString().c_str());
    advDevice = advertisedDevice;
    doConnect = true;
    NimBLEDevice::getScan()->stop();
  }
}ScanCallbacks;
uint8_t battery = 0;

void writeHIDReport(uint8_t* dataArr) {
  if (pConnectedClient != nullptr) {
    NimBLERemoteService*  pSrvc = pConnectedClient->getService(uuidServiceHid);
    for (NimBLERemoteCharacteristic* pChr : pSrvc->getCharacteristics()) {
      if (pChr->canWrite()) pChr->writeValue(dataArr, 8U, false);
    }
  }
}
void notifyCB_Battery(NimBLERemoteCharacteristic* pRemoteCharacteristic,uint8_t* pData, size_t length, bool isNotify) {
  battery = pData[0];
}

void notifyCB_HID(NimBLERemoteCharacteristic* pRemoteCharacteristic,uint8_t* pData, size_t length, bool isNotify) {
  memcpy(&Xpad, pData, sizeof(Xpad));
  switch (Xpad.tenkey) {
    case 1: Xpad.Up = true;  Xpad.Right = false; Xpad.Down = false; Xpad.Left = false; break;
    case 2: Xpad.Up = true;  Xpad.Right = true;  Xpad.Down = false; Xpad.Left = false; break;
    case 3: Xpad.Up = false; Xpad.Right = true;  Xpad.Down = false; Xpad.Left = false; break;
    case 4: Xpad.Up = false; Xpad.Right = true;  Xpad.Down = true;  Xpad.Left = false; break;
    case 5: Xpad.Up = false; Xpad.Right = false; Xpad.Down = true;  Xpad.Left = false; break;
    case 6: Xpad.Up = false; Xpad.Right = false; Xpad.Down = true;  Xpad.Left = true;  break;
    case 7: Xpad.Up = false; Xpad.Right = false; Xpad.Down = false; Xpad.Left = true;  break;
    case 8: Xpad.Up = true;  Xpad.Right = false; Xpad.Down = false; Xpad.Left = true;  break;
    default: Xpad.Up = Xpad.Right = Xpad.Down = Xpad.Left = false; break;
  }
  input_flag  = true;
}

bool connectToServer(const NimBLEAdvertisedDevice* advDevice) {
  NimBLEClient* pClient = nullptr;

  /** Check if we have a client we should reuse first **/
  if (NimBLEDevice::getCreatedClientCount()) {
    pClient = NimBLEDevice::getClientByPeerAddress(advDevice->getAddress());
    if (pClient) {
      if (!pClient->connect(advDevice, false)) {
        Serial.printf("Reconnect failed\n");
        return false;
      }
      Serial.printf("Reconnected client\n");
    } else {
      pClient = NimBLEDevice::getDisconnectedClient();
    }
  }
  if (!pClient) {
    if (NimBLEDevice::getCreatedClientCount() >= NIMBLE_MAX_CONNECTIONS) {
      Serial.printf("Max clients reached - no more connections available\n");
      return false;
    }
    pClient = NimBLEDevice::createClient();
    Serial.printf("New client created\n");
    pClient->setClientCallbacks(&clientCallbacks,false);
    //pClient->setConnectPhy(BLE_GAP_LE_PHY_1M_MASK);
    pClient->setConnectionParams(6, 6, 0, 150);
    //pClient->setConnectTimeout(500);
    if (!pClient->connect(advDevice)) {
      NimBLEDevice::deleteClient(pClient);
      Serial.printf("Failed to connect, deleted client\n");
      return false;
    }
  }
  if (!pClient->isConnected()) {
        if (!pClient->connect(advDevice)) {
            Serial.printf("Failed to connect\n");
            return false;
        }
  }
  Serial.printf("Connected to: %s RSSI: %d\n", pClient->getPeerAddress().toString().c_str(), pClient->getRssi());
  
  NimBLERemoteService* pSvc = pClient->getService(uuidServiceHid);
  for (NimBLERemoteCharacteristic* pChr : pSvc->getCharacteristics(true)) {
    if (pChr->canRead()) pChr->readValue();
    if (pChr->canNotify()) pChr->subscribe(true,&notifyCB_HID,true);
  }
  pSvc = pClient->getService(uuidServiceBattery);
  NimBLERemoteCharacteristic* pChr = pSvc->getCharacteristic("2a19");
  pChr->readValue();
  pChr->subscribe(true,&notifyCB_Battery,true);

  pConnectedClient = pClient;
  return true;
}

void PCA9865_init(){
  uint8_t buf[5];
  // Soft reset of the I2C chip
  Wire.beginTransmission(0x00);//First of all, Soft reset 
  Wire.write(0x06);
  Wire.endTransmission();

  Wire.beginTransmission(0x40);
  Wire.write(0xFE);
  Wire.write(0x52);//76.3Hz @IntOSC 25MHz
  //Wire.write(0x0C);//475Hz @IntOSC 25MHz, 
  Wire.endTransmission();

  Wire.beginTransmission(0x40);
  buf[0] = 0x00;
  buf[1] = 0x21;
  Wire.write(buf,2);
  Wire.endTransmission();
  // Set the mode 1 register to come out of sleep
}

void ServoWrite(uint8_t servo,int16_t throttle){
  uint8_t buf[3];
  uint16_t pwmVal;
  pwmVal = throttle;//469 @1.5ms
  Wire.beginTransmission(0x40);
  buf[0] = servo;//ch
  buf[1] = uint8_t(pwmVal & 0xff);
  buf[2] = uint8_t(pwmVal >> 8);
  Wire.write(buf,3);
  Wire.endTransmission();
}

void Send_serial_message(int16_t f_up,int16_t r_up,uint16_t sumo){
  DataPacket packet = {
    .data = {
        {'U', 'P'},
        f_up,
        r_up,
        sumo,
        {'E', 'D'}
    }
  }; 
  Serial2.write(packet.bytes,10);
  //Serial.write(packet.bytes,10);
  static uint8_t dataArr[8];
  dataArr[0] = 0x0f;
  dataArr[1] = 0;
  dataArr[2] = 0;
  dataArr[3] = 0;
  dataArr[4] = 100;
  dataArr[5] = 20;
  dataArr[6] = 0;
  dataArr[7] = 0;  
  writeHIDReport(dataArr);
}

void setup() {

  Xpad.Profile = 3; 
  Serial.begin();
  Serial2.begin(115200, SERIAL_8N1,3,2);

  NimBLEDevice::init("");
  NimBLEDevice::setOwnAddrType(BLE_OWN_ADDR_PUBLIC);
  NimBLEDevice::setSecurityAuth(true, false, false);
  NimBLEDevice::setPower(9); /* +9db */
  NimBLEScan* pScan = NimBLEDevice::getScan();
  pScan->setDuplicateFilter(2);
  pScan->setScanCallbacks(&ScanCallbacks);
  pScan->setActiveScan(true);
  pScan->setInterval(97);
  pScan->setFilterPolicy(BLE_HCI_SCAN_FILT_USE_WL);
  NimBLEDevice::whiteListAdd(targetDeviceAddress);
  pScan->setWindow(97);
  pScan->start(0);

}

void loop() {
  if (doConnect) {
    if (connectToServer(advDevice)) {
      Serial.printf("Success!!\n");
    } else {
      Serial.printf("Failed to connect, starting scan\n");
      NimBLEDevice::deleteBond(advDevice->getAddress());
    }
   doConnect = false;
  }

  if(Xpad.RB){
    btnRB_stat=true;
  }else if(btnRB_stat){
    btnRB_stat=false;
    Send_serial_message(20,20,0);
  }
  if(Xpad.LB){
    btnLB_stat=true;
  }else if(btnLB_stat){
    btnLB_stat=false;
    Send_serial_message(-20,-20,0);
  }
  if(input_flag){
    input_flag = false;
    
    if(Xpad.A) Serial.print("A ");
    if(Xpad.B) Serial.print("B ");
    if(Xpad.X) Serial.print("X ");
    if(Xpad.Y) Serial.print("Y ");
    if(Xpad.LB) Serial.print("LB ");
    if(Xpad.RB) Serial.print("RB ");
    Serial.println("");
    if(Xpad.Connect) Serial.print("Connect ");
    if(Xpad.Xbox) Serial.print("Xbox ");
    if(Xpad.Menu) Serial.print("Menu ");
    if(Xpad.View) Serial.print("View ");
    if(Xpad.LS) Serial.print("LS ");
    if(Xpad.RS) Serial.print("RS ");
    Serial.println("");
    if(Xpad.Up) Serial.print("Up ");
    if(Xpad.Down) Serial.print("Down ");
    if(Xpad.Left) Serial.print("Left ");
    if(Xpad.Right) Serial.print("Right ");
    Serial.println("");
    if(Xpad.P1) Serial.print("P1 ");
    if(Xpad.P2) Serial.print("P2 ");
    if(Xpad.P3) Serial.print("P3 ");
    if(Xpad.P4) Serial.print("P4 ");
    Serial.println("");
    Serial.printf("tLT_dep: %d, tRT_dep: %d\n",Xpad.tLT_dep,Xpad.tRT_dep);
    Serial.printf("profile: %d\n",Xpad.Profile);
    Serial.printf("Battery: %d\n",battery);
    Serial.printf("joyL: %05d,%05d\n",Xpad.LHori,Xpad.LVert);
    Serial.printf("joyR: %05d,%05d\n",Xpad.RHori,Xpad.RVert);
    Serial.printf("trigLT: %04d\n",Xpad.LT);
    Serial.printf("trigRT: %04d\n",Xpad.RT);
  }
}