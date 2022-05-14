#include "DHT.h"
#include<SFE_BMP180.h>
#include<LiquidCrystal.h>
#include<Wire.h>
#include<SoftwareSerial.h>
#define rs 2
#define en 3
#define d4 4
#define d5 5
#define d6 6
#define d7 7
#define pump 9
#define fan 11
#define DHTPIN 12
#define Buzzer 13
#define fsr A0
#define LDR1 A1
#define LDR2 A2
#define temperature A3
// A4 and A5 are used as I2c pins for the pressure sensor
#define DHTTYPE DHT11

DHT dht(DHTPIN, DHTTYPE);
SFE_BMP180 pressure;
SoftwareSerial serial_Change(10, 11);
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);
unsigned long Time_ = 0 , tempTime, dataTime = 0, lcd_Time = 0;
int people_inside = 0, set_People = 0, set_Temperature = 35, set_humidity = 45, set_fsr = 950, mode_controller = 0;
const short R1 = 9000, pressure_Out = 110;
char up = '3', down = '4', Data;
float Vo, ldr1, ldr2, logR2, temp, humidity;
float c1 = 1.009249522e-03,
      c2 = 2.378405444e-04, c3 = 2.019202697e-07;
double T, P;
bool tempLow = false, passed = false, temperature_State = false
                               , fsr_on = true, Buzzer_Active = false, people_State = false, data_Mode = false
                                   , update_lcd = false, humidityboo = false, f_mode = true, s_mode = false, fsr_Buz = false;

void setup() {
  dht.begin();
  pinMode(LDR1, INPUT);
  pinMode(LDR2, INPUT);
  pinMode(temperature, INPUT);
  pinMode(pump, OUTPUT);
  Serial.begin(9600);
  Serial.println("Start of measuring pressure");
  lcd.begin(16, 2);
  lcd.setCursor(5, 0);
  lcd.print("Weclome");
  lcd.setCursor(3, 1);
  lcd.print("Smart Room");
  tone(Buzzer, 3200);
  delay(5000);
  noTone(Buzzer);
  if (pressure.begin())
    Serial.println("done successfully");
  else
    Serial.println("Error occured");
  lcd.clear();
}

void loop() {

  if (Serial.available() > 0)  {
    Data = Serial.read();
    dataTime = millis();
    data_Mode = true;
    if (Data == '2')
    { temperature_State = true;
      people_State = false;
      setting_Temperature(Data);
    }

    else if (Data == '1')
    { people_State = true;
      temperature_State = false;
      setting_People(Data);

    }
   
    else if (Data == '3') {
      String str = "Humidity: ";
      str += String(humidity);
      Serial.println(str);

    }

    else if (temperature_State) {
      setting_Temperature(Data);
    }

    else if (people_State)
      setting_People(Data);

    else if (Data == '9') {
      if (set_humidity < 60)
        set_humidity++;
      String str = "Max Humidity: ";
      str += String(set_humidity);
      Serial.println(str);
    }

    else if (Data == '6') {
      if (set_humidity > 40)
        set_humidity--;
      String str = "Max Humidity: ";
      str += String(set_humidity);
      Serial.println(str);
    }
    else if ( Data == 'A')
    {
      fsr_on = true;
      Serial.println("Patient IN");
    }
    else if (Data == 'B')
    {
      Serial.println("No Patient");
      fsr_on = false;
      fsr_Buz=false;
      noTone(Buzzer);
    }
  }

  { float fsr_read = analogRead(fsr);
    if (fsr_on) {
      if (fsr_read > set_fsr)
      {
        fsr_Buz = true;
        digitalWrite(Buzzer, HIGH);
      }
      else  if (!Buzzer_Active ) {
        fsr_Buz = false;
        digitalWrite(Buzzer, LOW);
      }
    }
  }


  if (dataTime + 5000 < millis() && data_Mode)
    data_Mode = false;
  if (!data_Mode) {

    Vo = analogRead(temperature);
    logR2 = log(R1 * (1023.0 / Vo - 1.0));
    temp = (1.0 / (c1 + c2 * logR2 + c3 * logR2 * logR2 * logR2));
    temp = temp - 273.15;
    if (lcd_New()) {
      dataTime = millis();
      update_lcd = true;
      //      Serial.print("Temperature is: ");
      //      Serial.println(temp);
    }
    else
      update_lcd = false;

    ldr1 = analogRead(LDR1);
    ldr2 = analogRead(LDR2);

    if (Time_ + 1000 < millis())
    { if (ldr1 < 500 && ldr2 > 500 && !passed)
      { people_inside++;
        digitalWrite(pump, HIGH);
        Time_ = millis();
        passed = true;
      }
      else if (ldr1 > 500 && ldr2 < 500 && !passed )
      { people_inside--;
        Time_ = millis();
        passed = true;
      }
      else if (ldr1 > 500 && ldr2 > 500)
        passed = false;
    }

    pump_Time();
    check_people();


    if (temp > set_Temperature)
    { digitalWrite(fan, HIGH);
      humidityboo = true;
    }
    else if (!tempLow)
    {
      humidityboo = false;
      tempTime = millis();
      tempLow = true;
    }
    else
      fan_Time();

    {
      humidity = dht.readHumidity();
      if (!(humidity < set_humidity && humidity > set_humidity - 10))
        digitalWrite(fan, HIGH);
      else if (!humidity)
        digitalWrite(fan, LOW);
    }

    char status;
    status = pressure.startTemperature();
    if (status != 0) {
      delay(status);
      status = pressure.getTemperature(T);
      //      Serial.print(T);
      if (status != 0) {
        status = pressure.startPressure(3);
        if (status != 0)
        { delay(status);
          status = pressure.getPressure(P, T);
          if (status != 0)
          { if (update_lcd) {
              tempTime = millis();
              lcd_mode();
            }
            //            Serial.print("absolute pressure: ");
            //            Serial.print(P);
            //            Serial.println(" bar");
          }
        }
      }
    }
  }
}


void pump_Time() {
  if (Time_ + 1000 < millis())
    digitalWrite(pump, LOW);

}

void check_people() {
  if (people_inside > set_People )
  {
    tone(Buzzer, 850, 500);
    Buzzer_Active = true;
  }
  else {
    if (!fsr_Buz)
      noTone(Buzzer);
    Buzzer_Active = false;
  }

}

void fan_Time() {
  if (tempTime + 5000 < millis() && tempLow && check_pressure())
  { digitalWrite(fan, LOW);
    tempLow = false;
  }
}

bool check_pressure() {
  if (P <= pressure_Out)
  { digitalWrite(fan, HIGH);
    humidityboo = true;
    return false;
  }
  humidityboo = false;
  return true;
}

void setting_Temperature(char x) {
  if (x == '8')
    set_Temperature++;
  else if (x == '5')
    set_Temperature--;
  if (x == '8' || x == '5') {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Set temp :");
    lcd.print(set_Temperature);
    String str = "Max_Temp : ";
    str += String(set_Temperature);
    Serial.println(str);
  }
  if (x == '2')
  {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("temp :");
    lcd.print(temp);
    String str = "Temp : ";
    str += String(temp);
    Serial.println(str);
  }
}

void setting_People(char x) {
  if (x == '7')
    set_People++;
  else if (x == '4' && set_People > 0)
    set_People--;
  if (x == '7' || x == '4')
  {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Set People :");
    lcd.print(set_People);
    String str = "Max_people :";
    str += String(set_People);
    Serial.println(str);
  }
  else if (x == '1')
  { lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("People :");
    lcd.print(people_inside);
    Serial.print("People_In : ");
    Serial.println(people_inside);
  }
}


bool lcd_New() {
  if (dataTime + 1000 < millis())
  {
    mode_controller++;
    if (mode_controller == 8)
    {
      if (f_mode)
      {
        s_mode = true;
        f_mode = false;
        lcd.clear();
      }
      else
      {
        s_mode = false;
        f_mode = true;
        lcd.clear();
      }
      mode_controller = 0;
    }

    return true;
  }
  return false;
}

void lcd_mode()
{
  if (f_mode)
  {
    lcd.setCursor(0, 0);
    lcd.print("temp : ");
    lcd.print(temp);
    lcd.setCursor(0, 1);
    lcd.print("press: ");
    lcd.print(P);
    lcd.print("mb");
  }
  else
  {
    lcd.setCursor(0, 0);
    String str="Humidity: ";
    str+=String(humidity);
    lcd.print(str);
    lcd.setCursor(0, 1);
    lcd.print("Max People: ");
    lcd.print(set_People);
  }
}
