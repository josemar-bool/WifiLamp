#include <ESP8266WiFi.h>

// Definição dos pinos
#define LED_PIN    2
#define LAMP_PIN   0
#define BT_PIN     3

// Configuração de IP da rede
IPAddress myIP(192,168,0,118);
IPAddress myDNS(192,168,0,1);
IPAddress myMASC(255,255,255,0);

void hardwareInit(void);
void trataRequest(void);
void enviaResposta(WiFiClient client, bool lampada, bool led);
void atualizaSaidas(bool lampada, bool led);
bool trataChave(void);

// Dados da rede WiFi
const char* ssid = "######";
const char* password = "******";

const char* headerHTLM = "HTTP/1.1 200 OK\nContent-Type: text/html\n\n<!DOCTYPE HTML>\n<html>\n";
// Corpo da resposta segue a estrutura -> [{Lampada:XX,Led:ZZ}]
const char* tailHTML = "</html>\n";

bool statusLamp = 0;
bool funcaoLed = 1;
WiFiServer server(80);

void setup(){
Serial.begin(115200);
hardwareInit();

Serial.print("\nConectando a rede: ");
Serial.println(ssid);
WiFi.begin(ssid, password);                   // Conecta na rede
WiFi.config(myIP, myDNS, myMASC);             // Configura IP estático
int i = 0;
while (WiFi.status() != WL_CONNECTED){
  delay(1000);
  Serial.print(".");
  if(++i > 30) ESP.restart();                 // Reinicia ESP após 30 segundos
  }
Serial.println("\nWiFi Conectado!");
server.begin();                               // Inicia Server
Serial.print("Use essa URL para conectar: http://");
Serial.print(WiFi.localIP());
Serial.println("/");
}

void loop(){
trataRequest();                                 // Trata requisição do app
atualizaSaidas(statusLamp, funcaoLed);
if(trataChave()) statusLamp = !statusLamp;
}

void hardwareInit(void){                        // Inicializa hardware
  pinMode(LED_PIN, OUTPUT);
  pinMode(LAMP_PIN, OUTPUT);
  pinMode(BT_PIN, INPUT);
  digitalWrite(LED_PIN, HIGH);
  digitalWrite(LAMP_PIN, HIGH);
}

void trataRequest(void){                        // Trata requisição do app
  WiFiClient client = server.available();       // Testa se foi feita requisição
  if(!client) return;

  while(!client.available()) delay(1);          // Aguarda recepção dos dados

  String request = client.readStringUntil('\r');// Le dados
  client.flush();

  if(request.indexOf("status") != -1){
    enviaResposta(client, statusLamp, funcaoLed);// Responde requisição
    return;
  }

  if(request.indexOf("lamp") != -1){
    if(request.indexOf("on") != -1) statusLamp = true; // Liga lampada
    if(request.indexOf("off") != -1) statusLamp = false; // Desliga lampada
    enviaResposta(client, statusLamp, funcaoLed);
    return;
  }

  if(request.indexOf("led") != -1){
    if(request.indexOf("on") != -1) funcaoLed = true; // Liga função led
    if(request.indexOf("off") != -1) funcaoLed = false; // Desliga função led
    enviaResposta(client, statusLamp, funcaoLed);
    return;
  }
}

void enviaResposta(WiFiClient client, bool lampada, bool led){
  // Monta corpo da resposta seguindo a estrutura -> [{Lampada:XX,Led:ZZ}]
  String resposta = "[{Lampada:";
  if(lampada) resposta = resposta + "ON";
  else resposta = resposta + "OFF";
  resposta += ",Led:";
  if(led) resposta = resposta + "ON";
  else resposta = resposta + "OFF";
  resposta += "]}";

  client.print(headerHTLM);
  client.print(resposta);
  client.print(tailHTML);
}

void atualizaSaidas(bool lampada, bool led){
  if(lampada){
    digitalWrite(LAMP_PIN, LOW);
    digitalWrite(LED_PIN, HIGH);
  } else{
    digitalWrite(LAMP_PIN, HIGH);
    if(led) digitalWrite(LED_PIN, LOW);
    else digitalWrite(LED_PIN, HIGH);
  }
}

bool trataChave(void){
  const unsigned long Delay = 50;             // Variaveis para debouce da chave
  static unsigned long tempoDebounce = 0;
  static int estadoChave = HIGH, ultimaLeitura = HIGH;
  bool flag = false;
  int leitura = digitalRead(BT_PIN);

  if(leitura != ultimaLeitura) tempoDebounce = millis();

 if((millis() - tempoDebounce) > Delay){
   if(leitura != estadoChave){
     estadoChave = leitura;
     if(estadoChave == HIGH) flag = true;
   }
 }
 ultimaLeitura = leitura;
 return(flag);
}