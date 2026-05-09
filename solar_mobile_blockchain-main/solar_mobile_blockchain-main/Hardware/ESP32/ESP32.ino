#include <WiFi.h>
#include <Arduino.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <PZEM004Tv30.h>
#include "esp_system.h"
#include "esp_flash.h"
#include "secrets.h" // Wi-Fi 연결 정보 분리

// ----------- WiFi 설정 -----------
const int wifi_timeout = 10000;

// ----------- MQTT 설정 -----------
const char *mqtt_server = "5181e0e530b0411b835a3278624fe50e.s1.eu.hivemq.cloud"; // HiveMQ cluster URL
const int mqtt_port = 8883;                                                      // TCP/UDP : 1883, TLS : 8883
const char *mqtt_user = "ESP32_for_solar";
const char *mqtt_password = "Alsgur2312!";
const char *mqtt_topic = "solar-data";

// ----------- Wifi 및 MQTT 객체 생성 -----------
WiFiClientSecure espClient; // TLS 사용
PubSubClient mqtt_client(espClient);

// ----------- PZEM 설정 -----------
#if defined(ESP32)
PZEM004Tv30 pzem(Serial2, 34, 23); // (PZEM TX -> ESP32 RX :34), (PZEM RX -> ESP32 TX :23)
#else
PZEM004Tv30 pzem(Serial2);
#endif

// ----------- 전역: 디바이스 ID -----------
uint64_t device_ID = 0;

// ========== Flash Unique ID 읽기 ==========
void readFlashUniqueID()
{
    esp_err_t result = esp_flash_read_unique_chip_id(NULL, &device_ID);
    if (result == ESP_OK)
    {
        Serial.printf("Flash Unique ID: %llX\n", device_ID);
    }
    else
    {
        Serial.printf("Failed to read Flash Unique ID (err: %d)\n", result);
        device_ID = 0; // 실패 시 기본값
    }
}

// ========== WiFi 연결 함수 ==========
void connectToWiFi()
{
    bool connected = false;
    Serial.println("Connect to WiFi ...");

    for (int i = 0; i < WIFI_COUNT; i++)
    {
        Serial.print("Try : ");
        Serial.println(ssid_list[i]);

        WiFi.begin(ssid_list[i], password_list[i]);

        unsigned long startTime = millis();
        while (WiFi.status() != WL_CONNECTED && millis() - startTime < wifi_timeout)
        {
            delay(500);
            Serial.print(".");
        }

        if (WiFi.status() == WL_CONNECTED)
        {
            connected = true;
            Serial.print("Connected SSID: ");
            Serial.println(ssid_list[i]);
            Serial.print("IP Address: ");
            Serial.println(WiFi.localIP());
            break;
        }
        else
        {
            Serial.println("\nFailed : Try to connect another WiFi");
        }
    }

    if (!connected)
    {
        Serial.println("Failed to Connect all of WiFi");
        while (true)
            delay(1000); // 연결 실패 시 무한 대기
    }
}

// ========== NTP 시간 동기화 ==========
void initTime()
{
    configTime(9 * 3600, 0, "pool.ntp.org", "time.nist.gov"); // 9*3600 → KST(UTC+9)
    Serial.print("Synchronizing NTP time");
    struct tm timeinfo;
    while (!getLocalTime(&timeinfo))
    {
        Serial.print(".");
        delay(500);
    }
    Serial.println("\nGet Current Korean Time!");
}

String getFormattedTime()
{
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo))
    {
        return "1999-12-12 00:00:00"; // 실패 시
    }
    char buffer[25];
    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &timeinfo);
    return String(buffer);
}

// ========== MQTT 연결 함수 ==========
void connectToMQTT()
{
    while (!mqtt_client.connected())
    {
        String client_id = "solar-client-";
        client_id += String(WiFi.macAddress()); // 고유 식별자 + 네트워크 장치 식별자
        Serial.printf("Connecting to MQTT Server as %s ...\n", client_id.c_str());

        if (mqtt_client.connect(client_id.c_str(), mqtt_user, mqtt_password))
        {
            Serial.println("HiveMQ Server Connected!");
            // ESP32는 Publish만 수행 (subscribe 필요 시 별도 설정)
        }
        else
        {
            Serial.print("HiveMQ connection failed, state: ");
            Serial.println(mqtt_client.state());
            delay(2000);
        }
    }
}

// ========== MQTT 콜백 ==========
void mqttCallback(char *topic, byte *payload, unsigned int length)
{
    Serial.print("Message arrived in topic: ");
    Serial.println(topic);
    Serial.print("Message: ");
    for (unsigned int i = 0; i < length; i++)
    {
        Serial.print((char)payload[i]);
    }
    Serial.println();
}

// ========== PZEM 데이터 Publish ==========
void publishPZEMData()
{
    // float voltage = pzem.voltage();
    // float current = pzem.current();
    float power = pzem.power();
    // float energy = pzem.energy();

    String timestamp = getFormattedTime();

    String payload = "{";
    payload += "\"device_id\":\"" + String((uint32_t)(device_ID), HEX) + "\",";
    payload += "\"timestamp\":\"" + timestamp + "\",";

    if (isnan(power))
    {
        payload += "\"power\":\"NaN\"";
    }
    else
    {
        payload += "\"power\":" + String(power);
    }
    payload += "}";

    mqtt_client.publish(mqtt_topic, payload.c_str());
    Serial.print("[Success] Published: ");
    Serial.println(payload);
}

void setup()
{
    Serial.begin(115200);
    readFlashUniqueID();

    delay(3000);

    // ---- Wifi 연결 ----
    connectToWiFi();

    // ---- NTP 동기화 ----
    initTime();

    // ---- MQTT 설정/연결 ----
    espClient.setInsecure();
    mqtt_client.setServer(mqtt_server, mqtt_port);
    mqtt_client.setCallback(mqttCallback);
    connectToMQTT(); // MQTT 연결
}

void loop()
{
    if (!mqtt_client.connected())
    {
        connectToMQTT(); // 연결 유지
    }

    mqtt_client.loop();
    publishPZEMData(); // 전력 데이터 전송
    delay(3000);       // 3초 간격 전송
}
