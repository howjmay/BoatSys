#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>
#include <Servo.h>
#include <string.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#define ONE_WIRE_BUS "A11"
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

const int ServoPin = 9;
const int MotorRelayPin = 4;
const int DrainRelayPin = 3;
int x;
int y;
boolean CL = false;
boolean DrainStatus = false;
int millis_openDrain;

static unsigned long samplingTime;
static unsigned long printTime;
static float pHValue,voltage;

const int openDrain_delayTime = 500;
Servo myservo;

int time_spectro = 0;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  radio.begin();
  
  myservo.attach(ServoPin);  // 將 servo 物件連接到 pin

  pinMode(MotorRelayPin, OUTPUT);
  pinMode(DrainRelayPin, OUTPUT);

  digitalWrite(MotorRelayPin, LOW);
  digitalWrite(DrainRelayPin, LOW);
  
  pinMode(LED,OUTPUT); 
  sensors.begin();

  time_spectro = millis();

  Serial.println("Start:");
}

void loop() {
  // put your main code here, to run repeatedly:
  Spectro();
  Controller();
  SendData();
  
}
void Spectro() {
  if (millis() - time_spectro > 1000) {
    // read the input on analog pin 0:
    double sensorValue = analogRead(A0);
    //turn sensorValue into microvolt:
    double microvolt=sensorValue*(0.02/1023.0)*1000000.0;
    Serial.print(microvolt,3);
    Serial.write(" microvolt    time:");
    Serial.println(time_spectro);
    Serial.write(" value:");
    time_spectro = millis();  
  }
  
}

void Controller() {
  radio.openReadingPipe(0, address);
  radio.setPALevel(RF24_PA_MIN);
  radio.startListening();
  Serial.println("read");

  while (!radio.available()){
    // This loop is for waiting for the radio is available
  }
  
  if (radio.available()) {
   
    char text[32] = "";
    radio.read(&text, sizeof(text));

    ParseController(text);
    RudderControl();
    MotorControl();
    /*
    if (CL) {
      // Turning on all the sensor for sensing all the needed data
      //Sense();

      // Returning the Data
      
    }
    */
  }
}

void ParseController(char* text) {
  String str_x = "";
  String str_y = "";

  int section = 0;
  for (int i = 0; i < strlen(text); i++) {
    if (text[i] == ',') {
      section++;
    }
    else {
      switch (section) {
        case 0:
          str_x += text[i];
          break;
        case 1:
          str_y += text[i];
          break;
        case 2:
          if (text[i] == '1') {
            CL = true;
          }
          else if (text[i] == '0') {
            CL = false;
          }
          break;
      }
    }
  }
  x = str_x.toInt();
  y = str_y.toInt();

  Serial.print(x);
  Serial.print(", ");
  Serial.print(y);
  Serial.print(", ");
  Serial.println(CL);
}
void RudderControl() {
  double pos_d = (x + 512) / 1024.0 * 180;
  Serial.println((int) pos_d);
  myservo.write((int) pos_d);               // 告訴 servo 走到 'pos' 的angle
}
void MotorControl() {
  if (y > 40) {
    digitalWrite(MotorRelayPin, HIGH);
  }
  else if (y < -10 && CL == true) {
    // Turning ON the drain
    DrainStatus = true;
    digitalWrite(DrainRelayPin, HIGH);
    millis_openDrain = millis();
  }
  else {
    digitalWrite(MotorRelayPin, LOW);
  }

  if  (millis_openDrain > openDrain_delayTime && DrainStatus == true) {
    DrainStatus = false;
    digitalWrite(DrainRelayPin, LOW);
  }
  
}

void SendData() {
  radio.openWritingPipe(address);
  radio.setPALevel(RF24_PA_MIN);
  radio.stopListening();
  Serial.println("write");
  char sendText[200];
  if(millis() - printTime > printInterval)   //Every 800 milliseconds, print a numerical, convert the state of the LED indicator
  {
    Serial.print("Water Temperature: ");
    // 取得溫度讀數（攝氏）並輸出，
    // 參數0代表匯流排上第0個1-Wire裝置
    //Serial.println(sensors.getTempCByIndex(0));
    //Serial.print("Voltage:");
    //Serial.print(voltage,2);
    //Serial.print("    pH value: ");
    //Serial.println(pHValue,2);
    digitalWrite(LED, digitalRead(LED)^1);
    printTime=millis();

    String send_Text = "Temper" + String(sensors.getTempCByIndex(0), DEC) + "Voltage:" + String(voltage, DEC) + "pH" + String(pHValue, DEC);
    Serial.println(send_Text);
    //char sendText[50];
    send_Text.toCharArray(sendText, sizeof(sendText));
  }
  else {
    strcpy(sendText, "Reciever DICK");
    //Serial.println("FUCK YOU");
  }
  
  //sendText is the String data that we want to send
  radio.write(&sendText, sizeof(sendText));

  Serial.println("read");
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
  //delay(1000);
}

void PH_TempSense(){
  // 要求匯流排上的所有感測器進行溫度轉換
  sensors.requestTemperatures();

  samplingTime = millis();
  printTime = millis();
  
  //static unsigned long samplingTime = millis();
  //static unsigned long printTime = millis();
  //static float pHValue,voltage;
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
}

