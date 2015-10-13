
#include <UIPEthernet.h>
#include <HttpClient.h>
#include <Xively.h>
#include <dht11.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

char xivelyKey[] = "Your Key";
#define xivelyFeed Your_ID
char TemperatureID[] = "Temperature";
char HumidityID[] = "Humidity";
char PM2_5ID[] = "PM2_5";
byte mac[] = {
  0x70, 0x54, 0xD2, 0x98, 0x10, 0x66
};

XivelyDatastream datastreams[] = {
  XivelyDatastream(TemperatureID, sizeof(TemperatureID), DATASTREAM_FLOAT),
  XivelyDatastream(HumidityID, sizeof(HumidityID), DATASTREAM_FLOAT),
  XivelyDatastream(PM2_5ID, sizeof(PM2_5ID), DATASTREAM_FLOAT)
};

XivelyFeed feed(xivelyFeed, datastreams, 3 /* number of datastreams */);
EthernetClient client;
XivelyClient xivelyclient(client);

dht11 DHT11;
#define DHT11_PIN 3
LiquidCrystal_I2C lcd(0x27, 16, 2);

int measurePin = A3; //Connect dust sensor to Arduino A0 pin
int ledPower = 2;   //Connect 3 led driver pins of dust sensor to Arduino D2

int samplingTime = 280;
int deltaTime = 40;
int count = 0;
#define REPORT_TIME 6

float voMeasured = 0;
float voMeasuredLast = 123.0;
float dustDensity = 0;
void setup() {

  lcd.begin();
  pinMode(ledPower, OUTPUT);
  // start the Ethernet connection and the server:
  Ethernet.begin(mac);
  //Ethernet.begin(mac, IPAddress(172, 16, 110, 15), IPAddress(172, 16, 199, 1), IPAddress(172, 16, 110, 254) , IPAddress(255, 255, 255, 0));
  lcd.print(Ethernet.localIP());
  delay(5000);
}
int temperature = 0, humidity = 0;
void loop() {
  lcd.clear();
  if (DHT11.read(DHT11_PIN) == DHTLIB_OK)
  {
    temperature = DHT11.temperature;
    humidity = DHT11.humidity;
    lcd.print(DHT11.temperature);
    //lcd.print((char)223);
    lcd.print("\337C  ");
    lcd.print(DHT11.humidity);
    lcd.print("%");
  }

  digitalWrite(ledPower, LOW); // power on the LED
  delayMicroseconds(samplingTime);

  voMeasured = analogRead(measurePin); // read the dust value
  delayMicroseconds(deltaTime);
  digitalWrite(ledPower, HIGH); // turn the LED off

  //smooth noise
  voMeasured = voMeasuredLast + (voMeasured - voMeasuredLast) / 10.0;

  if (voMeasured < 150)
    dustDensity = 0.15 * (voMeasured - 130.0);
  else if (voMeasured < 160)
    dustDensity = 3 + 1.7 * (voMeasured - 150.0);
  else if (voMeasured < 220)
    dustDensity = 5 * (voMeasured - 150.0);
  else
    //0.6~3.6 = 123~737 map to 0~500 ug/m3
    dustDensity = voMeasured - 220.0;

  if (dustDensity < 0) dustDensity = 0.0;
  if (dustDensity > 600) dustDensity = 600.0;
  voMeasuredLast = voMeasured;
  lcd.setCursor(0, 1);
  lcd.print(dustDensity); // unit: ug/m3
  lcd.print(" ug/m3");
  count++;
  if (count >= REPORT_TIME)
  {
    count = 0;
    datastreams[0].setFloat(temperature);
    datastreams[1].setFloat(humidity);
    datastreams[2].setFloat(dustDensity);
    xivelyclient.put(feed, xivelyKey);
  }
  //lcd.println("send data");
  delay(5000);
}

