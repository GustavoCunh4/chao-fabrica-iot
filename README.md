# Chao de Fabrica IoT com ESP32-S3

Projeto IoT para monitoramento de chao de fabrica usando dois modulos ESP32-S3 Dev Module. Um ESP32 atua como emissor, fazendo a leitura dos sensores e enviando os dados via ESP-NOW. O outro ESP32 atua como receptor, recebe os pacotes via ESP-NOW, conecta no Wi-Fi e registra as leituras em uma tabela do Supabase.

## Arquivos do projeto

- `esp32_emissor_chao_fabrica/esp32_emissor_chao_fabrica.ino`: codigo do ESP32 emissor instalado no ponto de leitura dos sensores.
- `esp32_receptor_monitoramento/esp32_receptor_monitoramento.ino`: codigo do ESP32 receptor responsavel pelo monitoramento e envio ao Supabase.

## Especificacoes

### Placa

- 2x ESP32-S3 Dev Module.
- Programacao pelo Arduino IDE.
- Comunicacao local entre placas via ESP-NOW.
- Envio dos dados para o Supabase via Wi-Fi no ESP32 receptor.

### ESP32 emissor

Sensores e atuadores:

- Sensor ultrassonico nos pinos `TRIG = 5` e `ECHO = 18`.
- Sensor DHT22 no pino `15`.
- Sensor LDR analogico no pino `6`.
- Sensor PIR no pino `13`.
- LED verde no pino `4`.
- LED vermelho no pino `7`.

Configuracoes principais:

- Leitura a cada `2000 ms`.
- Tanque vazio: `20.0 cm`.
- Tanque cheio: `5.0 cm`.
- Alerta de nivel baixo abaixo de `20%`.
- Canal ESP-NOW configurado como `3`.
- MAC do receptor configurado no array `macReceptor`.

Dados enviados:

- Nivel de tinta em porcentagem.
- Temperatura.
- Umidade.
- Luminosidade.
- Presenca.
- Distancia medida.
- Alerta de nivel.
- Timestamp interno do ESP32.

### ESP32 receptor

Funcoes:

- Conecta ao Wi-Fi.
- Recebe pacotes ESP-NOW do emissor.
- Mostra os dados no Monitor Serial.
- Envia as leituras para a tabela `leituras_iot` no Supabase.
- Usa LED branco no pino `4` para indicar recebimento e timeout.

Credenciais ocultadas no codigo:

- SSID do Wi-Fi: `SEU_WIFI_SSID`.
- Senha do Wi-Fi: `SUA_SENHA_WIFI`.
- URL do projeto Supabase: `https://SEU-PROJETO.supabase.co`.
- Chave anon/public do Supabase: `SUA_SUPABASE_ANON_PUBLIC_KEY`.

## Tabela esperada no Supabase

O codigo envia os seguintes campos para a tabela `leituras_iot`:

- `nivel_tinta`
- `temperatura`
- `umidade`
- `luminosidade`
- `presenca`
- `distancia`
- `alerta_nivel`
- `timestamp_esp`

## Como usar no Arduino IDE

1. Abra o Arduino IDE.
2. Instale o suporte a placas ESP32 pelo Gerenciador de Placas.
3. Selecione a placa `ESP32S3 Dev Module`.
4. Instale a biblioteca `DHT sensor library` pelo Gerenciador de Bibliotecas.
5. Abra `esp32_emissor_chao_fabrica/esp32_emissor_chao_fabrica.ino`, selecione a porta do ESP32 emissor e envie o sketch.
6. Abra `esp32_receptor_monitoramento/esp32_receptor_monitoramento.ino`, preencha as credenciais ocultadas, selecione a porta do ESP32 receptor e envie o sketch.
7. No receptor, abra o Monitor Serial em `115200 baud` e verifique o canal do Wi-Fi exibido.
8. Se o canal do Wi-Fi do receptor for diferente de `3`, ajuste `canalWiFi` no codigo do emissor para o mesmo canal.
9. Se o MAC do receptor mudar, atualize o array `macReceptor` no codigo do emissor com o MAC exibido pelo ESP32 receptor/emissor no Monitor Serial.

## Observacoes

- A estrutura `struct_message` precisa permanecer igual nos dois sketches.
- O ESP-NOW e o Wi-Fi precisam operar no mesmo canal para a comunicacao funcionar corretamente.
- O receptor usa `client.setInsecure()` para simplificar a conexao HTTPS com o Supabase no ESP32.
- Antes de compartilhar o projeto, mantenha as credenciais reais fora dos arquivos versionados.
