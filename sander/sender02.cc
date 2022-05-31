// Bibliotecas necessárias.
#include <espnow.h>
#include <ESP8266WiFi.h>

// Define o id da placa do remetente.
#define BOARD_ID 2

// Utilizando o LED D0 da placa.
int LED = D0;

// Inserir o endereço MAC do recepctor.
uint8_t broadcastAddress[] = {0x60, 0x01, 0x94, 0x51, 0xDD, 0xA4}; //ESP8266

// Cria uma estrutura dos dados que irá ser enviado!
typedef struct struct_message {
    int id;
    int ldr;
    int readingId;
} struct_message;

// Armazena os valores das variáveis.
struct_message myData;

// Cria um intervalo/temporizador para pubicar as leituras.
unsigned long previousMillis = 0;   
const long interval = 2000;        

unsigned int readingId = 0;

// Insera o nome do Wi-Fi.
constexpr char WIFI_SSID[] = "Nome-do-Wifi";

// A função procura sua rede e obtém seu canal.
int32_t getWiFiChannel(const char *ssid) {
  if (int32_t n = WiFi.scanNetworks()) {
    for (uint8_t i=0; i<n; i++) {
      if (!strcmp(ssid, WiFi.SSID(i).c_str())) {
        return WiFi.channel(i);
      }
    }
  }
  return 0;
}

// A função lê e retorna a leitura de luminosidade do sensor.
int readLDR() {
  int l = analogRead(A0);
  return l;
}

// A função de retorno será executada quando uma mensagem for enviada.
// A função impre se a mensagem foi enviado com sucesso ou não.
void OnDataSent(uint8_t *mac_addr, uint8_t sendStatus) {
  Serial.print("Last Packet Send Status: ");
  if (sendStatus == 0){
    Serial.println("Delivery success");
  }
  else{
    Serial.println("Delivery fail");
  }
}
 
void setup() {

  Serial.begin(115200);

  pinMode(LED, OUTPUT);
  digitalWrite(LED, 0);

  WiFi.mode(WIFI_STA);

  // Define seu canal para corresponder ao canal WiFi do receptor.
  int32_t channel = getWiFiChannel(WIFI_SSID);

  WiFi.printDiag(Serial); 
  wifi_promiscuous_enable(1);
  wifi_set_channel(channel);
  wifi_promiscuous_enable(0);
  WiFi.printDiag(Serial);

  // Inicializa o ESP-NOW.
  if (esp_now_init() != 0) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }
  
  esp_now_set_self_role(ESP_NOW_ROLE_CONTROLLER);

  // Registra a função callback que será chamada quando
  // uma mensagem for enviada.
  esp_now_register_send_cb(OnDataSent);
 
  //  Registram e adicionam o receptor como um peer.
  esp_now_add_peer(broadcastAddress, ESP_NOW_ROLE_SLAVE, 1, NULL, 0);
}
 
void loop() {

  // Verifica se é hora de obter e enviar novas leituras.
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {
    
    previousMillis = currentMillis;
    
    myData.id = BOARD_ID;
    myData.ldr = readLDR();
    myData.readingId = readingId++;
    
    // Envia a estrutura da mensagem via ESP-NOW.
    esp_now_send(broadcastAddress, (uint8_t *) &myData, sizeof(myData));

    Serial.print("loop");
  }
}