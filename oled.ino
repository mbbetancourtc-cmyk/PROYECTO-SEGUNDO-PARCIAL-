/*
 * RECEPTOR CAN BUS - Pantalla OLED
 * Arduino con MCP2515 y OLED SSD1306
 * Muestra estado de puerta y nivel de luz
 */

#include <SPI.h>
#include <mcp2515.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// ===== CONFIGURACIÓN PANTALLA OLED =====
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define OLED_ADDR 0x3C  // Dirección I2C común (prueba 0x3D si no funciona)

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// ===== CONFIGURACIÓN CAN BUS =====
const int csPin = 10;
MCP2515 mcp2515(csPin);
struct can_frame canMsg;

// ===== VARIABLES DE ESTADO =====
bool puertaAbierta = false;
bool imanPresente = false;
int valorLDR = 0;
int umbralLuz = 500;

// ===== VARIABLES PARA ANIMACIÓN =====
bool parpadeo = false;
unsigned long lastBlink = 0;
const unsigned long blinkInterval = 500;

void setup() {
  Serial.begin(9600);
  Serial.println(F("Iniciando Receptor CAN..."));
  
  // ===== INICIALIZAR PANTALLA OLED =====
  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
    Serial.println(F("❌ ERROR: No se encontró OLED en 0x3C"));
    Serial.println(F("Prueba con dirección 0x3D"));
    while (true) {  // Detener ejecución
      delay(1000);
      Serial.print(".");
    }
  }
  
  // Mostrar pantalla de inicio
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(20, 20);
  display.println(F("RECEPTOR CAN"));
  display.setCursor(15, 35);
  display.println(F("Puerta Inteligente"));
  display.display();
  delay(2000);
  
  // ===== INICIALIZAR MCP2515 =====
  SPI.begin();
  mcp2515.reset();
  
  // MISMA velocidad que el transmisor: 125KBPS
  if (mcp2515.setBitrate(CAN_125KBPS, MCP_8MHZ) == MCP2515::ERROR_OK) {
    Serial.println(F("✅ MCP2515 configurado correctamente"));
  } else {
    Serial.println(F("❌ ERROR: Fallo en MCP2515"));
  }
  
  mcp2515.setNormalMode();
  
  Serial.println(F("✅ Receptor listo! Esperando datos..."));
  Serial.println();
}

void loop() {
  // ===== LEER MENSAJES CAN =====
  if (mcp2515.readMessage(&canMsg) == MCP2515::ERROR_OK) {
    
    // Verificar que sea el ID correcto (0x100)
    if (canMsg.can_id == 0x100) {
      
      // Extraer datos del mensaje
      puertaAbierta = (canMsg.data[0] == 1);
      imanPresente = (canMsg.data[1] == 1);
      valorLDR = map(canMsg.data[2], 0, 255, 0, 1023);
      umbralLuz = map(canMsg.data[3], 0, 255, 0, 1023);
      
      // ===== MONITOR SERIAL =====
      Serial.print("📥 RECIBIDO | ");
      Serial.print("Puerta: ");
      Serial.print(puertaAbierta ? "ABIERTA" : "CERRADA");
      Serial.print(" | Reed: ");
      Serial.print(imanPresente ? "IMAN✓" : "IMAN✗");
      Serial.print(" | LDR: ");
      Serial.print(valorLDR);
      
      if (valorLDR < umbralLuz) {
        Serial.println(" (LUZ ON)");
      } else {
        Serial.println(" (LUZ OFF)");
      }
      
      // Actualizar pantalla OLED
      actualizarOLED();
    }
  }
  
  // Actualizar animación de parpadeo (solo si puerta abierta)
  if (puertaAbierta && millis() - lastBlink >= blinkInterval) {
    lastBlink = millis();
    parpadeo = !parpadeo;
    actualizarOLED();  // Refrescar con parpadeo
  }
  
  delay(10);
}

// ===== FUNCIÓN PARA ACTUALIZAR PANTALLA OLED =====
void actualizarOLED() {
  display.clearDisplay();
  
  // ----- TÍTULO -----
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println(F("ESTADO DE PUERTA"));
  display.drawLine(0, 10, SCREEN_WIDTH, 10, SSD1306_WHITE);
  
  // ----- ESTADO DE PUERTA (GRANDE) -----
  display.setTextSize(2);
  
  if (puertaAbierta) {
    // PUERTA ABIERTA
    if (parpadeo) {
      display.setTextColor(SSD1306_WHITE);
      display.setCursor(20, 18);
      display.println(F("ABIERTA"));
      
      // Indicador de alerta
      display.fillCircle(110, 25, 5, SSD1306_WHITE);
      display.setTextSize(1);
      display.setCursor(105, 35);
      display.println(F("!"));
    } else {
      display.setTextColor(SSD1306_WHITE);
      display.setCursor(20, 18);
      display.println(F("ABIERTA"));
    }
    
    // Ícono de puerta abierta
    display.drawRect(20, 40, 20, 18, SSD1306_WHITE);
    display.fillRect(22, 42, 16, 14, SSD1306_BLACK);
    display.drawLine(30, 40, 30, 58, SSD1306_WHITE);
    
  } else {
    // PUERTA CERRADA
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(15, 18);
    display.println(F("CERRADA"));
    
    // Ícono de puerta cerrada con candado
    display.drawRect(20, 40, 20, 18, SSD1306_WHITE);
    display.fillRect(22, 42, 16, 14, SSD1306_WHITE);
    
    // Candado
    display.fillRect(35, 45, 6, 8, SSD1306_BLACK);
    display.fillCircle(38, 43, 3, SSD1306_BLACK);
    display.fillCircle(38, 43, 2, SSD1306_WHITE);
  }
  
  // ----- INFORMACIÓN LDR -----
  display.setTextSize(1);
  display.setCursor(60, 40);
  display.print(F("LDR:"));
  
  // Barra de nivel de luz
  int barWidth = map(valorLDR, 0, 1023, 0, 50);
  display.drawRect(60, 50, 50, 8, SSD1306_WHITE);
  display.fillRect(61, 51, barWidth, 6, SSD1306_WHITE);
  
  // Indicador de luz ON/OFF
  display.setCursor(60, 25);
  if (valorLDR < umbralLuz) {
    display.print(F("LUZ ON"));
  } else {
    display.print(F("LUZ OFF"));
  }
  
  // ----- ESTADO REED -----
  display.setCursor(60, 15);
  display.print(F("IMAN:"));
  if (imanPresente) {
    display.print(F("SI"));
  } else {
    display.print(F("NO"));
  }
  
  display.display();
}
