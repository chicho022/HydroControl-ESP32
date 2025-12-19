#include <Arduino.h>
#include <WiFi.h>
#include <WiFiUdp.h>

/* ===== UART STM32 ===== */
#define STM_RX   16
#define STM_TX   17
#define STM_BAUD 115200

HardwareSerial STM(2);

/* ===== WIFI ===== */
const char* WIFI_SSID = "Lobitos";
const char* WIFI_PASS = "123456789";

/* ===== UDP ===== */
WiFiUDP udp;
const uint16_t UDP_PORT_TX = 5005;   // ESP32 → GUI
const uint16_t UDP_PORT_RX = 5006;   // GUI → ESP32
IPAddress GUI_IP(192,168,1,8);     // IP DE TU PC

/* ===== CONFIGURACIÓN FIJA ===== */
static const int SP_CM = 9;
static const uint32_t SP_PERIOD_MS   = 2000;
static const uint32_t MODE_PERIOD_MS = 5000;

/* ===== VARIABLES ===== */
static char rxLine[160];
static uint16_t rxIdx = 0;

static uint32_t lastSPSend   = 0;
static uint32_t lastModeSend = 0;
static int mode = 0;

/* Últimos valores enviados a GUI */
static float lastNivel   = 0.0f;
static float lastControl = 0.0f;

/* ===== UTIL ===== */
static float x100_to_float(long v) {
  return (float)v / 100.0f;
}

/* ===== PROCESAR MENSAJES STM32 ===== */
static void processLine(const char *line)
{
  /* --- Telemetría --- */
  if (strncmp(line, "T,", 2) == 0) {
    long d = 0, sp = 0, u = 0, m = 0;

    if (sscanf(line, "T,%ld,%ld,%ld,%ld", &d, &sp, &u, &m) == 4) {

      lastNivel   = x100_to_float(d);
      lastControl = x100_to_float(u);

      /* Envío UDP a la GUI */
      char msg[64];
      snprintf(msg, sizeof(msg), "%.2f,%.2f", lastNivel, lastControl);

      udp.beginPacket(GUI_IP, UDP_PORT_TX);
      udp.print(msg);
      udp.endPacket();

      Serial.print("UDP → GUI: ");
      Serial.println(msg);
      return;
    }
  }

  /* --- ACKs --- */
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

  /* UART STM32 */
  STM.begin(STM_BAUD, SERIAL_8N1, STM_RX, STM_TX);

  /* WIFI */
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWiFi conectado");
  Serial.print("IP ESP32: ");
  Serial.println(WiFi.localIP());

  /* UDP */
  udp.begin(UDP_PORT_RX);

  Serial.println("ESP32 GATEWAY UART ↔ UDP");
  Serial.println("--------------------------------");

  /* SP inicial */
  STM.print("SP,");
  STM.print(SP_CM);
  STM.print("\n");
}

void loop()
{
  uint32_t now = millis();

  /* ===== UDP RX (GUI → ESP32) ===== */
  int packetSize = udp.parsePacket();
  if (packetSize) {
    char buf[64];
    int len = udp.read(buf, sizeof(buf) - 1);
    buf[len] = '\0';

    if (strncmp(buf, "SP:", 3) == 0) {
      STM.print("SP,");
      STM.print((int)atof(buf + 3));
      STM.print("\n");
    }
    else if (strncmp(buf, "MODE:", 5) == 0) {
      STM.print("MODE,");
      STM.print(buf + 5);
      STM.print("\n");
    }
  }

  /* ===== Reenvío periódico SP ===== */
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

  /* ===== Recepción UART STM32 ===== */
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
