#include <BME280.h>
#include <Wire.h>
#include <ESP8266WiFi.h>

/*
  OakWunderground
  More info: https://github.com/dlichterman/oakwunderground
  Copyright - Attribution-ShareAlike 4.0 International (CC BY-SA 4.0)
  http://creativecommons.org/licenses/by-sa/4.0/

  Posts data to wunderground, requires use of the deep sleep for power saving - wake-rst soldered
 */
int Sleep_pin = 5;   // (Configure as INPUT_PULLUP to default HIGH)
int OakLEDpin = 1;   // Oak onboard LED pin 1
int sleepTimeS = 300; // 300 seconds - adjust as needed

BME280 bme;                   // Default : forced mode, standby time = 1000 ms
                              // Oversampling = pressure ×1, temperature ×1, humidity ×1, filter off,
bool metric = false;
bool live = true;     //Live flag - if true, sends to wunderground, if false, only to particle
WiFiClient client;


char SERVER[] = "rtupdate.wunderground.com";           // Realtime update server - RapidFire
char WEBPAGE [] = "GET /weatherstation/updateweatherstation.php?";
char ID [] = "";   //weather station id here
char PASSWORD [] = ""; //wunderground password here

ADC_MODE(ADC_VCC); // to use getVcc

// the setup function runs once when you press reset or power the board
void setup() {
  // initialize digital pin 13 as an output.
  pinMode(Sleep_pin, INPUT_PULLUP); // Use pullup mode to default HIGH
  pinMode(1, OUTPUT); // Use pullup mode to default HIGH

//DO WORK HERE
  digitalWrite(1, HIGH);   // turn the LED on (HIGH is the voltage level)
  delay(1000);              // wait for a second
  digitalWrite(1, LOW);    // turn the LED off by making the voltage LOW

  while(!bme.begin()){
    Particle.publish("Could not find BME280 sensor!");
    delay(1000);
  }

  float temp(NAN), hum(NAN), pres(NAN);
  uint8_t pressureUnit(2);                                            // unit: B000 = Pa, B001 = hPa, B010 = Hg, B011 = atm, B100 = bar, B101 = torr, B110 = N/m^2, B111 = psi
  bme.ReadData(pres, temp, hum, metric, pressureUnit);                // Parameters: (float& pressure, float& temp, float& humidity, bool celsius = false, uint8_t pressureUnit = 0x0)
  pres = pres + 0.6;
  float dewptf = (dewPoint(temp, hum));
  uint32_t getVcc = ESP.getVcc();
  
  String output = "";
  output = "Temp:" + String(temp) + "°F Pres:" + String(pres) + " Hum:" + String(hum) + " Voltage:" + String(getVcc);
  Particle.publish("Checkin",output.c_str());
  
//Send data to Weather Underground
 if(live)
 {
 if (client.connect(SERVER, 80)) { 
    
    // Ship it!
    client.print(WEBPAGE); 
    client.print("ID=");
    client.print(ID);
    client.print("&PASSWORD=");
    client.print(PASSWORD);
    client.print("&dateutc=");
    client.print("now");    //can use instead of RTC if sending in real time
    client.print("&tempf=");
    client.print(temp);
    client.print("&baromin=");
    client.print(pres);
    client.print("&dewptf=");
    client.print(dewptf);
    client.print("&humidity=");
    client.print(hum);
    client.print("&softwaretype=DIYArduinoOak&action=updateraw&realtime=1&rtfreq=300");//Rapid Fire
    client.println();
      
   }//End send loop 
 }
 
  if(live)
  {
    if (digitalRead(Sleep_pin) == HIGH) 
    {
      ESP.deepSleep(sleepTimeS * 1000000, WAKE_RF_DEFAULT); // Sleep
    }
  }
  else //debug - send every 5 to particle
  {
    if (digitalRead(Sleep_pin) == HIGH) 
    {
      ESP.deepSleep(5 * 1000000, WAKE_RF_DEFAULT); // Sleep
    } 
  }
}

double dewPoint(double tempf, double humidity)
{
  double A0= 373.15/(273.15 + tempf);
  double SUM = -7.90298 * (A0-1);
  SUM += 5.02808 * log10(A0);
  SUM += -1.3816e-7 * (pow(10, (11.344*(1-1/A0)))-1) ;
  SUM += 8.1328e-3 * (pow(10,(-3.49149*(A0-1)))-1) ;
  SUM += log10(1013.246);
  double VP = pow(10, SUM-3) * humidity;
  double T = log(VP/0.61078);   
  return (241.88 * T) / (17.558-T);
}

// the loop function runs over and over again forever
void loop() {

}
