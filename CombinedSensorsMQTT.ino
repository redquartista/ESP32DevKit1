
#include "WiFi.h"
#include <PubSubClient.h>
#include "DHTesp.h"
#include <driver/adc.h>

/*Pin Connections for Sensors*/
#define ProximityPin 5
#define buzzer 4
#define LED 2 //onboard led
#define GasPin ADC1_CHANNEL_0
#define SoilPin ADC1_CHANNEL_3
#define HallwayLED 19
#define KitchenLED 21 


#define tempHumidFreq 15 //Frequency at which temperature and humidity values are sent
#define minutes 60*1000 
#define gasThreshold 750 //Threshold value for gas
#define avgArraySize 10 




#define withMQTT
#define withAvg

/*WiFi Settings*/
//const char* ssid = "UPC2888929";
//const char* password = "Xtjmbpxs7bws";


const char* ssid = "EitDigital";
const char* password = "digital2019";


const char* mqtt_server = "test.mosquitto.org";

/*Variable Declarations*/
WiFiClient espClient;
PubSubClient client(espClient);
long lastMsg = -(tempHumidFreq+1)*minutes;
char msg[500];


DHTesp dht;
bool proxFlag = false;
int looper =0;
int soilAvgArray[avgArraySize];
int gasAvgArray[avgArraySize];


/*Function to connect to the WiFi and display the WiFi name*/
 
void setup_wifi() 
{

  delay(10);


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

/*Function that processes the data acquired from MQTT*/
void callback(char* topic, byte* payload, unsigned int length) 
{
  char * hallway = "5b957e88f3b6e2217420d1b4-smarthome/hallway";
  char * kitchen = "5b957e88f3b6e2217420d1b4-smarthome/kitchen";
  if( (strcmp(topic, hallway)==0) || (strcmp(topic, kitchen) == 0))
  {
    Serial.print("Message arrived [");
    Serial.print(topic);
    Serial.print("] ");
    for (int i = 0; i < length; i++) 
    {
      Serial.print((char)payload[i]);
    }
    Serial.println();

     if((strcmp(topic, hallway)==0)) //if Hallway LED topic is detected
     {
      
      //Switch on the LED if an 1 was received as first character
      if ((char)payload[0] == '1') 
      {

        Serial.println("Hallway LED ON");
        digitalWrite(HallwayLED, HIGH);
      } 
      else 
      {

      Serial.println("Hallway LED Off");
      digitalWrite(HallwayLED, LOW);
      }
      
     };

     if((strcmp(topic, kitchen)==0)) //if Kitchen LED topic is detected
     {
      
      //Switch on the LED if an 1 was received as first character
      if ((char)payload[0] == '1') 
      {

        Serial.println("Kitchen LED ON");
        digitalWrite(KitchenLED, HIGH);
      } 
      else 
      {
        Serial.println("Kitchen LED Off");
        digitalWrite(KitchenLED, LOW);
      }
      
     };
     
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
      client.subscribe("5b957e88f3b6e2217420d1b4-smarthome/#");
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


/*Initialize Average Arrays*/

/*Initialize a pin to interface proximity sensor as Input and the pins for LEDs, buzzer as ooutput*/
  pinMode(ProximityPin, INPUT);
  
  pinMode(LED, OUTPUT);
  digitalWrite(LED, LOW);

  pinMode(HallwayLED, OUTPUT);
  digitalWrite(HallwayLED, LOW);

  pinMode(KitchenLED, OUTPUT);
  digitalWrite(KitchenLED, LOW);

  pinMode(buzzer, OUTPUT);
  digitalWrite(buzzer, LOW);
  
  Serial.begin(115200);

  /*Wifi Setup*/
  #ifdef withMQTT
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
  client.subscribe("5b957e88f3b6e2217420d1b4-smarthome/#"); //Subscribe to the MQTT topic of Smart Home
  #endif

  /*ADC Configuration*/
  adc1_config_width(ADC_WIDTH_BIT_10);
  adc1_config_channel_atten(GasPin, ADC_ATTEN_DB_11);
  adc1_config_channel_atten(SoilPin, ADC_ATTEN_DB_11);

  delay(1000);
  
  #ifdef withAvg
    for(looper = 0; looper <= avgArraySize-1; looper++)
    { 
        soilAvgArray[looper] = adc1_get_raw(SoilPin);
        delay(10);
        gasAvgArray[looper] = adc1_get_raw(GasPin);
        delay(10);
    }
    Serial.println("Average Arrays Initiated!");

//     for(looper = 0; looper <= avgArraySize-1; looper++)
//    { 
//        Serial.print(soilAvgArray[looper]);
//        Serial.print("\n");
//        delay(10);
//    }
//
//     for(looper = 0; looper <= avgArraySize-1; looper++)
//    { 
//        Serial.print(gasAvgArray[looper]);
//        Serial.print("\n");
//        delay(10);
//    }
  #endif

  dht.setup(15, DHTesp::DHT11); // Connect DHT sensor to GPIO 15
  delay(1000);
  Serial.println("Notification Alerts are turned ON! ");
  delay(1000);
}


void loop() 
{
  #ifdef withMQTT  

  if (!client.connected()) 
  {
    reconnect();
  }
  client.loop();

  #endif
  




   /*---------------------------------------------*/


    delay(dht.getMinimumSamplingPeriod());

    
    
    long now = millis();
    if (now - lastMsg > tempHumidFreq*minutes) 
    {
          lastMsg = now;
          float temperature = dht.getTemperature();
          snprintf (msg, 50, "%f", temperature);
          Serial.print("Publish message: ");
          Serial.println(msg);
          #ifdef withMQTT
          client.publish("5b957e88f3b6e2217420d1b4-smarthome/temperature", msg);
          #endif

          
    
          float humidity = dht.getHumidity();
          snprintf (msg, 50, "%f", humidity);
          Serial.print("Publish message: ");
          Serial.println(msg);
          #ifdef withMQTT
          client.publish("5b957e88f3b6e2217420d1b4-smarthome/humidity", msg);
          #endif

            /* Moving average for soil value*/
          float avgSoilValue = 0;
          #ifdef withAvg
          for(looper=1; looper<avgArraySize-1; looper++)
          {
            avgSoilValue += (float) gasAvgArray[looper];
            gasAvgArray[looper-1] = gasAvgArray[looper];      
          }
          gasAvgArray[avgArraySize] = (float)adc1_get_raw(SoilPin);
          avgSoilValue += (float)adc1_get_raw(SoilPin);
          avgSoilValue = avgSoilValue/avgArraySize;
          #else
          avgSoilValue = (float)adc1_get_raw(SoilPin);
          #endif
          

          snprintf (msg, 50, "%d",(int)avgSoilValue);
          Serial.print("Soil ");
          Serial.println(msg);
          #ifdef withMQTT
          client.publish("5b957e88f3b6e2217420d1b4-smarthome/moisture", msg);
          #endif

                   
       }
      

    /* Moving average for gas value*/

    float avgGasValue =  0;

    #ifdef withAvg
    for(looper=1; looper<=avgArraySize-1; looper++)
    {
      avgGasValue += (float) gasAvgArray[looper];
      gasAvgArray[looper-1] = gasAvgArray[looper];      
    }

    gasAvgArray[avgArraySize]=(float)adc1_get_raw(GasPin); 
    avgGasValue += (float)adc1_get_raw(GasPin);
    avgGasValue = avgGasValue/(float)avgArraySize;
    #else
    avgGasValue = (float)adc1_get_raw(GasPin);
    #endif
    /*---------------------------------------------*/

    
    if(avgGasValue>gasThreshold)
    {
      digitalWrite(buzzer, HIGH);
      snprintf (msg, 50, "%d", (int)avgGasValue);
      Serial.print("Gas Threshold Reached ");
      Serial.println(msg);
      #ifdef withMQTT
      client.publish("5b957e88f3b6e2217420d1b4-smarthome/gas", msg);
      #endif

      
    }

    else
    {
      digitalWrite(buzzer, LOW);  
    };

    

   if((digitalRead(ProximityPin) == HIGH) && proxFlag == false)
  {
    proxFlag = true;
    snprintf (msg, 50, "ProximityHIGH");
    Serial.print("Publish message: ");
    Serial.println(msg);
      #ifdef withMQTT
      client.publish("5b957e88f3b6e2217420d1b4-smarthome/movement", msg);
      #endif
      digitalWrite(LED, HIGH);
  }
  
  else if(digitalRead(ProximityPin) == LOW)
  {
    //Serial.println("\nPresence NOT Detected!!!");
    digitalWrite(LED, LOW);
    proxFlag = false;
  };


}
