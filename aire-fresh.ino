#include "Arduino.h"
#include <WiFi.h>
#include "ESP32MQTTClient.h"

const char *ssid = "Politecnica";
const char *pass = "";

char *server = "mqtt://23.22.218.241:1883";
char *subscribeTopic = "esp32";
char *publishTopic = "esp32.mqtt";

ESP32MQTTClient mqttClient; 

const int mq9Pin = 34;  
const int pinRojo = 12;   
const int pinVerde = 13; 
const int pinAzul = 14;   

float lastConcentration = -1.0; 

void setup()
{
    Serial.begin(115200);
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
    int mq9Value = analogRead(mq9Pin);

    float concentration = map(mq9Value, 0, 4095, 0, 100); 
    
    // Verificar si se supera el límite de CO
    if (concentration <= 66) { 
      // Encender el LED en color verde
      digitalWrite(pinRojo, LOW);
      digitalWrite(pinVerde, HIGH);
      digitalWrite(pinAzul, LOW);
    } else if (concentration > 66 && concentration <= 85) { 
      // Encender el LED en color azul
      digitalWrite(pinRojo, LOW);
      digitalWrite(pinVerde, LOW);
      digitalWrite(pinAzul, HIGH);
    } else { 
      // Encender el LED en color rojo
      digitalWrite(pinRojo, HIGH);
      digitalWrite(pinVerde, LOW);
      digitalWrite(pinAzul, LOW);
    }

    Serial.print("Concentración de CO: ");
    Serial.print(concentration);
    Serial.println(" ppm");

    if (concentration != lastConcentration)
    {
      lastConcentration = concentration;

      // Envía el valor de concentración a RabbitMQ
      String msg = String(concentration);
      sendToRabbitMQ(msg);
      // Muestra el mensaje enviado a rabbit
      Serial.print("Msn enviado: ");
      Serial.println(msg);
    }

    delay(5000);
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
