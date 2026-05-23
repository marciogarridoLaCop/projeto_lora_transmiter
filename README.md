# Transmissor LoRa — Estação Meteorológica

> Projeto de transmissão de dados ambientais via rádio LoRa desenvolvido no **LaCop / UFF**
> *(Laboratório de Computação e Percepção — Universidade Federal Fluminense)*

---

## Sumário

- [Visão Geral](#visão-geral)
- [Hardware Necessário](#hardware-necessário)
- [Diagrama de Ligações](#diagrama-de-ligações)
- [Arquitetura do Sistema](#arquitetura-do-sistema)
- [Fluxo de Execução](#fluxo-de-execução)
- [Telas do Display OLED](#telas-do-display-oled)
- [Payload LoRa](#payload-lora)
- [Saída Serial](#saída-serial)
- [Estrutura do Projeto](#estrutura-do-projeto)
- [Bibliotecas Utilizadas](#bibliotecas-utilizadas)
- [Instalação e Configuração](#instalação-e-configuração)
- [Configuração do PlatformIO](#configuração-do-platformio)
- [Como Compilar e Gravar](#como-compilar-e-gravar)
- [Créditos](#créditos)

---

## Visão Geral

Este projeto implementa um **transmissor LoRa de dados meteorológicos** embarcado em uma placa **Heltec WiFi LoRa 32 V2**. O sistema lê temperatura, pressão atmosférica e altitude a partir do sensor **BMP085/BMP180**, exibe os valores em tempo real no display **OLED SSD1306** e transmite um pacote **JSON** via rádio **LoRa na frequência de 868 MHz** a cada 5 segundos.

### Características principais

| Característica        | Detalhe                              |
|-----------------------|--------------------------------------|
| Microcontrolador      | ESP32 (Heltec WiFi LoRa 32 V2)       |
| Sensor ambiental      | BMP085 / BMP180                      |
| Display               | OLED SSD1306 — 128 × 64 px           |
| Protocolo de rádio    | LoRa — 868 MHz (Europa / Brasil)     |
| Formato do payload    | JSON                                 |
| Intervalo de envio    | 5 segundos                           |
| Framework             | Arduino via PlatformIO               |
| Velocidade serial     | 115 200 baud                         |

---

## Hardware Necessário

| Componente                   | Quantidade |
|------------------------------|------------|
| Heltec WiFi LoRa 32 V2       | 1          |
| Sensor BMP085 ou BMP180      | 1          |
| Antena LoRa 868 MHz          | 1          |
| Cabo USB para programação    | 1          |

> **Atenção:** conecte sempre a antena antes de energizar o módulo LoRa para evitar danos ao amplificador de RF.

---

## Diagrama de Ligações

### Barramento I²C (compartilhado — OLED + BMP180)

```
Heltec WiFi LoRa 32 V2          BMP085 / BMP180
┌──────────────────────┐         ┌─────────────┐
│  GPIO 4  (SDA)  ─────┼─────────┤ SDA         │
│  GPIO 15 (SCL)  ─────┼─────────┤ SCL         │
│  3.3V           ─────┼─────────┤ VCC         │
│  GND            ─────┼─────────┤ GND         │
└──────────────────────┘         └─────────────┘

Heltec WiFi LoRa 32 V2          OLED SSD1306 (interno)
┌──────────────────────┐         ┌─────────────┐
│  GPIO 4  (SDA)  ─────┼─────────┤ SDA         │
│  GPIO 15 (SCL)  ─────┼─────────┤ SCL         │
│  GPIO 16 (RST)  ─────┼─────────┤ RST         │
│  3.3V           ─────┼─────────┤ VCC         │
│  GND            ─────┼─────────┤ GND         │
└──────────────────────┘         └─────────────┘
```

> O OLED e o BMP180 compartilham o mesmo barramento I²C (SDA=4, SCL=15). O endereço I²C do OLED é **0x3C**.

### Barramento SPI (módulo LoRa — interno à placa)

```
Heltec WiFi LoRa 32 V2          Módulo LoRa SX1276 (interno)
┌──────────────────────┐         ┌─────────────┐
│  GPIO 5  (SCK)  ─────┼─────────┤ SCK         │
│  GPIO 19 (MISO) ─────┼─────────┤ MISO        │
│  GPIO 27 (MOSI) ─────┼─────────┤ MOSI        │
│  GPIO 18 (SS)   ─────┼─────────┤ NSS         │
│  GPIO 14 (RST)  ─────┼─────────┤ RST         │
│  GPIO 26 (DIO0) ─────┼─────────┤ DIO0        │
└──────────────────────┘         └─────────────┘
                                       │
                                   Antena 868 MHz
```

### Tabela resumo de pinos

| GPIO | Função     | Periférico        |
|------|------------|-------------------|
| 4    | SDA        | OLED + BMP180     |
| 15   | SCL        | OLED + BMP180     |
| 16   | RST        | OLED              |
| 5    | SCK        | LoRa (SPI)        |
| 19   | MISO       | LoRa (SPI)        |
| 27   | MOSI       | LoRa (SPI)        |
| 18   | SS (CS)    | LoRa (SPI)        |
| 14   | RST        | LoRa              |
| 26   | DIO0 (IRQ) | LoRa              |

---

## Arquitetura do Sistema

```
┌───────────────────────────────────────────────────────────┐
│                  Heltec WiFi LoRa 32 V2                   │
│                                                           │
│  ┌─────────────┐   I²C    ┌──────────────────────────┐   │
│  │  BMP180     │◄────────►│                          │   │
│  │  Sensor     │          │        ESP32             │   │
│  │  Temp/Press │          │    (Firmware Arduino)    │   │
│  └─────────────┘          │                          │   │
│                           │  • Lê sensor             │   │
│  ┌─────────────┐   I²C    │  • Monta JSON            │   │
│  │  OLED       │◄────────►│  • Exibe no display      │   │
│  │  SSD1306    │          │  • Transmite via LoRa    │   │
│  │  128×64 px  │          │                          │   │
│  └─────────────┘          └──────────┬───────────────┘   │
│                                      │ SPI               │
│  ┌─────────────┐                     ▼                   │
│  │  LoRa       │◄────────────────────────────────────────┤
│  │  SX1276     │                                         │
│  │  868 MHz    │─────────► Antena ──────► Receptor LoRa  │
│  └─────────────┘                                         │
└───────────────────────────────────────────────────────────┘
```

---

## Fluxo de Execução

```
                        ┌──────────┐
                        │  SETUP   │
                        └────┬─────┘
                             │
                    ┌────────▼────────┐
                    │ Inicializa OLED │
                    │ (RST, Wire,     │
                    │  SSD1306)       │
                    └────────┬────────┘
                             │
                    ┌────────▼────────┐
                    │  Exibe logo     │
                    │  LaCop / UFF   │
                    │  (2 segundos)   │
                    └────────┬────────┘
                             │
                    ┌────────▼────────┐
                    │ Inicializa      │
                    │ BMP180 via I²C  │
                    └────────┬────────┘
                             │
                    ┌────────▼────────┐
                    │ Inicializa LoRa │
                    │ SPI + 868 MHz   │
                    └────────┬────────┘
                             │
                        ┌────▼─────┐
                        │  LOOP    │◄──────────────────┐
                        └────┬─────┘                   │
                             │                         │
                    ┌────────▼────────┐                │
                    │ Lê BMP180:      │                │
                    │ • Temperatura   │                │
                    │ • Pressão       │                │
                    │ • Altitude      │                │
                    └────────┬────────┘                │
                             │                         │
                    ┌────────▼────────┐                │
                    │ Monta payload   │                │
                    │ JSON com ID,    │                │
                    │ temp, press,    │                │
                    │ altitude        │                │
                    └────────┬────────┘                │
                             │                         │
                    ┌────────▼────────┐                │
                    │ Transmite via   │                │
                    │ LoRa 868 MHz    │                │
                    └────────┬────────┘                │
                             │                         │
                    ┌────────▼────────┐                │
                    │ Atualiza OLED   │                │
                    │ com status e    │                │
                    │ dados lidos     │                │
                    └────────┬────────┘                │
                             │                         │
                    ┌────────▼────────┐                │
                    │  Aguarda 5s     │────────────────►│
                    └─────────────────┘
```

---

## Telas do Display OLED

O display OLED 128×64 exibe informações em diferentes fases do programa:

### Tela 1 — Inicialização do Sistema

Exibida logo após o boot enquanto os periféricos são verificados:

```
┌────────────────────────────┐
│OLED OK                     │
│Teste BMP + LoRa            │
│                            │
│                            │
│                            │
│                            │
│                            │
│                            │
└────────────────────────────┘
```

---

### Tela 2 — Logo LaCop / UFF

Exibida por **2 segundos** após a inicialização do display. Apresenta o logotipo do **Laboratório de Computação e Percepção da UFF** em formato bitmap XBM (127×64 pixels):

```
┌────────────────────────────┐
│                            │
│    ██  ██  ████   ████     │
│    ██  ██  ██ ██  ██       │
│    ██  ██  ████   ██       │
│    ██  ██  ██     ██       │
│     ████   ██     ████     │
│                            │
│  LaCop  ─────────────────  │
└────────────────────────────┘
        (bitmap real da UFF)
```

---

### Tela 3 — Status do Sensor BMP180

Exibida durante a inicialização do sensor:

**Sucesso:**
```
┌────────────────────────────┐
│Sensor BMP OK               │
│                            │
│                            │
│                            │
│                            │
│                            │
│                            │
│                            │
└────────────────────────────┘
```

**Falha:**
```
┌────────────────────────────┐
│Erro BMP085/BMP180          │
│                            │
│                            │
│                            │
│                            │
│                            │
│                            │
│                            │
└────────────────────────────┘
```

---

### Tela 4 — Status do Rádio LoRa

Exibida durante a inicialização do rádio LoRa:

**Sucesso:**
```
┌────────────────────────────┐
│LoRa OK                     │
│                            │
└────────────────────────────┘
```

**Falha:**
```
┌────────────────────────────┐
│Erro LoRa                   │
│                            │
└────────────────────────────┘
```

---

### Tela 5 — Operação Normal (Loop Principal)

Exibida a cada 5 segundos após o envio bem-sucedido de um pacote:

```
┌────────────────────────────┐
│LoRa enviado                │  ← status do envio
│                            │
│ID: 42                      │  ← número sequencial do pacote
│                            │
│Temp: 24.50 C               │  ← temperatura (°C)
│                            │
│Press: 101325 Pa            │  ← pressão atmosférica (Pa)
│                            │
│Alt: 112.30 m               │  ← altitude calculada (m)
└────────────────────────────┘
```

Em caso de falha no envio:

```
┌────────────────────────────┐
│Falha LoRa                  │  ← indica falha na transmissão
│                            │
│ID: 43                      │
│...                         │
└────────────────────────────┘
```

---

## Payload LoRa

Cada pacote transmitido é um objeto **JSON** com os campos abaixo:

```json
{
  "id": 42,
  "temperatura": 24.50,
  "pressao": 101325,
  "altitude": 112.30
}
```

| Campo         | Tipo    | Unidade | Descrição                                        |
|---------------|---------|---------|--------------------------------------------------|
| `id`          | inteiro | —       | Contador sequencial, incrementado a cada envio   |
| `temperatura` | float   | °C      | Temperatura ambiente lida pelo BMP180            |
| `pressao`     | inteiro | Pa      | Pressão atmosférica absoluta                     |
| `altitude`    | float   | m       | Altitude calculada com pressão de referência     |

> **Pressão de referência:** o cálculo de altitude usa o valor `102791 Pa` como pressão ao nível do mar, configurável em `main.cpp:200`.

---

## Saída Serial

Monitorando a porta serial em **115 200 baud**, a saída típica é:

```
Teste BMP + LoRa + OLED, sem Heltec.begin
Inicializando OLED...
OLED OK
Inicializando Sensor BMP180...
Sensor BMP085/BMP180 OK
Inicializando Radio LoRa...
LoRa OK

----- Enviando LoRa -----
{"id":0,"temperatura":24.50,"pressao":101325,"altitude":112.30}
Pacote LoRa enviado com sucesso

----- Enviando LoRa -----
{"id":1,"temperatura":24.52,"pressao":101318,"altitude":112.67}
Pacote LoRa enviado com sucesso
```

---

## Estrutura do Projeto

```
Transmissor Lora/
│
├── src/
│   ├── main.cpp          # Código principal: setup, loop, inicializações
│   ├── logo.h            # Função logo() — exibe bitmap no OLED na inicialização
│   └── lacop.h           # Bitmap XBM do logo LaCop/UFF (127×64 px)
│
├── include/
│   └── README            # Orientações PlatformIO para headers globais
│
├── lib/
│   └── README            # Orientações PlatformIO para bibliotecas locais
│
├── test/
│   └── README            # Orientações PlatformIO para testes unitários
│
├── platformio.ini         # Configuração da plataforma e dependências
└── README.md              # Este arquivo
```

### Descrição dos arquivos principais

#### `src/main.cpp`

Arquivo central do firmware. Contém:

- **`inicializaDisplay()`** — Realiza o reset por hardware do OLED, inicializa o barramento I²C e configura o SSD1306.
- **`inicializaSensor()`** — Detecta o BMP085/BMP180 no barramento I²C compartilhado.
- **`inicializaLoRa()`** — Configura o SPI e inicializa o rádio LoRa na frequência de 868 MHz.
- **`enviaDados()`** — Lê o sensor, monta o JSON, transmite via LoRa e atualiza o display.
- **`mostraDisplay()`** — Renderiza os dados de telemetria no OLED.
- **`setup()`** — Sequência de inicialização dos periféricos.
- **`loop()`** — Chama `enviaDados()` a cada 5 segundos.

#### `src/logo.h`

Contém a função `logo()` que:
1. Carrega o bitmap da `lacop.h` (formato XBM little-endian).
2. Inverte a ordem dos bits de cada byte via `reverseBits()` para compatibilidade com a biblioteca Adafruit.
3. Desenha o logotipo centralizado no OLED via `drawBitmap()`.

#### `src/lacop.h`

Bitmap raw do logotipo LaCop/UFF em formato XBM:
- Dimensões: **127 × 64 pixels**
- Codificação: array de bytes `unsigned char` (little-endian, LSB first)

---

## Bibliotecas Utilizadas

| Biblioteca                       | Finalidade                                   |
|----------------------------------|----------------------------------------------|
| `sandeepmistry/LoRa`             | Driver do rádio SX1276 (LoRa)                |
| `adafruit/Adafruit BMP085 Library` | Leitura de temperatura, pressão e altitude |
| `adafruit/Adafruit GFX Library`  | Primitivas gráficas 2D para displays         |
| `adafruit/Adafruit SSD1306`      | Driver do display OLED SSD1306               |
| `arduino-libraries/NTPClient`    | Sincronização de tempo via NTP (futuro uso)  |

---

## Instalação e Configuração

### Pré-requisitos

- [Visual Studio Code](https://code.visualstudio.com/)
- [PlatformIO IDE Extension](https://platformio.org/install/ide?install=vscode)
- Driver USB-Serial (CP210x ou CH340 conforme o cabo utilizado)

### Passos

1. **Clone o repositório:**

```bash
git clone https://github.com/marciogarridoLaCop/projeto_lora_transmiter.git
cd projeto_lora_transmiter
```

2. **Abra no VS Code:**

```bash
code .
```

3. **Aguarde o PlatformIO** instalar automaticamente todas as dependências listadas em `platformio.ini`.

4. **Conecte a placa** via USB e verifique a porta serial gerada (ex: `/dev/cu.usbserial-0001` no macOS ou `COM3` no Windows).

5. **Ajuste a porta** em `platformio.ini` se necessário:

```ini
monitor_port = /dev/cu.usbserial-0001   ; macOS
; monitor_port = COM3                   ; Windows
```

---

## Configuração do PlatformIO

```ini
[env:heltec_wifi_lora_32_V2]
platform        = espressif32
board           = heltec_wifi_lora_32_V2
framework       = arduino

monitor_port    = /dev/cu.usbserial-0001
monitor_speed   = 115200
upload_speed    = 921600

board_build.filesystem = littlefs

lib_deps =
  sandeepmistry/LoRa
  adafruit/Adafruit BMP085 Library
  adafruit/Adafruit GFX Library
  adafruit/Adafruit SSD1306
  arduino-libraries/NTPClient
```

| Parâmetro               | Valor                     | Descrição                          |
|-------------------------|---------------------------|------------------------------------|
| `platform`              | `espressif32`             | Família ESP32                      |
| `board`                 | `heltec_wifi_lora_32_V2`  | Variante exata da placa            |
| `framework`             | `arduino`                 | Framework Arduino                  |
| `monitor_speed`         | `115200`                  | Baud rate do monitor serial        |
| `upload_speed`          | `921600`                  | Alta velocidade de gravação        |
| `board_build.filesystem`| `littlefs`                | Sistema de arquivos na flash       |

---

## Como Compilar e Gravar

### Via PlatformIO (VS Code)

| Ação          | Botão na barra inferior | Atalho         |
|---------------|-------------------------|----------------|
| Compilar      | ✔ Build                 | `Ctrl+Alt+B`   |
| Gravar        | → Upload                | `Ctrl+Alt+U`   |
| Monitor serial| Serial Monitor          | `Ctrl+Alt+S`   |

### Via terminal PlatformIO

```bash
# Compilar
pio run

# Compilar e gravar
pio run --target upload

# Abrir monitor serial
pio device monitor
```

---

## Ajuste da Pressão de Referência

A altitude é calculada com base em uma pressão de referência ao nível do mar. O valor padrão no código é `102791 Pa`, adequado para a localidade do laboratório. Para ajustar ao seu local:

```cpp
// src/main.cpp:200
float altitude = bmp.readAltitude(102791);
//                                ^^^^^^
//                  Substitua pelo valor local em Pascal
```

Você pode obter a pressão de referência de estações meteorológicas próximas (INMET, Weather Underground, etc.).

---

## Créditos

Desenvolvido no **LaCop — Laboratório de Computação e Percepção**
**Universidade Federal Fluminense (UFF)**

| Responsável      | Contato                        |
|------------------|--------------------------------|
| Marcio Garrido   | marciogarrido@id.uff.br        |

---

*Projeto acadêmico de pesquisa em redes de sensores sem fio de longo alcance (LPWAN).*
