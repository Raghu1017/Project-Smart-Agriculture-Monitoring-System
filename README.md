#Smart Agriculture Monitoring System Using NodeMCU, Anedya IoT Cloud & Streamlit


Overview
This repository contains the code and documentation for a comprehensive Smart Agriculture Monitoring System. The system uses a NodeMCU (ESP32) to collect environmental data from various sensors, stores the data on Anedya IoT Cloud, and visualizes it in real-time using a Streamlit dashboard.
Features Real-time Monitoring: Measures temperature, humidity, and soil moisture levels.
Cloud Storage: Stores data on Anedya IoT Cloud for remote access and long-term storage.
Interactive Dashboard: Visualizes data using a Streamlit-based web application.




Components
NodeMCU (ESP32)
DHT22 Sensor (Temperature and Humidity)
Two Soil Moisture Sensor FOR Two Pots 
2 channel relay module


Setup Instructions
Hardware Connections

DHT11Sensor:

VCC to +5V
GND to GND
Data to GPIO D12 

Soil Moisture Sensor For Pot 1:
VCC to +5V
GND to GND
Signal to GPIO D34

Soil Moisture Sensor For Pot 2:
VCC to +5V
GND to GND
Signal to GPIO D35

2 channel relay module:
VCC to +5V
GND to GND
IN1 to GPIO D26
IN2 to GPIO D27




Install Required Libraries

WiFi.h            
PubSubClient.h     
WiFiClientSecure.h  
ArduinoJson.h
TimeLib.h
DHT.h  



Upload Code to NodeMCU
Open the Smart-Agriculture-Monitoring-System.ino file in the Arduino IDE. Configure your WiFi credentials and Anedya IoT Cloud API endpoint in the code. Then upload the code to your NodeMCU. This file is available in Firmware folder.



Set Up Anedya IoT Cloud
Create an account on Anedya IoT Cloud.
Set up a new project and configure the endpoints for data reception.


Install Streamlit and Run Dashboard
Ensure you have Python and pip installed. Then install Streamlit and run the dashboard.


pip install streamlit

pip install -r requirements.txt 

streamlit run dashboard.py

The dashboard.py script should be located in the root directory of the cloned repository.

Contributing

Contributions are welcome! Please fork the repository and submit a pull request.
