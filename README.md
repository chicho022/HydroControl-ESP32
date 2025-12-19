Este proyecto implementa una pasarela de comunicaciones basada en ESP32, cuyo objetivo es conectar una STM32 con una interfaz gráfica en PC, utilizando:

UART para la comunicación cableada con la STM32

UDP sobre WiFi para la comunicación inalámbrica con la GUI

La ESP32 no realiza control, sino que actúa exclusivamente como intermediario, retransmitiendo comandos y datos entre ambos extremos.

🧠 Arquitectura del sistema
PC (GUI Python)
   │
   │  UDP (WiFi)
   ▼
ESP32  ←──────── UART ───────→  STM32
   │
   └── Pasarela de comunicaciones


La GUI permite al usuario:

Visualizar nivel y señal de control

Enviar setpoint (SP) y modo de control

La STM32:

Ejecuta el control del sistema

Envía telemetría periódica

La ESP32:

Traduce y reenvía mensajes entre ambos

🔌 Tecnologías utilizadas

ESP32 (Framework Arduino / PlatformIO)

UART (comunicación serial cableada)

WiFi

UDP (User Datagram Protocol)

Python + Tkinter (GUI externa)

📥 Formato de comunicación
📤 STM32 → ESP32 (UART)

Mensajes de telemetría:

T,d,sp,u,m


Donde:

d = nivel (×100)

sp = setpoint (×100)

u = señal de control (×100)

m = modo de control

Ejemplo:

T,1234,1200,-350,1

📡 ESP32 → GUI (UDP)

Formato enviado:

nivel,control


Ejemplo:

12.34,-3.50

🖥 GUI → ESP32 (UDP)

Comandos enviados desde la interfaz gráfica:

SP:15
MODE:1

🔁 ESP32 → STM32 (UART)

Los comandos recibidos por UDP se reenvían sin modificar formato:

SP,15
MODE,1

⚙️ Configuración del proyecto (ESP32)
1️⃣ Requisitos

ESP32 compatible con Arduino

PlatformIO

Acceso a una red WiFi

STM32 conectada por UART

2️⃣ Configuración WiFi

En main.cpp, modificar:

const char* WIFI_SSID = "NOMBRE_DE_TU_WIFI";
const char* WIFI_PASS = "CONTRASEÑA";

3️⃣ Configuración de red (IP de la GUI)

Modificar la IP de la PC donde corre la GUI:

IPAddress GUI_IP(192,168,1,8);


⚠️ Esta IP puede cambiar al cambiar de router o red WiFi.
Se recomienda verificarla con ipconfig (Windows) o ifconfig (Linux).

4️⃣ Puertos utilizados
Uso	Puerto
ESP32 → GUI	5005
GUI → ESP32	5006
▶️ Uso del sistema (paso a paso)
🔹 1. Cargar el firmware en la ESP32

Compilar y subir el código desde PlatformIO

Abrir el Monitor Serial (115200 baudios)

Se mostrará algo como:

WiFi conectado
IP ESP32: 192.168.1.42

🔹 2. Conectar la STM32

Conectar pines UART:

ESP32 TX → STM32 RX

ESP32 RX → STM32 TX

GND común

🔹 3. Ejecutar la GUI en la PC

Verificar que la GUI:

Escuche en 0.0.0.0

Use los mismos puertos UDP

Al recibir datos:

Se actualizan las gráficas

Se muestra el estado del sistema

🔹 4. Enviar comandos desde la GUI

Cambiar Setpoint

Cambiar Modo de control

La ESP32 los retransmitirá inmediatamente a la STM32.

✅ Comportamiento esperado

La STM32 siempre recibe SP y MODE

La GUI siempre refleja el estado del sistema

La ESP32 no toma decisiones, solo comunica

El sistema sigue funcionando aunque:

Cambie el SP dinámicamente

Cambie el modo de control

🧪 Validación y pruebas

Probado en:

Simulación Software-in-the-Loop (SIL)

Comunicación real ESP32 ↔ PC

Integración con STM32 real

Comunicación robusta incluso con pérdida ocasional de paquetes UDP

🎓 Uso académico

Este proyecto está diseñado con fines educativos, para demostrar:

Arquitectura de sistemas embebidos

Separación de responsabilidades

Integración de protocolos heterogéneos

Comunicación en tiempo real

📌 Autor

Proyecto desarrollado como parte de un trabajo académico de Sistemas Embebidos, integrando control, comunicación y supervisión.
