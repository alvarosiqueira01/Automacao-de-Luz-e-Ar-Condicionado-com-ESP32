#include <PubSubClient.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include <NTPClient.h>
 
#include <OneWire.h>
#include <DallasTemperature.h>
 

#define LIGHTING 14 // GPIO que está ligado o relé da iluminacao
#define COOLING 18 // GPIO que está ligado o relé da refrigeracao

/* Configura os tópicos do MQTT */
#define TOPIC_SUBSCRIBE_LIGHTING       "topic_on_off_lights"
#define TOPIC_SUBSCRIBE_COOLING        "topic_on_off_cooling"
#define TOPIC_SUBSCRIBE_TIME_LIGHT     "topic_set_time_lighting"
#define TOPIC_SUBSCRIBE_TIME_COOL      "topic_set_time_cooling"
#define TOPIC_PUBLISH_TEMPERATURE "topic_sensor_temperature"

#define PUBLISH_DELAY 2000   // Atraso da publicação (2 segundos)

#define ID_MQTT "esp32_mqtt" // id mqtt (para identificação de sessão)

const char *SSID = "Wokwi-GUEST"; // SSID / nome da rede WI-FI que deseja se conectar
const char *PASSWORD = "";        // Senha da rede WI-FI que deseja se conectar

// URL do broker MQTT que se deseja utilizar
const char* MQTT_SERVER = "test.mosquitto.org";

int BROKER_PORT = 1883; // Porta do Broker MQTT

unsigned long publishUpdate;

// GPIO do sansor de temperatura
const int oneWireBus = 12;     
OneWire oneWire(oneWireBus);
DallasTemperature sensors(&oneWire);

static char strTemperature[10] = {0};
int uppLimitTemp= 27;
int infLimitTemp = 20;
int onLimitHourCool = 20;
int onLimitMinuteCool = 0;
int onLimitHourLight = 17;
int onLimitMinuteLight = 30;
int offLimitHourCool = 5;
int offLimitMinuteCool = 0;
int offLimitHourLight = 21;
int offLimitMinuteLight = 30;
float currentTemperature;
int pinState;
int currentHour;
int currentMinutes;
int currentSeconds;
int manuallyTurnOn = 0;
int manuallyTurnOff = 0;

// Configurações do Servidor NTP
const char* servidorNTP = "a.st1.ntp.br"; // Servidor NTP para pesquisar a hora
 
const int fusoHorario = -10800; // Fuso horário em segundos (-03h = -10800 seg)
const int taxaDeAtualizacao = 1800000; // Taxa de atualização do servidor NTP em milisegundos
 
WiFiUDP ntpUDP; // Declaração do Protocolo UDP
NTPClient timeClient(ntpUDP, servidorNTP, fusoHorario, 60000);

// Variáveis e objetos globais
WiFiClient espClient;         // Cria o objeto espClient
PubSubClient MQTT(espClient); // Instancia o Cliente MQTT passando o objeto espClient

/* Prototypes */
float getTemperature(void);
int getTime(void);

void initWiFi(void);
void initMQTT(void);
void callbackMQTT(char *topic, byte *payload, unsigned int length);
void reconnectMQTT(void);
void reconnectWiFi(void);
void checkWiFIAndMQTT(void);

/* Faz a leitura da temperatura (sensor DHT22)
   Retorno: temperatura (graus Celsius) */
float getTemperature(void)
{
  sensors.requestTemperatures(); 
  float temperatureC = sensors.getTempCByIndex(0);

  return temperatureC;
}


void setCooling(void){
  currentTemperature = getTemperature();
  pinState = digitalRead(COOLING);
  timeClient.update();
  currentHour = timeClient.getHours();
  currentMinutes = timeClient.getMinutes();
  currentSeconds = timeClient.getSeconds();
  if (((currentTemperature >= uppLimitTemp) && pinState == LOW && manuallyTurnOff == 0) || ((currentHour == onLimitHourCool && currentMinutes == onLimitMinuteCool && currentSeconds < 20) && pinState == LOW)){
    digitalWrite(COOLING,HIGH);
  }

  if ((currentTemperature <= infLimitTemp && pinState == HIGH && manuallyTurnOn == 0) || ((currentHour == offLimitHourCool && currentMinutes == offLimitMinuteCool && currentSeconds < 20) && pinState == HIGH)){
    digitalWrite(COOLING,LOW);
  }

  if (uppLimitTemp - currentTemperature > 2 && manuallyTurnOff == 1){
    manuallyTurnOff = 0;
  }
  
  if (currentTemperature - infLimitTemp > 2 && manuallyTurnOn == 1){
    manuallyTurnOn = 0;
  } 

}

void setLighting(void){
  pinState = digitalRead(LIGHTING);
  timeClient.update();
  currentHour = timeClient.getHours();
  currentMinutes = timeClient.getMinutes();
  currentSeconds = timeClient.getSeconds();
  if ((currentHour == onLimitHourLight && currentMinutes == onLimitMinuteLight && currentSeconds < 20) && pinState == LOW){
    digitalWrite(LIGHTING, HIGH);
  }

  if ((currentHour == offLimitHourLight && currentMinutes == offLimitMinuteLight && currentSeconds < 20) && pinState == HIGH){
    digitalWrite(LIGHTING, LOW);
  }

}

/* Inicializa e conecta-se na rede WI-FI desejada */
void initWiFi(void)
{
  delay(10);
  Serial.println("------Conexao WI-FI------");
  Serial.print("Conectando-se na rede: ");
  Serial.println(SSID);
  Serial.println("Aguarde");

  reconnectWiFi();
}

/* Inicializa os parâmetros de conexão MQTT(endereço do broker, porta e seta
  função de callback) */
void initMQTT(void)
{
  MQTT.setServer(MQTT_SERVER, BROKER_PORT); // Informa qual broker e porta deve ser conectado
  MQTT.setCallback(callbackMQTT);           // Atribui função de callback (função chamada quando qualquer informação de um dos tópicos subescritos chega)
}

/* Função de callback  esta função é chamada toda vez que uma informação
   de um dos tópicos subescritos chega */
void callbackMQTT(char *topic, byte *payload, unsigned int length)
{
  String msg;

  // Obtem a string do payload recebido
  for (int i = 0; i < length; i++) {
    char c = (char)payload[i];
    msg += c;
  }

  Serial.printf("Chegou a seguinte string via MQTT: %s do topico: %s\n", msg, topic);

  /* Toma ação dependendo da string recebida */
  if (msg.equals("Ligar ar condicionado")) {
    digitalWrite(COOLING, HIGH);
    Serial.print("Ar condicionado ligado por comando MQTT");
    manuallyTurnOn = 1;
  }

  if (msg.equals("Desligar ar condicionado")) {
    digitalWrite(COOLING, LOW);
    Serial.print("Ar condicionado desligado por comando MQTT");
    manuallyTurnOff = 1;
  }

  if (msg.equals("Ligar lampadas")) {
    digitalWrite(LIGHTING, HIGH);
    Serial.print("Lampadas ligadas por comando MQTT");
  }

  if (msg.equals("Desligar lampadas")) {
    digitalWrite(LIGHTING, LOW);
    Serial.print("Lampadas desligadas por comando MQTT");
  }

  
}

/* Reconecta-se ao broker MQTT (caso ainda não esteja conectado ou em caso de a conexão cair)
   em caso de sucesso na conexão ou reconexão, o subscribe dos tópicos é refeito. */
void reconnectMQTT(void)
{
  while (!MQTT.connected()) {
    Serial.print("* Tentando se conectar ao Broker MQTT: ");
    Serial.println(MQTT_SERVER);
    if (MQTT.connect(ID_MQTT)) {
      Serial.println("Conectado com sucesso ao broker MQTT!");
      MQTT.subscribe(TOPIC_SUBSCRIBE_LIGHTING);
      MQTT.subscribe(TOPIC_SUBSCRIBE_COOLING);
      MQTT.subscribe(TOPIC_SUBSCRIBE_TIME_LIGHT);
      MQTT.subscribe(TOPIC_SUBSCRIBE_TIME_COOL);
    } else {
      Serial.println("Falha ao reconectar no broker.");
      Serial.println("Nova tentativa de conexao em 2 segundos.");
      delay(2000);
    }
  }
}

/* Verifica o estado das conexões WiFI e ao broker MQTT.
  Em caso de desconexão (qualquer uma das duas), a conexão é refeita. */
void checkWiFIAndMQTT(void)
{
  if (!MQTT.connected())
    reconnectMQTT(); // se não há conexão com o Broker, a conexão é refeita

  reconnectWiFi(); // se não há conexão com o WiFI, a conexão é refeita
}

void reconnectWiFi(void)
{
  // se já está conectado a rede WI-FI, nada é feito.
  // Caso contrário, são efetuadas tentativas de conexão
  if (WiFi.status() == WL_CONNECTED)
    return;

  WiFi.begin(SSID, PASSWORD); // Conecta na rede WI-FI

  while (WiFi.status() != WL_CONNECTED) {
    delay(100);
    Serial.print(".");
  }

  Serial.println();
  Serial.print("Conectado com sucesso na rede ");
  Serial.print(SSID);
  Serial.println("IP obtido: ");
  Serial.println(WiFi.localIP());
}

void setup()
{
  Serial.begin(115200);

  pinMode(LIGHTING, OUTPUT);
  digitalWrite(LIGHTING, LOW);
  pinMode(COOLING, OUTPUT);
  digitalWrite(COOLING, LOW);

  // Inicializacao do sensor de temperatura
  sensors.begin();

  // Inicializa a conexao Wi-Fi
  initWiFi();

  // Inicializa a conexao ao broker MQTT
  initMQTT();

  timeClient.begin();
}

void loop()
{

  /* Repete o ciclo após 2 segundos */
  if ((millis() - publishUpdate) >= PUBLISH_DELAY) {
    publishUpdate = millis();
    // Verifica o funcionamento das conexões WiFi e ao broker MQTT
    checkWiFIAndMQTT();

    setCooling();
    setLighting();
    // Formata as strings a serem enviadas para o dashboard (campos texto)
    sprintf(strTemperature, "%.2fC", getTemperature());

    // Envia as strings ao dashboard MQTT
    MQTT.publish(TOPIC_PUBLISH_TEMPERATURE, strTemperature);

    // Keep-alive da comunicação com broker MQTT
    MQTT.loop();
  }
}

