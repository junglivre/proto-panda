#pragma once
#include "config.hpp"
#ifdef ENABLE_BLE
#include <NimBLEDevice.h>
#include "Arduino.h"
#include "config.hpp"
#include <map>
#include <vector>
#include <stack>
#include <cstring> 
#include "bluetooth/servicehandler.hpp"

class BleManager;

class AdvertisedDeviceCallbacks: public NimBLEScanCallbacks {
  void onResult(const NimBLEAdvertisedDevice* advertisedDevice) override;
  //void onDiscovered(const NimBLEAdvertisedDevice* advertisedDevice) override;
  void onScanEnd(const NimBLEScanResults& scanResults, int reason) override ;
  
  public:
    BleManager *bleObj;
};


class BleManager{
  public:
    BleManager():clientCount(0), maxClients(2),lastScanClearTime(0),m_scanStartAt(0),m_started(false),m_canScan(false),m_pauseScan(false),m_clearAndRestart(false),m_logDiscoveredClients(false),isScanning(false),nextId(0),m_scanningByAddress(false),m_mutex(xSemaphoreCreateMutex()){}
    bool begin();
    bool beginRadio(int powerLevel=ESP_PWR_LVL_P9);
    void update();
    void sendUpdatesToLua();
    void beginScanning();

    
    void setMaximumControls(int n){maxClients = n;};

    int getConnectedClientsCount(){
      return handlers.size();
    }

    bool isElementIdConnected(int id);

    bool hasChangedClients();
    void setLogDiscoveredClients(bool log){
      m_logDiscoveredClients = log;
    }

    void SetScanModeByAddress(bool mode){
      m_scanningByAddress = mode;
    }
    bool IsScanningByAddress(){
      return m_scanningByAddress;
    }

    bool canLogDiscoveredClients(){
      return m_logDiscoveredClients;
    }

    void requestClearResults(){
      lastScanClearTime = millis() + 30 * 1000;
    }


    void AddAcceptedService(std::string name, BleServiceHandler* obj);

    PSRAMMap<std::string, BleServiceHandler*> &GetPairedDevices(){
      return pairedHandlers;
    }

    void AddPairedDeviceAddress(std::string addr, BleServiceHandler* obj);
    PSRAMMap<std::string, BleServiceHandler*> &GetAcceptedServices(){
      return handlers;
    }

    void setScanningMode(bool scan);

    BluetoothDeviceHandler* getDeviceById(int conn);

    int GetClientIdFromControllerId(uint32_t id);
    int GetRSSI(int clientId);
    bool IsStarted(){
      return m_started;
    };

    static BleManager* Get();
  private:

    bool TryConnectByService(const NimBLEAdvertisedDevice* advertisedDevice);
    bool TryConnectByAddress(const NimBLEAdvertisedDevice* advertisedDevice);
    void RequestConnection(const NimBLEAdvertisedDevice* advertisedDevice, BleServiceHandler* handler);

    PSRAMMap<std::string, BleServiceHandler*> pairedHandlers; //Handlers are stored by their UUID
    PSRAMMap<std::string, BleServiceHandler*> handlers; //Handlers are stored by their UUID
    PSRAMMap<std::string, BluetoothDeviceHandler*> clients; //Clients are stored by their address

    bool connectToServer();
    uint16_t clientCount;
  
    uint32_t  maxClients, lastScanClearTime, m_scanStartAt;
    bool m_started, m_canScan, m_pauseScan, m_clearAndRestart, m_logDiscoveredClients, isScanning;
    std::stack<uint8_t> availableIds;
    uint8_t nextId;
    bool m_scanningByAddress;


    SemaphoreHandle_t m_mutex;
    static BleManager *m_myself;

    ConnectionRequest toConnect;

    friend class AdvertisedDeviceCallbacks;
    friend class ClientCallbacks;
};

extern BleManager g_remoteControls;

#endif