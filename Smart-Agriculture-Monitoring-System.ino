/*
                            Room Monitoring Using ESP32 + DHT sensor (mqtt)
                Disclaimer: This code is for hobbyists for learning purposes. Not recommended for production use!!

                            # Dashboard Setup
                             - Create account and login to the dashboard
                             - Create new project.
                             - Create variables: temperature, humidity, soilMoisture1, soilMoisture2.
                             - Create a node (e.g., for home- Room1 or study room).
                            Note: Variable Identifier is essential; fill it accurately.

                            # Hardware Setup
                             - Properly identify your sensor's pins.
                             - Connect sensor VCC pin to 3V3.
                             - Connect sensor GND pin to GND.
                             - Connect sensor signal pin to D5.

                  Note: The code is tested on the "Esp-32 Wifi, Bluetooth, Dual Core Chip Development Board (ESP-WROOM-32)"

                                                                                           Dated: 26-March-2024
 
*/ 
#include <Arduino.h>

// Emulate Hardware Sensor?
bool virtual_sensor = true;

#include <WiFi.h>            //library to handle wifi connection 
#include <PubSubClient.h>     // library to establish mqtt connection
#include <WiFiClientSecure.h>  //library to maintain the secure connection 
#include <ArduinoJson.h> // Include the Arduino library to make json or abstract the value from the json
#include <TimeLib.h>     // Include the Time library to handle time synchronization with ATS (Anedya Time Services)
#include <DHT.h>         // Include the DHT library for humidity and temperature sensor handling

String regionCode = "ap-in-1";                   // Anedya region code (e.g., "ap-in-1" for Asia-Pacific/India) | For other country code, visity [https://docs.anedya.io/device/intro/#region]
const char *deviceID = "0377529f-d6fe-4933-aabd-6a8cb3275958"; // Fill your device Id , that you can get from your node description
const char *connectionkey = "ce98411eaf1287e37840053bc0596a56";  // Fill your connection key, that you can get from your node description
// WiFi credentials
const char *ssid = "Raghu_12345";     // Replace with your WiFi name
const char *pass = "raghunandan@password@"; // Replace with your WiFi password

// MQTT connection settings
const char *mqtt_broker = "mqtt.ap-in-1.anedya.io";  // MQTT broker address
const char *mqtt_username = deviceID;  // MQTT username
const char *mqtt_password = connectionkey;  // MQTT password
const int mqtt_port = 8883;  // MQTT port
String responseTopic = "$anedya/device/" + String(deviceID) + "/response";  // MQTT topic for device responses
String errorTopic = "$anedya/device/" + String(deviceID) + "/errors";  // MQTT topic for device errors

// Root CA Certificate
//fill anedya root certificate. it can be get from [https://docs.anedya.io/device/mqtt-endpoints/#tls]
const char *ca_cert = R"EOF(                           
-----BEGIN CERTIFICATE-----
MIICDDCCAbOgAwIBAgITQxd3Dqj4u/74GrImxc0M4EbUvDAKBggqhkjOPQQDAjBL
MQswCQYDVQQGEwJJTjEQMA4GA1UECBMHR3VqYXJhdDEPMA0GA1UEChMGQW5lZHlh
MRkwFwYDVQQDExBBbmVkeWEgUm9vdCBDQSAzMB4XDTI0MDEwMTAwMDAwMFoXDTQz
MTIzMTIzNTk1OVowSzELMAkGA1UEBhMCSU4xEDAOBgNVBAgTB0d1amFyYXQxDzAN
BgNVBAoTBkFuZWR5YTEZMBcGA1UEAxMQQW5lZHlhIFJvb3QgQ0EgMzBZMBMGByqG
SM49AgEGCCqGSM49AwEHA0IABKsxf0vpbjShIOIGweak0/meIYS0AmXaujinCjFk
BFShcaf2MdMeYBPPFwz4p5I8KOCopgshSTUFRCXiiKwgYPKjdjB0MA8GA1UdEwEB
/wQFMAMBAf8wHQYDVR0OBBYEFNz1PBRXdRsYQNVsd3eYVNdRDcH4MB8GA1UdIwQY
MBaAFNz1PBRXdRsYQNVsd3eYVNdRDcH4MA4GA1UdDwEB/wQEAwIBhjARBgNVHSAE
CjAIMAYGBFUdIAAwCgYIKoZIzj0EAwIDRwAwRAIgR/rWSG8+L4XtFLces0JYS7bY
5NH1diiFk54/E5xmSaICIEYYbhvjrdR0GVLjoay6gFspiRZ7GtDDr9xF91WbsK0P
-----END CERTIFICATE-----
)EOF";

long long submitTimer;   //timer variable for request handling
String timeRes, submitRes;  //variable to store the response 

// Define the type of DHT sensor (DHT11, DHT21, DHT22, AM2301, AM2302, AM2321)
#define DHT_TYPE DHT11
// Define the pin connected to the DHT sensor
#define DHT_PIN 12 // pin marked as D5 on the ESP32
#define SOIL_MOISTURE_PIN1 34 // Analog pin for soil moisture sensor 1
#define SOIL_MOISTURE_PIN2 35 // Analog pin for soil moisture sensor 2
#define RELAY_PIN1 26 // Digital pin for relay 1
#define RELAY_PIN2 27 // Digital pin for relay 2
float temperature;
float humidity;
int soilMoistureValue1;
int soilMoistureValue2;

// Function Declarations
void connectToMQTT();
void mqttCallback(char *topic, byte *payload, unsigned int length);
void setDevice_time();                                       // Function to configure the device time with real-time from ATS (Anedya Time Services)
void anedya_submitData(String datapoint, float sensor_data); // Function to submit data to the Anedya server

// WiFi and MQTT client initialization
WiFiClientSecure esp_client;
PubSubClient mqtt_client(esp_client);

// Create a DHT object
DHT dht(DHT_PIN, DHT_TYPE);

void setup()
{
  Serial.begin(115200); // Initialize serial communication with  your device compatible baud rate
  delay(1500);          // Delay for 1.5 seconds

  // Connect to WiFi network
  WiFi.begin(ssid, pass);
  Serial.println();
  Serial.print("Connecting to WiFi...");
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.print("Connected, IP address: ");
  Serial.println(WiFi.localIP());

  submitTimer = millis();
  pinMode(RELAY_PIN1, OUTPUT);
  pinMode(RELAY_PIN2, OUTPUT);
  digitalWrite(RELAY_PIN1, HIGH);
  digitalWrite(RELAY_PIN2, HIGH);
  
  // Set Root CA certificate
  esp_client.setCACert(ca_cert);
  mqtt_client.setServer(mqtt_broker, mqtt_port);  // Set the MQTT server address and port for the MQTT client to connect to anedya broker
  mqtt_client.setKeepAlive(60);  // Set the keep alive interval (in seconds) for the MQTT connection to maintain connectivity
  mqtt_client.setCallback(mqttCallback);  // Set the callback function to be invoked when MQTT messages are received
  connectToMQTT(); // Attempt to establish a connection to the anedya broker

  mqtt_client.subscribe(responseTopic.c_str());  //subscribe to get response
  mqtt_client.subscribe(errorTopic.c_str());    //subscibe to get error

  setDevice_time();
  // Initialize the DHT sensor
  dht.begin();
}

void loop()
{
  if (!virtual_sensor)
  {
    // Read the temperature and humidity from the DHT sensor
    Serial.println("Fetching data from the Physical sensor");
    temperature = dht.readTemperature();
    humidity = dht.readHumidity();
    if (isnan(humidity) || isnan(temperature))
    {
      Serial.println("Failed to read from DHT !"); // Output error message to serial console
      delay(1000);
      return;
    }
  }
  else
  {
    // Generate random temperature and humidity values
    Serial.println("Fetching data from the Virtual sensor");
    temperature = random(20, 30);
    humidity = random(60, 80);
  }
  Serial.print("Temperature : ");
  Serial.println(temperature);

  // Submit sensor data to Anedya server
  anedya_submitData("temperature", temperature); // submit data to the Anedya

  Serial.print("Humidity : ");
  Serial.println(humidity);

  anedya_submitData("humidity", humidity); // submit data to the Anedya

  // Read the soil moisture sensor values
  int rawSoilMoistureValue1 = analogRead(SOIL_MOISTURE_PIN1);
  int rawSoilMoistureValue2 = analogRead(SOIL_MOISTURE_PIN2);
  
  Serial.print("Raw Soil Moisture 1: ");
  Serial.println(rawSoilMoistureValue1);
  Serial.print("Raw Soil Moisture 2: ");
  Serial.println(rawSoilMoistureValue2);
  
  soilMoistureValue1 = map(rawSoilMoistureValue1, 0, 4095, 100, 0);
  soilMoistureValue2 = map(rawSoilMoistureValue2, 0, 4095, 100, 0);

  Serial.print("Mapped Soil Moisture 1: ");
  Serial.println(soilMoistureValue1);
  Serial.print("Mapped Soil Moisture 2: ");
  Serial.println(soilMoistureValue2);

  anedya_submitData("soilMoisture1", soilMoistureValue1); // submit soil moisture data to Anedya
  anedya_submitData("soilMoisture2", soilMoistureValue2); // submit soil moisture data to Anedya

  // Control the relay based on soil moisture value
  if (soilMoistureValue1 < 30) {
    digitalWrite(RELAY_PIN1, LOW); // Turn on the relay 1
  } else {
    digitalWrite(RELAY_PIN1, HIGH); // Turn off the relay 1
  }

  if (soilMoistureValue2 < 30) {
    digitalWrite(RELAY_PIN2, LOW); // Turn on the relay 2
  } else {
    digitalWrite(RELAY_PIN2, HIGH); // Turn off the relay 2
  }

  Serial.println("-------------------------------------------------");
  delay(1000);
}

void connectToMQTT()
{
  while (!mqtt_client.connected())
  {
    const char* client_id = deviceID;
    Serial.print("Connecting to Anedya Broker....... ");
    if (mqtt_client.connect(client_id, mqtt_username, mqtt_password))
    {
      Serial.println("Connected to Anedya broker");
    }
    else
    {
      Serial.print("Failed to connect to Anedya broker, rc=");
      Serial.print(mqtt_client.state());
      Serial.println(" Retrying in 5 seconds.");
      delay(5000);
    }
  }
}

void mqttCallback(char *topic, byte *payload, unsigned int length)
{
  char res[150] = "";

  for (unsigned int i = 0; i < length; i++)
  {
    res[i] = payload[i];
  }
  String str_res(res);
  if (str_res.indexOf("deviceSendTime") != -1)
  {
    timeRes = str_res;
  }
  else
  {
    submitRes = str_res;
  }
}

// Function to configure time synchronization with Anedya server
// For more info, visit [https://docs.anedya.io/devicehttpapi/http-time-sync/]
void setDevice_time()
{
  String timeTopic = "$anedya/device/" + String(deviceID) + "/time/json";
  const char *mqtt_topic = timeTopic.c_str();
  // Attempt to synchronize time with Anedya server
  if (mqtt_client.connected())
  {
    Serial.print("Time synchronizing......");

    boolean timeCheck = true; // iteration to re-sync to ATS (Anedya Time Services), in case of failed attempt
    long long deviceSendTime;
    long long timeTimer = millis();
    while (timeCheck)
    {
      mqtt_client.loop();
      unsigned int iterate = 2000;
      if (millis() - timeTimer >= iterate)
      {
        Serial.print(" .");
        timeTimer = millis();
        deviceSendTime = millis();

        // Prepare the request payload
        StaticJsonDocument<200> requestPayload;            // Declare a JSON document with a capacity of 200 bytes
        requestPayload["deviceSendTime"] = deviceSendTime; // Add a key-value pair to the JSON document
        String jsonPayload;                                // Declare a string to store the serialized JSON payload
        serializeJson(requestPayload, jsonPayload);        // Serialize the JSON document into a string
        const char *jsonPayloadLiteral = jsonPayload.c_str();
        mqtt_client.publish(mqtt_topic, jsonPayloadLiteral);
      }

      if (timeRes != "")
      {
        String strResTime(timeRes);

        // Parse the JSON response
        DynamicJsonDocument jsonResponse(100);     // Declare a JSON document with a capacity of 200 bytes
        deserializeJson(jsonResponse, strResTime); // Deserialize the JSON response from the server into the JSON document

        long long serverReceiveTime = jsonResponse["serverReceiveTime"]; // Get the server receive time from the JSON document
        long long serverSendTime = jsonResponse["serverSendTime"];       // Get the server send time from the JSON document

        long long deviceRecTime = millis();                                                                // Get the device receive time
        long long currentTime = (serverReceiveTime + serverSendTime + deviceRecTime - deviceSendTime) / 2; // Compute the current time
        long long currentTimeSeconds = currentTime / 1000;                                                 // Convert current time to seconds

        setTime(currentTimeSeconds); // Set the device time based on the computed current time
        Serial.println("\n synchronized!");
        timeCheck = false;
      }
    }
  }
  else
  {
    connectToMQTT();
  }
}

// Function to submit data to Anedya server
// For more info, visit [https://docs.anedya.io/devicehttpapi/submitdata/]
void anedya_submitData(String datapoint, float sensor_data)
{
  boolean check = true;
  String strSubmitTopic = "$anedya/device/" + String(deviceID) + "/submitdata/json";
  const char *submitTopic = strSubmitTopic.c_str();
  while (check)
  {
    if (mqtt_client.connected())
    {
      if (millis() - submitTimer >= 2000)
      {
        submitTimer = millis();
        long long current_time = now();                     // Get the current time
        long long current_time_milli = current_time * 1000; // Convert current time to milliseconds

        String jsonStr = "{\"data\":[{\"variable\": \"" + datapoint + "\",\"value\":" + String(sensor_data) + ",\"timestamp\":" + String(current_time_milli) + "}]}";
        const char *submitJsonPayload = jsonStr.c_str();
        mqtt_client.publish(submitTopic, submitJsonPayload);
      }
      mqtt_client.loop();
      if (submitRes != "")
      {
        DynamicJsonDocument jsonResponse(100);    // Declare a JSON document with a capacity of 200 bytes
        deserializeJson(jsonResponse, submitRes); // Deserialize the JSON response from the server into the JSON document

        int errorCode = jsonResponse["errCode"]; // Get the server receive time from the JSON document
        if (errorCode == 0)
        {
          Serial.println("Data pushed to Anedya!!");
        }
        else if (errorCode == 4040)
        {
          Serial.println("Failed to push data!!");
          Serial.println("unknown variable Identifier");
          Serial.println(submitRes);
        }
        else
        {
          Serial.println("Failed to push data!!");
          Serial.println(submitRes);
        }
        check = false;
        submitTimer=5000;
      }
    }
    else
    {
      connectToMQTT();
    }
  }
}
