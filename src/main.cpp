#include <Arduino.h>

// ===============================
// UART STM32
// ===============================
#define STM_RX   16
#define STM_TX   17
#define STM_BAUD 115200

HardwareSerial STM(2);

// ===============================
// Timers
// ===============================
static uint32_t lastSend = 0;
static const uint32_t SEND_PERIOD_MS = 5000;

// ===============================
// RX buffer
// ===============================
static char rxLine[128];
static uint8_t rxIdx = 0;

// ===============================
// Procesamiento de líneas STM32
// ===============================
static void processLine(const char *line)
{
  // Caso: D,<dist>,SP=<sp_x100>
  if (strncmp(line, "D,", 2) == 0) {
    int dist = 0;
    long sp_x100 = 0;

    if (sscanf(line, "D,%d,SP=%ld", &dist, &sp_x100) == 2) {
      float sp = sp_x100 / 100.0f;

      Serial.print("Nivel(cm)=");
      Serial.print(dist);
      Serial.print("   SP=");
      Serial.println(sp, 2);
      return;
    }
  }

  // Caso: SP_SET,SP=1000
  if (strncmp(line, "SP_SET,SP=", 10) == 0) {
    long sp_x100 = atol(line + 10);
    float sp = sp_x100 / 100.0f;

    Serial.print(">> Setpoint actualizado a ");
    Serial.println(sp, 2);
    return;
  }

  // Caso: BOOT,SP=1000
  if (strncmp(line, "BOOT,SP=", 8) == 0) {
    long sp_x100 = atol(line + 8);
    float sp = sp_x100 / 100.0f;

    Serial.print("[BOOT] SP=");
    Serial.println(sp, 2);
    return;
  }

  // Caso: SP_NOW,SP=1000
  if (strncmp(line, "SP_NOW,SP=", 10) == 0) {
    long sp_x100 = atol(line + 10);
    float sp = sp_x100 / 100.0f;

    Serial.print("[SP_NOW] SP=");
    Serial.println(sp, 2);
    return;
  }

  // Caso: HB,SP=1000
  if (strncmp(line, "HB,SP=", 6) == 0) {
    long sp_x100 = atol(line + 6);
    float sp = sp_x100 / 100.0f;

    Serial.print("[HB] SP=");
    Serial.println(sp, 2);
    return;
  }

  // Caso: ACK_SP,1000
  if (strncmp(line, "ACK_SP,", 7) == 0) {
    long sp_x100 = atol(line + 7);
    float sp = sp_x100 / 100.0f;

    Serial.print("ACK STM32 SP=");
    Serial.println(sp, 2);
    return;
  }

  // Debug genérico
  Serial.print("STM32: [");
  Serial.print(line);
  Serial.println("]");
}

// ===============================
// Setup
// ===============================
void setup() {
  Serial.begin(115200);
  delay(300);

  STM.begin(STM_BAUD, SERIAL_8N1, STM_RX, STM_TX);

  Serial.println("ESP32 UART listo (PlatformIO)");
  Serial.println("Enviando SP alternado cada 5 s");
  Serial.println("--------------------------------");
}

// ===============================
// Loop
// ===============================
void loop() {
  uint32_t now = millis();

  /* ---- Envío periódico de SP ---- */
  static bool tog = false;

  if (now - lastSend >= SEND_PERIOD_MS) {
    lastSend = now;
    tog = !tog;

    int sp = tog ? 10 : 16;
    STM.printf("SP,%d\n", sp);

    Serial.print(">> SP,");
    Serial.println(sp);
  }

  /* ---- Recepción STM32 ---- */
  while (STM.available()) {
    char c = (char)STM.read();

    if (c == '\n' || c == '\r') {
      if (rxIdx > 0) {
        rxLine[rxIdx] = 0;
        processLine(rxLine);
        rxIdx = 0;
      }
    } else {
      if (rxIdx < sizeof(rxLine) - 1) {
        rxLine[rxIdx++] = c;
      } else {
        rxIdx = 0; // overflow → reset
      }
    }
  }
}
