# Sistema de Monitoreo y Control IoT mediante MQTT (Smart Garden)

Este proyecto implementa una arquitectura IoT basada en el protocolo MQTT para el monitoreo de humedad y control de una bomba de agua.

## Arquitectura del Sistema
* **Broker MQTT y Dashboard:** Raspberry Pi 3 ejecutando Mosquitto y Node-RED vía Docker.
* **Nodo Edge (Publicador/Suscriptor):** ESP32 programado en C/C++.
* **Sensores y Actuadores:** Sensor capacitivo de humedad, sensor ultrasónico de nivel y mini bomba de agua de 5V.

## Instrucciones para Replicar el Entorno (Capa de Red)
1. Instalar Docker y Docker Compose en la Raspberry Pi.
2. Clonar este repositorio.
3. Navegar a la carpeta `/capa_red` y ejecutar `docker compose up -d`.

## Árbol de Tópicos MQTT
* Humedad de tierra: `escom/iot/equipo/jardin/humedad`
* Nivel de tanque: `escom/iot/equipo/jardin/deposito/nivel`
* Control de bomba: `escom/iot/equipo/jardin/bomba/control`
