# Sistema de Monitoreo y Control IoT mediante MQTT

Este proyecto implementa una arquitectura IoT basada en el protocolo MQTT para el monitoreo de humedad de suelo y nivel de agua, con control automático y manual de una bomba de riego.

---

## Arquitectura del Sistema

```
[ESP32 + Sensores]  →  MQTT  →  [Raspberry Pi 3 - Broker Mosquitto]  →  [Node-RED Dashboard]
     (Capa Edge)                        (Capa de Red)                     (Visualización)
```

### Componentes

| Capa | Componente | Función |
|------|-----------|---------|
| Edge | ESP32 Dev Module | Publicador y suscriptor MQTT |
| Edge | Sensor capacitivo de humedad de suelo | Mide humedad de la tierra |
| Edge | Sensor ultrasónico HC-SR04 | Mide nivel de agua en el tinaco |
| Edge | Módulo relé + bomba de agua 5V | Actuador físico de riego |
| Red | Raspberry Pi 3 | Servidor Mosquitto (Broker MQTT) |
| Red | Docker + docker-compose | Despliegue portátil de servicios |
| Visualización | Node-RED | Dashboard de control y analítica |

---

## Árbol de Tópicos MQTT

```
escom/iot/equipo/jardin/
├── humedad          → Publica lectura del sensor de humedad (QoS 0)
├── deposito/nivel   → Publica distancia/nivel del tinaco en cm (QoS 0)
└── bomba/control    → Suscripción para encender/apagar la bomba (QoS 1)
                       Valores: "ON" / "OFF"
```

**Criterio de QoS:**
- `QoS 0` para lecturas de sensores: si se pierde un dato no es crítico.
- `QoS 1` para control de la bomba: garantiza que la orden de actuación sea recibida.

---

## Lista de Materiales

- ESP32 Dev Module
- Sensor capacitivo de humedad de suelo V2.0
- Sensor ultrasónico HC-SR04
- Módulo relé de 1 canal (5V)
- Mini bomba de agua sumergible (5V)
- Raspberry Pi 3 + MicroSD 16GB + fuente de poder 5V 2.5A
- Protoboard + cables jumper DuPont
- Fuente de alimentación externa 5V (para bomba y relé)
- Resistencia 1kΩ (protección pin ECHO del HC-SR04)

---

## Conexión de Hardware (ESP32)

| Componente | Pin ESP32 |
|-----------|-----------|
| Sensor humedad (AOUT) | GPIO 34 |
| Relé (IN) | GPIO 26 |
| HC-SR04 (TRIG) | GPIO 5 |
| HC-SR04 (ECHO) | GPIO 18 |
| Sensor humedad (VCC) | 3.3V |
| Relé (VCC) | Fuente externa 5V |
| Bomba (control) | Via relé |
| Todos los GND | GND común |

> ⚠️ **Importante:** Alimentar el relé y la bomba con fuente externa de 5V. Unir los GND del ESP32 y la fuente externa. Colocar resistencia de 1kΩ en el pin ECHO para proteger el GPIO del ESP32.

---

## Estructura del Repositorio

```
proyecto-riego-mqtt/
├── capa_edge/
│   └── riego_esp32/        # Firmware del ESP32 (C/C++ Arduino)
├── capa_red/               # Configuración de Raspberry Pi y Docker
│   ├── docker-compose.yml
│   └── mosquitto/
│       └── config/
│           └── mosquitto.conf
└── README.md
```

---

## Instalación y Configuración

### 1. Firmware del ESP32

**Requisitos:**
- Arduino IDE 2.x
- Soporte ESP32: agregar en Archivo → Preferencias → URLs adicionales:
  ```
  https://dl.espressif.com/dl/package_esp32_index.json
  ```
- Librería: `PubSubClient` by Nick O'Leary (Gestor de librerías)

**Configuración:**


```cpp
const char* ssid       = "TU_WIFI";
const char* password   = "TU_CONTRASEÑA";
const char* mqtt_server = "IP_DE_TU_RASPBERRY"; // ej. 192.168.0.137
```

**Subir a la placa:**
1. Selecciona: Tools → Board → `ESP32 Dev Module`
2. Selecciona: Tools → Port → `COMx` (Windows) o `/dev/ttyUSB0` (Linux)
3. Haz clic en Upload (→)

---

### 2. Raspberry Pi 3 — Broker + Dashboard (Docker)

**Requisitos:**
- Raspberry Pi OS instalado
- Docker y Docker Compose instalados

**Instalar Docker en Raspberry Pi:**
```bash
curl -fsSL https://get.docker.com | sh
sudo usermod -aG docker $USER
```

**Clonar el repositorio y levantar los servicios:**
```bash
git clone https://github.com/alonsoC21/proyecto-riego-mqtt.git
cd proyecto-riego-mqtt/capa_red
docker-compose up -d
```

Esto levanta dos contenedores:
- **Mosquitto** (Broker MQTT) → puerto `1883`
- **Node-RED** (Dashboard) → puerto `1880`

**Archivo `mosquitto/config/mosquitto.conf`:**
```
listener 1883
allow_anonymous true
```

**Archivo `docker-compose.yml`:**
```yaml
version: '3.8'
services:
  mosquitto:
    image: eclipse-mosquitto:latest
    container_name: iot_mosquitto
    restart: unless-stopped
    ports:
      - "1883:1883"
      - "9001:9001"
    volumes:
      - ./mosquitto/config/mosquitto.conf:/mosquitto/config/mosquitto.conf
      - ./mosquitto/data:/mosquitto/data
      - ./mosquitto/log:/mosquitto/log

  nodered:
    image: nodered/node-red:latest
    container_name: iot_nodered
    restart: unless-stopped
    ports:
      - "1880:1880"
    volumes:
      - ./nodered_data:/data
    depends_on:
      - mosquitto
    environment:
      - TZ=America/Mexico_City
```

---

### 3. Dashboard Node-RED

1. Abre un navegador y accede a: `http://<IP_RASPBERRY>:1880`
2. Importa el flujo desde `capa_red/flows.json` (si está disponible) o configúralo manualmente.
3. El dashboard de visualización está en: `http://<IP_RASPBERRY>:1880/ui`

**Flujos configurados:**
- Suscripción a `escom/iot/equipo/jardin/humedad` → Gauge "Humedad tierra"
- Suscripción a `escom/iot/equipo/jardin/deposito/nivel` → Gauge "Nivel tinaco"
- Switch "Bomba de agua" → Publica `ON`/`OFF` en `escom/iot/equipo/jardin/bomba/control`

---

## Lógica de Funcionamiento

### Modo Automático (ESP32)

El ESP32 evalúa las lecturas cada 5 segundos:

```
Si (tierra seca) Y (tinaco con agua):
    → Publica "ON" en bomba/control  → Bomba ENCIENDE
    → Serial: "AUTO: Regando..."

Si (tierra húmeda) O (tinaco vacío):
    → Publica "OFF" en bomba/control → Bomba APAGA
    → Serial: "AUTO: Bomba apagada"
```

### Modo Manual (Dashboard)

Desde el dashboard en Node-RED, el switch "Bomba de agua" envía directamente `ON` u `OFF` al tópico de control, sobreescribiendo el modo automático.

---

## Pruebas de Conectividad

Para verificar que el Broker recibe mensajes, desde cualquier terminal en la red:

```bash
# Suscribirse a todos los tópicos del proyecto
mosquitto_sub -h <IP_RASPBERRY> -t "escom/iot/equipo/jardin/#" -v

# Publicar manualmente una orden a la bomba
mosquitto_pub -h <IP_RASPBERRY> -t "escom/iot/equipo/jardin/bomba/control" -m "ON"
```

---

## Tecnologías Utilizadas

- **C/C++ (Arduino Framework)** — Firmware ESP32
- **MQTT / Eclipse Mosquitto** — Protocolo de mensajería IoT
- **Docker + docker-compose** — Despliegue portátil de servicios
- **Node-RED** — Dashboard de visualización y control
- **Raspberry Pi OS** — Sistema operativo del servidor
