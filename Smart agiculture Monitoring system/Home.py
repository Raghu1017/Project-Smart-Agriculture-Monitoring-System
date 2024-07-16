import streamlit as st
import pandas as pd
import altair as alt
import requests
import smtplib
from email.mime.text import MIMEText
from email.mime.multipart import MIMEMultipart
from utils.anedya import anedya_config
from utils.anedya import anedya_sendCommand
from utils.anedya import anedya_getValue
from utils.anedya import anedya_setValue
from utils.anedya import fetchHumidityData
from utils.anedya import fetchTemperatureData
from utils.anedya import fetchSoilMoistureData1
from utils.anedya import fetchSoilMoistureData2
from streamlit_autorefresh import st_autorefresh

nodeId = "69708f86-218a-11ef-84ba-139754672fc5"
apiKey = "6eeb0930dd4ed0ef5438bb4b19967d5cf180155f48b8f203ab66bf156ff361d9"
weatherApiKey = "dce0bf783920e1d0009ad2ffa60730b3"
city = "agra"


st.set_page_config(page_title="Smart Agriculture Monitoring System", layout="wide")
count = st_autorefresh(interval=10000, limit=None, key="auto-refresh-handler")

# SMTP configuration for Gmail
smtp_server = "smtp.gmail.com"
smtp_port = 587
smtp_username = "raghusahani444@gmail.com"
smtp_password = "uwqj vqnf uqbi quxl"
recipient_email = "itsop2580@gmail.com"

def send_email(subject, body):
    msg = MIMEMultipart()
    msg['From'] = smtp_username
    msg['To'] = recipient_email
    msg['Subject'] = subject
    msg.attach(MIMEText(body, 'plain'))
    
    with smtplib.SMTP(smtp_server, smtp_port) as server:
        server.starttls()
        server.login(smtp_username, smtp_password)
        server.sendmail(smtp_username, recipient_email, msg.as_string())

# Helper function to fetch weather data from WeatherAPI
def fetchWeatherForecast(city: str) -> dict:
    url = f"http://api.weatherapi.com/v1/forecast.json?key={weatherApiKey}&q={city}&days=4&aqi=no&alerts=no"
    response = requests.get(url)
    if response.status_code == 200:
        data = response.json()
        return data
    else:
        st.write("")
        return {}

# Function to check rain forecast
def checkRainForecast(weather_data: dict):
    forecast = weather_data.get("forecast", {}).get("forecastday", [])
    for day in forecast:
        condition = day.get("day", {}).get("condition", {}).get("text", "")
        if "rain" in condition.lower():
            return True
    return False

# Function to control the motor based on rain forecast
def controlMotorBasedOnRain():
    weather_data = fetchWeatherForecast(city)
    if checkRainForecast(weather_data):
        st.error("Rain expected in the next 4 days. Turning off the motor.")
        anedya_sendCommand("Motor", "OFF")
        anedya_setValue("Motor", False)
        send_email("Rain Forecast Alert", "Rain is expected in the next 4 days. The motor has been turned off.")
    else:
        st.success("No rain expected in the next 4 days. Motor remains on.")
        anedya_sendCommand("Motor", "ON")
        anedya_setValue("Motor", True)

# Function to manually control the motor
def controlMotorManually(action: str):
    if action == "ON":
        st.success("Manually turning ON the motor.")
        anedya_sendCommand("Motor", "ON")
        anedya_setValue("Motor", True)
        send_email("Motor Control Alert", "The motor has been manually turned ON.")
    elif action == "OFF":
        st.error("Manually turning OFF the motor.")
        anedya_sendCommand("Motor", "OFF")
        anedya_setValue("Motor", False)
        send_email("Motor Control Alert", "The motor has been manually turned OFF.")

def main():
    anedya_config(nodeId, apiKey)
    global humidityData, temperatureData, soilMoistureData1, soilMoistureData2

    if "LoggedIn" not in st.session_state:
        st.session_state.LoggedIn = False

    if "CurrentHumidity" not in st.session_state:
        st.session_state.CurrentHumidity = 0

    if "CurrentTemperature" not in st.session_state:
        st.session_state.CurrentTemperature = 0

    if "CurrentSoilMoisture1" not in st.session_state:
        st.session_state.CurrentSoilMoisture1 = 0

    if "CurrentSoilMoisture2" not in st.session_state:
        st.session_state.CurrentSoilMoisture2 = 0

    if st.session_state.LoggedIn is False:
        drawLogin()
    else:
        humidityData = fetchHumidityData()
        temperatureData = fetchTemperatureData()
        soilMoistureData1 = fetchSoilMoistureData1()
        soilMoistureData2 = fetchSoilMoistureData2()
        drawDashboard()

def drawLogin():
    cols = st.columns([1, 0.8, 1], gap='small')
    with cols[0]:
        pass
    with cols[1]:
        st.title("Anedya Demo Dashboard", anchor=False)
        username_inp = st.text_input("Username")
        password_inp = st.text_input("Password", type="password")
        submit_button = st.button(label="Submit")

        if submit_button:
            if username_inp == "admin" and password_inp == "admin":
                st.session_state.LoggedIn = True
                st.rerun()
            else:
                st.error("Invalid Credential!")
    with cols[2]:
        print()

def drawDashboard():
    headercols = st.columns([1, 0.1, 0.1], gap="small")
    with headercols[0]:
        st.title("Smart Agriculture Monitoring System", anchor=False)
    with headercols[1]:
        st.button("Refresh")
    with headercols[2]:
        logout = st.button("Logout")

    if logout:
        st.session_state.LoggedIn = False
        st.rerun()

    st.markdown("This dashboard provides live view of Agriculture Monitoring System.")

    st.subheader("Current Status")
    cols = st.columns(4, gap="medium")
    with cols[0]:
        st.metric(label="Humidity", value=str(st.session_state.CurrentHumidity) + " %")
        if st.session_state.CurrentHumidity < 30:
            send_email("Low Humidity Alert", "Humidity is below 30%. Current value: {}%".format(st.session_state.CurrentHumidity))
    with cols[1]:
        st.metric(label="Temperature", value=str(st.session_state.CurrentTemperature) + "  °C")
    with cols[2]:
        st.metric(label="Pot 1", value=str(st.session_state.CurrentSoilMoisture1) + " %")
        if st.session_state.CurrentSoilMoisture1 < 20:
            send_email("Low Soil Moisture Alert", "Soil Moisture 1 is below 20%. Current value: {}%".format(st.session_state.CurrentSoilMoisture1))
    with cols[3]:
        st.metric(label="Pot 2", value=str(st.session_state.CurrentSoilMoisture2) + " %")
        if st.session_state.CurrentSoilMoisture2 < 20:
            send_email("Low Soil Moisture Alert", "Soil Moisture 2 is below 20%. Current value: {}%".format(st.session_state.CurrentSoilMoisture2))

    button_cols = st.columns(4)
    if button_cols[0].button("Rain Forecast"):
        controlMotorBasedOnRain()


    charts = st.columns(4, gap="small")
    with charts[0]:
        st.subheader("Humidity")
        humidity_chart_an = alt.Chart(humidityData).mark_area(
            line={'color': '#1fa2ff'},
            color=alt.Gradient(
                gradient='linear',
                stops=[alt.GradientStop(color='#1fa2ff', offset=1),
                       alt.GradientStop(color='rgba(255,255,255,0)', offset=0)],
                x1=1,
                x2=1,
                y1=1,
                y2=0,
            ),
            interpolate='monotone',
            cursor='crosshair'
        ).encode(
            x=alt.X(
                "Datetime:T",
                axis=alt.Axis(format="%Y-%m-%d %H:%M:%S", title="Datetime", tickCount=10, grid=True, tickMinStep=5),
            ),
            y=alt.Y(
                "aggregate:Q", 
                scale=alt.Scale(domain=[20, 100]),
                axis=alt.Axis(title="Humidity (%)", grid=True, tickCount=10),
            ),
            tooltip=[alt.Tooltip('Datetime:T', format="%Y-%m-%d %H:%M:%S", title="Time"),
                     alt.Tooltip('aggregate:Q', format="0.2f", title="Value")],
        ).properties(height=400).interactive()
        st.altair_chart(humidity_chart_an, use_container_width=True)

    with charts[1]:
        st.subheader("Temperature")
        temperature_chart_an = alt.Chart(temperatureData).mark_area(
            line={'color': '#1fa2ff'},
            color=alt.Gradient(
                gradient='linear',
                stops=[alt.GradientStop(color='#1fa2ff', offset=1),
                       alt.GradientStop(color='rgba(255,255,255,0)', offset=0)],
                x1=1,
                x2=1,
                y1=1,
                y2=0,
            ),
            interpolate='monotone',
            cursor='crosshair'
        ).encode(
            x=alt.X(
                "Datetime:T",
                axis=alt.Axis(format="%Y-%m-%d %H:%M:%S", title="Datetime", tickCount=10, grid=True, tickMinStep=5),
            ),
            y=alt.Y(
                "aggregate:Q",
                scale=alt.Scale(zero=False, domain=[10, 50]),
                axis=alt.Axis(title="Temperature (°C)", grid=True, tickCount=10),
            ),
            tooltip=[alt.Tooltip('Datetime:T', format="%Y-%m-%d %H:%M:%S", title="Time"),
                     alt.Tooltip('aggregate:Q', format="0.2f", title="Value")],
        ).properties(height=400).interactive()
        st.altair_chart(temperature_chart_an, use_container_width=True)

    with charts[2]:
        st.subheader("Soil Moisture Pot1")
        soil_moisture_chart_an = alt.Chart(soilMoistureData1).mark_area(
            line={'color': '#1fa2ff'},
            color=alt.Gradient(
                gradient='linear',
                stops=[alt.GradientStop(color='#1fa2ff', offset=1),
                       alt.GradientStop(color='rgba(255,255,255,0)', offset=0)],
                x1=1,
                x2=1,
                y1=1,
                y2=0,
            ),
            interpolate='monotone',
            cursor='crosshair'
        ).encode(
            x=alt.X(
                "Datetime:T",
                axis=alt.Axis(format="%Y-%m-%d %H:%M:%S", title="Datetime", tickCount=10, grid=True, tickMinStep=5),
            ),
            y=alt.Y(
                "aggregate:Q",
                scale=alt.Scale(domain=[0, 100]),
                axis=alt.Axis(title="Soil Moisture (%)", grid=True, tickCount=10),
            ),
            tooltip=[alt.Tooltip('Datetime:T', format="%Y-%m-%d %H:%M:%S", title="Time"),
                     alt.Tooltip('aggregate:Q', format="0.2f", title="Value")],
        ).properties(height=400).interactive()
        st.altair_chart(soil_moisture_chart_an, use_container_width=True)

    with charts[3]:
        st.subheader("Soil Moisture Pot2")
        soil_moisture_chart_an = alt.Chart(soilMoistureData2).mark_area(
            line={'color': '#1fa2ff'},
            color=alt.Gradient(
                gradient='linear',
                stops=[alt.GradientStop(color='#1fa2ff', offset=1),
                       alt.GradientStop(color='rgba(255,255,255,0)', offset=0)],
                x1=1,
                x2=1,
                y1=1,
                y2=0,
            ),
            interpolate='monotone',
            cursor='crosshair'
        ).encode(
            x=alt.X(
                "Datetime:T",
                axis=alt.Axis(format="%Y-%m-%d %H:%M:%S", title="Datetime", tickCount=10, grid=True, tickMinStep=5),
            ),
            y=alt.Y(
                "aggregate:Q",
                scale=alt.Scale(domain=[0, 100]),
                axis=alt.Axis(title="Soil Moisture (%)", grid=True, tickCount=10),
            ),
            tooltip=[alt.Tooltip('Datetime:T', format="%Y-%m-%d %H:%M:%S", title="Time"),
                     alt.Tooltip('aggregate:Q', format="0.2f", title="Value")],
        ).properties(height=400).interactive()
        st.altair_chart(soil_moisture_chart_an, use_container_width=True)

if __name__ == "__main__":
    main()
