#include <WiFi.h>
#include <PubSubClient.h>

// --- Credenciales de Red y MQTT ---
const char* ssid = "Nombre red wifi";
const char* password = "contraseña";
const char* mqtt_server = "192.168.0.137"; 

// --- Definición de Pines ---
const int pinHumedad = 32;
const int pinTrig = 5;
const int pinEcho = 18;
const int pinRele = 26;

WiFiClient espClient;
PubSubClient client(espClient);
unsigned long lastMsg = 0;

// --- LÓGICA DE AUTOMATIZACIÓN ---
bool modoManual = false;
unsigned long tiempoManual = 0;
const int TIEMPO_PAUSA_MANUAL = 5000; // El switch manual dura 30 segundos

// ¡Ajusta estos valores según tus pruebas físicas!
const int UMBRAL_SECO = 300;       // Si la lectura es MAYOR a esto, la tierra está seca
const int NIVEL_MINIMO_AGUA = 15;   // Si la distancia es MENOR a 15cm, SÍ hay agua en el tinaco

void callback(char* topic, byte* payload, unsigned int length) {
  String mensaje = "";
  for (int i = 0; i < length; i++) {
    mensaje += (char)payload[i];
  }
  
  if (String(topic) == "escom/iot/equipo/jardin/bomba/control") {
    // Si el usuario usa el Dashboard, activamos el Modo Manual
    modoManual = true; 
    tiempoManual = millis(); 
    
    if (mensaje == "1" || mensaje == "ON" || mensaje == "true") {
      digitalWrite(pinRele, LOW); // Encender
      Serial.println(">>> MODO MANUAL: BOMBA ENCENDIDA (Pausa auto de 30s) <<<");
    } else if (mensaje == "0" || mensaje == "OFF" || mensaje == "false") {
      digitalWrite(pinRele, HIGH); // Apagar
      Serial.println(">>> MODO MANUAL: BOMBA APAGADA (Pausa auto de 30s) <<<");
    }
  }
}

void reconnect() {
  while (!client.connected()) {
    String clientId = "ESP32Client-";
    clientId += String(random(0xffff), HEX);
    if (client.connect(clientId.c_str())) {
      Serial.println("Conectado a MQTT!");
      client.subscribe("escom/iot/equipo/jardin/bomba/control");
    } else {
      delay(5000);
    }
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(pinHumedad, INPUT);
  pinMode(pinTrig, OUTPUT);
  pinMode(pinEcho, INPUT);
  pinMode(pinRele, OUTPUT);
  
  digitalWrite(pinRele, HIGH); // Arranca apagado

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  unsigned long now = millis();
  
  // 1. Verificar si ya se acabó el tiempo de pausa manual
  if (modoManual && (now - tiempoManual > TIEMPO_PAUSA_MANUAL)) {
      modoManual = false;
      Serial.println("--- REGRESANDO A MODO AUTOMÁTICO ---");
  }

  // 2. Lectura y publicación cada 3 segundos
  if (now - lastMsg > 3000) {
    lastMsg = now;

    int valorHumedad = analogRead(pinHumedad);
    
    digitalWrite(pinTrig, LOW);
    delayMicroseconds(2);
    digitalWrite(pinTrig, HIGH);
    delayMicroseconds(10);
    digitalWrite(pinTrig, LOW);
    long duracion = pulseIn(pinEcho, HIGH);
    long distancia_cm = duracion * 0.034 / 2;

    char tempString[8];
    dtostrf(valorHumedad, 1, 0, tempString);
    client.publish("escom/iot/equipo/jardin/humedad", tempString);
    
    dtostrf(distancia_cm, 1, 2, tempString);
    client.publish("escom/iot/equipo/jardin/deposito/nivel", tempString);
    
    // --- 3. EJECUTAR LÓGICA AUTOMÁTICA ---
    if (!modoManual) {
        // Condición: Tierra seca Y Tinaco con agua
        if (valorHumedad > UMBRAL_SECO && distancia_cm < NIVEL_MINIMO_AGUA) {
            digitalWrite(pinRele, LOW); // Encender bomba
            Serial.println("AUTO: Regando... (Tierra seca y Tinaco con agua)");
        } else {
            digitalWrite(pinRele, HIGH); // Apagar bomba
            Serial.println("AUTO: Bomba apagada (Humedad OK o Tinaco vacío)");
        }
    }
  }
}