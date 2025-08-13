// Reference: https://github.com/h2zero/NimBLE-Arduino/blob/master/examples/NimBLE_Client/NimBLE_Client.ino

#include <NimBLEDevice.h>
#include <Wire.h>

bool input_flag = false;
bool btnA, btnB, btnX, btnY;
bool btnShare, btnStart, btnSelect, btnXbox;
bool btnLB, btnRB, btnLS, btnRS;
bool btnDirUp, btnDirLeft, btnDirRight, btnDirDown;
bool btnL1_elt, btnL2_elt, btnR1_elt, btnR2_elt;
uint16_t joyLHori = 32768;
uint16_t joyLVert = 32768;
uint16_t joyRHori = 32768;
uint16_t joyRVert = 32768;
uint16_t trigLT, trigRT;
uint8_t profile;

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

static const NimBLEAdvertisedDevice* advDevice;
static NimBLEClient* pConnectedClient = nullptr;

static const uint16_t controllerAppearance = 964;
static const String controllerManufacturerDataNormal = "060000";
static const String controllerManufacturerDataSearching = "0600030080";

enum class ConnectionState : uint8_t {
  Connected = 0,
  WaitingForFirstNotification = 1,
  Found = 2,
  Scanning = 3,
};

ConnectionState connectionState = ConnectionState::Scanning;
uint32_t msScanTime = 4000; /** 0 = scan forever */
uint8_t retryCountInOneConnection = 3;
unsigned long retryIntervalMs = 100;
NimBLEClient* pClient = nullptr;

class ClientCallbacks : public NimBLEClientCallbacks {
 public:
  ConnectionState* pConnectionState;
  ClientCallbacks(ConnectionState* pConnectionState){
    this->pConnectionState = pConnectionState;
  }
  void onConnect(NimBLEClient* pClient) {
    *pConnectionState = ConnectionState::WaitingForFirstNotification;
    // pClient->updateConnParams(120,120,0,60);
  }
  void onDisconnect(NimBLEClient* pClient, int reason) {
    *pConnectionState = ConnectionState::Scanning;
    pConnectedClient = nullptr;
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
};

class ScanCallbacks : public NimBLEScanCallbacks {
 public:
  ScanCallbacks(String strTargetDeviceAddress,
                ConnectionState* pConnectionState) {
    if (strTargetDeviceAddress != "") {
      this->targetDeviceAddress =
          new NimBLEAddress(strTargetDeviceAddress.c_str(), 0);
    }
    this->pConnectionState = pConnectionState;
  }

 private:
  NimBLEAddress* targetDeviceAddress = nullptr;
  ConnectionState* pConnectionState;
  void onResult(const NimBLEAdvertisedDevice* advertisedDevice) override {
    auto pHex = NimBLEUtils::dataToHexString(
      (uint8_t*)advertisedDevice->getManufacturerData().data(),
      advertisedDevice->getManufacturerData().length());
    if ((targetDeviceAddress != nullptr &&
         advertisedDevice->getAddress().equals(*targetDeviceAddress)) ||
        (targetDeviceAddress == nullptr &&
         advertisedDevice->getAppearance() == controllerAppearance &&
         (strcmp(pHex.c_str(), controllerManufacturerDataNormal.c_str()) == 0 ||
          strcmp(pHex.c_str(), controllerManufacturerDataSearching.c_str()) == 0) &&
         advertisedDevice->getServiceUUID().equals(uuidServiceHid))){
            *pConnectionState = ConnectionState::Found;
            advDevice = advertisedDevice;
    }
  }
};

ScanCallbacks* scanCBs = new ScanCallbacks("98:7a:14:40:27:b3", &connectionState);
ClientCallbacks* clientCBs = new ClientCallbacks(&connectionState);
uint8_t battery = 0;
static const int deviceAddressLen = 6;
uint8_t deviceAddressArr[deviceAddressLen];

void writeHIDReport(uint8_t* dataArr, size_t dataLen) {
  if (pConnectedClient == nullptr) {
    return;
  }
  NimBLEClient* pClient = pConnectedClient;
  auto pService = pClient->getService(uuidServiceHid);
  for (auto pChara : pService->getCharacteristics()) {
    if (pChara->canWrite()) {
      pChara->writeValue(dataArr, dataLen, false);
    }
  }
}
void startScan() {
  connectionState = ConnectionState::Scanning;
  auto pScan = NimBLEDevice::getScan();
  pScan->setDuplicateFilter(false);
  pScan->setScanCallbacks(scanCBs);
  pScan->setInterval(97);
  pScan->setWindow(97);
  pScan->start(msScanTime);
}
bool isConnected() {
  return connectionState == ConnectionState::WaitingForFirstNotification ||connectionState == ConnectionState::Connected;
}

void notifyCB(NimBLERemoteCharacteristic* pRemoteCharacteristic,
              uint8_t* pData, size_t length, bool isNotify) {
  auto sUuid = pRemoteCharacteristic->getRemoteService()->getUUID();
  connectionState = ConnectionState::Connected;
  if (sUuid.equals(uuidServiceHid)) {
    joyLHori = ((uint16_t)pData[1] << 8) | pData[0];
    joyLVert = ((uint16_t)pData[3] << 8) | pData[2];
    joyRHori = ((uint16_t)pData[5] << 8) | pData[4];
    joyRVert = ((uint16_t)pData[7] << 8) | pData[6];
    trigLT = ((uint16_t)pData[9] << 8) | pData[8];
    trigRT = ((uint16_t)pData[11] << 8) | pData[10];
    btnDirUp = (pData[12] == 1 || pData[12] == 2 || pData[12] == 8);
    btnDirRight = (2 <= pData[12]) && (pData[12] <= 4);
    btnDirDown = (4 <= pData[12]) && (pData[12] <= 6);
    btnDirLeft = (6 <= pData[12]) && (pData[12] <= 8);
    btnA      = pData[13] & 0b00000001;
    btnB      = pData[13] & 0b00000010;
    btnX      = pData[13] & 0b00001000;
    btnY      = pData[13] & 0b00010000;
    btnLB     = pData[13] & 0b01000000;
    btnRB     = pData[13] & 0b10000000;
    btnSelect = pData[14] & 0b00000100;
    btnStart  = pData[14] & 0b00001000;
    btnXbox   = pData[14] & 0b00010000;
    btnLS     = pData[14] & 0b00100000;
    btnRS     = pData[14] & 0b01000000;
    btnShare  = pData[15] & 0b00000001;
    profile   = pData[16];
    btnR1_elt = pData[18] & 0b00000001;
    btnR2_elt = pData[18] & 0b00000010;
    btnL1_elt = pData[18] & 0b00000100;
    btnL2_elt = pData[18] & 0b00001000;
    input_flag = true;
  } else {
    if (sUuid.equals(uuidServiceBattery)) {
      battery = pData[0];
    } else {
    }
  }
}

bool afterConnect(NimBLEClient* pClient) {
  memcpy(deviceAddressArr, pClient->getPeerAddress().getBase(),deviceAddressLen);
  for (auto pService : pClient->getServices(true)) {
    auto sUuid = pService->getUUID();
    if (!sUuid.equals(uuidServiceHid) && !sUuid.equals(uuidServiceBattery)) {
      continue;  // skip
    }
    for (auto pChara : pService->getCharacteristics(true)) {
      if (pChara->canRead()) {// Reading value is required for subscribe
        auto str = pChara->readValue();
        if (str.size() == 0) {
          str = pChara->readValue();
        }
      }
      if (pChara->canNotify()) {
        if (pChara->subscribe(true,std::bind(&notifyCB, std::placeholders::_1,std::placeholders::_2, std::placeholders::_3,std::placeholders::_4),true)) {}
      }
    }
  }
  return true;
}

bool connectToServer(const NimBLEAdvertisedDevice* advDevice) {
  NimBLEClient* pClient = nullptr;

  /** Check if we have a client we should reuse first **/
  if (NimBLEDevice::getCreatedClientCount()) {
    pClient = NimBLEDevice::getClientByPeerAddress(advDevice->getAddress());
    if (pClient) {
      pClient->connect();
    }
  }
  if (!pClient) {
    if (NimBLEDevice::getCreatedClientCount() >= NIMBLE_MAX_CONNECTIONS) {
      return false;
    }
    pClient = NimBLEDevice::createClient();
    pClient->setClientCallbacks(clientCBs, true);
    pClient->connect(advDevice, true);
  }
  int retryCount = retryCountInOneConnection;
  while (!pClient->isConnected()) {
    if (retryCount <= 0) {
      return false;
    }
    delay(retryIntervalMs);
    // Serial.println(pClient->toString().c_str());
    pClient->connect(true);
    --retryCount;
  }
  bool result = afterConnect(pClient);
  if (!result) {
    return result;
  }
  pConnectedClient = pClient;
  return true;
}

void PCA9865_init(){
  uint8_t buf[5];
  // Soft reset of the I2C chip
  //Block write via the all leds register to turn off all servo and motor outputs
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
  Serial.write(packet.bytes,10);
}

void setup() {

  profile = 3; 
  Serial.begin(115200);
  Serial2.begin(115200, SERIAL_8N1,3,2);

  NimBLEDevice::init("");
  NimBLEDevice::setOwnAddrType(BLE_OWN_ADDR_PUBLIC);
  NimBLEDevice::setSecurityAuth(true, false, false);
  NimBLEDevice::setPower(9); /* +9db */

}

void loop() {
  if (!isConnected()) {
    if (advDevice != nullptr) {
      auto connectionResult = connectToServer(advDevice);
      if (!connectionResult || !isConnected()) {
        NimBLEDevice::deleteBond(advDevice->getAddress());
        // reset();
        connectionState = ConnectionState::Scanning;
      } else {
        input_flag=true;
      }
      advDevice = nullptr;
    } else if (!NimBLEDevice::getScan()->isScanning()) {
      // reset();
      startScan();
    }
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
  
  if(input_flag){

    input_flag = false;

    char buf1[10];
    char buf2[10];
    char buf3[10];
    char buf4[10];
    char buf5[8];
    char buf6[8];
    sprintf(buf1, "%05d", joyLHori);
    sprintf(buf2, "%05d", joyLVert);
    sprintf(buf3, "%05d", joyRHori);
    sprintf(buf4, "%05d", joyRVert);
    sprintf(buf5, "%04d", trigLT);
    sprintf(buf6, "%04d", trigRT);
    /*Serial.print("Y: " + String(btnY) + " " +
      "X: " + String(btnX) + " " +
      "B: " + String(btnB) + " " +
      "A: " + String(btnA) + " " +
      "LB: " + String(btnLB) + " " +
      "RB: " + String(btnRB) + "\n" +
      "Select: " + String(btnSelect) + " " +
      "Start: " + String(btnStart) + " " +
      "Xbox: " + String(btnXbox) + " " +
      "Share: " + String(btnShare) + " " +
      "LS: " + String(btnLS) + " " +
      "RS: " + String(btnRS) + "\n" +
      "L1_elt: " + String(btnL1_elt) + " " +
      "L2_elt: " + String(btnL2_elt) + " " +
      "R1_elt: " + String(btnR1_elt) + " " +
      "R2_elt: " + String(btnR2_elt) + "\n" +
      "Up: " + String(btnDirUp) + " " +
      "Right: " + String(btnDirRight) + " " +
      "Down: " + String(btnDirDown) + " " +
      "Left: " + String(btnDirLeft) + "\n"
      "profile: " + String(profile) + "\n" +
      "joyLHori: " + buf1 + "\n" +
      "joyLVert: " + buf2 + "\n" +
      "joyRHori: " + buf3 + "\n" +
      "joyRVert: " + buf4 + "\n" +
      "trigLT: " + buf5 + "\n" +
      "trigRT: " + buf6 + "\n" +
      "battery: " + String(battery) + "\n");*/
  }
}
