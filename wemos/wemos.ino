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
String userId = "";
String deviceId = "";

unsigned long tempoInicial;
bool jaConectadoSTA = false;

// ====================================================================
// SALVAR / CARREGAR EEPROM
// ====================================================================
void salvarCredenciais(String ssid, String senha, String wsHost, String userId, String deviceId) {

  // SSID
  EEPROM.write(0, ssid.length());
  for (int i = 0; i < ssid.length(); i++) EEPROM.write(1 + i, ssid[i]);

  // SENHA
  EEPROM.write(33, senha.length());
  for (int i = 0; i < senha.length(); i++) EEPROM.write(34 + i, senha[i]);

  // HOST
  EEPROM.write(64, wsHost.length());
  for (int i = 0; i < wsHost.length(); i++) EEPROM.write(65 + i, wsHost[i]);

  // USER ID
  EEPROM.write(100, userId.length());
  for (int i = 0; i < userId.length(); i++) EEPROM.write(101 + i, userId[i]);

  // DEVICE ID
  EEPROM.write(115, deviceId.length());
  for (int i = 0; i < deviceId.length(); i++) EEPROM.write(116 + i, deviceId[i]);

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

  // HOST
  int wsHostLen = EEPROM.read(64);
  wsHostSalvo = "";
  for (int i = 0; i < wsHostLen; i++) wsHostSalvo += char(EEPROM.read(65 + i));

  // USER ID
  int uidLen = EEPROM.read(100);
  userId = "";
  for (int i = 0; i < uidLen; i++) userId += char(EEPROM.read(101 + i));

  // DEVICE ID
  int didLen = EEPROM.read(115);
  deviceId = "";
  for (int i = 0; i < didLen; i++) deviceId += char(EEPROM.read(116 + i));
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
      "<!DOCTYPE html>"
      "<html lang='pt-BR'>"
      "<head>"
      "<meta charset='utf-8' />"
      "<meta name='viewport' content='width=device-width,initial-scale=1' />"
      "<title>Configurações — Home Assistant</title>"
      "<style>"
      ":root{--bg:#0f1724;--card:#0b1220;--muted:#9aa4b2;--accent:#7c5cff;--accent-2:#00d4ff;--glass:rgba(255,255,255,0.04);--radius:14px;--gap:16px;--shadow:0 6px 30px rgba(2,6,23,0.6);--success:#22c55e;}"
      "*{box-sizing:border-box}"
      "html,body{height:100%}"
      "body{margin:0;font-family:Inter,sans-serif;background:var(--bg);color:#e6eef8;display:flex;align-items:center;justify-content:center;padding:32px;}"
      ".card{width:100%;max-width:920px;background:var(--glass);border-radius:var(--radius);padding:28px;box-shadow:var(--shadow);border:1px solid rgba(255,255,255,0.05);display:grid;grid-template-columns:1fr;gap:24px;}"
      "h1{margin:0;font-size:22px;}"
      "label{font-size:13px;color:var(--muted);margin-bottom:6px;display:block}"
      "input{width:100%;padding:10px 12px;border-radius:10px;border:1px solid rgba(255,255,255,0.04);background:transparent;color:inherit;font-size:14px;}"
      "fieldset{border-radius:12px;border:1px solid rgba(255,255,255,0.05);padding:16px;background:var(--glass);}"
      "legend{padding:0 8px;font-weight:600;color:var(--accent);font-size:13px;}"
      "button{background:linear-gradient(90deg,var(--accent),var(--accent-2));color:#051225;border:none;padding:12px 16px;border-radius:10px;font-weight:700;cursor:pointer;width:100%;}"
      "</style>"
      "</head>"
      "<body>"
      "<main class='card'>"
      "<h1>Configurar Dispositivo</h1>"
      "<form action='/save' method='post'>"

      "<fieldset>"
      "<legend>Conexão Wireless</legend>"
      "<label>SSID</label>"
      "<input type='text' name='ssid' value='"
      + ssidSalvo + "'>"
                    "<label>Senha</label>"
                    "<input type='password' name='pass' value='"
      + senhaSalva + "'>"
                     "</fieldset>"

                     "<fieldset>"
                     "<legend>Conexão Servidor</legend>"
                     "<label>Host + Porta + Path</label>"
                     "<input type='text' name='wshost' value='"
      + wsHostSalvo + "'>"
                      "</fieldset>"

                      "<fieldset>"
                      "<legend>Identificação</legend>"
                      "<label>User ID</label>"
                      "<input type='text' name='userid' value='"
      + userId + "'>"
                 "<label>Device ID</label>"
                 "<input type='text' name='deviceid' disabled value='"
      + deviceId + "'>"
                   "</fieldset>"

                   "<button type='submit'>Salvar Configurações</button>"
                   "</form>"
                   "</main>"
                   "</body>"
                   "</html>";

    server.send(200, "text/html", html);
  });



  server.on("/save", HTTP_POST, []() {
    String s1 = server.arg("ssid");
    String s2 = server.arg("pass");
    String wsh = server.arg("wshost");
    String uid = server.arg("userid");
    String did = server.arg("deviceid");

    if (s1 != ssidSalvo || s2 != senhaSalva || wsh != wsHostSalvo || uid != userId || did != deviceId) {

      salvarCredenciais(s1, s2, wsh, uid, did);
    }

    server.send(200, "text/html",
                "<h2>Configurações Salvas!</h2><p>Reinicie o dispositivo.</p>");
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

  if (wsHostSalvo.length() == 0) {
    Serial.println("WS não configurado.");
    return;
  }

  String url = wsHostSalvo;

  bool secure = url.startsWith("wss://");
  url.replace("wss://", "");
  url.replace("ws://", "");

  int idxPort = url.indexOf(':');
  int idxPath = url.indexOf('/');

  if (idxPort < 0) {
    Serial.println("URL inválida: falta porta");
    return;
  }

  String host = url.substring(0, idxPort);
  String portStr;
  String path;

  // ======================================
  // Caso NÃO tenha path → usar "/"
  // ======================================
  if (idxPath == -1) {
    portStr = url.substring(idxPort + 1);
    path = "/";
  } else {
    portStr = url.substring(idxPort + 1, idxPath);
    path = url.substring(idxPath); 
  }

  int port = portStr.toInt();

  // Adiciona userId e deviceId no final do path
  if (path.indexOf('?') == -1) {
    path += "ws?userId=" + userId + "&deviceId=" + deviceId;
  } else {
    path += "&userId=" + userId + "&deviceId=" + deviceId;
  }

  Serial.println("HOST: " + host);
  Serial.println("PORT: " + String(port));
  Serial.println("PATH: " + path);

  ws.begin(host.c_str(), port, path.c_str(), secure ? "wss" : "ws");
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
  deviceId = getDeviceId();
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
      WiFi.begin(ssidSalvo.c_str(), senhaSalva.c_str());

      unsigned long t = millis();
      while (millis() - t < 10000) {
        if (WiFi.status() == WL_CONNECTED) {
          Serial.println("Conectado ao WiFi!");
          Serial.println(WiFi.localIP());
          deviceId = getDeviceId();
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
