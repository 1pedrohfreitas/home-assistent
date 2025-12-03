#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <EEPROM.h>
#include <ArduinoJson.h>
#include <WebSocketsClient.h>

#define EEPROM_SIZE 128

// GPIOs dos relés
#define OUTPUT_1 16
#define OUTPUT_2 5
#define OUTPUT_3 4
#define OUTPUT_4 14

ESP8266WebServer server(80);
WebSocketsClient ws;

String ssidSalvo = "";
String senhaSalva = "";


String wsHostSalvo = "";
int wsPortSalvo = 0;
String userId = "";

unsigned long tempoInicial;
bool jaConectadoSTA = false;

// ====================================================================
// SALVAR / CARREGAR EEPROM
// ====================================================================
void salvarCredenciais(String ssid, String senha, String wsHost, int wsPort) {

  // SSID
  EEPROM.write(0, ssid.length());
  for (int i = 0; i < ssid.length(); i++) EEPROM.write(1 + i, ssid[i]);

  // SENHA
  EEPROM.write(33, senha.length());
  for (int i = 0; i < senha.length(); i++) EEPROM.write(34 + i, senha[i]);

  // WS HOST
  EEPROM.write(64, wsHost.length());
  for (int i = 0; i < wsHost.length(); i++) EEPROM.write(65 + i, wsHost[i]);

  // WS PORT (2 bytes)
  EEPROM.write(100, (wsPort >> 8) & 0xFF);
  EEPROM.write(101, wsPort & 0xFF);

  EEPROM.commit();
}

void carregarCredenciais() {

  // SSID
  int ssidLen = EEPROM.read(0);
  ssidSalvo = "";
  for (int i = 0; i < ssidLen; i++) ssidSalvo += char(EEPROM.read(1 + i));

  // SENHA
  int senhaLen = EEPROM.read(33);
  senhaSalva = "";
  for (int i = 0; i < senhaLen; i++) senhaSalva += char(EEPROM.read(34 + i));

  // WS HOST
  int wsHostLen = EEPROM.read(64);
  wsHostSalvo = "";
  for (int i = 0; i < wsHostLen; i++) wsHostSalvo += char(EEPROM.read(65 + i));

  // WS PORT
  wsPortSalvo = (EEPROM.read(100) << 8) | EEPROM.read(101);
}

// ====================================================================
// ACCESS POINT
// ====================================================================
void iniciarAP() {
  WiFi.mode(WIFI_AP);
  WiFi.softAP("WEMOS_CONFIG", "12345678");
}

// ====================================================================
// WEB SERVER HTTP
// ====================================================================
void iniciarServidor() {
  server.on("/", HTTP_GET, []() {
    String html =
      "<html><body>"
      "<h2>Configurar WiFi - "+userId+"</h2>"
      "<form method='POST' action='/save'>"
      "SSID:<br><input name='ssid'><br>"
      "Senha:<br><input name='pass' type='password'><br>"
      "<br><h3>WebSocket</h3>"
      "Host:<br><input name='wshost'><br>"
      "Porta:<br><input name='wsport' type='number'><br><br>"
      "<button type='submit'>Salvar</button>"
      "</form>"
      "</body></html>";

    server.send(200, "text/html", html);
  });

  server.on("/save", HTTP_POST, []() {

    String s1 = server.arg("ssid");
    String s2 = server.arg("pass");
    String wsh = server.arg("wshost");
    int wsp = server.arg("wsport").toInt();

    salvarCredenciais(s1, s2, wsh, wsp);

    server.send(200, "text/html", "Salvo! Reinicie o dispositivo.");
  });

  server.on("/sysinfo", HTTP_GET, []() {
    StaticJsonDocument<64> doc;
    doc["type"] = "Wemos D1 + 4 Relés";
    String msg;
    serializeJson(doc, msg);
    server.send(200, "application/json", msg);
  });

  server.on("/rele", HTTP_GET, handleReleControl);

  server.begin();
}

// ====================================================================
// CONTROLE DOS RELÉS
// ====================================================================
void handleReleControl() {

  if (!server.hasArg("gpio") || !server.hasArg("state")) {
    server.send(400, "text/plain", "Missing gpio/state");
    return;
  }

  int gpio = server.arg("gpio").toInt();
  int req = server.arg("state").toInt();

  // relé ativo LOW
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
// WEBSOCKET
// ====================================================================
void webSocketEvent(WStype_t type, uint8_t* payload, size_t length) {

  switch (type) {

    case WStype_CONNECTED:
      Serial.println("WS conectado!");
      ws.sendTXT("device_connected");
      break;

    case WStype_DISCONNECTED:
      Serial.println("WS desconectado.");
      break;

    case WStype_TEXT:
      {
        Serial.printf("Mensagem WS: %s\n", payload);

        StaticJsonDocument<128> doc;
        DeserializationError err = deserializeJson(doc, payload);

        if (err) {
          ws.sendTXT("{\"error\":\"json_invalido\"}");
          return;
        }

        if (!doc.containsKey("gpio") || !doc.containsKey("state")) {
          ws.sendTXT("{\"error\":\"faltando_gpio_ou_state\"}");
          return;
        }

        int gpio = doc["gpio"];
        int req = doc["state"];

        // active LOW
        int state = req ? LOW : HIGH;

        switch (gpio) {
          case 1: digitalWrite(OUTPUT_1, state); break;
          case 2: digitalWrite(OUTPUT_2, state); break;
          case 3: digitalWrite(OUTPUT_3, state); break;
          case 4: digitalWrite(OUTPUT_4, state); break;
          default:
            ws.sendTXT("{\"error\":\"gpio_invalido\"}");
            return;
        }

        // resposta confirmando execução
        StaticJsonDocument<128> resp;
        resp["gpio"] = gpio;
        resp["state"] = req;
        resp["status"] = "ok";

        String out;
        serializeJson(resp, out);
        ws.sendTXT(out);

        break;
      }
  }
}

void conectarWS() {

  if (wsHostSalvo.length() == 0 || wsPortSalvo == 0) {
    Serial.println("WS não configurado.");
    return;
  }

  String path = "/ws?userId=" + userId;

  Serial.print("Conectando WSS em: ");
  Serial.print(wsHostSalvo);
  Serial.print(":");
  Serial.println(wsPortSalvo);

 ws.beginSSL(wsHostSalvo.c_str(), wsPortSalvo, path.c_str());

  ws.onEvent(webSocketEvent);
  ws.setReconnectInterval(5000);
}

// ====================================================================
// SETUP
// ====================================================================
void setup() {

  Serial.begin(115200);
  EEPROM.begin(EEPROM_SIZE);

  carregarCredenciais();

  pinMode(OUTPUT_1, OUTPUT);
  pinMode(OUTPUT_2, OUTPUT);
  pinMode(OUTPUT_3, OUTPUT);
  pinMode(OUTPUT_4, OUTPUT);

  digitalWrite(OUTPUT_1, HIGH);
  digitalWrite(OUTPUT_2, HIGH);
  digitalWrite(OUTPUT_3, HIGH);
  digitalWrite(OUTPUT_4, HIGH);

  iniciarAP();
        userId = getDeviceId();
      Serial.println("DeviceID:");
      Serial.println(userId);
  iniciarServidor();

  tempoInicial = millis();
}

// ====================================================================
// LOOP PRINCIPAL
// ====================================================================
void loop() {

  server.handleClient();
  ws.loop();  // sempre rodar

  // só executa tentativa uma vez
  if (!jaConectadoSTA && millis() - tempoInicial > 60000) {

    if (ssidSalvo.length() > 0) {
      Serial.println("Trocando para STA...");
      WiFi.mode(WIFI_STA);
      delay(100);

      userId = getDeviceId();
      Serial.println("DeviceID:");
      Serial.println(userId);
      WiFi.begin(ssidSalvo.c_str(), senhaSalva.c_str());

      unsigned long t = millis();
      while (millis() - t < 10000) {
        if (WiFi.status() == WL_CONNECTED) {
          Serial.println("Conectado ao WiFi!");
          Serial.println(WiFi.localIP());

          jaConectadoSTA = true;
          conectarWS();
          break;
        }
        delay(500);
      }
    }

    if (!jaConectadoSTA) {
      Serial.println("Falhou STA, mantendo AP.");
    }
  }
}
String getDeviceId() {
  String mac = WiFi.macAddress();
  mac.replace(":", "");  // tirar os dois-pontos
  return mac;
}
