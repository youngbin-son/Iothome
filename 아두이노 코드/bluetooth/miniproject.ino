#include <SoftwareSerial.h>
#include <Wire.h>
#include <DHT.h>
#include <TimerOne.h>
#include <LiquidCrystal_I2C.h>
#include <Stepper.h>
#include <MsTimer2.h>

#define DEBUG
#define BUTTON_PIN 2
#define MOTOR_PIN 3
#define DHTPIN 4
#define LED_BUILTIN_PIN 13
#define CDS_PIN A0
#define DHTTYPE DHT11  // DHT11/22

#define ARR_CNT 5
#define CMD_SIZE 60
char lcdLine1[17] = " Sweet MY Home";
char lcdLine2[17] = "";
char sendBuf[CMD_SIZE];
char recvId[10] = "KSH_LIN";
char getSensorId[10];
int buttonState;            // the current reading from the input pin
int lastButtonState = LOW;  // the previous reading from the input pin
int sensorTime;
unsigned long lastDebounceTime = 0;  // the last time the output pin was toggled
unsigned long debounceDelay = 50; 
bool ledOn = false;         // LED의 현재 상태 (on/off)
bool timerIsrFlag = false;
unsigned int secCount;
int cds = 0;
bool cdsFlag = false;
float humi;
float temp;
int getSensorTime;
const int STEPS_PER_REV = 200; 
const int OPEN_THRESHOLD = 80;
const int CLOSE_THRESHOLD = 40;
  


Stepper myStepper(STEPS_PER_REV, 5,7,6,8);
LiquidCrystal_I2C lcd(0x27, 16, 2);
DHT dht(DHTPIN, DHTTYPE);
SoftwareSerial BTSerial(10, 11);  // RX ==>BT:TXD, TX ==> BT:RX
//시리얼 모니터에서 9600bps, line ending 없음 설정 후 AT명령 --> OK 리턴
//AT+NAMEiotxx ==> OKname   : 이름 변경 iotxx
void setup() {
#ifdef DEBUG
  Serial.begin(9600);
  Serial.println("setup() start!");
#endif
  lcd.init();
  lcd.backlight();
  lcdDisplay(0, 0, lcdLine1);
  lcdDisplay(0, 1, lcdLine2);
  //  pinMode(BUTTON_PIN, INPUT_PULLUP);    //MCU내부 풀업 활성화
  pinMode(BUTTON_PIN, INPUT);
  pinMode(MOTOR_PIN, OUTPUT);
  pinMode(LED_BUILTIN_PIN, OUTPUT);
  BTSerial.begin(9600);  // set the data rate for the SoftwareSerial port
  Timer1.initialize(1000000);
  Timer1.attachInterrupt(timerIsr);  // timerIsr to run every 1 seconds
  myStepper.setSpeed(60);
  dht.begin();

}

void loop() {
  if (BTSerial.available())
    bluetoothEvent();

  if (timerIsrFlag) {
    timerIsrFlag = false;
    if (!(secCount % 2)) {  //2초에 한번 실행
      cds = map(analogRead(CDS_PIN), 0, 1023, 0, 100);
      humi = dht.readHumidity();
      temp = dht.readTemperature();

      sprintf(lcdLine2, "C:%d T:%d H:%d", cds, (int)temp, (int)humi);
      lcdDisplay(0, 1, lcdLine2);

#ifdef DEBUG
      Serial.println(lcdLine2);
#endif

      if ((cds >= 60) && cdsFlag) {
        openBlind();
        cdsFlag = false;
        sprintf(sendBuf, "[%s]CDS@%d\n", recvId, cds);
        BTSerial.write(sendBuf, strlen(sendBuf));
      } else if ((cds < 60) && !cdsFlag) {
        closeBlind();
        cdsFlag = true;
        sprintf(sendBuf, "[%s]CDS@%d\n", recvId, cds);
        BTSerial.write(sendBuf, strlen(sendBuf));
      }
      if (getSensorTime != 0 && !(secCount % getSensorTime)) {
        sprintf(sendBuf, "[%s]SENSOR@%d@%d@%d\n", recvId, cds, (int)temp, (int)humi);
        BTSerial.write(sendBuf);
      }
    }
    if (sensorTime != 0 && !(secCount % sensorTime ))
    {
      sprintf(sendBuf, "[SON_SQL]SENSOR@%d@%d@%d\r\n", cds, (int)temp, (int)humi);
      /*      char tempStr[5];
            char humiStr[5];
            dtostrf(humi, 4, 1, humiStr);  //50.0 4:전체자리수,1:소수이하 자리수
            dtostrf(temp, 4, 1, tempStr);  //25.1
            sprintf(sendBuf,"[%s]SENSOR@%d@%s@%s\r\n",getSensorId,cdsValue,tempStr,humiStr);
      */
      BTSerial.write(sendBuf, strlen(sendBuf));
      BTSerial.flush();
    }
    
    if(!(secCount % 5))
    {
      sprintf(sendBuf, "[SON_SQL]SENSOR@%d@%d@%d\r\n", cds, (int)temp, (int)humi);
      BTSerial.write(sendBuf, strlen(sendBuf));
      BTSerial.flush();
    }

  }
  buttonDebounce();
#ifdef DEBUG
  if (Serial.available())
    BTSerial.write(Serial.read());
#endif
} 
void bluetoothEvent() {
  int i = 0;
  char* pToken;
  char* pArray[ARR_CNT] = { 0 };
  char recvBuf[CMD_SIZE] = { 0 };
  int len = BTSerial.readBytesUntil('\n', recvBuf, sizeof(recvBuf) - 1);

#ifdef DEBUG
  Serial.print("Recv : ");
  Serial.println(recvBuf);
#endif

  pToken = strtok(recvBuf, "[@]");
  while (pToken != NULL) {
    pArray[i] = pToken;
    if (++i >= ARR_CNT)
      break;
    pToken = strtok(NULL, "[@]");
  }
  //recvBuf : [XXX_LIN]LED@ON
  //pArray[0] = "XXX_LIN"   : 송신자 ID
  //pArray[1] = "LED"
  //pArray[2] = "ON"
  //pArray[3] = 0x0



  if ((strlen(pArray[1]) + strlen(pArray[2])) < 16) {
    sprintf(lcdLine2, "%s %s", pArray[1], pArray[2]);
    lcdDisplay(0, 1, lcdLine2);
  }
  if(!strcmp(pArray[0], "SON_BT"))
  {
    //처리 후
    return;
  }

  if (!strcmp(pArray[1], "LED")) {
    if (!strcmp(pArray[2], "ON")) {
      digitalWrite(LED_BUILTIN_PIN, HIGH);
    } else if (!strcmp(pArray[2], "OFF")) {
      digitalWrite(LED_BUILTIN_PIN, LOW);
    }
    sprintf(sendBuf, "[%s]%s@%s\n", pArray[0], pArray[1], pArray[2]);
  } else if (!strcmp(pArray[1], "GETSENSOR")) {
    if (pArray[2] == NULL) {
      sprintf(sendBuf, "[%s]SENSOR@%d@%d@%d\n", pArray[0], cds, (int)temp, (int)humi);
      strcpy(recvId, pArray[0]);
      getSensorTime = 0;
    } else {
      getSensorTime = atoi(pArray[2]);
      return;
    }
  } else if (!strcmp(pArray[1], "MOTOR")) {
    int motorPwm = atoi(pArray[2]);
    motorPwm = map(motorPwm, 0, 100, 0, 255);
#ifdef DEBUG
    Serial.println(motorPwm);
#endif
    analogWrite(MOTOR_PIN, motorPwm);
    sprintf(sendBuf, "[%s]%s@%s\n", pArray[0], pArray[1], pArray[2]);
  } else if (!strncmp(pArray[1], " New", 4))  // New Connected
  {
    return;
  } else if (!strncmp(pArray[1], " Alr", 4))  //Already logged
  {
    return;
  }
    else
      return;

#ifdef DEBUG
  Serial.print("Send : ");
  Serial.print(sendBuf);
#endif
  BTSerial.write(sendBuf);
}
void timerIsr() {
  timerIsrFlag = true;
  secCount++;
}
void lcdDisplay(int x, int y, char* str) {
  int len = 16 - strlen(str);
  lcd.setCursor(x, y);
  lcd.print(str);
  for (int i = len; i > 0; i--)
    lcd.write(' ');
}

void buttonDebounce() {
  int reading = digitalRead(BUTTON_PIN);
  if (reading != lastButtonState) {
    lastDebounceTime = millis();
  }

  if ((millis() - lastDebounceTime) > debounceDelay) 
  {
    if (reading != buttonState) {
      buttonState = reading;
      if (buttonState == HIGH) {
        ledOn = !ledOn;                        // LED 상태 값 반전
        digitalWrite(LED_BUILTIN_PIN, ledOn);  // LED 상태 변경
        sprintf(sendBuf, "[%s]BUTTON@%s\n", recvId, ledOn ? "ON" : "OFF");
        BTSerial.write(sendBuf);
    #ifdef DEBUG
        Serial.print(sendBuf);
    #endif
      }
    }
  }
  lastButtonState = reading;
}



void openBlind() {
  
  // 여기에 블라인드를 열기 위한 동작 추가
  myStepper.step(4096);  // 한 바퀴 2048 반 바퀴 1024

  // 혹은 다른 방식으로 블라인드를 열도록 설정
}

// 블라인드 닫기 함수
void closeBlind() {

  myStepper.step(-4096);

  // 혹은 다른 방식으로 블라인드를 닫도록 설정
}

