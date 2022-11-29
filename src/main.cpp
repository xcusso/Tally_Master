/*
 TALLY MASTER

 Firmware per gestionar un sistema de Tally's inhalambrics.

 */
/*
TODO

Poder selecionar el WIFI LOCAL
WEb display per veure tally operatius, bateries i funcions
Fer menu selecció funció local
Implentar Display
Implentar hora
Implentar mostrar texte
Lectura valors reals bateria

*/
/*
  Basat en:

  Rui Santos
  Complete project details at https://RandomNerdTutorials.com/?s=esp-now
  Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files.
  The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
  Based on JC Servaye example: https://github.com/Servayejc/esp_now_web_server/
*/
#include <Arduino.h>
#include <esp_now.h>
#include <WiFi.h>
#include "ESPAsyncWebServer.h"
#include "AsyncTCP.h"
#include <ArduinoJson.h>
#include <Adafruit_NeoPixel.h> //Control neopixels

#define VERSIO M1 // Versió del software

// Bool per veure missatges de debug
bool debug = true;

// Set your Board and Server ID
#define BOARD_ID 0 // Cal definir cada placa amb el seu numero

// TODO FALTEN DEFINES X PINS GIPO
// Define PINS
// Botons i leds locals
#define BOTO_ROIG_PIN 16
#define BOTO_VERD_PIN 5
#define LED_ROIG_PIN 17
#define LED_VERD_PIN 18
#define MATRIX_PIN 4

// Define Quantitat de leds
#define LED_COUNT 72 // 8x8 + 8

// Define sensor battery
#define BATTERY_PIN 36

// Definim PINS 

// ******** EQUIP A
// VIA 
// I Input O Output
// Només son 4 i no hi ha tensió
// TODO: FAREM SERVIR PLACA i2C PER AMPLIAR GPIO - CAL REDEFINIR
const uint8_t GPIA_PIN[4] = {2, 15, 19, 0}; // El bit 1 encen el led local del ESp32 - Atenció 0 Pullup!!!
const uint8_t GPOA_PIN[1] = {23};           // No tenim tants bits - Un sol bit de confirmacio
// El bit de confirmació pot obrir canal de la taula

// ******** EQUIP B
// YAMAHA QL 
// I Input O Output
// 5 Ins 5 Outs
// TODO: FAREM SERVIR PLACA i2C PER AMPLIAR GPIO - CAL REDEFINIR
uint8_t const GPIB_PIN[6] = {39, 34, 35, 32, 33, 25};
//uint8_t const GPOB_PIN[5] = {26, 27, 14, 12, 13};
uint8_t const GPOB_PIN[5] = {26, 27, 17, 16, 13}; //El 12 donava problemes al fer boot, el 14 treu PWM


// Declarem neopixels
Adafruit_NeoPixel llum(LED_COUNT, MATRIX_PIN, NEO_GRB + NEO_KHZ800);

// Definim els colors GRB
const uint8_t COLOR[][6] = {{0, 0, 0},        // 0- NEGRE
                            {255, 0, 0},      // 1- ROIG
                            {0, 0, 255},      // 2- BLAU
                            {255, 0, 0},      // 3- VERD
                            {255, 128, 0},    // 4- GROC
                            {128, 128, 0},    // 5- TARONJA
                            {255, 255, 255}}; // 6- BLANC

uint8_t color_matrix = 0; // Per determinar color local

// Variables
// Fem arrays de dos valors la 0 és anterior la 1 actual
bool BOTO_LOCAL_ROIG[] = {false, false};
bool BOTO_LOCAL_VERD[] = {false, false};

// Valor dels leds (dels polsadors)
bool LED_LOCAL_ROIG = false;
bool LED_LOCAL_VERD = false;
uint16_t BATTERY_LOCAL_READ[] = {0, 0};

// Variables de gestió
bool LOCAL_CHANGE = false; // Per saber si alguna cosa local ha canviat

unsigned long temps_set_config = 0;      // Temps que ha d'estar apretat per configuracio
const unsigned long temps_config = 5000; // Temps per disparar opció config
bool pre_mode_configuracio = false;      // Inici mode configuració
bool mode_configuracio = false;          // Mode configuració

// Replace with your network credentials (STATION)
const char *ssid = "exteriors";
const char *password = "exteriors";
// TODO: Poder seleccionar via WEB la Wifi on connectar.

esp_now_peer_info_t slave;
int chan;

enum MessageType
{
  PAIRING,
  DATA,
  TALLY,
  BATERIA,
  CLOCK
};
MessageType messageType;

// Definim les funcions del Tally
enum TipusFuncio
{
  LLUM,
  CONDUCTOR,
  PRODUCTOR
};
TipusFuncio funcio_local = LLUM;

int counter = 0;

// Structure example to receive data
// Must match the sender structure
// TODO ELIMINAR
typedef struct struct_message
{
  uint8_t msgType;
  uint8_t id;
  float temp;
  float hum;
  unsigned int readingId;
} struct_message;
// ELIMINAR FINS AQUI

// Estructura pairing
typedef struct struct_pairing
{ // new structure for pairing
  uint8_t msgType;
  uint8_t id;
  uint8_t macAddr[6];
  uint8_t channel;
} struct_pairing;

// Estrucrtura dades per enviar a slaves
typedef struct struct_message_to_slave
{
  uint8_t msgType;
  uint8_t funcio;      // Identificador de la funcio del tally
  bool led_roig;       // llum confirmació cond polsador vermell
  bool led_verd;       // llum confirmació cond polsador verd
  uint8_t color_tally; // Color indexat del tally
  // text per mostrar a pantalla
} struct_message_to_slave;

// Estrucrtura dades rebuda de slaves
typedef struct struct_message_from_slave
{
  uint8_t msgType;
  uint8_t id;     // Identificador del tally
  uint8_t funcio; // Identificador de la funcio del tally
  bool boto_roig;
  bool boto_verd;
} struct_message_from_slave;

// Estructura dades per rebre bateries
typedef struct struct_bateria_info
{
  uint8_t msgType;
  uint8_t id;             // Identificador del tally
  float volts;            // Lectura en volts
  float percent;          // Percentatge carrega
  unsigned int readingId; // Identificador de lectura
} struct__bateria_info;

// Estructura dades per rebre clock
// TODO

struct_message incomingReadings;
struct_message outgoingSetpoints;
struct_pairing pairingData;
struct_message_from_slave fromSlave; // dades del master cap al tally
struct_message_to_slave toSlave;     // dades del tally cap al master

AsyncWebServer server(80);
AsyncEventSource events("/events");

// TODO: Adaptar web als valors dels Tallys
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <title>ESP-NOW DASHBOARD</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <link rel="stylesheet" href="https://use.fontawesome.com/releases/v5.7.2/css/all.css" integrity="sha384-fnmOCqbTlWIlj8LyTjo7mOUStjsKC4pOpQbqyi7RrhN7udi9RwhKkMHpvLbHG9Sr" crossorigin="anonymous">
  <link rel="icon" href="data:,">
  <style>
    html {font-family: Arial; display: inline-block; text-align: center;}
    p {  font-size: 1.2rem;}
    body {  margin: 0;}
    .topnav { overflow: hidden; background-color: #2f4468; color: white; font-size: 1.7rem; }
    .content { padding: 20px; }
    .card { background-color: white; box-shadow: 2px 2px 12px 1px rgba(140,140,140,.5); }
    .cards { max-width: 700px; margin: 0 auto; display: grid; grid-gap: 2rem; grid-template-columns: repeat(auto-fit, minmax(300px, 1fr)); }
    .reading { font-size: 2.8rem; }
    .packet { color: #bebebe; }
    .card.temperature { color: #fd7e14; }
    .card.humidity { color: #1b78e2; }
  </style>
</head>
<body>
  <div class="topnav">
    <h3>ESP-NOW DASHBOARD</h3>
  </div>
  <div class="content">
    <div class="cards">
      <div class="card temperature">
        <h4><i class="fas fa-thermometer-half"></i> BOARD #1 - TEMPERATURE</h4><p><span class="reading"><span id="t1"></span> &deg;C</span></p><p class="packet">Reading ID: <span id="rt1"></span></p>
      </div>
      <div class="card humidity">
        <h4><i class="fas fa-tint"></i> BOARD #1 - HUMIDITY</h4><p><span class="reading"><span id="h1"></span> &percnt;</span></p><p class="packet">Reading ID: <span id="rh1"></span></p>
      </div>
      <div class="card temperature">
        <h4><i class="fas fa-thermometer-half"></i> BOARD #2 - TEMPERATURE</h4><p><span class="reading"><span id="t2"></span> &deg;C</span></p><p class="packet">Reading ID: <span id="rt2"></span></p>
      </div>
      <div class="card humidity">
        <h4><i class="fas fa-tint"></i> BOARD #2 - HUMIDITY</h4><p><span class="reading"><span id="h2"></span> &percnt;</span></p><p class="packet">Reading ID: <span id="rh2"></span></p>
      </div>
    </div>
  </div>
<script>
if (!!window.EventSource) {
 var source = new EventSource('/events');
 
 source.addEventListener('open', function(e) {
  console.log("Events Connected");
 }, false);
 source.addEventListener('error', function(e) {
  if (e.target.readyState != EventSource.OPEN) {
    console.log("Events Disconnected");
  }
 }, false);
 
 source.addEventListener('message', function(e) {
  console.log("message", e.data);
 }, false);
 
 source.addEventListener('new_readings', function(e) {
  console.log("new_readings", e.data);
  var obj = JSON.parse(e.data);
  document.getElementById("t"+obj.id).innerHTML = obj.temperature.toFixed(2);
  document.getElementById("h"+obj.id).innerHTML = obj.humidity.toFixed(2);
  document.getElementById("rt"+obj.id).innerHTML = obj.readingId;
  document.getElementById("rh"+obj.id).innerHTML = obj.readingId;
 }, false);
}
</script>
</body>
</html>)rawliteral";

void readDataToSend()
{
  outgoingSetpoints.msgType = DATA;
  outgoingSetpoints.id = 0; // Servidor te la id 0, els esclaus la 1,2,3..
  outgoingSetpoints.temp = random(0, 40);
  outgoingSetpoints.hum = random(0, 100);
  outgoingSetpoints.readingId = counter++; // Cada vegada que enviem dades incrementem el contador
}

// Simulem lectura de bateria
// TODO Unificar lectura volts i convertir a nivells
float readBateriaVolts()
{
  volt = random(0, 40); // = analogRead(BATTERY_PIN)
  return volt;
}

// Simulem lectura percentatge bateria
float readBateriaPercent()
{
  percent = random(0, 100);
  return percent;
}

// Posar llum a un color
void escriure_matrix(uint8_t color)
{
  // GBR
  uint8_t G = COLOR[color][1];
  uint8_t B = COLOR[color][2];
  uint8_t R = COLOR[color][0];
  for (int i = 0; i < LED_COUNT; i++)
  {
    llum.setPixelColor(i, llum.Color(G, B, R));
  }
  llum.show();
  if (debug)
  {
    Serial.print("Color: ");
    Serial.println(color);
    Serial.print("R: ");
    Serial.println(R);
    Serial.print("G: ");
    Serial.println(G);
    Serial.print("B: ");
    Serial.println(B);
  }
}

void llegir_botons()
{
  BOTO_LOCAL_ROIG[1] = !digitalRead(BOTO_ROIG_PIN); // Els botons son PULLUP per tant els llegirem al revés
  BOTO_LOCAL_VERD[1] = !digitalRead(BOTO_VERD_PIN);
  // Detecció canvi de botons locals
  if (BOTO_LOCAL_ROIG[0] != BOTO_LOCAL_ROIG[1])
  {
    /// HEM POLSAT EL BOTO ROIG
    LOCAL_CHANGE = true;
    BOTO_LOCAL_ROIG[0] = BOTO_LOCAL_ROIG[1];
    if (debug)
    {
      Serial.print("Boto local ROIG: ");
      Serial.println(BOTO_LOCAL_ROIG[0]);
    }
  }

  if (BOTO_LOCAL_VERD[0] != BOTO_LOCAL_VERD[1])
  {
    /// HEM POLSAT EL BOTO VERD
    LOCAL_CHANGE = true;
    BOTO_LOCAL_VERD[0] = BOTO_LOCAL_VERD[1];
    if (debug)
    {
      Serial.print("Boto local VERD: ");
      Serial.println(BOTO_LOCAL_VERD[0]);
    }
  }
}

void escriure_leds()
{
  digitalWrite(LED_ROIG_PIN, LED_LOCAL_ROIG);
  digitalWrite(LED_VERD_PIN, LED_LOCAL_VERD);
}

// ---------------------------- esp_ now -------------------------
void printMAC(const uint8_t *mac_addr)
{
  char macStr[18];
  snprintf(macStr, sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x",
           mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
  Serial.print(macStr);
}

bool addPeer(const uint8_t *peer_addr)
{ // add pairing Funció per afgir Peers
  memset(&slave, 0, sizeof(slave));
  const esp_now_peer_info_t *peer = &slave;
  memcpy(slave.peer_addr, peer_addr, 6);

  slave.channel = chan; // pick a channel
  slave.encrypt = 0;    // no encryption
  // check if the peer exists
  bool exists = esp_now_is_peer_exist(slave.peer_addr);
  if (exists)
  {
    // Slave already paired.
    Serial.println("Already Paired");
    return true;
  }
  else
  {
    esp_err_t addStatus = esp_now_add_peer(peer);
    if (addStatus == ESP_OK)
    {
      // Pair success
      Serial.println("Pair success");
      return true;
    }
    else
    {
      Serial.println("Pair failed");
      return false;
    }
  }
}

// callback when data is sent
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status)
{
  Serial.print("Last Packet Send Status: ");
  Serial.print(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success to " : "Delivery Fail to ");
  printMAC(mac_addr);
  Serial.println();
}

void OnDataRecv(const uint8_t *mac_addr, const uint8_t *incomingData, int len)
{
  Serial.print(len);
  Serial.print(" bytes of data received from : ");
  printMAC(mac_addr);
  Serial.println();
  StaticJsonDocument<1000> root;
  String payload;
  uint8_t type = incomingData[0]; // first message byte is the type of message
  if (!mode_configuracio)         // Si no estem en mode configuració
  {
    switch (type)
    {
    case DATA: // the message is data type
      memcpy(&incomingReadings, incomingData, sizeof(incomingReadings));
      // create a JSON document with received data and send it by event to the web page
      root["id"] = incomingReadings.id;
      root["temperature"] = incomingReadings.temp;
      root["humidity"] = incomingReadings.hum;
      root["readingId"] = String(incomingReadings.readingId);
      serializeJson(root, payload);
      Serial.print("event send :");
      serializeJson(root, Serial);
      events.send(payload.c_str(), "new_readings", millis());
      Serial.println();
      break;

    case TALLY: // Missatge del SLAVE TALLY
      memcpy(&fromSlave, incomingData, sizeof(fromSlave));
      if (debug)
      {
        Serial.print("Tally ID = ");
        Serial.println(fromSlave.id)
            Serial.print("Funció  = ");
        Serial.println(fromSlave.funcio);
        Serial.print("Led roig = ");
        Serial.println(fromSlave.boto_roig);
        Serial.print("Led verd= ");
        Serial.println(fromSlave.boto_verd);
      }
      // PROCESSAR DADES REBUDES
      break;

    case BATERIA: // Missatge del SLAVE TALLY
      memcpy(&fromSlave, incomingData, sizeof(fromSlave));
      if (debug)
      {
        /*
        Imprimir valors també a web
        Serial.print("Tally ID = ");
        Serial.println(fromSlave.id)
            Serial.print("Funció  = ");
        Serial.println(fromSlave.funcio);
        Serial.print("Led roig = ");
        Serial.println(fromSlave.boto_roig);
        Serial.print("Led verd= ");
        Serial.println(fromSlave.boto_verd);
        */
      }
      // PROCESSAR DADES REBUDES
      break;

    case PAIRING: // the message is a pairing request
      memcpy(&pairingData, incomingData, sizeof(pairingData));
      Serial.println(pairingData.msgType);
      Serial.println(pairingData.id);
      Serial.print("Pairing request from: ");
      printMAC(mac_addr);
      Serial.println();
      Serial.println(pairingData.channel);
      if (pairingData.id > 0)
      { // do not replay to server itself (No es respon a ell mateix)
        if (pairingData.msgType == PAIRING)
        {
          pairingData.id = 0; // 0 is server
          // Server is in AP_STA mode: peers need to send data to server soft AP MAC address
          WiFi.softAPmacAddress(pairingData.macAddr);
          pairingData.channel = chan;
          Serial.println("send response");
          esp_err_t result = esp_now_send(mac_addr, (uint8_t *)&pairingData, sizeof(pairingData)); // Respon al emissor
          addPeer(mac_addr);
        }
      }
      break;
    }
  }
}

void Menu_configuracio()
{
  // Desenvolupar aqui el mode configuració
  // TODO:  Seleccionar entre LLUM, CONDUCTOR i PRODUCTOR
  // Veure com generem menu
  /*
  Si opcio 1
    funcio_local = LLUM;
  Si opció 2
    funcio_local = CONDUCTOR;
  Si opcio 3
    funcio_local = PRODUCTOR;
  */
  // Comunicar nova funció
}

void detectar_mode_configuracio()
{
  if (LOCAL_CHANGE)
  {
    if (BOTO_LOCAL_ROIG[0] && BOTO_LOCAL_VERD[0] && !pre_mode_configuracio)
    {
      // Tenim els dos polsadors apretats i no estem en pre_mode_configuracio
      // Entrarem al mode CONFIG
      temps_set_config = millis();  // Llegim el temps actual per entrar a mode config
      pre_mode_configuracio = true; // Situem el flag en pre-mode-confi
      if (debug)
      {
        Serial.print("PRE CONFIGURACIO MODE");
      }
    }

    if ((!BOTO_LOCAL_ROIG[0] || !BOTO_LOCAL_VERD[0]) && pre_mode_configuracio)
    { // Si deixem de pulsar botons i estavem en pre_mode_de_configuracio
      if ((millis()) >= (temps_config + temps_set_config))
      {                                // Si ha pasat el temps d'activació
        mode_configuracio = true;      // Entrem en mode configuracio
        pre_mode_configuracio = false; // Sortim del mode preconfiguracio
        if (debug)
        {
          Serial.print("CONFIGURACIO MODE");
        }
        // TODO: Cridar mode config
        Menu_configuracio();
      }
      else
      {
        pre_mode_configuracio = false; // Cancelem la preconfiguracio
        mode_configuracio = false;     // Cancelem la configuracio
        if (debug)
        {
          Serial.print("CANCELEM CONFIGURACIO MODE");
        }
      }
    }
  }
}

void initESP_NOW()
{
  // Init ESP-NOW
  if (esp_now_init() != ESP_OK)
  {
    Serial.println("Error initializing ESP-NOW");
    return;
  }
  esp_now_register_send_cb(OnDataSent);
  esp_now_register_recv_cb(OnDataRecv);
}

void setup()
{
  // Initialize Serial Monitor
  Serial.begin(115200);

  Serial.println();
  Serial.print("Server MAC Address:  ");
  Serial.println(WiFi.macAddress());

  // Set the device as a Station and Soft Access Point simultaneously
  WiFi.mode(WIFI_AP_STA);
  // Set device as a Wi-Fi Station
  // Haurem de veure que passa si no te una connexió wifi

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(1000);
    Serial.println("Setting as a Wi-Fi Station..");
  }

  Serial.print("Server SOFT AP MAC Address:  ");
  Serial.println(WiFi.softAPmacAddress());

  chan = WiFi.channel();
  Serial.print("Station IP Address: ");
  Serial.println(WiFi.localIP());
  Serial.print("Wi-Fi Channel: ");
  Serial.println(WiFi.channel());

  initESP_NOW(); // Iniciem el EspNow

  // Start Web server
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send_P(200, "text/html", index_html); });

  // Events
  events.onConnect([](AsyncEventSourceClient *client)
                   {
    if(client->lastId()){
      Serial.printf("Client reconnected! Last message ID that it got is: %u\n", client->lastId());
    }
    // send event with message "hello!", id current millis
    // and set reconnect delay to 1 second
    client->send("hello!", NULL, millis(), 10000); });
  server.addHandler(&events);

  // start server
  server.begin();
}

void loop()
{
  static unsigned long lastEventTime = millis();
  static const unsigned long EVENT_INTERVAL_MS = 5000; // Envia cada 5 segons informació
  // Cal canviar el loop per fer-lo quan es rebi un GPIO
  if ((millis() - lastEventTime) > EVENT_INTERVAL_MS)
  {
    events.send("ping", NULL, millis());
    lastEventTime = millis();
    readDataToSend();
    esp_now_send(NULL, (uint8_t *)&outgoingSetpoints, sizeof(outgoingSetpoints));
  }
}
