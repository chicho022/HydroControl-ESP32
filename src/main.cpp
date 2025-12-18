#include <Arduino.h>

// ===============================
// UART STM32
// ===============================
#define STM_RX   16
#define STM_TX   17
#define STM_BAUD 115200

HardwareSerial STM(2);

// ===============================
// Buffers y estado
// ===============================
static char rxLine[128];
static uint8_t rxIdx = 0;

static uint32_t lastModeSend = 0;
static const uint32_t MODE_PERIOD_MS = 5000;
static int mode = 0;

// ===============================
// Utilidades
// ===============================
static float x100_to_float(long v) {
  return (float)v / 100.0f;
}

// ===============================
// Procesamiento de mensajes STM32
// ===============================
static void processLine(const char *line)
{
  // ACK_MODE,<m>
  if (strncmp(line, "ACK_MODE,", 9) == 0) {
    int m = atoi(line + 9);
    Serial.print("ACK STM32 MODE=");
    Serial.println(m);
    return;
  }

  // MODE_SET / MODE_NOW / HB
  if (strncmp(line, "MODE_SET", 8) == 0 ||
      strncmp(line, "MODE_NOW", 8) == 0 ||
      strncmp(line, "HB", 2) == 0) {

    long sp_x100 = 0;
    long mode_i = 0;

    if (sscanf(line, "%*[^,],SP=%ld,MODE=%ld", &sp_x100, &mode_i) == 2) {
      Serial.print("[");
      Serial.print(strncmp(line, "HB", 2) == 0 ? "HB" : "MODE");
      Serial.print("] SP=");
      Serial.print(x100_to_float(sp_x100), 2);
      Serial.print("   MODE=");
      Serial.println((int)mode_i);
      return;
    }
  }

  // D,<dist>,SP=<x100>,MODE=<m>
  if (strncmp(line, "D,", 2) == 0) {
    int dist = 0;
    long sp_x100 = 0;
    long mode_i = 0;

    if (sscanf(line, "D,%d,SP=%ld,MODE=%ld", &dist, &sp_x100, &mode_i) == 3) {
      Serial.print("Nivel(cm)=");
      Serial.print(dist);
      Serial.print("   SP=");
      Serial.print(x100_to_float(sp_x100), 2);
      Serial.print("   MODE=");
      Serial.println((int)mode_i);
      return;
    }
  }

  // Mensaje genérico
  Serial.print("STM32: ");
  Serial.println(line);
}

// ===============================
// Setup
// ===============================
void setup() {
  Serial.begin(115200);
  delay(300);

  STM.begin(STM_BAUD, SERIAL_8N1, STM_RX, STM_TX);

  Serial.println("ESP32 UART listo (PlatformIO)");
  Serial.println("Envio: SP una vez y luego alterno MODE cada 5s");
  Serial.println("--------------------------------");

  STM.print("SP,10\n");
  Serial.println(">> SP,10");
}

// ===============================
// Loop
// ===============================
void loop() {
  uint32_t now = millis();

  // ---- Envío periódico de MODE ----
  if (now - lastModeSend >= MODE_PERIOD_MS) {
    lastModeSend = now;
    mode = 1 - mode;

    STM.print("MODE,");
    STM.print(mode);
    STM.print("\n");

    Serial.print(">> MODE,");
    Serial.println(mode);
  }

  // ---- Recepción STM32 ----
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
        rxIdx++;
        rxLine[rxIdx - 1] = c;
      } else {
        rxIdx = 0; // overflow → reset
      }
    }
  }
}
