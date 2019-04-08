
#include "WiFi.h"
#include <PubSubClient.h>
#include "DHTesp.h"
#include <driver/adc.h>

#define ProximityPin 5
#define LED 2
#define tempHumidFreq 15
#define minutes 60*1000
#define gasThreshold 7750


/*WiFi Settings*/
const char* ssid = "UPC2888929";
const char* password = "Xtjmbpxs7bws";
const char* mqtt_server = "test.mosquitto.org";
//
//const char* ssid = "EitDigital";
//const char* password = "digital2019";
//const char* mqtt_server = "test.mosquitto.org";

/*Variable Declarations*/
WiFiClient espClient;
PubSubClient client(espClient);
long lastMsg = -(tempHumidFreq+1)*minutes;
char msg[500];
int value = 0;
float analogReadVar;
DHTesp dht;
bool proxFlag = false;

void setup_wifi() 
{

  delay(10);

  pinMode(ProximityPin, INPUT);
  pinMode(LED, OUTPUT);
  digitalWrite(LED, LOW);
  
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) 
  {
    delay(500);
    Serial.print(".");
  }

  randomSeed(micros());

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void callback(char* topic, byte* payload, unsigned int length) 
{
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) 
  {
    Serial.print((char)payload[i]);
  }
  Serial.println();

  // Switch on the LED if an 1 was received as first character
  if ((char)payload[0] == '1') 
  {
    //digitalWrite(BUILTIN_LED, LOW);   // Turn the LED on (Note that LOW is the voltage level
    // but actually the LED is on; this is because
    // it is active low on the ESP-01)
  } else 
  {
    //digitalWrite(BUILTIN_LED, HIGH);  // Turn the LED off by making the voltage HIGH
  }

}

void reconnect() 
{
  // Loop until we're reconnected
  while (!client.connected()) 
  {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "ESP32-redquartista";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (client.connect(clientId.c_str())) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      client.publish("5b957e88f3b6e2217420d1b4-smarthome", "hello world");
      // ... and resubscribe
      client.subscribe("5b957e88f3b6e2217420d1b4-smarthome");
    } 
    else 
    {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void setup() 
{
//  pinMode(BUILTIN_LED, OUTPUT);     // Initialize the BUILTIN_LED pin as an output
  Serial.begin(115200);
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);

  /*ADC Configuration*/
  adc1_config_width(ADC_WIDTH_BIT_12);
  adc1_config_channel_atten(ADC1_CHANNEL_0,ADC_ATTEN_DB_0);
  
  dht.setup(15, DHTesp::DHT11); // Connect DHT sensor to GPIO 17
  delay(1000);
  Serial.println("Notification Alerts are turned ON! ");
  delay(1000);
}

void delayMinutes(uint8_t mins)
{
    uint16_t i;
    
    for(i=0; i > (60*mins); i++)
    {
      delay(1000); 
    } 
}

void loop() 
{

  if (!client.connected()) 
  {
    reconnect();
  }
  client.loop();
  

//  long now = millis();
//  if (now - lastMsg > 2000) 
//  {
//    lastMsg = now;
//    ++value;

    delay(dht.getMinimumSamplingPeriod());

    
    
    long now = millis();
    if (now - lastMsg > tempHumidFreq*minutes) 
    {
          lastMsg = now;
          float temperature = dht.getTemperature();
          snprintf (msg, 50, "%f", temperature);
          Serial.print("Publish message: ");
          Serial.println(msg);
          client.publish("5b957e88f3b6e2217420d1b4-smarthome/temperature", msg);
    
          float humidity = dht.getHumidity();
          snprintf (msg, 50, "%f", humidity);
          Serial.print("Publish message: ");
          Serial.println(msg);
          client.publish("5b957e88f3b6e2217420d1b4-smarthome/moisture", msg);
                   
     }
      
        
    int gasValue = adc1_get_raw(ADC1_CHANNEL_0);
    if(gasValue>gasThreshold)
    {
      snprintf (msg, 50, "%d", gasValue);
      Serial.print("Gas Threshold Reached ");
      Serial.println(msg);
      client.publish("5b957e88f3b6e2217420d1b4-smarthome/gas", msg);
    }
    


   if((digitalRead(ProximityPin) == HIGH) && proxFlag == false)
  {
    proxFlag = true;
    snprintf (msg, 50, "ProximityHIGH");
    Serial.print("Publish message: ");
    Serial.println(msg);
    client.publish("5b957e88f3b6e2217420d1b4-smarthome/movement", msg);
    digitalWrite(LED, HIGH);
  }
  
  else if(digitalRead(ProximityPin) == LOW)
  {
    //Serial.println("\nPresence NOT Detected!!!");
    digitalWrite(LED, LOW);
    proxFlag = false;
  }


}
