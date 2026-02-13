/*
 * TRANSMISOR CAN BUS - Reed Switch + LDR
 * Arduino con MCP2515
 * Envía estado de puerta y nivel de luz
 */

#include <SPI.h>
#include <mcp2515.h>

// ===== CONFIGURACIÓN DE PINES =====
const int reedPin = 2;     // Reed Switch (digital)
const int ldrPin = A0;     // LDR (analógico)
const int csPin = 10;      // Chip Select para MCP2515

// ===== CONFIGURACIÓN CAN BUS =====
MCP2515 mcp2515(csPin);
struct can_frame canMsg;

// ===== VARIABLES =====
int umbralLuz = 500;       // Ajusta este valor según tu ambiente
unsigned long lastSend = 0;
const unsigned long sendInterval = 500;  // Enviar cada 500ms

void setup() {
  Serial.begin(9600);
  Serial.println(F("Iniciando Transmisor CAN..."));
  
  // Configurar pines
  pinMode(reedPin, INPUT_PULLUP);  // Pull-up interno ACTIVADO
  pinMode(ldrPin, INPUT);
  
  // Inicializar MCP2515
  SPI.begin();
  mcp2515.reset();
  
  // Configurar velocidad CAN (MUY IMPORTANTE: 125KBPS, no KBTPS)
  mcp2515.setBitrate(CAN_125KBPS, MCP_8MHZ);
  mcp2515.setNormalMode();
  
  Serial.println(F("✅ Transmisor listo!"));
  Serial.println(F("Enviando datos cada 500ms..."));
  Serial.println();
}

void loop() {
  // Enviar datos cada intervalo
  if (millis() - lastSend >= sendInterval) {
    
    // ===== LEER SENSORES =====
    // Reed Switch: LOW = imán presente (puerta cerrada)
    // Con PULLUP: digitalRead = LOW cuando hay imán
    bool imanPresente = (digitalRead(reedPin) == LOW);
    
    // LDR: valor analógico 0-1023
    int valorLDR = analogRead(ldrPin);
    
    // ===== DETERMINAR ESTADO DE PUERTA =====
    // PUERTA CERRADA: imán presente
    // PUERTA ABIERTA: imán ausente
    bool puertaAbierta = !imanPresente;  // true = abierta, false = cerrada
    
    // ===== PREPARAR MENSAJE CAN =====
    canMsg.can_id = 0x100;        // ID único para este transmisor
    canMsg.can_dlc = 4;           // 4 bytes de datos
    
    // Byte 0: Estado de la puerta (0 = cerrada, 1 = abierta)
    canMsg.data[0] = puertaAbierta ? 1 : 0;
    
    // Byte 1: Estado del reed switch (0 = sin imán, 1 = con imán)
    canMsg.data[1] = imanPresente ? 1 : 0;
    
    // Byte 2: Valor LDR (0-255, version reducida)
    canMsg.data[2] = map(valorLDR, 0, 1023, 0, 255);
    
    // Byte 3: Umbral de luz (0-255, version reducida)
    canMsg.data[3] = map(umbralLuz, 0, 1023, 0, 255);
    
    // ===== ENVIAR MENSAJE CAN =====
    mcp2515.sendMessage(&canMsg);
    
    // ===== MONITOR SERIAL =====
    Serial.print("📤 ENVIADO | ");
    Serial.print("Puerta: ");
    Serial.print(puertaAbierta ? "ABIERTA" : "CERRADA");
    Serial.print(" | Reed: ");
    Serial.print(imanPresente ? "IMAN✓" : "IMAN✗");
    Serial.print(" | LDR: ");
    Serial.print(valorLDR);
    
    // Indicador de luz
    if (valorLDR < umbralLuz) {
      Serial.print(" (LUZ ENCENDIDA)");
    } else {
      Serial.print(" (LUZ APAGADA)");
    }
    Serial.println();
    
    lastSend = millis();
  }
  
  delay(10);  // Pequeña pausa para estabilidad
}
