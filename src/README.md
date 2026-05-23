# Projetos PlatformIO: Sender e Receiver

Este diretorio tem dois projetos separados para VS Code + PlatformIO:

- `Sender`: le o BMP085/BMP180, mostra no OLED e transmite via LoRa.
- `Receiver`: recebe LoRa, mostra no OLED e serve uma pagina web por WiFi.

## 1. Instalar o ambiente

1. Instale o VS Code.
2. No VS Code, instale a extensao **PlatformIO IDE**.
3. Reinicie o VS Code depois da instalacao.

## 2. Abrir o Sender

1. No VS Code, use **File > Open Folder...**.
2. Abra a pasta:

   ```txt
   platformio-projects/Sender
   ```

3. Aguarde o PlatformIO instalar a plataforma ESP32 e as bibliotecas.
4. Conecte a placa Sender no USB.
5. Clique em **PlatformIO > Upload** ou rode:

   ```bash
   pio run --target upload
   ```

6. Abra o monitor serial:

   ```bash
   pio device monitor
   ```

## 3. Abrir o Receiver

1. No VS Code, use **File > Open Folder...**.
2. Abra a pasta:

   ```txt
   platformio-projects/Receiver
   ```

3. Edite o WiFi em `src/main.cpp`:

   ```cpp
   const char* ssid     = "SEU_WIFI";
   const char* password = "SUA_SENHA";
   ```

4. Conecte a placa Receiver no USB.
5. Suba o sketch:

   ```bash
   pio run --target upload
   ```

6. Suba os arquivos web da pasta `data` para o LittleFS:

   ```bash
   pio run --target uploadfs
   ```

7. Abra o monitor serial:

   ```bash
   pio device monitor
   ```

8. Aperte `RST/EN` na placa e procure o IP:

   ```txt
   WiFi connected.
   IP address:
   192.168.x.x
   ```

9. Acesse no navegador:

   ```txt
   http://192.168.x.x
   ```

## 4. Melhorar a pagina web

No Receiver, edite os arquivos:

- `data/index.html`
- `data/style.css`
- `data/app.js`

Sempre que alterar arquivos dentro de `data`, rode:

```bash
pio run --target uploadfs
```

Se alterar `src/main.cpp`, rode:

```bash
pio run --target upload
```

## 5. Pontos importantes

- Sender e Receiver precisam usar a mesma frequencia LoRa:

  ```cpp
  #define BAND 868E6
  ```

- O receptor usa `LittleFS`, entao a pagina web nao depende mais de HTML embutido no codigo.
- Se sua placa nao compilar com `board = heltec_wifi_lora_32_V2`, abra o `platformio.ini` e teste `heltec_wifi_lora_32`.
