#include <Arduino.h>

#define STM_RX   16
#define STM_TX   17
#define STM_BAUD 115200

HardwareSerial STM(2);

/* ===== CONFIGURACIÓN FIJA ===== */
static const int SP_CM = 12;              // setpoint fijo
static const uint32_t SP_PERIOD_MS = 2000;
static const uint32_t MODE_PERIOD_MS = 5000;

/* ===== VARIABLES ===== */
static char rxLine[160];
static uint16_t rxIdx = 0;

static uint32_t lastSPSend   = 0;
static uint32_t lastModeSend = 0;
static int mode = 0;

static float x100_to_float(long v) {
  return (float)v / 100.0f;
}

/* ===== PROCESAR MENSAJES STM32 ===== */
static void processLine(const char *line)
{
  if (strncmp(line, "T,", 2) == 0) {
    long d = 0, sp = 0, u = 0, m = 0;

    if (sscanf(line, "T,%ld,%ld,%ld,%ld", &d, &sp, &u, &m) == 4) {
      Serial.print("Nivel(cm)=");
      Serial.print(x100_to_float(d), 2);
      Serial.print("   SP=");
      Serial.print(x100_to_float(sp), 2);
      Serial.print("   u=");
      Serial.print(x100_to_float(u), 2);
      Serial.print("   MODE=");
      Serial.println((int)m);
      return;
    }
  }

  if (strncmp(line, "ACK_SP,", 7) == 0) {
    Serial.print("ACK SP=");
    Serial.println(x100_to_float(atol(line + 7)), 2);
    return;
  }

  if (strncmp(line, "ACK_MODE,", 9) == 0) {
    Serial.print("ACK MODE=");
    Serial.println(atoi(line + 9));
    return;
  }

  Serial.print("STM32: ");
  Serial.println(line);
}

void setup()
{
  Serial.begin(115200);
  delay(300);

  STM.begin(STM_BAUD, SERIAL_8N1, STM_RX, STM_TX);

  Serial.println("ESP32 AUTO UART");
  Serial.println("SP fijo y cambio automatico de modo");
  Serial.println("--------------------------------");

  // SP inicial
  STM.print("SP,");
  STM.print(SP_CM);
  STM.print("\n");
}

void loop()
{
  uint32_t now = millis();

  /* ===== Reenvío periódico de SP ===== */
  if (now - lastSPSend >= SP_PERIOD_MS) {
    lastSPSend = now;
    STM.print("SP,");
    STM.print(SP_CM);
    STM.print("\n");
  }

  /* ===== Cambio automático de modo ===== */
  if (now - lastModeSend >= MODE_PERIOD_MS) {
    lastModeSend = now;
    mode = 1 - mode;
    STM.print("MODE,");
    STM.print(mode);
    STM.print("\n");

    Serial.print(">> MODE cambiado a ");
    Serial.println(mode);
  }

  /* ===== Recepción UART ===== */
  while (STM.available()) {
    char c = (char)STM.read();

    if (c == '\n' || c == '\r') {
      if (rxIdx > 0) {
        rxLine[rxIdx] = '\0';
        processLine(rxLine);
        rxIdx = 0;
      }
    } else {
      if (rxIdx < sizeof(rxLine) - 1) {
        rxLine[rxIdx++] = c;
      } else {
        rxIdx = 0; // overflow seguro
      }
    }
  }
}
