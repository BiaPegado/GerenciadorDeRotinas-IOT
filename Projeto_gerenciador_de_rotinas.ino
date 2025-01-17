#include <SPI.h>
#include <MFRC522.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <UniversalTelegramBot.h>
#include <WiFiClientSecure.h>

#define PINO_SS 21
#define PINO_RST 22
#define IO_USERNAME  "abiapegado014@gmail.com" 
#define IO_KEY       "teste123" 

const char* ssid = "CABO_5C06_2G";
const char* password ="zSJ58Tka5d";

const char* BOT_TOKEN = TELEGRAM_TOKEN;
const String CHAT_ID = "6697203230";  

WiFiClientSecure clientSecure;
UniversalTelegramBot bot(BOT_TOKEN, clientSecure);
unsigned long bot_lasttime;
const unsigned long BOT_MTBS = 1000;
const char* mqttserver = "maqiatto.com";
const int mqttport = 1883;
const char* mqttUser = IO_USERNAME;
const char* mqttPassword = IO_KEY;

WiFiClient espClient;
PubSubClient client(espClient);
unsigned long lastMsg = 0;

//Cartao: 99 01 3D 53
//TAG: E1 95 D0 0E

void setup_wifi() {

  delay(10);
 
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  randomSeed(micros());

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Tentando conexão MQTT...");
    String clientId = "ESP32 - Sensores";
    clientId += String(random(0xffff), HEX);
    if (client.connect(clientId.c_str(), mqttUser, mqttPassword)) {
      Serial.println("conectado");
      client.publish("abiapegado014@gmail.com/statusled", "Iniciando Comunicação"); 
      client.publish("abiapegado014@gmail.com/rotinaAtual", "Iniciando Comunicação");
      Serial.println("PUBLICOU");
    } else {
      Serial.print("Falha, rc=");
      Serial.print(client.state());
      Serial.println(" Tentando novamente em 5s");
      delay(5000);
    }
  }
}

int PORTAO = 27;
int LUZES = 32;
int AC = 4;
int PC = 14;

char item[100];
int lista_comandos[] = {PORTAO, LUZES, AC, PC};
int tamanho_lista = sizeof(lista_comandos) / sizeof(lista_comandos[0]);

MFRC522 mfrc522(PINO_SS, PINO_RST);
void setup() {
  Serial.begin(9600); 
  SPI.begin();

  pinMode(PORTAO, OUTPUT);
  pinMode(LUZES, OUTPUT);
  pinMode(AC, OUTPUT);
  pinMode(PC, OUTPUT);

  mfrc522.PCD_Init();
  setup_wifi();
  client.setServer(mqttserver, 1883);

  clientSecure.setCACert(TELEGRAM_CERTIFICATE_ROOT);
}
void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  if (millis() - bot_lasttime > BOT_MTBS)
  {
    int numNewMessages = bot.getUpdates(bot.last_message_received + 1);

    while (numNewMessages)
    {
      handleNewMessages(numNewMessages);
      numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    }

    bot_lasttime = millis();
  }

 if (!mfrc522.PICC_IsNewCardPresent()) return;
 if (!mfrc522.PICC_ReadCardSerial()) return; 
    Serial.print("UID da tag:"); 
    String conteudo= ""; 
 for (byte i = 0; i < mfrc522.uid.size; i++) {
    if(mfrc522.uid.uidByte[i] < 0x10){
      Serial.print(" 0");
    }
    else{
      Serial.print(" ");
    }
      Serial.print(mfrc522.uid.uidByte[i], HEX);
    if(mfrc522.uid.uidByte[i] < 0x10){
      conteudo.concat(String(" 0"));
    }
    else{
      conteudo.concat(String(" "));
    }
    conteudo.concat(String(mfrc522.uid.uidByte[i], HEX));
  }
 Serial.println();
 conteudo.toUpperCase();
 if (conteudo.substring(1) == "99 01 3D 53") {
  Serial.println("Iniciando Rotina 1!");
  strcpy(item, "");
  resetarRotinas();
  for(int i=0; i<tamanho_lista; i++){
    digitalWrite(lista_comandos[i], HIGH);
    client.publish("abiapegado014@gmail.com/statusled", "1");
    client.publish("abiapegado014@gmail.com/itemRotina", "1");
    strcat(item, "\n");                  
    strcat(item, identificarRotina(i));
    delay(200);
  }
  client.publish("abiapegado014@gmail.com/rotinaAtual", item);
  enviarMensagemTelegram("Rotina 1 ativada: " + String(item));
 }
 else if(conteudo.substring(1) == "E1 95 D0 0E"){
  Serial.println("Iniciando Rotina 2!");
  strcpy(item, "");
  resetarRotinas();
  for(int i=tamanho_lista-1; i>=0; i--){
    if((i+2)%2 == 0){
      digitalWrite(lista_comandos[i], HIGH);
      client.publish("abiapegado014@gmail.com/statusled", "1");
      client.publish("abiapegado014@gmail.com/itemRotina", "2");
      Serial.println(i);
      strcat(item, "\n");                    
      strcat(item, identificarRotina(i));
    }
    delay(200);
  }
  
  client.publish("abiapegado014@gmail.com/rotinaAtual", item);
  enviarMensagemTelegram("Rotina 2 ativada: " + String(item));
 }
 delay(1000);
}

void resetarRotinas(){
  for(int i=0; i<tamanho_lista; i++){
    digitalWrite(lista_comandos[i], LOW);
  }
  client.publish("abiapegado014@gmail.com/itemRotina", "0");
  client.publish("abiapegado014@gmail.com/statusled", "0");
  client.publish("abiapegado014@gmail.com/rotinaAtual", "Nenhum");
  delay(500);
}

char* identificarRotina(int opcao) {
  static char dispositivo[15];  
  switch (opcao) {
    case 0:
      strcpy(dispositivo, "Portao"); 
      break;
    case 1:
      strcpy(dispositivo, "Luzes"); 
      break;
    case 2:
      strcpy(dispositivo, "Aquecedor");  
      break;
    case 3:
      strcpy(dispositivo, "PC");  
      break;
    default:
      strcpy(dispositivo, "desconhecido");  
      break;
  }
  return dispositivo;  
}

void handleNewMessages(int numNewMessages)
{
  for (int i = 0; i < numNewMessages; i++)
  {
    String chat_id = bot.messages[i].chat_id;
    String text = bot.messages[i].text;

    String from_name = bot.messages[i].from_name;
    if (from_name == "")
      from_name = "Guest";

    if (text == "/off")
    {
      resetarRotinas();
      enviarMensagemTelegram("Rotinas foram resetadas.");
      Serial.println("Rotinas foram resetadas.");
    }
    
    if (text == "/start")
    {
      String welcome = "Bem-vindo ao bot Gerenciador de Rotinas, " + from_name + ".\n";
      welcome += "/off : para resetar rotinas";
      bot.sendMessage(chat_id, welcome, "Markdown");
    }
  }
}


void enviarMensagemTelegram(String mensagem) {
  bot.sendMessage(CHAT_ID, mensagem, "");
}


