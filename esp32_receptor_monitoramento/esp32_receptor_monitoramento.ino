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

// Certificado da CA raiz usada pela cadeia TLS do Supabase (ISRG Root X1,
// Let's Encrypt). Verificado em 2026-08-13 via
// `openssl s_client -connect supabase.co:443 -showcerts`, PEM baixado
// diretamente de https://letsencrypt.org/certs/isrgrootx1.pem. Fixar a CA
// raiz (em vez do leaf/intermediario) e uma escolha de baixa manutencao:
// raizes trocam em escala de decada, nao a cada 60-90 dias como os
// certificados de folha do Let's Encrypt. O ISRG Root X1 e valido ate 2035.
static const char* SUPABASE_ROOT_CA = R"EOF(
-----BEGIN CERTIFICATE-----
MIIFazCCA1OgAwIBAgIRAIIQz7DSQONZRGPgu2OCiwAwDQYJKoZIhvcNAQELBQAw
TzELMAkGA1UEBhMCVVMxKTAnBgNVBAoTIEludGVybmV0IFNlY3VyaXR5IFJlc2Vh
cmNoIEdyb3VwMRUwEwYDVQQDEwxJU1JHIFJvb3QgWDEwHhcNMTUwNjA0MTEwNDM4
WhcNMzUwNjA0MTEwNDM4WjBPMQswCQYDVQQGEwJVUzEpMCcGA1UEChMgSW50ZXJu
ZXQgU2VjdXJpdHkgUmVzZWFyY2ggR3JvdXAxFTATBgNVBAMTDElTUkcgUm9vdCBY
MTCCAiIwDQYJKoZIhvcNAQEBBQADggIPADCCAgoCggIBAK3oJHP0FDfzm54rVygc
h77ct984kIxuPOZXoHj3dcKi/vVqbvYATyjb3miGbESTtrFj/RQSa78f0uoxmyF+
0TM8ukj13Xnfs7j/EvEhmkvBioZxaUpmZmyPfjxwv60pIgbz5MDmgK7iS4+3mX6U
A5/TR5d8mUgjU+g4rk8Kb4Mu0UlXjIB0ttov0DiNewNwIRt18jA8+o+u3dpjq+sW
T8KOEUt+zwvo/7V3LvSye0rgTBIlDHCNAymg4VMk7BPZ7hm/ELNKjD+Jo2FR3qyH
B5T0Y3HsLuJvW5iB4YlcNHlsdu87kGJ55tukmi8mxdAQ4Q7e2RCOFvu396j3x+UC
B5iPNgiV5+I3lg02dZ77DnKxHZu8A/lJBdiB3QW0KtZB6awBdpUKD9jf1b0SHzUv
KBds0pjBqAlkd25HN7rOrFleaJ1/ctaJxQZBKT5ZPt0m9STJEadao0xAH0ahmbWn
OlFuhjuefXKnEgV4We0+UXgVCwOPjdAvBbI+e0ocS3MFEvzG6uBQE3xDk3SzynTn
jh8BCNAw1FtxNrQHusEwMFxIt4I7mKZ9YIqioymCzLq9gwQbooMDQaHWBfEbwrbw
qHyGO0aoSCqI3Haadr8faqU9GY/rOPNk3sgrDQoo//fb4hVC1CLQJ13hef4Y53CI
rU7m2Ys6xt0nUW7/vGT1M0NPAgMBAAGjQjBAMA4GA1UdDwEB/wQEAwIBBjAPBgNV
HRMBAf8EBTADAQH/MB0GA1UdDgQWBBR5tFnme7bl5AFzgAiIyBpY9umbbjANBgkq
hkiG9w0BAQsFAAOCAgEAVR9YqbyyqFDQDLHYGmkgJykIrGF1XIpu+ILlaS/V9lZL
ubhzEFnTIZd+50xx+7LSYK05qAvqFyFWhfFQDlnrzuBZ6brJFe+GnY+EgPbk6ZGQ
3BebYhtF8GaV0nxvwuo77x/Py9auJ/GpsMiu/X1+mvoiBOv/2X/qkSsisRcOj/KK
NFtY2PwByVS5uCbMiogziUwthDyC3+6WVwW6LLv3xLfHTjuCvjHIInNzktHCgKQ5
ORAzI4JMPJ+GslWYHb4phowim57iaztXOoJwTdwJx4nLCgdNbOhdjsnvzqvHu7Ur
TkXWStAmzOVyyghqpZXjFaH3pO3JLF+l+/+sKAIuvtd7u+Nxe5AW0wdeRlN8NwdC
jNPElpzVmbUq4JUagEiuTDkHzsxHpFKVK7q4+63SM1N95R1NbdWhscdCb+ZAJzVc
oyi3B43njTOQ5yOf+1CceWxG1bQVs5ZufpsMljq4Ui0/1lvh+wjChP4kqKOJ2qxq
4RgqsahDYVvTH9w7jXbyLeiNdd8XM2w9U/t7y0Ff/9yi0GE44Za4rF2LN9d11TPA
mRGunUHBcnWEvgJBQl9nJEiU0Zsnvgc/ubhPgXRR4Xq37Z0j4r7g1SgEEzwxA57d
emyPxgcYxn/eR44/KJ4EBs+lVDR3veyJm+kXQ99b21/+jh5Xos1AnX5iItreGCc=
-----END CERTIFICATE-----
)EOF";

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

// Controle nao-bloqueante da piscada do LED branco ao receber pacote ESPNOW.
// O callback roda no contexto da task de WiFi/LWIP; um delay() ali dentro
// bloqueia essa task e pode causar perda de pacotes ou watchdog. Em vez de
// bloquear, o callback so liga o LED e marca ate quando ele deve ficar
// aceso; o loop() desliga quando o tempo passar.
unsigned long ledPiscaAte = 0;

// =======================
// CALLBACK ESPNOW
// =======================
void onDataRecv(const esp_now_recv_info_t *info, const uint8_t *incomingData, int len) {
  if (len == sizeof(dadosRecebidos)) {
    memcpy(&dadosRecebidos, incomingData, sizeof(dadosRecebidos));
    memcpy(&ultimoPacote, &dadosRecebidos, sizeof(dadosRecebidos));

    novoPacoteRecebido = true;
    ultimoRecebimento = millis();

    // Piscada rapida = recebeu pacote ESPNOW (nao-bloqueante: so liga o LED
    // e agenda o desligamento; quem desliga e o loop())
    digitalWrite(pinoLEDBranco, HIGH);
    ledPiscaAte = millis() + 80;
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

  // Fixamos a CA raiz em vez de chamar client.setInsecure(). setInsecure()
  // desativa toda a validacao do certificado do servidor, o que abre a
  // conexao a ataques man-in-the-middle (qualquer certificado, de qualquer
  // emissor, seria aceito). setCACert() valida a cadeia apresentada pelo
  // Supabase contra a raiz ISRG Root X1 acima.
  //
  // Ressalva honesta: supabaseUrl acima ainda e um placeholder
  // (SEU-PROJETO.supabase.co), entao esta cadeia foi confirmada contra o
  // dominio apex supabase.co, nao contra o projeto real que sera usado em
  // producao, e o handshake nunca foi testado em hardware (sem placa
  // disponivel no momento desta mudanca). Se o projeto real estiver atras
  // de um dominio customizado (ex.: Cloudflare na frente do Supabase), a
  // cadeia pode ser diferente.
  //
  // Se o Monitor Serial mostrar falha de conexao HTTPS (erro mbedTLS por
  // volta do connect/handshake), NAO volte para client.setInsecure() sem
  // deixar isso explicito no codigo e no commit. Em vez disso, rode
  // `openssl s_client -connect SEU-PROJETO.supabase.co:443 -showcerts`
  // contra o dominio real do projeto, confirme a cadeia apresentada e
  // troque o PEM de SUPABASE_ROOT_CA se ela for diferente da usada aqui.
  WiFiClientSecure client;
  client.setCACert(SUPABASE_ROOT_CA);

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

  // Desliga o LED branco da piscada de recebimento sem bloquear o loop
  // (contraparte nao-bloqueante do que era feito com delay() dentro do
  // callback onDataRecv)
  if (ledPiscaAte > 0 && millis() > ledPiscaAte) {
    digitalWrite(pinoLEDBranco, LOW);
    ledPiscaAte = 0;
  }

  if (ultimoRecebimento > 0 && millis() - ultimoRecebimento > tempoTimeout) {
    Serial.println("ALERTA: sem dados recebidos nos ultimos 5 segundos");

    digitalWrite(pinoLEDBranco, HIGH);
    delay(500);
    digitalWrite(pinoLEDBranco, LOW);
    delay(500);
  }
}
