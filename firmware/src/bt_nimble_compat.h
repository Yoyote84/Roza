#pragma once
// Adaptateur ROZA original vers NimBLE-Arduino 2.5.2 (licence amont Apache-2.0).
// Les écrans et fonctions publiques ROZA restent exposés par bt.cpp.
#include <NimBLEDevice.h>
#ifdef USING_NIMBLE_ARDUINO_HEADERS
#include "nimble/nimble/host/include/host/ble_hs.h"
#include "nimble/nimble/host/include/host/ble_sm.h"
#include "NimBLEConnInfo.h"
#else
#include "host/ble_hs.h"
#include "host/ble_sm.h"
#include "NimBLEConnInfo.h"
#endif
#include <NimBLEScan.h>
#include <NimBLEAdvertisedDevice.h>
#include <NimBLEServer.h>
#include <NimBLEService.h>
#include <NimBLECharacteristic.h>
#include <NimBLEHIDDevice.h>
#include <NimBLEExtAdvertising.h>
#include <NimBLEAddress.h>
#include <NimBLEUUID.h>
#include <string>
#include <cstddef>
#include <new>
#ifdef BLEDevice
#undef BLEDevice
#endif
#ifdef BLEServer
#undef BLEServer
#endif
#ifdef BLEServerCallbacks
#undef BLEServerCallbacks
#endif
#ifdef BLECharacteristic
#undef BLECharacteristic
#endif
#ifdef BLEAdvertisedDevice
#undef BLEAdvertisedDevice
#endif
#ifdef BLEAdvertisedDeviceCallbacks
#undef BLEAdvertisedDeviceCallbacks
#endif
#ifdef BLEScan
#undef BLEScan
#endif
#ifdef BLEAddress
#undef BLEAddress
#endif
#ifdef BLEAdvertising
#undef BLEAdvertising
#endif

class RozaBleAdvertisedDevice {
 public:
  explicit RozaBleAdvertisedDevice(const NimBLEAdvertisedDevice* device) : device_(device) {}
  NimBLEAddress getAddress() const { return device_->getAddress(); }
  uint8_t getAddressType() const { return device_->getAddressType(); }
  std::string getName() const { return device_->getName(); }
  int8_t getRSSI() const { return device_->getRSSI(); }
  int8_t getTXPower() const { return device_->getTXPower(); }
  uint16_t getAppearance() const { return device_->getAppearance(); }
  bool haveName() const { return device_->haveName(); }
  bool haveRSSI() const { return device_->getRSSI() > -127; }
  bool haveTXPower() const { return device_->haveTXPower(); }
  bool haveAppearance() const { return device_->haveAppearance(); }
  bool haveServiceUUID() const { return device_->haveServiceUUID(); }
  bool haveManufacturerData() const { return device_->haveManufacturerData(); }
  bool haveServiceData() const { return device_->haveServiceData(); }
  NimBLEUUID getServiceUUID() const { return device_->getServiceUUID(0); }
  NimBLEUUID getServiceDataUUID() const { return device_->getServiceDataUUID(0); }
  std::string getManufacturerData() const { return device_->getManufacturerData(0); }
  std::string getServiceData() const { return device_->getServiceData(0); }
 private:
  const NimBLEAdvertisedDevice* device_;
};

class RozaBleAdvertisedDeviceCallbacks {
 public:
  virtual ~RozaBleAdvertisedDeviceCallbacks() = default;
  virtual void onResult(RozaBleAdvertisedDevice device) = 0;
};

class RozaBleScan {
 private:
  class CallbackBridge final : public NimBLEScanCallbacks {
   public:
    void set(RozaBleAdvertisedDeviceCallbacks* callback) { callback_ = callback; }
    void onResult(const NimBLEAdvertisedDevice* device) override {
      if (callback_ && device) callback_->onResult(RozaBleAdvertisedDevice(device));
    }
   private:
    RozaBleAdvertisedDeviceCallbacks* callback_ = nullptr;
  };
  CallbackBridge bridge_;
  NimBLEScan* scan() const { return NimBLEDevice::getScan(); }
 public:
  void setAdvertisedDeviceCallbacks(RozaBleAdvertisedDeviceCallbacks* callbacks, bool wantDuplicates) {
    bridge_.set(callbacks);
    scan()->setScanCallbacks(&bridge_, wantDuplicates);
    scan()->setPhy(NimBLEScan::SCAN_ALL);
  }
  void setActiveScan(bool active) { scan()->setActiveScan(active); }
  void setInterval(uint16_t intervalMs) { scan()->setInterval(intervalMs); }
  void setWindow(uint16_t windowMs) { scan()->setWindow(windowMs); }
  bool start(uint32_t seconds, bool isContinue = false) {
    scan()->getResults(seconds * 1000UL, isContinue);
    return true;
  }
  bool start(uint32_t seconds, std::nullptr_t, bool isContinue = false) {
    return scan()->start(seconds * 1000UL, isContinue, true);
  }
  bool stop() { return scan()->stop(); }
  void clearResults() { scan()->clearResults(); }
};

class RozaBleAdvertising {
 public:
  RozaBleAdvertising()
      : advertisement_(BLE_HCI_LE_PHY_1M, BLE_HCI_LE_PHY_1M),
        scanResponse_(BLE_HCI_LE_PHY_1M, BLE_HCI_LE_PHY_1M) {}
  void clearData() {
    advertisement_.clearData();
    scanResponse_.clearData();
    configured_ = false;
    scanResponseEnabled_ = false;
  }
  void invalidate() { configured_ = false; }
  bool setAppearance(uint16_t appearance) {
    configured_ = false;
    return advertisement_.setAppearance(appearance);
  }
  bool addServiceUUID(const NimBLEUUID& uuid) {
    configured_ = false;
    return advertisement_.addServiceUUID(uuid);
  }
  bool setScanResponse(bool enabled) {
    scanResponseEnabled_ = enabled;
    advertisement_.setLegacyAdvertising(true);
    advertisement_.setConnectable(true);
    advertisement_.setScannable(enabled);
    if (enabled) scanResponse_.setName("Cardputer ADV", true);
    configured_ = false;
    return true;
  }
  bool start() {
    NimBLEExtAdvertising* advertising = NimBLEDevice::getAdvertising();
    if (!advertising) return false;
    if (!configured_) {
      advertisement_.setLegacyAdvertising(true);
      advertisement_.setConnectable(true);
      advertisement_.setScannable(scanResponseEnabled_);
      if (!advertisement_.setFlags(0x02)) return false;
      if (!advertising->setInstanceData(0, advertisement_)) return false;
      if (scanResponseEnabled_ && !advertising->setScanResponseData(0, scanResponse_)) return false;
      configured_ = true;
    }
    return advertising->start(0);
  }
  bool stop() {
    NimBLEExtAdvertising* advertising = NimBLEDevice::getAdvertising();
    return advertising && advertising->stop(0);
  }
 private:
  NimBLEExtAdvertisement advertisement_;
  NimBLEExtAdvertisement scanResponse_;
  bool configured_ = false;
  bool scanResponseEnabled_ = false;
};

class RozaBleHidDevice {
 public:
  explicit RozaBleHidDevice(NimBLEServer* server)
      : server_(server), hid_(server ? new (std::nothrow) NimBLEHIDDevice(server) : nullptr) {}
  ~RozaBleHidDevice() { delete hid_; }
  bool isValid() const {
    return server_ && hid_ && hid_->getDeviceInfoService() &&
           hid_->getHidService() && hid_->getBatteryService();
  }
  NimBLECharacteristic* inputReport(uint8_t reportId) {
    return isValid() ? hid_->getInputReport(reportId) : nullptr;
  }
  bool setManufacturer(const std::string& name) { return isValid() && hid_->setManufacturer(name); }
  void pnp(uint8_t sig, uint16_t vid, uint16_t pid, uint16_t version) { hid_->setPnp(sig, vid, pid, version); }
  void hidInfo(uint8_t country, uint8_t flags) { hid_->setHidInfo(country, flags); }
  void reportMap(uint8_t* map, uint16_t length) { hid_->setReportMap(map, length); }
  bool startServices() { return isValid() && server_->start(); }
  void setBatteryLevel(uint8_t level) { if (isValid()) hid_->setBatteryLevel(level); }
  NimBLEService* hidService() { return isValid() ? hid_->getHidService() : nullptr; }
 private:
  NimBLEServer* server_;
  NimBLEHIDDevice* hid_;
};

class RozaBleDevice {
 public:
  static bool init(const char* name) { return NimBLEDevice::init(std::string(name)); }
  static bool deinit(bool clearAll = false) { return NimBLEDevice::deinit(clearAll); }
  static NimBLEServer* createServer() { return NimBLEDevice::createServer(); }
  static RozaBleScan* getScan() { static RozaBleScan scan; return &scan; }
  static RozaBleAdvertising* getAdvertising() { static RozaBleAdvertising advertising; return &advertising; }
  static bool startAdvertising() { return getAdvertising()->start(); }
  static bool stopAdvertising() { return getAdvertising()->stop(); }
};
