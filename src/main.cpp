#include <WiFi.h>
// #include <WiFiClient.h>
// #include <WiFiClientSecure.h>
#include <WebServer.h>
#include <ArduinoJson.h>
#include <FS.h>
#include <LittleFS.h>
#include <Update.h>
#include <Ticker.h>
#include <esp_wifi.h>
#include <ETH.h>
#include <ESPmDNS.h>
#include <DNSServer.h>
#include <CCTools.h>
#include <WireGuard-ESP32.h>
#include <CronAlarms.h>

// NO SSL SUPPORT in current SDK
// #define ASYNC_TCP_SSL_ENABLED 1

#include "config.h"
#include "web.h"
#include "log.h"
#include "etc.h"
#include "mqtt.h"
#include "zb.h"
#include "version.h"
// #include "const/hw.h"
#include "per.h"
#include "main.h"

#include "esp_system.h"
#include "esp_task_wdt.h"

/*
#ifdef ETH_CLK_MODE
#undef ETH_CLK_MODE
#endif
*/

extern BrdConfigStruct brdConfigs[BOARD_CFG_CNT];

LEDControl ledControl;

ThisConfigStruct hwConfig;

SystemConfigStruct systemCfg;
NetworkConfigStruct networkCfg;
VpnConfigStruct vpnCfg;
MqttConfigStruct mqttCfg;

SysVarsStruct vars;

extern int btnFlag;

bool updWeb = false;

int networkOverseerCounter = 0;

// Ticker tmrNetworkOverseer(handleTmrNetworkOverseer, overseerInterval, 0, MILLIS);
Ticker tmrNetworkOverseer;

IPAddress apIP(192, 168, 1, 1);
DNSServer dnsServer;
WiFiServer server(ZB_TCP_PORT, MAX_SOCKET_CLIENTS);

CCTools CCTool(Serial2);

MDNSResponder mDNS;

void initLan()
{
// ETH.begin(ethConfigs[ethIdx].phyType, ethConfigs[ethIdx].addr, ethConfigs[ethIdx].mdcPin, ethConfigs[ethIdx].mdiPin, ethConfigs[ethIdx].pwrPin, ethConfigs[ethIdx].clkMode)
//
#ifdef TASMOTA_PLATFORM
  if (ETH.begin(hwConfig.eth.phyType, hwConfig.eth.addr, hwConfig.eth.mdcPin, hwConfig.eth.mdiPin, hwConfig.eth.pwrPin, hwConfig.eth.clkMode))
#else
  if (ETH.begin(hwConfig.eth.addr, hwConfig.eth.pwrPin, hwConfig.eth.mdcPin, hwConfig.eth.mdiPin, hwConfig.eth.phyType, hwConfig.eth.clkMode)) // hwConfig.eth.pwrAltPin))
#endif
  {
    String modeString = networkCfg.ethDhcp ? "DHCP" : "Static";
    LOGD("LAN start ok, %s", modeString.c_str());
    // ConfigSettings.disconnectEthTime = millis();
    if (!networkCfg.ethDhcp)
    {
      LOGD("Static IP");
      ETH.config(networkCfg.ethIp, networkCfg.ethGate, networkCfg.ethMask, networkCfg.ethDns1, networkCfg.ethDns2);
    }
    ETH.enableIpV6();
    // ETH.enableIPv6();
    // ETH.printTo(Serial);
  }
  else
  {
    LOGD("LAN start err");
    // esp_eth_stop();
  }
}

void startSocketServer()
{
  server.begin(systemCfg.socketPort);
  server.setNoDelay(true);
}

void startServers(bool usb = false)
{

  if (!vars.apStarted)
  {
    xTaskCreate(setClock, "setClock", 2048, NULL, 9, NULL);
    if (!usb)
    {
      startSocketServer();
    }
    if (vpnCfg.wgEnable)
    {
      wgBegin();
    }
    if (mqttCfg.enable)
    {
      mqttConnectSetup();
    }
  }

  initWebServer();

  startAP(false);

  /*if (!vars.apStarted)
  {
    if (vpnCfg.wgEnable)
    {
      wgBegin();
    }
  }*/

  mDNS_start();
  /* //not available now
  if (vpnCfg.hnEnable)
  {
    hnBegin();
  }
  */
}

void handleTmrNetworkOverseer()
{
  // switch (systemCfg.workMode)
  //{
  // case WORK_MODE_NETWORK:
  networkOverseerCounter++;
  if (!networkCfg.wifiEnable && !networkCfg.ethEnable)
  {
    if (!vars.apStarted)
    {
      LOGD("Both interfaces disabled. Start AP");
      startAP(true);
      connectWifi();
    }
  }
  if (networkCfg.wifiEnable)
  {
    LOGD("WiFi.status = %s", String(WiFi.status()).c_str());

    if (WiFi.isConnected())
    {
      LOGD("WIFI CONNECTED");
      // tmrNetworkOverseer.stop();
      tmrNetworkOverseer.detach();
      if (!vars.firstUpdCheck)
      {
        firstUpdCheck();
      }
    }
    else
    {
      if (!vars.zbFlashing)
      {
        if (networkOverseerCounter > overseerMaxRetry)
        {
          LOGD("WIFI counter overflow");
          startAP(true);
          connectWifi();
        }
      }
    }
  }
  if (networkCfg.ethEnable)
  {
    if (vars.connectedEther)
    {
      LOGD("LAN CONNECTED");
      // tmrNetworkOverseer.stop();
      tmrNetworkOverseer.detach();
      if (vars.apStarted)
      {
        startAP(false);
      }
      if (!vars.firstUpdCheck)
      {
        firstUpdCheck();
      }
    }
    else
    {
      // if (tmrNetworkOverseer.counter() > overseerMaxRetry)
      if (networkOverseerCounter > overseerMaxRetry)
      {
        LOGD("LAN counter overflow!");
        startAP(true);
      }
    }
  }
  // break;
  /*case WORK_MODE_USB:
    if (tmrNetworkOverseer.counter() > 3)
    { // 10 seconds for wifi connect
      if (WiFi.isConnected())
      {
        tmrNetworkOverseer.stop();
        // startServers(true);
      }
      else
      {
        initLan();
        if (tmrNetworkOverseer.counter() > 6)
        { // 3sec for lan
          if (vars.connectedEther)
          {
            tmrNetworkOverseer.stop();
            // startServers(true);
          }
          else
          {                            // no network interfaces
            tmrNetworkOverseer.stop(); // stop timer
            startAP(true);
          }
        }
      }
    }
    break;
  default:
    break;*/
  //}
}

void NetworkEvent(WiFiEvent_t event)
{
  const char *wifiKey = "WiFi";
  const char *ethKey = "ETH";
  // esp_err_t result5;
  switch (event)
  {
  case ARDUINO_EVENT_ETH_START: // 18: // SYSTEM_EVENT_ETH_START:
    LOGD("%s Started", ethKey);
    //  ConfigSettings.disconnectEthTime = millis();
    ETH.setHostname(systemCfg.hostname);
    break;
  case ARDUINO_EVENT_ETH_CONNECTED: // 20: // SYSTEM_EVENT_ETH_CONNECTED:
    LOGD("%s Connected", ethKey);
    break;
  case ARDUINO_EVENT_ETH_GOT_IP: // 22: // SYSTEM_EVENT_ETH_GOT_IP:
    // startServers();
    LOGI("%s MAC: %s, IP: %s, Mask: %s, Gw: %s, DNS: %s, %dMbps", ethKey,
         ETH.macAddress().c_str(),
         ETH.localIP().toString().c_str(),
         ETH.subnetMask().toString().c_str(),
         ETH.gatewayIP().toString().c_str(),
         ETH.dnsIP().toString().c_str(),
         ETH.linkSpeed());

    vars.connectedEther = true;
    // checkDNS(true);
    //  ConfigSettings.disconnectEthTime = 0;
    break;

  case ARDUINO_EVENT_ETH_GOT_IP6:
    LOGI("ETH IPv6 %s", ETH.localIPv6().toString().c_str());
    vars.connectedEther = true;
    vars.ethIPv6 = true;
    break;

  case ARDUINO_EVENT_ETH_DISCONNECTED: // 21:  //SYSTEM_EVENT_ETH_DISCONNECTED:
    LOGD("%s Disconnected", ethKey);
    vars.connectedEther = false;
    vars.ethIPv6 = false;
    // ConfigSettings.disconnectEthTime = millis();
    // if (tmrNetworkOverseer.state() == STOPPED) //&& systemCfg.workMode == WORK_MODE_NETWORK)
    if (!tmrNetworkOverseer.active())
    {
      // tmrNetworkOverseer.start();
      tmrNetworkOverseer.attach(overseerInterval, handleTmrNetworkOverseer);
    }
    break;
  case 27: // case SYSTEM_EVENT_ETH_STOP: // 27:
  case ARDUINO_EVENT_ETH_STOP:
    LOGD("%s Stopped", ethKey);
    vars.connectedEther = false;
    // ConfigSettings.disconnectEthTime = millis();
    // if (tmrNetworkOverseer.state() == STOPPED)
    if (!tmrNetworkOverseer.active())
    {
      // tmrNetworkOverseer.start();
      tmrNetworkOverseer.attach(overseerInterval, handleTmrNetworkOverseer);
    }
    break;
  case ARDUINO_EVENT_WIFI_STA_GOT_IP: // SYSTEM_EVENT_STA_GOT_IP:
    // startServers();
    LOGI("%s MAC: %s, IP: %s, Mask: %s, Gw: %s, DNS: %s", wifiKey,
         WiFi.macAddress().c_str(),
         WiFi.localIP().toString().c_str(),
         WiFi.subnetMask().toString().c_str(),
         WiFi.gatewayIP().toString().c_str(),
         WiFi.dnsIP().toString().c_str());
    // checkDNS(true);
    LOGD("WiFi TX %s", String(WiFi.getTxPower()).c_str());

    /*result5 = esp_wifi_set_protocol(WIFI_IF_STA, WIFI_PROTOCOL_11G | WIFI_PROTOCOL_11N);
    if (result5 == ESP_OK)
    {
      Serial.println("Wi-Fi protocol set successfully.");
    }
    else
    {
      Serial.printf("Error setting Wi-Fi protocol: %d\n", result5);
    }

    uint8_t cur_mode;
    esp_wifi_get_protocol(WIFI_IF_STA, &cur_mode);
    Serial.print("Current Wi-Fi protocol: ");
    if (cur_mode & WIFI_PROTOCOL_11B)
      Serial.print("802.11b ");
    if (cur_mode & WIFI_PROTOCOL_11G)
      Serial.print("802.11g ");
    if (cur_mode & WIFI_PROTOCOL_11N)
      Serial.print("802.11n ");
    Serial.println();*/
    break;
  case ARDUINO_EVENT_WIFI_STA_GOT_IP6: // SYSTEM_EVENT_STA_GOT_IP6:
    LOGI("WiFi IPv6 %s", WiFi.localIPv6().toString().c_str());
    break;
  case ARDUINO_EVENT_WIFI_STA_DISCONNECTED: // SYSTEM_EVENT_STA_DISCONNECTED:
    LOGD("%s STA DISCONNECTED", wifiKey);
    // if (tmrNetworkOverseer.state() == STOPPED)
    if (!tmrNetworkOverseer.active())
    {
      // tmrNetworkOverseer.start();
      tmrNetworkOverseer.attach(overseerInterval, handleTmrNetworkOverseer);
    }
    break;
  default:
    break;
  }
}

void startAP(const bool start)
{
  String tag = "sAP";
  LOGD("begin cmd=%d, state=%d", start, vars.apStarted);

  if (vars.apStarted)
  {
    if (!start)
    {
      if (!networkCfg.wifiEnable)
      {
        WiFi.softAPdisconnect(true); // off wifi
      }
      else
      {
        WiFi.mode(WIFI_STA);
      }
      dnsServer.stop();
      vars.apStarted = false;
    }
  }
  else
  {
    if (!start)
      return;
    LOGD("WIFI_AP_STA");
    WiFi.mode(WIFI_AP_STA); // WIFI_AP_STA for possible wifi scan in wifi mode
    WiFi.disconnect();
    WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
    WiFi.softAP(vars.deviceId); //, WIFIPASS);

    // if DNSServer is started with "*" for domain name, it will reply with
    // provided IP to all DNS request
    dnsServer.setErrorReplyCode(DNSReplyCode::NoError);
    dnsServer.start(53, "*", apIP);
    WiFi.setSleep(false);
    // ConfigSettings.wifiAPenblTime = millis();
    // LOGD("startServers()");
    startServers();
    vars.apStarted = true;
  }
}

void stopWifi()
{
  LOGD("stopWifi");
  WiFi.disconnect();
  WiFi.mode(WIFI_OFF);
}

void connectWifi()
{
  static uint8_t timeout = 0;
  if (WiFi.status() == WL_IDLE_STATUS && timeout < 20)
  { // connection in progress
    LOGD("WL_IDLE_STATUS");
    timeout++;
    return;
  }
  else
  {
    timeout = 0;
    LOGD("timeout");
  }
  WiFi.persistent(false);

  // Dont work on Arduino framework

  /*uint8_t cur_mode;
  esp_wifi_get_protocol(WIFI_IF_STA, &cur_mode);
  Serial.print("wifi mode ");
  String result = "";
  result += String(cur_mode, DEC);
  Serial.println(result);

    esp_wifi_set_protocol(WIFI_IF_STA, WIFI_PROTOCOL_11G | WIFI_PROTOCOL_11N); // networkCfg.wifiMode); // WIFI_PROTOCOL_11B | ); //

  Serial.print("wifi mode setup ");
  esp_err_t result2 = esp_wifi_set_protocol(WIFI_IF_STA, WIFI_PROTOCOL_11G | WIFI_PROTOCOL_11N);
  Serial.println(result2);

  cur_mode = -1;
  esp_wifi_get_protocol(WIFI_IF_STA, &cur_mode);
  Serial.print("wifi mode ");
  result = "";
  result += String(cur_mode, DEC);
  Serial.println(result);*/

  if ((strlen(networkCfg.wifiSsid) >= 2) && (strlen(networkCfg.wifiPass) >= 8))
  {
    LOGD("Ok SSID & PASS");
    if (vars.apStarted)
    {
      LOGD("WiFi.mode(WIFI_AP_STA)");
      WiFi.mode(WIFI_AP_STA);
    }
    else
    {
      WiFi.setHostname(systemCfg.hostname);
      LOGD("WiFi.mode(WIFI_STA)");
      WiFi.mode(WIFI_STA);
    }
    delay(100);

    WiFi.setSleep(false);

    if (!networkCfg.wifiDhcp)
    {
      WiFi.config(networkCfg.wifiIp, networkCfg.wifiGate, networkCfg.wifiMask, networkCfg.wifiDns1, networkCfg.wifiDns2);
      LOGD("WiFi.config");
    }
    else
    {
      WiFi.config(INADDR_NONE, INADDR_NONE, INADDR_NONE, INADDR_NONE);
      LOGD("Try DHCP");
    }
    WiFi.setScanMethod(WIFI_ALL_CHANNEL_SCAN);
    WiFi.setSortMethod(WIFI_CONNECT_AP_BY_SIGNAL);

    WiFi.setAutoReconnect(true);

    // Dont work on Arduino framework
    /*uint8_t wifiProtocols = WIFI_PROTOCOL_LR;

    uint8_t currentProtocols;
    esp_err_t resultGetProt = esp_wifi_get_protocol(WIFI_IF_STA, &currentProtocols);
    if (resultGetProt == ESP_OK)
    {
      Serial.printf("Current WiFi protocols: 0x%X\n", currentProtocols);
    }
    else
    {
      Serial.printf("Failed to get current WiFi protocols: 0x%X\n", resultGetProt);
    }

    // Объединение текущих и желаемых настроек протоколов
    uint8_t newProtocols = wifiProtocols;

    // Установка новых протоколов WiFi перед началом подключения
    esp_err_t resultWifiProtSet = esp_wifi_set_protocol(WIFI_IF_STA, newProtocols);
    if (resultWifiProtSet == ESP_OK)
    {
      Serial.println("WiFi protocols set successfully");
      resultGetProt = esp_wifi_get_protocol(WIFI_IF_STA, &currentProtocols);
      if (resultGetProt == ESP_OK)
      {
        Serial.printf("Current WiFi protocols: 0x%X\n", currentProtocols);
      }
      else
      {
        Serial.printf("Failed to get current WiFi protocols: 0x%X\n", resultGetProt);
      }
    }
    else
    {
      Serial.printf("Failed to set WiFi protocols: 0x%X\n", resultWifiProtSet);
      if (resultWifiProtSet == ESP_ERR_WIFI_NOT_INIT)
      {
        Serial.println("WiFi is not initialized by esp_wifi_init");
      }
      else if (resultWifiProtSet == ESP_ERR_WIFI_IF)
      {
        Serial.println("Invalid interface");
      }
      else if (resultWifiProtSet == ESP_ERR_INVALID_ARG)
      {
        Serial.println("Invalid argument");
      }
      else
      {
        Serial.println("Unknown error");
      }
    }
    */

    WiFi.begin(networkCfg.wifiSsid, networkCfg.wifiPass);
    WiFi.setTxPower(networkCfg.wifiPower);
    WiFi.enableIpV6();
    // LOGD("WiFi TX %s", String(WiFi.getTxPower()).c_str());

    /*esp_err_t result = esp_wifi_set_protocol(WIFI_IF_STA, WIFI_PROTOCOL_11G | WIFI_PROTOCOL_11N);
    if (result == ESP_OK)
    {
      Serial.println("Wi-Fi protocol set successfully.");
    }
    else
    {
      Serial.printf("Error setting Wi-Fi protocol: %d\n", result);
    }*/
    LOGD("WiFi.begin");
  }
  else
  {
    // if (!(systemCfg.workMode == WORK_MODE_USB && systemCfg.keepWeb))
    //{ // dont start ap in keepWeb
    LOGD("NO SSID & PASS ");
    if (!vars.connectedEther)
    {
      LOGD("and problem with LAN");
      startAP(true);
      LOGD("so setupWifiAP");
    }
    else
    {
      LOGD("but LAN is OK");
    }
    // }
  }
}

void mDNS_start()
{
  const char *names[] = {"_xzg", "_zigbee-gateway", "_zigstar_gw"};

  const char *http = "_http";
  const char *tcp = "_tcp";
  if (!mDNS.begin(systemCfg.hostname))
  {
    LOGD("Error setting up mDNS responder!");
  }
  else
  {
    LOGI("mDNS responder started on %s.local", String(systemCfg.hostname).c_str());
    //----- WEB ------
    mDNS.addService(http, tcp, 80);
    //--zeroconf zha--
    for (int i = 0; i < sizeof(names) / sizeof(names[0]); i++)
    {
      mDNS.addService(names[i], tcp, systemCfg.socketPort);
      mDNS.addServiceTxt(names[i], tcp, "version", "1.0");
      mDNS.addServiceTxt(names[i], tcp, "radio_type", "znp");
      mDNS.addServiceTxt(names[i], tcp, "serial_number", String(CCTool.chip.ieee));
      mDNS.addServiceTxt(names[i], tcp, "baud_rate", String(systemCfg.serialSpeed));
      mDNS.addServiceTxt(names[i], tcp, "data_flow_control", "software");
      mDNS.addServiceTxt(names[i], tcp, "board", String(hwConfig.board));
    }
  }
}

void networkStart()
{
  // if ((systemCfg.workMode != WORK_MODE_USB) || systemCfg.keepWeb)
  //{ // start network overseer
  // if (tmrNetworkOverseer.state() == STOPPED)
  if (!tmrNetworkOverseer.active())
  {
    // tmrNetworkOverseer.start();
    tmrNetworkOverseer.attach(overseerInterval, handleTmrNetworkOverseer);
  }
  WiFi.onEvent(NetworkEvent);
  if (networkCfg.ethEnable)
    initLan();
  if (networkCfg.wifiEnable)
    connectWifi();
  //}

  // if (!systemCfg.disableWeb && ((systemCfg.workMode != WORK_MODE_USB) || systemCfg.keepWeb))
  //   updWeb = true; // handle web server
  if (!systemCfg.disableWeb)
    updWeb = true; // handle web server
  // if (systemCfg.workMode == WORK_MODE_USB && systemCfg.keepWeb)
  //   connectWifi(); // try 2 connect wifi
}

void setupCoordinatorMode()
{
  if (systemCfg.workMode > 2 || systemCfg.workMode < 0)
  {
    LOGW("WRONG MODE, set to Network");
    systemCfg.workMode = WORK_MODE_NETWORK;
  }

  String workModeString = systemCfg.workMode ? "USB" : "Network";
  LOGI("%s", workModeString.c_str());

  switch (systemCfg.workMode)
  {
  case WORK_MODE_USB:
    ledControl.modeLED.mode = LED_ON;
    delay(100);
    usbModeSet(ZIGBEE);
    startServers(true);
    break;
  case WORK_MODE_NETWORK:
    ledControl.powerLED.mode = LED_BLINK_1Hz;
    delay(100);
    usbModeSet(XZG);
    startServers();
    break;
  default:
    break;
  }
}

void getEspUpdateTask(void *pvParameters)
{
  TaskParams *params = static_cast<TaskParams *>(pvParameters);
  LOGI("getEspUpdateTask %s", params->url);
  getEspUpdate(params->url);
  vTaskDelete(NULL);
}

void timerCallback(TimerHandle_t xTimer)
{
  TaskParams *params = static_cast<TaskParams *>(pvTimerGetTimerID(xTimer));
  xTaskCreate(getEspUpdateTask, "getEspUpdateTask", 8192, params, 1, NULL);
}

void checkFileSys()
{
  FirmwareInfo fwInfo = fetchLatestEspFw("fs");
  File commitFile = LittleFS.open("/x/commit", "r");
  if (!commitFile)
  {
    LOGI("Commit file not found");
    vars.needFsDownload = true;
  }
  else
  {
    String gitSha = fwInfo.sha.substring(0, 7);
    String fileSha = commitFile.readString().substring(0, 7).c_str();

    LOGI("Commit file found: Git: %s, File: %s", gitSha.c_str(), fileSha.c_str());

    if (gitSha.length() == 7 && gitSha != fileSha)
    {
      LOGI("Found new FS commit");
      vars.needFsDownload = true;
    }
    commitFile.close();
  }

  if (vars.needFsDownload)
  {
    LOGI("Downloading FS");

    static String urlString = fwInfo.url;
    static TaskParams params = {urlString.c_str()};

    TimerHandle_t timer = xTimerCreate("StartTaskTimer", pdMS_TO_TICKS(5000), pdFALSE, &params, timerCallback);
    if (timer != NULL)
    {
      xTimerStart(timer, 0);
    }
  }
}

void setup()
{
  Serial.begin(115200); // todo ifdef DEBUG

  // String tag = "SETUP";

  initNVS();

  getDeviceID(vars.deviceId); // need for mqtt, vpn, mdns, wifi ap and so on

  loadSystemConfig(systemCfg);
  loadNetworkConfig(networkCfg);
  loadVpnConfig(vpnCfg);
  loadMqttConfig(mqttCfg);

  // LOAD System vars and create FS / start
  if (!LittleFS.begin(false, "/lfs2", 10))
  {
    LOGD("Error with FS - try to download");
    vars.needFsDownload = true;
    // return;
  }

  loadHwConfig(hwConfig);
  if (String(hwConfig.board).length() < 4)
  {
    LOGD("No HW config in NVS. Try to load from file");
    if (!loadFileConfigHW())
    {
      int searchId = 0;
      if (hwConfig.board[0] == 'i')
      {
        // Проверка, что второй и третий символы являются цифрами
        if (isdigit(hwConfig.board[1]) && isdigit(hwConfig.board[2]))
        {
          searchId = (hwConfig.board[1] - '0') * 10 + (hwConfig.board[2] - '0');
        }
        else
        {
          LOGD("Invalid board ID format: %s", hwConfig.board);
        }
      }
      LOGD("hwConfig.board: %s", hwConfig.board);
      LOGD("searchId: %d", searchId);
      ThisConfigStruct *newConfig = findBrdConfig(searchId);
      if (newConfig)
      {
        LOGD("Find. Saving config");
        saveHwConfig(*newConfig);

        if (!newConfig->eth.mdcPin == -1 && !newConfig->eth.mdiPin == -1)
        {
          networkCfg.ethEnable = true;
          saveNetworkConfig(networkCfg);
        }

        LOGD("Calc and save temp offset");
        float CPUtemp = getCPUtemp(true);
        int offset = CPUtemp - 30;
        systemCfg.tempOffset = int(offset);
        saveSystemConfig(systemCfg);

        restartDevice();
      }
    }
  }

  if (hwConfig.eth.mdcPin == -1 || hwConfig.eth.mdiPin == -1)
  {
    if (networkCfg.ethEnable)
    {
      networkCfg.ethEnable = false;
      saveNetworkConfig(networkCfg);
    }
  }

  String cfg = makeJsonConfig(&networkCfg, &vpnCfg, &mqttCfg, &systemCfg, NULL, NULL);
  LOGI("\n%s", cfg.c_str());

  vars.apStarted = false;

  networkStart();

  /*while (WiFi.status() != WL_CONNECTED && !vars.connectedEther)
  {
    delay(1000);
    LOGD("Wait for network");
  }*/

  // AVOID USING PIN 0
  if (hwConfig.mist.btnPin > 0)
  {
    buttonInit();
  }

  if (hwConfig.mist.ledModePin > 0)
  {
    ledModeSetup();
  }

  if (hwConfig.mist.ledPwrPin > 0)
  {
    ledPwrSetup();
  }

  if (hwConfig.mist.uartSelPin > 0)
  {
    pinMode(hwConfig.mist.uartSelPin, OUTPUT);
    // vars.hwUartSelIs = true;
    //  usbModeSet(XZG);
    bool fixState = (hwConfig.mist.uartSelPlr == 1) ? LOW : HIGH;
    digitalWrite(hwConfig.mist.uartSelPin, fixState);
  }

  if ((hwConfig.zb.txPin > 0) && (hwConfig.zb.rxPin > 0) && (hwConfig.zb.rstPin > 0) && (hwConfig.zb.bslPin > 0))
  {
    Serial2.begin(systemCfg.serialSpeed, SERIAL_8N1, hwConfig.zb.rxPin, hwConfig.zb.txPin); // start zigbee serial
    int BSL_PIN_MODE = 0;
    if (CCTool.begin(hwConfig.zb.rstPin, hwConfig.zb.bslPin, BSL_PIN_MODE))
    {
      zbHwCheck();
    }
  }

  if (vars.hwBtnIs)
  {
    buttonSetup();
  }

  setLedsDisable(); // with setup ?? // move to vars ?

  vars.connectedClients = 0;

  xTaskCreate(updateWebTask, "update Web Task", 8192, NULL, 8, NULL);

  printNVSFreeSpace();

  if (systemCfg.zbRole == COORDINATOR || systemCfg.zbRole == UNDEFINED)
  {
    /*if (zbFwCheck())
    {
      if (systemCfg.zbRole == UNDEFINED)
      {
        systemCfg.zbRole = COORDINATOR;
        saveSystemConfig(systemCfg);
      }
    }*/
    zbFwCheck();
    LOGI("[RCP] FW: %s", String(CCTool.chip.fwRev).c_str());
  }
  else
  {
    LOGI("[RCP] role: %s", String(systemCfg.zbRole).c_str());
  }
  LOGI("[ESP] FW: %s", String(VERSION).c_str());

  // LOGI("Load cfg %s", hwConfig.board);

  setupCoordinatorMode();

  setup1wire(check1wire());

  cfg = makeJsonConfig(NULL, NULL, NULL, NULL, &vars, NULL);
  LOGI("\n%s", cfg.c_str());

  LOGI("done");
}

WiFiClient client[10];

void socketClientConnected(int client, IPAddress ip)
{
  if (vars.connectedSocket[client] != true)
  {
    printLogMsg("Connected client " + String(client + 1) + " from " + ip.toString().c_str());
    if (vars.connectedClients == 0)
    {
      vars.socketTime = millis();
      mqttPublishIo("socket", 1);
      ledControl.powerLED.mode = LED_ON;
    }
    vars.connectedSocket[client] = true;
    vars.connectedClients++;
  }
}

void socketClientDisconnected(int client)
{
  if (vars.connectedSocket[client] != false)
  {
    LOGD("Disconnected client %d", client);
    vars.connectedSocket[client] = false;
    vars.connectedClients--;
    if (vars.connectedClients == 0)
    {
      vars.socketTime = millis();
      mqttPublishIo("socket", 0);
      ledControl.powerLED.mode = LED_BLINK_1Hz;
    }
  }
}

void printRecvSocket(size_t bytes_read, uint8_t net_buf[BUFFER_SIZE])
{
  char output_sprintf[3];
  if (bytes_read > 0)
  {
    String tmpTime;
    String buff = "";
    unsigned long timeLog = millis();
    tmpTime = String(timeLog, DEC);
    logPush('[');
    for (int j = 0; j < tmpTime.length(); j++)
    {
      logPush(tmpTime[j]);
    }
    logPush(']');
    logPush(' ');
    logPush('-');
    logPush('>');

    for (int i = 0; i < bytes_read; i++)
    {
      sprintf(output_sprintf, "%02x", net_buf[i]);
      logPush(' ');
      logPush(output_sprintf[0]);
      logPush(output_sprintf[1]);
    }
    logPush('\n');
  }
}

void printSendSocket(size_t bytes_read, uint8_t serial_buf[BUFFER_SIZE])
{
  char output_sprintf[3];
  String tmpTime;
  String buff = "";
  unsigned long timeLog = millis();
  tmpTime = String(timeLog, DEC);
  logPush('[');
  for (int j = 0; j < tmpTime.length(); j++)
  {
    logPush(tmpTime[j]);
  }
  logPush(']');
  logPush(' ');
  logPush('<');
  logPush('-');
  for (int i = 0; i < bytes_read; i++)
  {
    // if (serial_buf[i] == 0x01)
    //{
    // }
    sprintf(output_sprintf, "%02x", serial_buf[i]);
    logPush(' ');
    logPush(output_sprintf[0]);
    logPush(output_sprintf[1]);
    // if (serial_buf[i] == 0x03)
    // {

    //}
  }
  logPush('\n');
}

void loop(void)
{
  if (btnFlag && vars.hwBtnIs)
  {
    buttonLoop();
  }

  // tmrNetworkOverseer.update();
  if (updWeb)
  {
    webServerHandleClient();
  }
  else
  {
    if (vars.connectedClients == 0)
    {
      webServerHandleClient();
    }
  }

  if (!vars.zbFlashing)
  {

    if (systemCfg.workMode == WORK_MODE_USB)
    {
      if (Serial2.available())
      {
        Serial.write(Serial2.read());
        Serial.flush();
      }
      if (Serial.available())
      {
        Serial2.write(Serial.read());
        Serial2.flush();
      }
      return;
    }

    else if (systemCfg.workMode == WORK_MODE_NETWORK)
    {
      uint16_t net_bytes_read = 0;
      uint8_t net_buf[BUFFER_SIZE];
      uint16_t serial_bytes_read = 0;
      uint8_t serial_buf[BUFFER_SIZE];

      if (server.hasClient())
      {
        for (byte i = 0; i < MAX_SOCKET_CLIENTS; i++)
        {
          if (!client[i] || !client[i].connected())
          {
            if (client[i])
            {
              client[i].stop();
            }
            if (systemCfg.fwEnabled)
            {
              if (server.hasClient())
              {
                WiFiClient TempClient2 = server.available();
                IPAddress clientIp = TempClient2.remoteIP();

                if (isValidIp(clientIp) && isIpInSubnet(clientIp, systemCfg.fwIp, systemCfg.fwMask))
                {
                  printLogMsg(String("[SOCK IP FW] Accepted from IP: ") + clientIp.toString());
                  client[i] = TempClient2;
                  // TempClient2.stop();
                  continue;
                }
                else
                {
                  printLogMsg(String("[SOCK IP FW] Rejected from IP: ") + clientIp.toString());
                  // TempClient2.stop();
                }
              }
            }
            else
            {
              client[i] = server.available();
              continue;
            }
          }
        }
        WiFiClient TempClient = server.available();
        TempClient.stop();
      }

      for (byte cln = 0; cln < MAX_SOCKET_CLIENTS; cln++)
      {
        if (client[cln])
        {
          socketClientConnected(cln, client[cln].remoteIP());
          while (client[cln].available())
          { // read from LAN
            net_buf[net_bytes_read] = client[cln].read();
            if (net_bytes_read < BUFFER_SIZE - 1)
              net_bytes_read++;
          } // send to Zigbee
          Serial2.write(net_buf, net_bytes_read);
          // print to web console
          printRecvSocket(net_bytes_read, net_buf);
          net_bytes_read = 0;
        }
        else
        {
          socketClientDisconnected(cln);
        }
      }

      if (Serial2.available())
      {
        while (Serial2.available())
        { // read from Zigbee
          serial_buf[serial_bytes_read] = Serial2.read();
          if (serial_bytes_read < BUFFER_SIZE - 1)
            serial_bytes_read++;
        }
        // send to LAN
        for (byte cln = 0; cln < MAX_SOCKET_CLIENTS; cln++)
        {
          if (client[cln])
            client[cln].write(serial_buf, serial_bytes_read);
        }
        // print to web console
        printSendSocket(serial_bytes_read, serial_buf);
        serial_bytes_read = 0;
      }

      /*if (mqttCfg.enable)
      {
        // mqttLoop();
      }*/
    }
    if (vpnCfg.wgEnable && vars.vpnWgInit)
    {
      wgLoop();
    }

    if (WiFi.getMode() == WIFI_MODE_AP || WiFi.getMode() == WIFI_MODE_APSTA)
    {
      dnsServer.processNextRequest();
    }
    Cron.delay();
  }
}
