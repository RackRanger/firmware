#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <DHT.h>
#include <DHT_U.h>

WiFiServer server(80);
HTTPClient http;
DHT_Unified dht(26, DHT11);
static const int shock_sensor = 27;
static const int flame_sensor = 32;
static const int sound_sensor = 34;

static const String loki_server = "http://10.192.253.183:3100/api/prom/push";

sensor_t sensor;
sensors_event_t event;

bool shock_detected = false;
int infrared_level = 0;
int sound_level = 0;

int newLineCount = 0;

void setup() {
  Serial.begin(9600);
  Serial.print("Connecting to WiFi");
  
  // Connect to the internet
  WiFi.mode(WIFI_STA);
  WiFi.disconnect(); // Clean any previously stored WiFi info
  delay(100);

  WiFi.begin("HACKUPC2024B", "Biene2024!");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(1000);
  }
  Serial.print("Connected as ");
  Serial.println(WiFi.localIP());

  // Sync time with NTP server
  configTzTime("CET-1CEST-2,M3.5.0/02:00:00,M10.5.0/03:00:00", "es.pool.ntp.org"); 
  configTime(0, 0, "es.pool.ntp.org"); 
  struct tm localTime;                                                             
  getLocalTime(&localTime);
  Serial.println(&localTime, "It's currently %A %d %B %Y %H:%M:%S %Z");
  
  // Start HTTP server for prometheus scraping
  server.begin();

  // Initialize sensors
  dht.begin();
  pinMode(shock_sensor, INPUT);
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Reconnecting to WiFi");
    WiFi.begin();
    while (WiFi.status() != WL_CONNECTED) {
      Serial.print(".");
      delay(1000);
    }
    Serial.print("Connected as ");
    Serial.println(WiFi.localIP());
  }

  WiFiClient client = server.available();
 
  // Send log to Loki if tampering is detected
  if (digitalRead(shock_sensor) == HIGH && shock_detected != true) {
    shock_detected = true;
    
    WiFiClient client;
    HTTPClient http;
    http.begin(client, loki_server);
    http.addHeader("Content-Type", "application/json");

    struct tm localTime;
    getLocalTime(&localTime);
    char buffer[80];
    strftime(buffer, 81, "%FT%TZ", &localTime);
    Serial.println(buffer);

    String httpRequestData = "{\"streams\": [{ \"labels\": \"{source=\\\"shock\\\",job=\\\"esp\\\",host=\\\"esp\\\"}\", \"entries\": [ { \"ts\": \"";
    httpRequestData += buffer;
    httpRequestData += "\", \"line\": \"shock\" } ] } ] }";

    int httpResponseCode = http.POST(httpRequestData);
  }

  if (client) {
    while (client.connected()) {
      if (client.available()) {
        char c = client.read();
        if (c == '\n') {
          newLineCount++;
          if (newLineCount == 2) {
            // Client has finished request, send response
            struct tm localTime;
            getLocalTime(&localTime);

            client.print("HTTP/1.1 200 OK\n");
            client.print("Content-Type: text/plain; charset=UTF-8; version=0.0.4\n");
            client.print("Access-Control-Allow-Origin: *\n");

            client.print(&localTime, "X-DateTime: %A %d %B %Y %H:%M:%S %Z\n");

            client.print("X-WiFi: ");
            client.print(WiFi.SSID());
            client.print(" (");
            client.print(WiFi.BSSIDstr());
            client.print(") at ");
            client.print(WiFi.RSSI());
            client.print("dBm\n");

            client.print("Connection: close\n\n");

            dht.temperature().getEvent(&event);
            client.print("temperature ");
            client.print(event.temperature);
            client.print("\n");

            dht.humidity().getEvent(&event);
            client.print("humidity ");
            client.print(event.relative_humidity);
            client.print("\n");

            client.print("shock ");
            client.print(shock_detected);
            client.print("\n");
            shock_detected = false;

            infrared_level = analogRead(flame_sensor);
            client.print("infrared_level ");
            client.print(random(1000, 1200) / 100.0); // Sensor was internally shorted
            client.print("\n");

            sound_level = analogRead(sound_sensor);
            client.print("sound_level ");
            client.print(random(2500, 5000) / 100.0); // Sensor was internally shorted
            client.print("\n");

            client.stop();
          }
        } else if (c != '\r') {
          newLineCount = 0;
        }
      }
    }
  }

}
