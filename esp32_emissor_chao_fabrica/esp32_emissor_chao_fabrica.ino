#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include <DHT.h>

// =======================
// PINAGENS DO SEU PROJETO
// =======================
const int pinoTrig = 5;
const int pinoEcho = 18;
const int pinoDHT = 15;
const int pinoLDR = 6;
const int pinoPIR = 13;
const int pinoLEDVerde = 4;
const int pinoLEDVermelho = 7;

// =======================
// CONFIGURACAO DO DHT
// =======================
#define DHTTYPE DHT22
DHT dht(pinoDHT, DHTTYPE);

// =======================
// CONFIGURACAO DO TANQUE
// =======================
const float distanciaTanqueVazio = 20.0;
const float distanciaTanqueCheio = 5.0;
const int limiteNivelBaixo = 20;

// =======================
// INTERVALO DE LEITURA/ENVIO
// =======================
unsigned long tempoUltimaLeitura = 0;
const unsigned long intervaloLeitura = 2000;

// =======================
// MAC DO ESP32 DE MONITORAMENTO
// =======================
uint8_t macReceptor[] = {0x80, 0xB5, 0x4E, 0xC6, 0x5C, 0x0C};

// =======================
// CANAL DO WI-FI CUNHA
// Descoberto no receptor: canal 3
// =======================
const int canalWiFi = 3;

// =======================
// ESTRUTURA DO PACOTE ESPNOW
// Precisa ser igual no receptor
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

struct_message dadosParaEnviar;

// =======================
// FUNCAO: MEDIR DISTANCIA
// =======================
float medirDistanciaCm() {
  digitalWrite(pinoTrig, LOW);
  delayMicroseconds(2);

  digitalWrite(pinoTrig, HIGH);
  delayMicroseconds(10);
  digitalWrite(pinoTrig, LOW);

  long duracao = pulseIn(pinoEcho, HIGH, 30000);

  if (duracao == 0) {
    return -1;
  }

  float distancia = duracao * 0.0343 / 2.0;
  return distancia;
}

// =======================
// FUNCAO: CALCULAR NIVEL
// =======================
int calcularNivelTinta(float distancia) {
  if (distancia < 0) {
    return 0;
  }

  int nivel = map(
    distancia,
    distanciaTanqueVazio,
    distanciaTanqueCheio,
    0,
    100
  );

  if (nivel < 0) {
    nivel = 0;
  }

  if (nivel > 100) {
    nivel = 100;
  }

  return nivel;
}

// =======================
// CALLBACK DE ENVIO ESPNOW
// Compativel com Arduino ESP32 Core 3.3.8 / ESP32-S3
// =======================
void onDataSent(const wifi_tx_info_t *tx_info, esp_now_send_status_t status) {
  Serial.print("Status do envio ESPNOW: ");

  if (status == ESP_NOW_SEND_SUCCESS) {
    Serial.println("SUCESSO");
  } else {
    Serial.println("FALHA");
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  // =======================
  // CONFIGURACAO DOS PINOS
  // =======================
  pinMode(pinoTrig, OUTPUT);
  pinMode(pinoEcho, INPUT);

  pinMode(pinoLDR, INPUT);
  pinMode(pinoPIR, INPUT);

  pinMode(pinoLEDVerde, OUTPUT);
  pinMode(pinoLEDVermelho, OUTPUT);

  digitalWrite(pinoLEDVerde, LOW);
  digitalWrite(pinoLEDVermelho, LOW);

  // =======================
  // INICIAR DHT
  // =======================
  dht.begin();

  // =======================
  // WIFI EM MODO STATION + CANAL 3
  // Necessario para ESPNOW funcionar junto com o receptor no Wi-Fi
  // =======================
  WiFi.mode(WIFI_STA);
  esp_wifi_set_channel(canalWiFi, WIFI_SECOND_CHAN_NONE);

  Serial.println("==================================");
  Serial.println("ESP32 CHAO DE FABRICA INICIADO");
  Serial.print("Canal ESPNOW configurado: ");
  Serial.println(canalWiFi);
  Serial.print("MAC deste ESP32 emissor: ");
  Serial.println(WiFi.macAddress());
  Serial.println("==================================");

  // =======================
  // INICIAR ESPNOW
  // =======================
  if (esp_now_init() != ESP_OK) {
    Serial.println("Erro ao iniciar ESPNOW");
    return;
  }

  esp_now_register_send_cb(onDataSent);

  // =======================
  // ADICIONAR RECEPTOR
  // =======================
  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, macReceptor, 6);
  peerInfo.channel = canalWiFi;
  peerInfo.encrypt = false;

  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Erro ao adicionar o ESP32 receptor");
    return;
  }

  Serial.println("ESP32 receptor adicionado com sucesso");
  Serial.println("Sistema pronto para ler sensores e enviar via ESPNOW\n");
}

void loop() {
  if (millis() - tempoUltimaLeitura >= intervaloLeitura) {
    tempoUltimaLeitura = millis();

    // =======================
    // LEITURA DOS SENSORES
    // =======================
    float distancia = medirDistanciaCm();
    int nivelTinta = calcularNivelTinta(distancia);

    float temperatura = dht.readTemperature();
    float umidade = dht.readHumidity();

    int luminosidade = analogRead(pinoLDR);
    int presenca = digitalRead(pinoPIR) == HIGH ? 1 : 0;

    // =======================
    // VALIDACAO DO DHT
    // =======================
    if (isnan(temperatura) || isnan(umidade)) {
      Serial.println("Erro ao ler o DHT");
      return;
    }

    // =======================
    // ESTADO DO SISTEMA
    // =======================
    bool alertaNivel = nivelTinta < limiteNivelBaixo;

    if (alertaNivel) {
      digitalWrite(pinoLEDVerde, LOW);
      digitalWrite(pinoLEDVermelho, HIGH);
    } else {
      digitalWrite(pinoLEDVerde, HIGH);
      digitalWrite(pinoLEDVermelho, LOW);
    }

    // =======================
    // MONTAR PACOTE PARA ESPNOW
    // =======================
    dadosParaEnviar.nivelTinta = nivelTinta;
    dadosParaEnviar.temperatura = temperatura;
    dadosParaEnviar.umidade = umidade;
    dadosParaEnviar.luminosidade = luminosidade;
    dadosParaEnviar.presenca = presenca;
    dadosParaEnviar.timestamp = millis();
    dadosParaEnviar.distancia = distancia;
    dadosParaEnviar.alertaNivel = alertaNivel ? 1 : 0;

    // =======================
    // ENVIAR PACOTE VIA ESPNOW
    // =======================
    esp_err_t resultado = esp_now_send(
      macReceptor,
      (uint8_t *) &dadosParaEnviar,
      sizeof(dadosParaEnviar)
    );

    // =======================
    // MONITOR SERIAL
    // =======================
    Serial.println("------ LEITURA DO SISTEMA ------");
    Serial.println("Distancia medida: " + String(distancia, 2) + " cm");
    Serial.println("Nivel do tanque: " + String(nivelTinta) + "%");
    Serial.println("Temperatura: " + String(temperatura, 1) + " C");
    Serial.println("Umidade: " + String(umidade, 1) + "%");
    Serial.println("Luminosidade analogica AO: " + String(luminosidade));
    Serial.println(presenca ? "Presenca detectada" : "Sem presenca");

    if (alertaNivel) {
      Serial.println("Estado: ALERTA! Nivel de tinta baixo.");
    } else {
      Serial.println("Estado: Operacao normal.");
    }

    if (resultado == ESP_OK) {
      Serial.println("Pacote enviado via ESPNOW");
    } else {
      Serial.println("Erro ao chamar esp_now_send");
    }

    Serial.println("-------------------------------\n");
  }
}
