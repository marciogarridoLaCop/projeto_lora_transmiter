/*
  * Teste de leitura do sensor BMP085/BMP180 e envio dos dados por LoRa.
  * O display OLED é usado para mostrar o status do sistema e os dados lidos.
  * 
  * Conexões:
  * - Sensor BMP085/BMP180: SDA=4, SCL=15 (compartilhado com o OLED)
  * - OLED SSD1306: SDA=4, SCL=15, RST=16
  * - LoRa: SCK=5, MISO=19, MOSI=27, SS=18, RST=14, DIO0=26
  * 
  * Bibliotecas usadas:
  * - Adafruit_BMP085
  * - Adafruit_GFX
  * - Adafruit_SSD1306
  * - LoRa
*/


#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>
#include <LoRa.h>
#include <Adafruit_BMP085.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <logo.h>

// BMP085/BMP180
#define SDA_PIN 4
#define SCL_PIN 15

// OLED Heltec WiFi LoRa 32 V2
#define OLED_SDA 4
#define OLED_SCL 15
#define OLED_RST 16
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

// LoRa Heltec WiFi LoRa 32 V2
#define LORA_SCK 5
#define LORA_MISO 19
#define LORA_MOSI 27
#define LORA_SS 18
#define LORA_RST 14
#define LORA_DIO0 26

#define BAND 868E6

Adafruit_BMP085 bmp;
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RST);

bool sensorOK = false;
bool loraOK = false;
bool displayOK = false;

int readingID = 0;

void inicializaDisplay() {
  Serial.println("Inicializando OLED...");

  pinMode(OLED_RST, OUTPUT);
  digitalWrite(OLED_RST, LOW);
  delay(20);
  digitalWrite(OLED_RST, HIGH);
  delay(20);

  // Mantem o mesmo barramento I2C que ja funcionou com o sensor
  Wire.begin(OLED_SDA, OLED_SCL, 100000);
  delay(100);

  displayOK = display.begin(SSD1306_SWITCHCAPVCC, 0x3C, false, false);

  if (!displayOK) {
    Serial.println("ERRO: OLED SSD1306 nao inicializou");
    return;
  }

  display.clearDisplay();
  display.setTextColor(WHITE);
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("OLED OK");
  display.println("Teste BMP + LoRa");
  display.display();

  Serial.println("OLED OK");
}

void inicializaSensor() {
  Serial.println("Inicializando Sensor BMP180...");

  // Nao chama Wire.begin de novo se o display ja inicializou o barramento.
  // Sensor e OLED estao no mesmo I2C: SDA=4, SCL=15.
  sensorOK = bmp.begin(BMP085_STANDARD, &Wire);

  if (!sensorOK) {
    Serial.println("ERRO: BMP085/BMP180 nao encontrado");

    if (displayOK) {
      display.clearDisplay();
      display.setCursor(0, 0);
      display.println("Erro BMP085/BMP180");
      display.display();
    }

    return;
  }

  Serial.println("Sensor BMP085/BMP180 OK");

  if (displayOK) {
    display.clearDisplay();
    display.setCursor(0, 0);
    display.println("Sensor BMP OK");
    display.display();
  }
}

void inicializaLoRa() {
  Serial.println("Inicializando Radio LoRa...");

  SPI.begin(LORA_SCK, LORA_MISO, LORA_MOSI, LORA_SS);
  LoRa.setPins(LORA_SS, LORA_RST, LORA_DIO0);

  loraOK = LoRa.begin(BAND);

  if (!loraOK) {
    Serial.println("ERRO: LoRa nao inicializou");

    if (displayOK) {
      display.clearDisplay();
      display.setCursor(0, 0);
      display.println("Erro LoRa");
      display.display();
    }

    return;
  }

  Serial.println("LoRa OK");

  if (displayOK) {
    display.clearDisplay();
    display.setCursor(0, 0);
    display.println("LoRa OK");
    display.display();
  }
}

void mostraDisplay(float temperatura, int32_t pressao, float altitude, String dados, bool enviado) {
  if (!displayOK) {
    return;
  }

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);

  display.setCursor(0, 0);
  if (enviado) {
    display.println("LoRa enviado");
  } else {
    display.println("Falha LoRa");
  }

  display.setCursor(0, 12);
  display.print("ID: ");
  display.println(readingID);

  display.setCursor(0, 24);
  display.print("Temp: ");
  display.print(temperatura);
  display.println(" C");

  display.setCursor(0, 36);
  display.print("Press: ");
  display.print(pressao);
  display.println(" Pa");

  display.setCursor(0, 48);
  display.print("Alt: ");
  display.print(altitude);
  display.println(" m");

  display.display();
}

void enviaDados() {
  if (!sensorOK) {
    Serial.println("Sensor indisponivel, nao enviando");
    return;
  }

  if (!loraOK) {
    Serial.println("LoRa indisponivel, nao enviando");
    return;
  }

  float temperatura = bmp.readTemperature();
  int32_t pressao = bmp.readPressure();
  float altitude = bmp.readAltitude(102791);

  String dados = "{";
  dados += "\"id\":" + String(readingID);
  dados += ",\"temperatura\":" + String(temperatura);
  dados += ",\"pressao\":" + String(pressao);
  dados += ",\"altitude\":" + String(altitude);
  dados += "}";

  Serial.println();
  Serial.println("----- Enviando LoRa -----");
  Serial.println(dados);

  LoRa.beginPacket();
  LoRa.print(dados);
  int resultado = LoRa.endPacket();

  bool enviado = resultado == 1;

  if (enviado) {
    Serial.println("Pacote LoRa enviado com sucesso");
  } else {
    Serial.println("Falha ao enviar pacote LoRa");
  }

  mostraDisplay(temperatura, pressao, altitude, dados, enviado);

  readingID++;
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println();
  Serial.println("Teste BMP + LoRa + OLED, sem Heltec.begin");

  inicializaDisplay();
  delay(500);

  logo();
  delay(2000);

  inicializaSensor();
  delay(500);

  inicializaLoRa();
  delay(500);
}

void loop() {
  enviaDados();
  delay(5000);
}