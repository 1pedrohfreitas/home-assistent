#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <EEPROM.h>
#include <ArduinoJson.h>
#include <ArduinoWebsockets.h>

using namespace websockets;

#define EEPROM_SIZE 128

// GPIOs dos relés
#define OUTPUT_1 16
#define OUTPUT_2 5
#define OUTPUT_3 4
#define OUTPUT_4 14

ESP8266WebServer server(80);
WebsocketsClient ws;     // <<< AGORA FUNCIONA

String ssidSalvo = "";
String senhaSalva = "";
String userId = "";
String deviceId = "";

unsigned long tempoInicial;
bool jaConectadoSTA = false;

// ====================================================================
// DEVICE ID
// ====================================================================
String getDeviceId() {
  String mac = WiFi.macAddress();
  mac.replace(":", "");
  return mac;
}

// ====================================================================
// EEPROM
// ====================================================================
void salvarCredenciais(String ssid, String senha, String uid, String did) {

  EEPROM.write(0, ssid.length());
  for (int i = 0; i < ssid.length(); i++) EEPROM.write(1 + i, ssid[i]);

  EEPROM.write(33, senha.length());
  for (int i = 0; i < senha.length(); i++) EEPROM.write(34 + i, senha[i]);

  EEPROM.write(100, uid.length());
  for (int i = 0; i < uid.length(); i++) EEPROM.write(101 + i, uid[i]);

  EEPROM.write(115, did.length());
  for (int i = 0; i < did.length(); i++) EEPROM.write(116 + i, did[i]);

  EEPROM.commit();
}

void carregarCredenciais() {

  int ssidLen = EEPROM.read(0);
  ssidSalvo = "";
  for (int i = 0; i < ssidLen; i++) ssidSalvo += char(EEPROM.read(1 + i));

  int senhaLen = EEPROM.read(33);
  senhaSalva = "";
  for (int i = 0; i < senhaLen; i++) senhaSalva += char(EEPROM.read(34 + i));

  int uidLen = EEPROM.read(100);
  userId = "";
  for (int i = 0; i < uidLen; i++) userId += char(EEPROM.read(101 + i));

  int didLen = EEPROM.read(115);
  deviceId = "";
  for (int i = 0; i < didLen; i++) deviceId += char(EEPROM.read(116 + i));
}

// ====================================================================
// RELÉS
// ====================================================================
void handleReleControl() {

  if (!server.hasArg("gpio") || !server.hasArg("state")) {
    server.send(400, "text/plain", "Missing gpio/state");
    return;
  }

  int gpio = server.arg("gpio").toInt();
  int req = server.arg("state").toInt();
  int state = req ? LOW : HIGH;

  switch (gpio) {
    case 1: digitalWrite(OUTPUT_1, state); break;
    case 2: digitalWrite(OUTPUT_2, state); break;
    case 3: digitalWrite(OUTPUT_3, state); break;
    case 4: digitalWrite(OUTPUT_4, state); break;
  }

  server.send(200, "text/plain", "OK");
}

// ====================================================================
// WEBSOCKET EVENTS
// ====================================================================
void onWsMessage(WebsocketsMessage msg) {
  Serial.printf("WS MSG: %s\n", msg.data().c_str());

  StaticJsonDocument<128> doc;
  if (deserializeJson(doc, msg.data())) {
    ws.send("{\"error\":\"json_invalido\"}");
    return;
  }

  if (!doc.containsKey("gpio") || !doc.containsKey("state")) {
    ws.send("{\"error\":\"faltando_gpio_ou_state\"}");
    return;
  }

  int gpio = doc["gpio"];
  int req = doc["state"];
  int state = req ? LOW : HIGH;

  switch (gpio) {
    case 1: digitalWrite(OUTPUT_1, state); break;
    case 2: digitalWrite(OUTPUT_2, state); break;
    case 3: digitalWrite(OUTPUT_3, state); break;
    case 4: digitalWrite(OUTPUT_4, state); break;
  }

  StaticJsonDocument<128> resp;
  resp["gpio"] = gpio;
  resp["state"] = req;
  resp["status"] = "ok";

  String out;
  serializeJson(resp, out);
  ws.send(out);
}

void onWsEvent(WebsocketsEvent evt, String data) {
  if (evt == WebsocketsEvent::ConnectionOpened) {
    Serial.println("WS conectado!");
    ws.send("device_connected");
  }

  if (evt == WebsocketsEvent::ConnectionClosed) {
    Serial.println("WS desconectado!");
  }
}

// ====================================================================
// WEBSOCKET CONNECT
// ====================================================================
void conectarWS() {

  if (userId.length() == 0 || deviceId.length() == 0) {
    Serial.println("UserID/DeviceID inválidos");
    return;
  }

  String url = "wss://api.my-devices.online/ws?userId=" + userId +
               "&deviceId=" + deviceId;

  ws.setInsecure();
  ws.onMessage(onWsMessage);
  ws.onEvent(onWsEvent);

  Serial.println("Conectando WS: " + url);
  ws.connect(url);
}

// ====================================================================
// AP + WEB SERVER
// ====================================================================
void iniciarAP() {
  WiFi.mode(WIFI_AP);
  WiFi.softAP("WEMOS_CONFIG", "12345678");
}

void iniciarServidor() {

  server.on("/", HTTP_GET, []() {

    String html =
      "<html><body><h1>Configurações</h1>"
      "<form action='/save' method='post'>"
      "SSID:<input name='ssid' value='" + ssidSalvo + "'><br>"
      "Senha:<input name='pass' value='" + senhaSalva + "'><br>"
      "UserID:<input name='userid' value='" + userId + "'><br>"
      "DeviceID:<input disabled value='" + deviceId + "'><br>"
      "<button>Salvar</button>"
      "</form></body></html>";

    server.send(200, "text/html", html);
  });

  server.on("/save", HTTP_POST, []() {
    salvarCredenciais(server.arg("ssid"),
                      server.arg("pass"),
                      server.arg("userid"),
                      deviceId);

    server.send(200, "text/html", "<h2>OK. Reinicie.</h2>");
  });

  server.on("/rele", HTTP_GET, handleReleControl);

  server.begin();
}

// ====================================================================
// SETUP
// ====================================================================
void setup() {

  Serial.begin(115200);
  EEPROM.begin(EEPROM_SIZE);

  carregarCredenciais();
  deviceId = getDeviceId();

  pinMode(OUTPUT_1, OUTPUT);
  pinMode(OUTPUT_2, OUTPUT);
  pinMode(OUTPUT_3, OUTPUT);
  pinMode(OUTPUT_4, OUTPUT);

  digitalWrite(OUTPUT_1, HIGH);
  digitalWrite(OUTPUT_2, HIGH);
  digitalWrite(OUTPUT_3, HIGH);
  digitalWrite(OUTPUT_4, HIGH);

  iniciarAP();
  iniciarServidor();

  tempoInicial = millis();
}

// ====================================================================
// LOOP
// ====================================================================
void loop() {

  server.handleClient();
  ws.poll();

  if (!jaConectadoSTA && millis() - tempoInicial > 60000) {

    if (ssidSalvo.length() > 0) {
      WiFi.mode(WIFI_STA);
      WiFi.begin(ssidSalvo.c_str(), senhaSalva.c_str());

      unsigned long t = millis();
      while (millis() - t < 10000) {
        if (WiFi.status() == WL_CONNECTED) {
          jaConectadoSTA = true;
          conectarWS();
          break;
        }
        delay(500);
      }
    }
  }
}
