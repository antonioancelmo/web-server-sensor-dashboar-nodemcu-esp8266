// Bibliotecas necessárias.
#include <espnow.h>
#include <ESP8266WiFi.h>
#include "ESPAsyncWebServer.h"
#include "ESPAsyncTCP.h"
#include <Arduino_JSON.h>

// As credenciais do Wi-Fi que será conectado.
const char* ssid = "Nome-Do-Wifi";
const char* password = "Sua-Senha";

// Cria uma estrutura que contenha os dados que será recebido.
typedef struct struct_message {
  int id;
  float ldr;
  unsigned int readingId;
} struct_message;

// armazenerá valores das variáveis.
struct_message incomingReadings;

// Cria uma variável JSON.
JSONVar board;

// Cria um servidor web assíncrono na porta 80.
AsyncWebServer server(80); 

// Eventos enviados pelo servidor permitem que uma página da Web (cliente) obtenha atualizações de um servidor.
// Usaremos isso para exibir automaticamente novas leituras na
// página do servidor web quando um novo pacote ESP-NOW chegar. 
AsyncEventSource events("/events");

// A função será executada quando receber um novo paote ESP-NOW.
void OnDataRecv(uint8_t * mac_addr, uint8_t *incomingData, uint8_t len) { 
  
  // Imprime o endereço MAC do remetente
  char macStr[18];
  Serial.print("Packet received from: ");
  snprintf(macStr, sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x",
           mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
  Serial.println(macStr);

  // Copia as informações da variável incomingData para variável incomingReadings.
  memcpy(&incomingReadings, incomingData, sizeof(incomingReadings));
  
  // Cria uma variável string JSON com as infromações recebidas.
  board["id"] = incomingReadings.id;
  board["ldr"] = incomingReadings.ldr;
  board["readingId"] = String(incomingReadings.readingId);
  String jsonString = JSON.stringify(board);

  // Envia as informações para o navegador como um evento.
  events.send(jsonString.c_str(), "new_readings", millis());
  
  // Imprime as informações no Monitor Serial.
  Serial.printf("Board ID %u: %u bytes\n", incomingReadings.id, len);
  Serial.printf("ldr value: %4.2f \n", incomingReadings.ldr);
  Serial.printf("readingID value: %d \n", incomingReadings.readingId);
  Serial.println();
}

// Página com HTLM, CSS e JS
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <title>SMARTCONTROL</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <link rel="stylesheet" href="https://use.fontawesome.com/releases/v5.7.2/css/all.css" integrity="sha384-fnmOCqbTlWIlj8LyTjo7mOUStjsKC4pOpQbqyi7RrhN7udi9RwhKkMHpvLbHG9Sr" crossorigin="anonymous">
  <link rel="icon" href="data:,">
  <style>
    html {font-family: Arial; display: inline-block; text-align: center;}
    h1 {  font-size: 2rem;}
    body {  margin: 0;}
    .topnav { overflow: hidden; background-color: #2f4468; color: white; font-size: 1.7rem; }
    .content { padding: 20px; }
    .card { background-color: white; box-shadow: 2px 2px 12px 1px rgba(140,140,140,.5); }
    .cards { max-width: 700px; margin: 0 auto; display: grid; grid-gap: 2rem; grid-template-columns: repeat(auto-fit, minmax(320px, 1fr)); }
    .reading { font-size: 2.8rem; }
    .timestamp { color: #bebebe; font-size: 1rem; }
    .card-title{ font-size: 1.2rem; font-weight : bold; }
    .card.temperature { color: #B10F2E; }
    .card.humidity { color: #50B8B4; }
    button {font-size: 12px; padding: 12px 24px; background-color: #5397b0; color: white; border: 2px solid white; transition-duration: 0.4s;}
    button:hover { background-color: white; color: black; border: 2px solid #5397b0;
}
  </style>
</head>
<body>
  <div class="topnav">
    <h1>SMARTCONTROL</h1>
  </div>
  <div class="content">
    <div class="cards">
      <div class="card temperature">
        <p class="card-title"><i class="fas fa-lightbulb"></i> LDR #1 </p><p><span class="reading"><span id="t1"></span> <br> <button onclick="sendData(1)"> LED ON </button> <button onclick="sendData(0)"> LED OFF </button></span></p><p class="timestamp">Last Reading: <span id="rt1"></span></p>
      </div>
      <div class="card temperature">
        <p class="card-title"><i class="fas fa-lightbulb"></i> LDR #2 </p><p><span class="reading"><span id="t2"></span> <br> <button onclick="sendData(1)"> LED ON </button> <button onclick="sendData(0)"> LED OFF </button></span></p><p class="timestamp">Last Reading: <span id="rt2"></span></p>
      </div>
     
    </div>
  </div>
<script>

function getDateTime() {
  var currentdate = new Date();
  var datetime = currentdate.getDate() + "/"
  + (currentdate.getMonth()+1) + "/"
  + currentdate.getFullYear() + " at "
  + currentdate.getHours() + ":"
  + currentdate.getMinutes() + ":"
  + currentdate.getSeconds();
  return datetime;
}
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
  document.getElementById("t"+obj.id).innerHTML = obj.ldr;
  document.getElementById("rt"+obj.id).innerHTML = getDateTime();
  document.getElementById("rh"+obj.id).innerHTML = getDateTime();
 }, false);
}
</script>
</body>
</html>)rawliteral";


void setup() {
  
  Serial.begin(115200);
  
  // Define como ponto de acesso e estação.
  WiFi.mode(WIFI_AP_STA);

  // Conectam à rede e imprimem o endereço IP.
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Setting as a Wi-Fi Station..");
  }
  Serial.print("Station IP Address: ");
  Serial.println(WiFi.localIP());
  Serial.print("Wi-Fi Channel: ");
  Serial.println(WiFi.channel());

  // Inicializa o ESP-NOW
  if (esp_now_init() != 0) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  // Função de retorno de chamada. É executada quando um
  // novo pacote ESP-NOW chegar.  
  esp_now_register_recv_cb(OnDataRecv);

  // Quando for acessado o IP raiz, a página será recarregada! 
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", index_html);
  });
  
  // Configura a origem do evento no servidor.
  events.onConnect([](AsyncEventSourceClient *client){
    if(client->lastId()){
      Serial.printf("Client reconnected! Last message ID that it got is: %u\n", client->lastId());
    }
    client->send("hello!", NULL, millis(), 10000);
  });
  server.addHandler(&events);
  
  // Inicia o servidor.
  server.begin();
}
 
void loop() {

  // Envia um ping a cada 2 segundos. 
  static unsigned long lastEventTime = millis();
  static const unsigned long EVENT_INTERVAL_MS = 2000;
  if ((millis() - lastEventTime) > EVENT_INTERVAL_MS) {
    events.send("ping",NULL,millis());
    lastEventTime = millis();
  }
}