#include <WiFi.h>
#include <esp_now.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>

// =======================
// WIFI
// =======================
const char* ssid = "SEU_WIFI_SSID";
const char* password = "SUA_SENHA_WIFI";

// =======================
// SUPABASE
// =======================
// Use SEM barra no final
String supabaseUrl = "https://SEU-PROJETO.supabase.co";

// Cole aqui sua anon public key
String supabaseKey = "SUA_SUPABASE_ANON_PUBLIC_KEY";

// Nome da tabela criada no Supabase
String tabelaSupabase = "leituras_iot";

// =======================
// LED BRANCO DO RECEPTOR
// =======================
const int pinoLEDBranco = 4; // ajuste se o LED branco estiver em outro pino

// =======================
// STRUCT RECEBIDA VIA ESPNOW
// Precisa ser igual a struct do emissor
// =======================
typedef struct struct_message {
  int nivelTinta;
  float temperatura;
  float umidade;
  int luminosidade;
  int presenca;
  unsigned long timestamp;
  float distancia;
  int alertaNivel;
} struct_message;

struct_message dadosRecebidos;
struct_message ultimoPacote;

bool novoPacoteRecebido = false;

unsigned long ultimoRecebimento = 0;
const unsigned long tempoTimeout = 5000;

// =======================
// CALLBACK ESPNOW
// =======================
void onDataRecv(const esp_now_recv_info_t *info, const uint8_t *incomingData, int len) {
  if (len == sizeof(dadosRecebidos)) {
    memcpy(&dadosRecebidos, incomingData, sizeof(dadosRecebidos));
    memcpy(&ultimoPacote, &dadosRecebidos, sizeof(dadosRecebidos));

    novoPacoteRecebido = true;
    ultimoRecebimento = millis();

    // Piscada rapida = recebeu pacote ESPNOW
    digitalWrite(pinoLEDBranco, HIGH);
    delay(80);
    digitalWrite(pinoLEDBranco, LOW);
  }
}

// =======================
// CONECTAR WIFI
// =======================
void conectarWiFi() {
  if (WiFi.status() == WL_CONNECTED) {
    return;
  }

  Serial.println("Conectando ao Wi-Fi...");
  WiFi.begin(ssid, password);

  int tentativas = 0;

  while (WiFi.status() != WL_CONNECTED && tentativas < 30) {
    delay(500);
    Serial.print(".");
    tentativas++;
  }

  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("Wi-Fi conectado!");

    Serial.print("IP do ESP32: ");
    Serial.println(WiFi.localIP());

    // ESTA E A PARTE NOVA IMPORTANTE
    Serial.print("Canal do Wi-Fi: ");
    Serial.println(WiFi.channel());

  } else {
    Serial.println("Falha ao conectar no Wi-Fi.");
  }
}

// =======================
// ENVIAR PARA SUPABASE
// =======================
void enviarParaSupabase(struct_message dados) {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Wi-Fi desconectado. Tentando reconectar...");
    conectarWiFi();

    if (WiFi.status() != WL_CONNECTED) {
      Serial.println("Nao foi possivel enviar ao Supabase: sem Wi-Fi.");
      return;
    }
  }

  WiFiClientSecure client;
  client.setInsecure();

  HTTPClient http;

  String endpoint = supabaseUrl + "/rest/v1/" + tabelaSupabase;

  http.begin(client, endpoint);

  http.addHeader("Content-Type", "application/json");
  http.addHeader("apikey", supabaseKey);
  http.addHeader("Authorization", "Bearer " + supabaseKey);
  http.addHeader("Prefer", "return=minimal");

  String json = "{";
  json += "\"nivel_tinta\":" + String(dados.nivelTinta) + ",";
  json += "\"temperatura\":" + String(dados.temperatura, 2) + ",";
  json += "\"umidade\":" + String(dados.umidade, 2) + ",";
  json += "\"luminosidade\":" + String(dados.luminosidade) + ",";
  json += "\"presenca\":" + String(dados.presenca) + ",";
  json += "\"distancia\":" + String(dados.distancia, 2) + ",";
  json += "\"alerta_nivel\":" + String(dados.alertaNivel) + ",";
  json += "\"timestamp_esp\":" + String(dados.timestamp);
  json += "}";

  Serial.println("Enviando para Supabase:");
  Serial.println(json);

  int httpResponseCode = http.POST(json);

  Serial.print("Resposta HTTP Supabase: ");
  Serial.println(httpResponseCode);

  if (httpResponseCode == 201 || httpResponseCode == 200 || httpResponseCode == 204) {
    Serial.println("Dados salvos no Supabase com sucesso!");
  } else {
    Serial.println("Erro ao salvar no Supabase.");
    String resposta = http.getString();
    Serial.println("Resposta do Supabase:");
    Serial.println(resposta);
  }

  http.end();
}

// =======================
// MOSTRAR PACOTE NO SERIAL
// =======================
void mostrarPacoteSerial(struct_message dados) {
  Serial.println("------ PACOTE RECEBIDO ------");

  Serial.print("Distancia medida: ");
  Serial.print(dados.distancia, 2);
  Serial.println(" cm");

  Serial.print("Nivel do tanque: ");
  Serial.print(dados.nivelTinta);
  Serial.println("%");

  Serial.print("Temperatura: ");
  Serial.print(dados.temperatura, 1);
  Serial.println(" C");

  Serial.print("Umidade: ");
  Serial.print(dados.umidade, 1);
  Serial.println("%");

  Serial.print("Luminosidade: ");
  Serial.println(dados.luminosidade);

  Serial.print("Presenca: ");
  Serial.println(dados.presenca ? "Detectada" : "Ausente");

  Serial.print("Timestamp millis: ");
  Serial.println(dados.timestamp);

  if (dados.alertaNivel == 1) {
    Serial.println("Estado recebido: ALERTA! Nivel de tinta baixo.");
  } else {
    Serial.println("Estado recebido: Operacao normal.");
  }

  Serial.println("-----------------------------\n");
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  pinMode(pinoLEDBranco, OUTPUT);
  digitalWrite(pinoLEDBranco, LOW);

  WiFi.mode(WIFI_STA);

  Serial.println("ESP32 DE MONITORAMENTO INICIADO");

  conectarWiFi();

  if (esp_now_init() != ESP_OK) {
    Serial.println("Erro ao iniciar ESPNOW");
    return;
  }

  esp_now_register_recv_cb(onDataRecv);

  Serial.println("ESPNOW iniciado.");
  Serial.println("Aguardando dados do ESP32 chao de fabrica...");
}

void loop() {
  if (novoPacoteRecebido) {
    novoPacoteRecebido = false;

    mostrarPacoteSerial(ultimoPacote);
    enviarParaSupabase(ultimoPacote);
  }

  if (ultimoRecebimento > 0 && millis() - ultimoRecebimento > tempoTimeout) {
    Serial.println("ALERTA: sem dados recebidos nos ultimos 5 segundos");

    digitalWrite(pinoLEDBranco, HIGH);
    delay(500);
    digitalWrite(pinoLEDBranco, LOW);
    delay(500);
  }
}
