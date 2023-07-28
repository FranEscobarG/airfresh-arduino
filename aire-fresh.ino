#include "Arduino.h"
#include <WiFi.h>
#include "ESP32MQTTClient.h"

const char *ssid = "Politecnica";
const char *pass = "";

char *server = "mqtt://23.22.218.241:1883";
char *subscribeTopic = "esp32";
char *publishTopic = "esp32.mqtt";

ESP32MQTTClient mqttClient; // All params are set later

const int mq9Pin = 34;   // Pin analógico al que está conectado el sensor MQ-9
const int pinRojo = 12;   // Pin para el LED rojo
const int pinVerde = 13;  // Pin para el LED verde
const int pinAzul = 14;   // Pin para el LED azul

float lastConcentration = -1.0; // Variable para almacenar el último valor de concentración enviado

void setup()
{
    Serial.begin(115200);
    // Configurar los pines del LED RGB como salidas
    pinMode(pinRojo, OUTPUT);
    pinMode(pinVerde, OUTPUT);
    pinMode(pinAzul, OUTPUT);
  
    log_i();
    log_i("setup, ESP.getSdkVersion(): ");
    log_i("%s", ESP.getSdkVersion());

    mqttClient.enableDebuggingMessages();

    mqttClient.setURI(server);
    mqttClient.enableLastWillMessage("lwt", "I am going offline");
    mqttClient.setKeepAlive(30);
    WiFi.begin(ssid, pass);
    WiFi.setHostname("c3test");
    mqttClient.loopStart();
}

void loop()
{
    // Leer el valor analógico del sensor MQ-9
    int mq9Value = analogRead(mq9Pin);

    // Convertir el valor analógico en una concentración de CO
    float concentration = map(mq9Value, 0, 4095, 0, 100); // Ajustar el rango según corresponda
    
    // Verificar si se supera el límite de CO
    if (concentration <= 66) { // Ajustar el límite según corresponda
      // Encender el LED en color verde
      digitalWrite(pinRojo, LOW);
      digitalWrite(pinVerde, HIGH);
      digitalWrite(pinAzul, LOW);
    } else if (concentration > 66 && concentration <= 85) { // Serial.println("¡WARNING! Niveles de CO peligrosos detectados");
      // Encender el LED en color azul
      digitalWrite(pinRojo, LOW);
      digitalWrite(pinVerde, LOW);
      digitalWrite(pinAzul, HIGH);
    } else { // Concentración > 85 // Serial.println("¡DANGER! Niveles de CO dañinos detectados");
      // Encender el LED en color rojo
      digitalWrite(pinRojo, HIGH);
      digitalWrite(pinVerde, LOW);
      digitalWrite(pinAzul, LOW);
    }

    // Mostrar el valor de concentración en el monitor serial
    Serial.print("Concentración de CO: ");
    Serial.print(concentration);
    Serial.println(" ppm");

    // Verificar si el valor de concentración es diferente al último valor enviado
    if (concentration != lastConcentration)
    {
      // Almacenar el nuevo valor de concentración como último valor enviado
      lastConcentration = concentration;

      // Envía el valor de concentración a RabbitMQ
      String msg = String(concentration);
      sendToRabbitMQ(msg);
      // Muestra el mensaje enviado a rabbit
      Serial.print("Msn enviado: ");
      Serial.println(msg);
    }

    delay(5000); // Esperar 6 segundos antes de la siguiente lectura
}


// Función para enviar el mensaje a RabbitMQ
void sendToRabbitMQ(String message)
{
    mqttClient.publish(publishTopic, message, 0, false);
}

void onConnectionEstablishedCallback(esp_mqtt_client_handle_t client)
{
    if (mqttClient.isMyTurn(client)) // Can be omitted if only one client
    {
        mqttClient.subscribe(subscribeTopic, [](const String &payload)
                             { log_i("%s: %s", subscribeTopic, payload.c_str()); });

        mqttClient.subscribe("bar/#", [](const String &topic, const String &payload)
                             { log_i("%s: %s", topic, payload.c_str()); });
    }
}

esp_err_t handleMQTT(esp_mqtt_event_handle_t event)
{
    mqttClient.onEventCallback(event);
    return ESP_OK;
}
