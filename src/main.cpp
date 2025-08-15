// Reference: https://github.com/h2zero/NimBLE-Arduino/blob/master/examples/NimBLE_Client/NimBLE_Client.ino

#include <NimBLEDevice.h>
#include <Wire.h>

bool input_flag = false;
bool btnA, btnB, btnX, btnY;
bool btnConnect, btnMenu, btnView, btnXbox;
bool btnLB, btnRB, btnLS, btnRS;
bool btnUp, btnLeft, btnRight, btnDown;
bool btnP1_elt, btnP2_elt, btnP3_elt, btnP4_elt;

uint16_t joyLHori = 0;
uint16_t joyLVert = 0;
uint16_t joyRHori = 0;
uint16_t joyRVert = 0;
uint16_t trigLT, trigRT;
uint8_t profile, tLT_dep, tRT_dep;

bool btnY_stat = false;
bool btnB_stat = false;
bool btnLB_stat = false;
bool btnRB_stat = false;

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

DataPacket packet = {0};

static NimBLEUUID uuidServiceBattery("180f");
static NimBLEUUID uuidServiceHid("1812");
static NimBLEAddress* targetDeviceAddress = new NimBLEAddress("98:7a:14:40:27:b3",false);

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
  joyLHori = ((uint16_t)pData[1] << 8) | pData[0];
  joyLVert = ((uint16_t)pData[3] << 8) | pData[2];
  joyRHori = ((uint16_t)pData[5] << 8) | pData[4];
  joyRVert = ((uint16_t)pData[7] << 8) | pData[6];
  trigLT = ((uint16_t)pData[9] << 8) | pData[8];
  trigRT = ((uint16_t)pData[11] << 8) | pData[10];
  switch (pData[12] & 0b00001111){
    case 1: btnUp = true;  btnRight = false; btnDown = false; btnLeft = false; break;
    case 2: btnUp = true;  btnRight = true;  btnDown = false; btnLeft = false; break;
    case 3: btnUp = false; btnRight = true;  btnDown = false; btnLeft = false; break;
    case 4: btnUp = false; btnRight = true;  btnDown = true;  btnLeft = false; break;
    case 5: btnUp = false; btnRight = false; btnDown = true;  btnLeft = false; break;
    case 6: btnUp = false; btnRight = false; btnDown = true;  btnLeft = true;  break;
    case 7: btnUp = false; btnRight = false; btnDown = false; btnLeft = true;  break;
    case 8: btnUp = true;  btnRight = false; btnDown = false; btnLeft = true;  break;
  }
  btnA        = pData[13] & 0b00000001;
  btnB        = pData[13] & 0b00000010;
  btnX        = pData[13] & 0b00001000;
  btnY        = pData[13] & 0b00010000;
  btnLB       = pData[13] & 0b01000000;
  btnRB       = pData[13] & 0b10000000;
  btnView     = pData[14] & 0b00000100;
  btnMenu     = pData[14] & 0b00001000;
  btnXbox     = pData[14] & 0b00010000;
  btnLS       = pData[14] & 0b00100000;
  btnRS       = pData[14] & 0b01000000;
  btnConnect  = pData[15] & 0b00000001;
  profile     = pData[16] & 0b00000011;
  tLT_dep     = pData[17] & 0b00000011;
  tRT_dep     = (pData[17] & 0b00001100) >> 2;
  btnP1_elt   = pData[18] & 0b00000001;
  btnP2_elt   = pData[18] & 0b00000010;
  btnP3_elt   = pData[18] & 0b00000100;
  btnP4_elt   = pData[18] & 0b00001000;
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
    pClient->setConnectPhy(BLE_GAP_LE_PHY_1M_MASK);
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

  profile = 3; 
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
  NimBLEDevice::whiteListAdd(*targetDeviceAddress);
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

  if(btnRB){
    btnRB_stat=true;
  }else if(btnRB_stat){
    btnRB_stat=false;
    Send_serial_message(20,20,0);
  }
  if(btnLB){
    btnLB_stat=true;
  }else if(btnLB_stat){
    btnLB_stat=false;
    Send_serial_message(-20,-20,0);
  }
  static unsigned long lastRead = 0;
  if(input_flag){
    input_flag = false;

    Serial.print(millis()-lastRead);
    Serial.println(" ms");
    lastRead = millis();
    /*if(btnA) Serial.print("A ");
    if(btnB) Serial.print("B ");
    if(btnX) Serial.print("X ");
    if(btnY) Serial.print("Y ");
    if(btnLB) Serial.print("LB ");
    if(btnRB) Serial.print("RB ");
    Serial.println("");
    if(btnConnect) Serial.print("Connect ");
    if(btnXbox) Serial.print("Xbox ");
    if(btnMenu) Serial.print("Menu ");
    if(btnView) Serial.print("View ");
    if(btnLS) Serial.print("LS ");
    if(btnRS) Serial.print("RS ");
    Serial.println("");
    if(btnUp) Serial.print("Up ");
    if(btnDown) Serial.print("Down ");
    if(btnLeft) Serial.print("Left ");
    if(btnRight) Serial.print("Right ");
    Serial.println("");
    if(btnP1_elt) Serial.print("P1_elt ");
    if(btnP2_elt) Serial.print("P2_elt ");
    if(btnP3_elt) Serial.print("P3_elt ");
    if(btnP4_elt) Serial.print("P4_elt ");
    Serial.println("");
    Serial.printf("tLT_dep: %d, tRT_dep: %d\n",tLT_dep,tRT_dep);
    Serial.printf("profile: %d\n",profile);
    Serial.printf("Battery: %d\n",battery);
    Serial.printf("joyL: %05d,%05d\n",joyLHori,joyLVert);
    Serial.printf("joyR: %05d,%05d\n",joyRHori,joyRVert);
    Serial.printf("trigLT: %04d\n",trigLT);
    Serial.printf("trigRT: %04d\n",trigRT);*/
  }
}