# Chao de Fabrica IoT com ESP32-S3

Projeto de faculdade, mas resolvendo um problema real de chao de fabrica: acompanhar o nivel de um tanque (tinta, no meu caso, mas serve pra qualquer liquido em reservatorio fechado), temperatura, umidade, luminosidade e presenca de alguem na area, tudo em tempo real e sem precisar ninguem passar la fisicamente pra olhar. Duas placas ESP32-S3, uma leitora e uma que fala com a nuvem.

## Por que dois ESP32 e nao um so

Da pra fazer isso com um unico ESP32 direto no Wi-Fi. Eu preferi separar em emissor (le os sensores) e receptor (fala com o Supabase) e ligar os dois por ESP-NOW, por um motivo simples: o ponto onde os sensores ficam nem sempre e o ponto com melhor sinal de Wi-Fi, e manter uma associacao Wi-Fi ativa o tempo todo custa energia e estabilidade que eu nao queria pagar no no de leitura.

ESP-NOW e um protocolo peer-to-peer da Espressif que roda por cima do radio Wi-Fi mas sem handshake de associacao, sem DHCP, sem manter conexao TCP. O emissor manda um pacote pequeno (a struct com os dados) direto pro MAC do receptor e pronto — latencia baixa, menos overhead, e o emissor nem precisa saber que existe uma rede Wi-Fi por tras. Quem lida com a parte "internet" — abrir conexao, autenticar, falar HTTPS com o Supabase — e so o receptor. Ou seja: um salto ESP-NOW (local, leve) mais um salto Wi-Fi (o receptor pra internet), em vez de dois nos brigando por handshake TCP/IP em paralelo.

O preco dessa escolha e sincronizar canal: ESP-NOW e Wi-Fi station compartilham o mesmo radio, entao emissor e receptor precisam estar no mesmo canal Wi-Fi pra se enxergarem. Isso aparece nos passos de setup mais abaixo.

## Arquitetura

```
[Sensores] -> ESP32 emissor -> ESP-NOW -> ESP32 receptor -> Wi-Fi/HTTPS -> Supabase (tabela leituras_iot)
```

- `esp32_emissor_chao_fabrica/esp32_emissor_chao_fabrica.ino`: fica no ponto de leitura. Le ultrassonico, DHT22, LDR e PIR a cada 2 segundos, monta a struct e manda via ESP-NOW.
- `esp32_receptor_monitoramento/esp32_receptor_monitoramento.ino`: recebe o pacote, mostra no Monitor Serial, e faz o POST pra tabela `leituras_iot` no Supabase.

## A decisao de TLS: setCACert em vez de setInsecure

Confissao: a primeira versao usava `client.setInsecure()` pra fechar a conexao HTTPS com o Supabase sem validar certificado nenhum — funciona, compila rapido, e e exatamente o tipo de atalho que deixa a porta aberta pra um ataque man-in-the-middle (qualquer certificado, de qualquer origem, seria aceito sem reclamar).

Troquei por pinning da CA raiz: o codigo carrega o certificado do ISRG Root X1 (a raiz que a Let's Encrypt usa, e a Let's Encrypt e quem emite o certificado do Supabase) e valida a cadeia apresentada pelo servidor contra ele via `client.setCACert(...)`. Confirmei a cadeia ao vivo com `openssl s_client -connect supabase.co:443 -showcerts` e peguei o PEM oficial direto do site da Let's Encrypt. Pinning de raiz (diferente de pinning de folha/intermediario) e uma escolha de baixa manutencao porque raiz troca em escala de decada — essa e valida ate 2035 — enquanto certificado de folha da Let's Encrypt roda em ciclos de 60-90 dias.

Ressalva honesta que deixo tambem comentada no codigo: `supabaseUrl` no repositorio ainda e um placeholder, entao essa cadeia foi verificada contra o dominio apex `supabase.co`, nao contra o projeto real que vou apontar em producao, e nunca testei o handshake em hardware de verdade (nao tinha placa disponivel na hora dessa mudanca). Se um dia eu colocar um dominio customizado na frente do Supabase (Cloudflare, por exemplo), a cadeia pode mudar. Se isso quebrar na placa real — erro de mbedTLS por volta do connect — o caminho certo e rodar o mesmo `openssl s_client` contra o dominio real do projeto e trocar o PEM, nao voltar pro `setInsecure()` sem deixar isso escrito.

## Referencia tecnica

### Pinagem

| Placa | Sensor/atuador | Pino |
|---|---|---|
| Emissor | Ultrassonico TRIG | 5 |
| Emissor | Ultrassonico ECHO | 18 |
| Emissor | DHT22 | 15 |
| Emissor | LDR (analogico) | 6 |
| Emissor | PIR | 13 |
| Emissor | LED verde (nivel ok) | 4 |
| Emissor | LED vermelho (alerta) | 7 |
| Receptor | LED branco (recebimento/timeout) | 4 |

### Dados enviados (ESP-NOW -> Supabase)

Struct compartilhada entre os dois sketches, com os campos gravados na tabela `leituras_iot`:

| Campo na struct | Coluna no Supabase |
|---|---|
| `nivelTinta` | `nivel_tinta` |
| `temperatura` | `temperatura` |
| `umidade` | `umidade` |
| `luminosidade` | `luminosidade` |
| `presenca` | `presenca` |
| `distancia` | `distancia` |
| `alertaNivel` | `alerta_nivel` |
| `timestamp` | `timestamp_esp` |

Nivel do tanque e calculado a partir da distancia ultrassonica: tanque vazio em 20 cm, cheio em 5 cm, alerta de nivel baixo abaixo de 20%.

### Flashando os dois boards

1. No Arduino IDE, instale o suporte a placas ESP32 e selecione `ESP32S3 Dev Module` pras duas.
2. Instale a lib `DHT sensor library`.
3. Flash o emissor (`esp32_emissor_chao_fabrica.ino`) primeiro, sem precisar mexer em nada.
4. No receptor (`esp32_receptor_monitoramento.ino`), preencha `ssid`, `password`, `supabaseUrl` e `supabaseKey` com os valores reais antes de enviar.
5. Abra o Monitor Serial do receptor em 115200 baud depois do flash e anote o canal Wi-Fi que ele imprime.
6. Se o canal for diferente de 3, ajuste `canalWiFi` no codigo do emissor — os dois precisam estar no mesmo canal pra ESP-NOW funcionar.
7. Se o MAC do receptor mudar (troca de placa, por exemplo), atualize `macReceptor` no emissor com o MAC que aparece no Serial do proprio receptor.

## Observacoes

- A struct `struct_message` precisa ficar identica nos dois sketches — e o contrato binario entre emissor e receptor.
- O MAC hardcoded no emissor (`macReceptor`) e so o identificador local de pareamento ESP-NOW da outra placa, nao e segredo nenhum.
- As credenciais no repositorio (`SEU_WIFI_SSID`, `SUA_SENHA_WIFI`, URL e chave do Supabase) sao placeholders de proposito — preencha com os valores reais so localmente, nunca commitando.
