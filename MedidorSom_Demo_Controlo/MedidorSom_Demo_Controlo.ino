// Import required libraries
#include "ESP8266WiFi.h"
#include <aREST.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_AM2320.h>
#include <Wire.h>
#include <Adafruit_MPL115A2.h>
#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"
#include <ArduinoJson.h>

// Create AM2320 Sensor instance
Adafruit_AM2320 am2320 = Adafruit_AM2320();

// Create aREST instance
aREST rest = aREST();

// Create MPL115A2 Sensor instance
Adafruit_MPL115A2 mpl115a2;

// WiFi parameters
const char *ssid = "LaumecPi";
const char *password = "laumec0123";

//MQTT Variables

#define AIO_SERVER "10.1.2.1" //"10.1.2.1" "192.168.200.157" (IP do Rpi tbm funciona)
#define AIO_SERVERPORT 1883   // use 8883 for SSL
#define AIO_USERNAME "3XAk628EgofdOfXUuqa3"
#define AIO_KEY ""
const char *topic = "v1/devices/me/telemetry";       //Topic where to write the information
const char *topic2 = "v1/devices/me/control/buzzer"; //Topic where to read the information
const char *topic3 = "v1/devices/me/control/limite"; //Topic where to read the information
const char *topic4 = "v1/devices/me/control/pitch";  //Topic where to read the information

// The port to listen for incoming TCP connections
#define LISTEN_PORT 80

// Create an instance of the servera
WiFiServer server(LISTEN_PORT);

//MQTT Variables
WiFiClient client2;
Adafruit_MQTT_Client mqtt(&client2, AIO_SERVER, AIO_SERVERPORT);

//MQTT Feeds
//Variaveis OUT
Adafruit_MQTT_Publish Medido = Adafruit_MQTT_Publish(&mqtt, topic);

// Variaveis IN
Adafruit_MQTT_Subscribe Buzzer = Adafruit_MQTT_Subscribe(&mqtt, topic2);
Adafruit_MQTT_Subscribe Limite = Adafruit_MQTT_Subscribe(&mqtt, topic3);
Adafruit_MQTT_Subscribe Pitch = Adafruit_MQTT_Subscribe(&mqtt, topic4);

void MQTT_connect();

// Variables to be exposed to the API
float temperature;
int humidity;
double db;
float pressureKPA, temperature2;
float amplitude;

//Sound sensor declare vars
const int ledPin = LED_BUILTIN;
const int buzzerpin = 14;
const int APin = A0;
boolean val = 0;
int Aval = 0;
//double Avalconv = 0;

//Json Vars
char output[212];

String ledcmd = "false";
  int pitchcmd = 200;
  int limcmd = 100;

//const char* esp = "esp_ilha1";

void setup(void)
{

  //Define micro pins
  pinMode(ledPin, OUTPUT);
  pinMode(APin, INPUT);
  pinMode(buzzerpin, OUTPUT); // Set buzzer - pin 9 as an output

  // Start Serial
  Serial.begin(115200);
  while (!Serial)
  {
    delay(10); // hang out until serial port opens
  }

  // Init Temp
  am2320.begin();

  //Init Pressure_temp
  mpl115a2.begin();

  // Init variables and expose them to REST API
  rest.variable("temperature", &temperature);
  rest.variable("humidity", &humidity);
  rest.variable("pressure", &pressureKPA);
  rest.variable("temperature2", &temperature2);
  rest.variable("Sound", &db);

  // Give name and ID to device
  rest.set_id("1");
  rest.set_name("ilha1");

  // Connect to WiFi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");

  // Start the server
  server.begin();
  Serial.println("Server started");

  // Print the IP address
  Serial.println(WiFi.localIP());

  mqtt.subscribe(&Buzzer);
  mqtt.subscribe(&Limite);
  mqtt.subscribe(&Pitch);
}

void loop()
{
  //MQTT
  MQTT_connect();
  // Reading sound
  //Aval = analogRead(APin);
  Soundlevel();


  db = amplitude;
  //db = Aval;

  Serial.println(db);
  /*if (val == HIGH) {
     digitalWrite(ledPin, HIGH);
    }
    else {
     digitalWrite(ledPin, LOW);
    }*/

  // Reading temperature and humidity
  temperature = am2320.readTemperature();
  humidity = am2320.readHumidity();


  // Reading temperature and pressure
  //float pressureKPA = 0, temperature2 = 0;
  pressureKPA = mpl115a2.getPressure();
  temperature2 = mpl115a2.getTemperature();
  //Serial.println(Aval);


  jsonparsing();
  //output.toCharArray(convertido,212);


  //MQTT publish
  // Now we can publish stuff!
  Serial.print(F("\nSending values "));
  Serial.println(output);
  Serial.print("...");
  if (!Medido.publish(output))
  {
    Serial.println(F("Failed"));
  }
  else
  {
    Serial.println(F("OK!"));
  }

  //MQTT receive
  Adafruit_MQTT_Subscribe *subscription;
  while ((subscription = mqtt.readSubscription(100))) //TIMEOUT MQTT ms
  {
    if (subscription == &Buzzer)
    {
      Serial.print(F("LED/Buzzer: "));
      Serial.println((char *)Buzzer.lastread);
    }
    else if (subscription == &Limite)
    {
      Serial.print(F("  Limite: "));
      Serial.println((char *)Limite.lastread);
    }
    else if (subscription == &Pitch)
    {
      Serial.print(F("  Freq: "));
      Serial.println((char *)Pitch.lastread);
    }
  }


   ledcmd = String((char *)Buzzer.lastread);
   pitchcmd = atoi((char *)Pitch.lastread);
   limcmd = atoi((char *)Limite.lastread);

  // BUZZER CONTROL
  if (ledcmd == "true" || db > limcmd)
  {
    digitalWrite(ledPin, 0);
    //tone(buzzerpin, 1000); // Send 1KHz sound signal...
    tone(buzzerpin, pitchcmd); // Send mqtt freq sound signal...
  }
  else if (ledcmd == "false" || db < limcmd)
  {
    digitalWrite(ledPin, 1);
    noTone(buzzerpin); // Stop sound...
  }

  if (ledcmd == "true" || db > limcmd)
  {
    Serial.println("NOISE LIMIT");
  }

  delay(1);

  // Handle REST calls
  WiFiClient client = server.available();
  if (!client)
  {
    return;
  }
  while (!client.available())
  {
    delay(1);
  }
  rest.handle(client);
}







void MQTT_connect()
{
  int8_t ret;

  // Stop if already connected.
  if (mqtt.connected())
  {
    return;
  }

  Serial.print("Connecting to MQTT... ");

  uint8_t retries = 3;
  while ((ret = mqtt.connect()) != 0)
  { // connect will return 0 for connected
    Serial.println(mqtt.connectErrorString(ret));
    Serial.println("Retrying MQTT connection in 5 seconds...");
    mqtt.disconnect();
    delay(5000); // wait 5 seconds
    retries--;
    if (retries == 0)
    {
      // basically die and wait for WDT to reset me
      while (1)
        ;
    }
  }
  Serial.println("MQTT Connected!");
}

void jsonparsing()
{

  const size_t capacity = JSON_ARRAY_SIZE(2) + JSON_OBJECT_SIZE(3) + JSON_OBJECT_SIZE(4) + 97;
  DynamicJsonBuffer jsonBuffer(capacity);

  JsonObject &root = jsonBuffer.createObject();
  root["esp"] = "esp_ilha1";
  root["time"] = "data_hora";

  JsonObject &data = root.createNestedObject("data");

  JsonArray &data_temperature = data.createNestedArray("temperature");
  data_temperature.add(temperature);
  data_temperature.add(temperature2);
  data["humidity"] = humidity;
  data["pressure"] = pressureKPA;
  data["db"] = db;

  root.printTo(output);
}

void Soundlevel()
{

  int stime = millis();
  int i = 0;
  float Soma = 0;
  int avalmax = -2000;
  int avalmin = 2000;
  while (millis() - stime < 100)
  {

    Aval = analogRead(APin);
    if (Aval >= avalmax)
    {
      avalmax = Aval;
    }
    else if (Aval <= avalmin)
    {
      avalmin = Aval;
    }
    amplitude = avalmax - avalmin;
    //i += 1;
    delay(1);
  }
}
