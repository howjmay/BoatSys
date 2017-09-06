#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#define ONE_WIRE_BUS A11
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

#define SensorPin A2            //pH meter Analog output to Arduino Analog Input 0
#define Offset -2.5           //deviation compensate
#define LED 13
#define samplingInterval 20
#define printInterval 800
#define ArrayLenth  40    //times of collection
int pHArray[ArrayLenth];   //Store the average value of the sensor feedback
int pHArrayIndex=0;    


RF24 radio(7, 8); // CNS, CE
const byte address[6] = "00001";

int time_spec = 0;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  radio.begin();
  radio.openReadingPipe(0, address);
  radio.setPALevel(RF24_PA_MIN);
  radio.startListening();

  pinMode(LED,OUTPUT);  
  sensors.begin();
  Serial.println("Start:");
}

void loop() {
  // put your main code here, to run repeatedly:
  Controller();
  Spec();
}
void Controller() {
  if (radio.available()) {
    char text[32] = "";
    radio.read(&text, sizeof(text));
    Serial.println(text);
  }
}


void Spec() {
  // read the input on analog pin 0:
  double sensorValue = analogRead(A0);
  //turn sensorValue into microvolt:
  double microvolt=sensorValue*(0.02/1023.0)*1000000.0;
  Serial.print(microvolt,3);
  Serial.write(" microvolt    time:");
  Serial.println(time);
  Serial.write(" value:");
  time_spec = time+1;

  /*************
   * TODO
   * use time difference to replce the delay method
   */ 
  //delay(1000);        
}

void PhMeter() {
  
  // 要求匯流排上的所有感測器進行溫度轉換
  sensors.requestTemperatures();
  
  static unsigned long samplingTime = millis();
  static unsigned long printTime = millis();
  static float pHValue,voltage;
  if(millis()-samplingTime > samplingInterval)
  {
      pHArray[pHArrayIndex++]=analogRead(SensorPin);
      if(pHArrayIndex==ArrayLenth)pHArrayIndex=0;
      voltage = avergearray(pHArray, ArrayLenth)*5.0/1024;
      pHValue = 3.5*voltage+Offset;
      // fixed
      pHValue = 14-pHValue;
      if(pHValue > 8) pHValue += 1.15;
      else if(pHValue < 6) pHValue -= 1.15; 
      samplingTime=millis();
  }
  if(millis() - printTime > printInterval)   //Every 800 milliseconds, print a numerical, convert the state of the LED indicator
  {
    Serial.print("Water Temperature: ");
    // 取得溫度讀數（攝氏）並輸出，
    // 參數0代表匯流排上第0個1-Wire裝置
    Serial.println(sensors.getTempCByIndex(0));
    Serial.print("Voltage:");
    Serial.print(voltage,2);
    Serial.print("    pH value: ");
    Serial.println(pHValue,2);
    digitalWrite(LED,digitalRead(LED)^1);
    printTime=millis();
  }
}
double avergearray(int* arr, int number)
{
  int i;
  int max,min;
  double avg;
  long amount=0;
  if(number<=0)
  {
    Serial.println("Error number for the array to avraging!/n");
    return 0;
  }
  if(number<5)
  {   
    //less than 5, calculated directly statistics
    for(i=0;i<number;i++)
    {
      amount+=arr[i];
    }
    avg = amount/number;
    return avg;
  }
  else
  {
    if(arr[0]<arr[1])
    {
      min = arr[0];max=arr[1];
    }
    else
    {
      min=arr[1];max=arr[0];
    }
    for(i=2;i<number;i++)
    {
      if(arr[i]<min)
      {
        amount+=min;        //arr<min
        min=arr[i];
      }
      else 
      {
        if(arr[i]>max)
        {
          amount+=max;    //arr>max
          max=arr[i];
        }
        else
        {
          amount+=arr[i]; //min<=arr<=max
        }
      }//if
    }//for
    avg = (double)amount/(number-2);
  }//if
  return avg;
  delay(1000);
}


