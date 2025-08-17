// Libraries 
#include <Wire.h> 
#include <WiFi.h>
#include <PubSubClient.h>
#include <DHTesp.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <ESP32Servo.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Arduino.h>
#include <time.h>
#include <numeric>  // Required for std::accumulate
#include <cmath>   // for log


// Definitions 
#define NTP_SERVER  "pool.ntp.org"
#define UTC_OFFSET 0
#define UTC_OFFSET_DST 0

#define SCREEN_WIDTH 128 // OLED display width, in pixels 
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define OLED_RESET -1 // Reset pin  -1  
#define SCREEN_ADDRESS 0x3C // Ox3D for 128x64

#define DHT 12
#define BUZZER 18
#define LED_1 15 
#define CANCEL 34 
#define UP 35 
#define DOWN 32 
#define OK 33 
#define DHT 12 
#define LDR 36
#define SERVO 16
#define BATTERY_ADC_PIN 39

float servoAngle = 0.0;



// Declare globally
float samplingInterval = 5.0;      // in seconds (default)
float sendingInterval = 120.0;     // in seconds (default)
float ldrValue = 0.0;
float ldrValues[100];
int readingIndex = 0;
unsigned long previousMillis = 0;


float MinAngle;
float IdealTemp;
float ControlFactor;

float Angle;
float Intensity;

float Temperature ;

Servo servoMotor;


// Define button press return values
#define PB_UP UP
#define PB_DOWN DOWN
#define PB_OK OK
#define PB_CANCEL CANCEL



// Object Declarations 
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire,OLED_RESET); 
DHTesp dhtSensor;

int n_notes = 8; 
int C = 262; 
int D = 294; 
int E = 330; 
int F = 349; 
int G = 392; 
int A = 440; 
int B = 494; 
int C_H = 523; 
int notes[] = {C, D, E, F, G, A, B, C_H}; 

int days = 0; 
int hours = 0; 
int minutes = 0; 
int seconds = 0; 

int last_displayed_sec = -1;
int last_displayed_min = -1;
int last_displayed_hour = -1;
int last_displayed_day = -1;


int n_alarms = 2; 
int alarm_hours[] = {0, 0}; 
int alarm_minutes[] = {0,0}; 
bool alarm_enabled[]={false,false};
bool alarm_triggered[] = {false, false};

unsigned long timeNow = 0; 
unsigned long timeLast = 0; 

int current_mode = 0; 
int max_modes = 5; 
String options[] = {"1 - Set Time", "2 - Set Alarm 1", "3 - Set Alarm 2", "4 - View Alarm",  "5 - Remove Alarm"}; 

float utc_offset = 0; // Store user-defined offset
int utc_offset_hr = 0; // Store user-defined offset
int utc_offset_min = 0; // Store user-defined offset

int prevHour = -1, prevMin = -1;
int Prev_TH=0;
//IoT part
char tempAr[6];
char AngleAr[6];
bool isScheduledON=false;
unsigned long scheduledOnTime;

WiFiClient espClient;
PubSubClient mqttClient(espClient);


WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);

//////


// function to print a line of text in a given text size and the given position
//text color will be white in the black background unless specified
void print_line(String text, int text_size, int row, int column , int mode=1) { 
  display.setTextSize(text_size);  //Normal 1:1 pixel scale =1 text size
  switch(mode){
    case 1:
      display.setTextColor(SSD1306_WHITE, SSD1306_BLACK);
      break; 
    case -1:
      display.setTextColor(SSD1306_BLACK , SSD1306_WHITE);
      break;  
    }
  display.setCursor(column, row); //start at (row,column) 
  display.println(text); 

  display.display(); 

}

const unsigned long debounceDelay = 50;
unsigned long lastDebounceTimes[4] = {0, 0, 0, 0};  // for UP, DOWN, CANCEL, OK

// Mapping button pins
const int buttonPins[4] = {UP, DOWN, CANCEL, OK};
const int buttonCodes[4] = {UP, DOWN, CANCEL, OK};  // Return values

int wait_for_button_press(unsigned long timeout = 5) {
  unsigned long startMillis = millis();

  while ((millis() - startMillis) < timeout * 1000) {
    for (int i = 0; i < 4; i++) {
      int pin = buttonPins[i];
      if (digitalRead(pin) == LOW) {
        // Debounce check
        if ((millis() - lastDebounceTimes[i]) > debounceDelay) {
          lastDebounceTimes[i] = millis();
          delay(200); // Acceptable here for UX simplicity
          return buttonCodes[i];
        }
      }
    }
  }

  return -1; // timeout
}


// function to automatically update the current time 
void update_time(void) { 
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    Serial.println("Failed to obtain time");
    print_line("Failed : Time obtaining " , 2, 0, 2, -1);
    delay(500);
    display.clearDisplay();

    print_line("Press Ok for Set Time manually" , 1, 0, 2, 1);
    print_line("Press cancel for Retry" , 1, 30, 2, 1);
    delay(1000);
    Time_Failed_Menu();
  }
  else{
  char day_str[8];  
  char hour_str[8]; 
  char min_str[8]; 
  char sec_str[8]; 
  strftime(day_str,8, "%d", &timeinfo); 
  strftime(sec_str,8, "%S", &timeinfo); 
  strftime(hour_str,8, "%H", &timeinfo); 
  strftime(min_str,8, "%M", &timeinfo); 

  days = atoi(day_str); 
  hours = atoi(hour_str); 
  minutes = atoi(min_str); 
  seconds = atoi(sec_str);
  print_time_now();

  }
  
} 


void Time_Failed_Menu(void){
  int btn_status = wait_for_button_press();  
  switch (btn_status) {
        case OK:
          update_time_manual();
          print_time_now();
          break;
        case -1:
          update_time_manual();
          print_time_now();
          break;
        case CANCEL:
          update_time();
          break;
        default:
          Serial.println("Invalid button status!");  
  }
}

//the manual timing method (at technical failure /absence of WiFi)
void update_time_manual(void){
  timeNow=millis()/1000;
  seconds=timeNow - timeLast;

  if (seconds >= 60){
    timeLast += 60;
    minutes += 1;
  }

  if (minutes == 60){
    minutes = 0;
    hours += 1;
  }

  if (hours == 24){
    hours =0;
    days += 1;
  }
}

// Optimizes screen updates by refreshing only the changed portions of the time display,
// preventing unnecessary full-screen redraws and reducing flickering.
// function to display the current time HH:MM:SS 
void print_time_now() {
  
  if (seconds != last_displayed_sec || minutes != last_displayed_min ||
      hours != last_displayed_hour || days != last_displayed_day) {
    print_line(":", 2, 20, 52);
    print_line(":", 2, 20, 84);
    if (days != last_displayed_day) {
      print_line("Day :" + String(days), 1, 0, 0);
      last_displayed_day = days;
      display.display();
    }

    if (hours != last_displayed_hour ){
      print_line(two_digitizer(hours), 2, 20, 30);
      last_displayed_hour = hours;
      display.display();
    }

    if (minutes != last_displayed_min ) {
      print_line(two_digitizer(minutes), 2, 20, 62);
      last_displayed_min = minutes;
      display.display();
    }

    if(seconds != last_displayed_sec){
      print_line(two_digitizer(seconds), 2, 20, 94);
      last_displayed_sec = seconds;
      display.display();
    }  
    }
}


//converting numbers into two digits
String two_digitizer(int value) {
  if (value >= 0) {
    if (value < 10) {
      return "0" + String(value);
    } else {
      return String(value);
    }
  } 
	else {
      return String(value);
  }
}


// function to automatically update the current time while checking for alarms 
void update_time_with_check_alarm() { 

  update_time();
  print_time_now(); 

  // check for alarms
  if (alarm_enabled) { 
  // iterating through all alarms 
    for (int i = 0; i < n_alarms ; i++) {
      if (alarm_enabled[i]  && hours == alarm_hours[i] && minutes == alarm_minutes[i]) { 
        ring_alarm(i); 
        alarm_triggered[i] = true; 
      } 
    } 
  } 
} 


// Ring an alarm 
void ring_alarm(int alarm_index) { 

  display.clearDisplay();
  
  if (!alarm_triggered[alarm_index]) {
    print_line("Medicine Time", 2, 0, 2, -1);
    print_line("Alarming", 2, 50, 2, -1);
  } else {
    print_line("Medicine Time", 2, 0, 2, -1);
    print_line("Snoozing", 2, 50, 2, -1);
  }

  // Light up LED 1 
  digitalWrite(LED_1, HIGH);
  delay(2000); 

  // Ring the buzzer 
  int btn_status = wait_for_button_press();  
  if (btn_status!=OK && btn_status!=CANCEL){
    for (int i = 0; i < n_notes; i++) { 
      tone(BUZZER, notes[i]); 
      delay(500); 
      noTone(BUZZER); 
      delay(2); 
    } 
  }
  else {
    digitalWrite(LED_1, LOW);
    noTone(BUZZER);
    alarm_triggered[alarm_index] = true;
    if (btn_status==OK){
      delay(300000);
      ring_alarm(alarm_index);
      return;
    }
    else if (btn_status==CANCEL){
      return;  
    }

  }
} 


//main function to reset time
void reset_time(){
  struct tm timeinfo;
  bool current_time_mode=getLocalTime(&timeinfo);
  switch (current_time_mode) {
    case true:
      reset_time_UTC();
      break;
    case false :
      reset_time_manual();
      break;
  }
  print_line("Time is set",2,20,0);
}

//set UTC time zone offset
void reset_time_UTC() { 
  while (true) { 
    display.clearDisplay();
    display.clearDisplay();
    print_line("Enter UTC Offset: ",2, 0, 2 );
    print_line(String(two_digitizer(utc_offset_hr)),2, 40, 20,-1 );
    print_line(":"+String(two_digitizer(utc_offset_min)), 2, 40, 50);   

    int btn_status_1 = wait_for_button_press();
    if (btn_status_1!=OK){
      switch (btn_status_1) {
        case PB_UP:
          utc_offset_hr++;
          if (utc_offset_hr > 12) utc_offset_hr = 12;  
          break;

        case PB_DOWN:
          utc_offset_hr--;
          if (utc_offset_hr < -12) utc_offset_hr = -12;  
          break;

        case PB_CANCEL:
          delay(200);
          return;
        }
      }
    else {break;}
    }
  while (true) { 
    display.clearDisplay();
    print_line("Enter UTC Offset: ",2, 0, 2 );
    print_line(String(two_digitizer(utc_offset_hr)),2, 40, 20);
    print_line(":"+String(two_digitizer(utc_offset_min)), 2, 40, 50,-1 );  

    int btn_status_2 = wait_for_button_press();
    if (btn_status_2!=OK){
      switch (btn_status_2) {
        case PB_UP:
        utc_offset_min += 15; 
        if (utc_offset_min > 45) utc_offset_min = 45;  
        break;

      case PB_DOWN:
        utc_offset_min -= 15; 
        if (utc_offset_min < 0) utc_offset_min = 0; 
        break;

      case PB_CANCEL:
        delay(200);
        return;
      }
    }
    else {break;}
  }
  utc_offset=utc_offset_hr + utc_offset_min/60.0;
  display.clearDisplay();
  print_line("Enter UTC Offset: " , 2, 0, 2);
  print_line(String(two_digitizer(utc_offset_hr))+":" +String(two_digitizer(utc_offset_min)) , 2, 40, 2);
  delay(1000);
  configTime(utc_offset * 3600, 0, NTP_SERVER);
}

//the manual timing method's reset time function (at technical failure /absence of WiFi)
void reset_time_manual() { 
  int Hour=0;
  int Minute=0;
  
  while (true) { 
    display.clearDisplay();
    print_line("Enter Current Time: ",2, 0, 2 );
    print_line(String(two_digitizer(Hour)),2, 40, 20,-1);
    print_line(":"+String(two_digitizer(Minute)), 2, 40, 50 );

    int btn_status_1 = wait_for_button_press();
    if (btn_status_1!=OK){
      switch (btn_status_1) {
        case PB_UP:
          Hour++;
          if (Hour > 23) Hour = 23; 
          break;

        case PB_DOWN:
          Hour--;
          if (Hour < 0) Hour = 0;  
          break;

        case PB_CANCEL:
          delay(200);
          return;
        }
      }
    else {break;}
    }
  while (true) { 
    display.clearDisplay();
    print_line("Enter Current Time: ",2, 0, 2 );
    print_line(String(two_digitizer(Hour)),2, 40, 20);
    print_line(":"+String(two_digitizer(Minute)), 2, 40, 50,-1 ); 

    int btn_status_2 = wait_for_button_press();
    if (btn_status_2!=OK){
      switch (btn_status_2) {
        case PB_UP:
        Minute++; 
        if (Minute > 59) Minute = 59;  
        break;

      case PB_DOWN:
        Minute--; 
        if (Minute < 0) Minute = 0;  
        break;

      case PB_CANCEL:
        delay(200);
        return;
      }
    }
    else {break;}
  }
  int Second =Hour*3600 + Minute*60;
  seconds+=Second;
  print_time_now();
  delay(1000);
    
}

// function to navigate through the menu 
void go_to_menu(void) { 
  while (digitalRead(CANCEL) == HIGH) { 
    display.clearDisplay(); 
    print_line(options[current_mode], 2, 0, 0); 

    int btn_status = wait_for_button_press(); 
    switch (btn_status) {
      case UP :
        current_mode += 1; 
        current_mode %= max_modes; 
        delay(200);
        break;

      case DOWN:
        current_mode = (current_mode - 1 + max_modes) % max_modes;
        delay(200);
        break;

      case OK:  
        Serial.println(current_mode); 
        delay(200); 
        run_mode (current_mode);
        break;
      
      case CANCEL: 
        delay(200); 
        return;  

      }
    }
  }




//function to run the modes
void run_mode(int mode) {
  switch (mode){
    case 0 :
      reset_time();
      break;
    case 1 :
      set_alarm(0);
      break;
    case 2 :
      set_alarm(1);
      break;
    case 3 :
      view_active_alarms();
      break;
    case 4 :
      remove_alarm();
      break;
    }
  }


//function to set alarm
void set_alarm(int alarm) { 
  int originalHour = alarm_hours[alarm];
  int originalMinute = alarm_minutes[alarm];
  //for keeping the original values safe(for cancel option)
  int Hour = originalHour;
  int Minute = originalMinute;
  
  while (true) { 
    display.clearDisplay();
    print_line("Enter Alarm Time: ",2, 0, 2 );
    print_line(String(two_digitizer(Hour)),2, 40, 20,-1);
    print_line(":"+String(two_digitizer(Minute)), 2, 40, 50 );

    int btn_status_1 = wait_for_button_press();
    if (btn_status_1!=OK){
      switch (btn_status_1) {
        case PB_UP:
          Hour++;
          if (Hour > 23) Hour = 23; 
          break;

        case PB_DOWN:
          Hour--;
          if (Hour < 0) Hour = 0;  
          break;

        case PB_CANCEL:
          alarm_hours[alarm] = originalHour;
          alarm_minutes[alarm] = originalMinute;
          delay(200);
          return;
        }
      }
    else {break;}
    }
  while (true) { 
    display.clearDisplay();
    print_line("Enter Alarm Time: ",2, 0, 2 );
    print_line(String(Hour),2, 40, 20);
    print_line(":"+String(Minute), 2, 40, 50,-1 ); 

    int btn_status_2 = wait_for_button_press();
    if (btn_status_2!=OK){
      switch (btn_status_2) {
        case PB_UP:
        Minute++; 
        if (Minute > 59) Minute = 59;  
        break;

      case PB_DOWN:
        Minute--; 
        if (Minute < 0) Minute = 0;  
        break;

      case PB_CANCEL:
        alarm_hours[alarm] = originalHour;
        alarm_minutes[alarm] = originalMinute;
        delay(200);
        return;
      }
    }
    else {break;}
  }
  alarm_hours[alarm]=Hour;
  alarm_minutes[alarm]=Minute;
  alarm_enabled[alarm] = true; 
  display. clearDisplay(); 
  print_line("Alarm "+String(alarm+1)+ " is set", 0, 0, 2); 
  delay(1000); 
}

//to view active alarm
void view_active_alarms() {
  display.clearDisplay();
  if (std::count(alarm_enabled, alarm_enabled + n_alarms, true)==0){print_line("No active alarm", 2, 0, 0);}
  else{
    for (int i = 0; i < n_alarms; i++) {
    if (alarm_enabled[i]==true ){
      print_line("Alarm " + String(i + 1) + ": " + 
                String(two_digitizer(alarm_hours[i])) + ":" + 
                String(two_digitizer(alarm_minutes[i])), 1, i * 10, 0);
  }
  }
  display.display();
  delay(3000);
}
}

//remove alarm
void remove_alarm() {
  display.clearDisplay();
  int cnt=std::count(alarm_enabled, alarm_enabled + n_alarms, true);
  if (cnt == 0) {
    display.clearDisplay();
    print_line("No Alarms Set", 2, 0, 30);
    delay(2000);
    return;
    }
  print_line("Select Alarm to Delete", 1, 0, 2);
  delay(2000);
  
  int alarm_index = 0;

  while (true) {
    display.clearDisplay();
    print_line("Delete Alarm ",1, 0, 0 );
    print_line("Alarm :"+String(alarm_index + 1),2, 40, 20);
    int btn_status = wait_for_button_press();
    switch (btn_status) {
        case UP:
          alarm_index = (alarm_index + 1) % n_alarms;
          break;

        case DOWN:
          alarm_index = (alarm_index - 1 + n_alarms) % n_alarms;  
          break;

        case OK:
          alarm_enabled[alarm_index] = false;
          display.clearDisplay();
          print_line("Alarm Deleted!", 2, 20, 30);
          delay(1000);
          return;  // Exit after deletion

        case CANCEL:
          return;
        }
    }
  }



//to monitor temperature & humidity and to trigger alarm when needed
int monitorTempHumidity(int prev=0) {
    TempAndHumidity data = dhtSensor.getTempAndHumidity();
    int Prev=prev;
    if (Prev==1){
      display.clearDisplay();
      last_displayed_sec = -1;
      last_displayed_min = -1;
      last_displayed_hour = -1;
      last_displayed_day = -1;
    }
    Temperature = data.temperature;
    float humidity = data.humidity;
    String(data.temperature,2).toCharArray(tempAr,6);

    // Add safety check for NaN values (sensor read failures)
    if (isnan(Temperature) || isnan(humidity)) {
        Serial.println("Failed to read from DHT sensor!");
        return prev;
    }

    bool tempWarning = (Temperature < 24.0 || Temperature > 32.0);
    bool humidWarning = (humidity < 65.0 || humidity > 80.0);

    if (tempWarning && humidWarning) {
        display.clearDisplay();
        print_line("Humidity & Temperature out of range", 2, 0, 2);
    } else if (tempWarning) {
        display.clearDisplay();
        print_line("Temperature out of range", 2, 0, 2);
    } else if (humidWarning) {
        display.clearDisplay();
        print_line("Humidity out of range", 2, 0, 2);
    } else {
        delay(500);
        return 0;
    }

    // Trigger alarm only if any warning was active
    digitalWrite(LED_1, HIGH);
    delay(2000);
    digitalWrite(LED_1, LOW);
    delay(2000);

    for (int i = 0; i < n_notes; i++) {
        tone(BUZZER, notes[i]);
        delay(500);
        noTone(BUZZER);
        delay(2);
    }


    return 1;
}


//IoT part

void setup(){
	Serial.begin(115200);

	pinMode (BUZZER, OUTPUT); 
  pinMode(LED_1, OUTPUT);
  pinMode(SERVO, INPUT); 
 

  pinMode(UP, INPUT);
  pinMode(DOWN, INPUT);
  pinMode(OK, INPUT);
  pinMode(CANCEL, INPUT);
  pinMode(DHT, INPUT);

  
  analogReadResolution(12);

  dhtSensor.setup(DHT, DHTesp::DHT22);
  servoMotor.setPeriodHertz(50);       // Standard servo PWM frequency
  servoMotor.attach(SERVO, 500, 2400);  // Pin, min/max pulse width in µs
  servoMotor.write(0); 

	if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) { 
    Serial.println(F("SSD1306 allocation failed")); 
    for (;;) ;// Don't proceed, loop forever
  } 

  display.display(); 
  delay(1000); 

  // Clear the buffer 
  display.clearDisplay(); 
  print_line("Welcome to Medibox", 2, 0, 0); // (String, text_size, cursor_row, cursor_column) 
  delay(5000);

	setupWiFi();
	setupMqtt();
  connectToBroker();

	timeClient.begin();
	timeClient.setTimeOffset(utc_offset);
	
  display.clearDisplay();
  print_line(":", 2, 20, 52);
  print_line(":", 2, 20, 84);
	pinMode(BUZZER,OUTPUT);
	digitalWrite(BUZZER, LOW);
}

void loop(){
	if (!mqttClient.connected()) {
    print_line("MQTT Failed", 1,58, 0);
    connectToBroker();
  }


	mqttClient.loop();

  servoMotor.write(Angle);
  
  //calculating battery percentage/voltage
  float batteryVoltage = readBatteryVoltage();
  int batteryPercent = calculateBatteryPercentage(batteryVoltage);
  print_line("|Battery: "+String(batteryPercent)+"%", 1, 0,48);
  mqttClient.publish("220563A-%", String(batteryPercent).c_str());

  updateLightReading();

  int tmp=Prev_TH;
  Prev_TH=monitorTempHumidity(Prev_TH);
  Serial.println(tempAr);
  if (mqttClient.connected()) {
    mqttClient.publish("220563A-TEMP", tempAr); 
  }

  if(Prev_TH!=1){
    if(tmp==1){
    display.clearDisplay();
    }
    update_time_with_check_alarm() ;
    print_line(" OK --> Menu ", 1, 45, 0,-1);

    
    int btn_status=wait_for_button_press(1);
    if (btn_status==OK){
      Serial.println("Button pressed!");
      go_to_menu();
      display.clearDisplay();
      update_time_with_check_alarm() ;
    }
  }
  gamma();
}


//function to update Light Intensity
void updateLightReading() {
  int maxReadings = sendingInterval / samplingInterval;
  if (maxReadings > 100) maxReadings = 100;

  unsigned long currentMillis = millis();

  if (currentMillis - previousMillis >= (unsigned long)(samplingInterval * 1000)) {
    previousMillis = currentMillis;

    int rawLDR = analogRead(LDR);

    float ldrValue = 1.0 - (rawLDR / 4063.0);
    ldrValue = constrain(ldrValue, 0.0, 1.0);

    // Store value for averaging
    ldrValues[readingIndex] = ldrValue;
    readingIndex++;

    if (readingIndex >= maxReadings) {
      float sum = 0.0;
      for (int i = 0; i < readingIndex; i++) {
        sum += ldrValues[i];
      }

      float average = sum / readingIndex;
      Intensity = average;
      mqttClient.publish("220563A-A-LDR", String(average, 4).c_str());

      readingIndex = 0;
    }
  }
}

//Function to set up the Wi-Fi
void setupWiFi(){
	unsigned long wifiTimeout = 10000; // timeout after 10 seconds
  unsigned long wifiStartTime = millis();

	Serial.println();
	Serial.print("connecting to ");
	Serial.println("Wokwi-GUEST");
	WiFi.begin("Wokwi-GUEST","");


	while (WiFi.status() != WL_CONNECTED){
		delay(500);
		Serial.print(".");

		//handling the situation of technical failure
    if (3*wifiTimeout>(millis() - wifiStartTime) > wifiTimeout) {
      Serial.println("WiFi is not connected");
      break;
    }
    else if (millis() - wifiStartTime > 3*wifiTimeout){
      Serial.println("Failed to connect to WiFI");
      print_line("Failed : WiFi connection " , 2, 0, 2, -1);
      delay(500);
      display.clearDisplay();

      print_line("Press Ok to switch to Manual mode" , 1, 0, 2, 1);
      print_line("Press cancel for Retry" , 1, 30, 2, 1);
      delay(1000);
      Time_Failed_Menu();
    }
    delay(250); 
    display.clearDisplay(); 
    print_line("Connecting to WiFi", 2, 0, 0);
  

	}
	if (WiFi.status() == WL_CONNECTED) {
    display.clearDisplay(); 
    print_line("Connected to WiFi", 2, 0, 0);

		//IoT
		Serial.print("WiFi connected");
		Serial.print("IP Address :");
		Serial.println(WiFi.localIP());


    configTime(UTC_OFFSET, UTC_OFFSET_DST, NTP_SERVER);
	}
}




void setupMqtt(){
	mqttClient.setServer("broker.hivemq.com", 1883);
	mqttClient.setCallback(receiveCallback);
}


void receiveCallback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");

  char payloadCharAr[length + 1];
	for (int i=0 ; i<length ; i++){
    payloadCharAr[i]=(char)payload[i];
		Serial.print((char)payload[i]);
	}

  payloadCharAr[length] = '\0'; // Null-terminate
  Serial.println();

  // Convert and assign floats based on topic
  if (strcmp(topic, "220563A-Servo") == 0) {
    servoAngle = atof(payloadCharAr);
    Serial.print("Updated servoAngle: ");
    Serial.println(servoAngle);
    
  } 
  else if (strcmp(topic, "220563A-Sampling") == 0) {
    samplingInterval = atof(payloadCharAr);
    Serial.print("Updated samplingInterval: ");
    Serial.println(samplingInterval);
  } 
  else if (strcmp(topic, "220563A-Sending") == 0) {
    sendingInterval = atof(payloadCharAr);
    Serial.print("Updated sendingInterval: ");
    Serial.println(sendingInterval);
  }
  else if (strcmp(topic,"220563A-UTC")==0){
    utc_offset = atof(payloadCharAr);
    Serial.print("Updated UTC: ");
    Serial.println(utc_offset);
    configTime(utc_offset * 3600, 0, NTP_SERVER);
	}
  else if (strcmp(topic,"220563A-MinAngle")==0){
    MinAngle = atof(payloadCharAr);
    Serial.print("Updated Minimum Angle: ");
    Serial.println(MinAngle);
	}
  else if (strcmp(topic,"220563A-IdealTemp")==0){
    IdealTemp = atof(payloadCharAr);
    Serial.print("Updated Ideal Temperature: ");
    Serial.println(IdealTemp);
	}
  else if (strcmp(topic,"220563A-ControlFactor")==0){
    ControlFactor = atof(payloadCharAr);
    Serial.print("Updated ControlFactor: ");
    Serial.println(ControlFactor);
	}
}

//function to calculate the servo angle
void gamma() {
    // Safety check to avoid invalid log argument or division by zero
    if (samplingInterval <= 0 || sendingInterval <= 0 || Temperature == 0) {
        Angle = MinAngle; // return minimum angle if inputs are invalid
    } else {
        // Compute the motor angle θ
        Angle = MinAngle + (180.0 - MinAngle) * Intensity * ControlFactor * log(sendingInterval / samplingInterval) * (IdealTemp / Temperature);
    }
    Serial.println(Angle );
    // Convert the angle to a string with 2 decimal places and publish
    String(Angle, 2).toCharArray(AngleAr, 6);
    mqttClient.publish("220563A-Servo", AngleAr);
}



void connectToBroker() {
  Serial.print("Connecting to MQTT broker...");

  String clientId = "ESP32Client-220563A-MSE";

  if (mqttClient.connect(clientId.c_str())) {
    Serial.println("Connected to MQTT.");
    mqttClient.subscribe("220563A-Servo");
    mqttClient.subscribe("220563A-Sampling");
    mqttClient.subscribe("220563A-Sending");
    mqttClient.subscribe("220563A-MinAngle");
    mqttClient.subscribe("220563A-IdealTemp");
    mqttClient.subscribe("220563A-ControlFactor");
    mqttClient.subscribe("220563A-UTC");
    mqttClient.subscribe("220563A-MAIN-ON-OFF");
  } else {
    Serial.print("MQTT Connect failed. Code: ");
    Serial.println(mqttClient.state());
    // Add detailed error interpretation
    switch (mqttClient.state()) {
      case -4: Serial.println("MQTT_CONNECTION_TIMEOUT"); break;
      case -3: Serial.println("MQTT_CONNECTION_LOST"); break;
      case -2: Serial.println("MQTT_CONNECT_FAILED"); break;
      case -1: Serial.println("MQTT_DISCONNECTED"); break;
      case 0:  Serial.println("MQTT_CONNECTING"); break;
      case 1:  Serial.println("MQTT_CONNECTED (Wrong protocol?)"); break;
      case 2:  Serial.println("MQTT_CONNECT_BAD_CLIENT_ID"); break;
      case 3:  Serial.println("MQTT_CONNECT_UNAVAILABLE"); break;
      case 4:  Serial.println("MQTT_CONNECT_BAD_CREDENTIALS"); break;
      case 5:  Serial.println("MQTT_CONNECT_UNAUTHORIZED"); break;
      default: Serial.println("Unknown error"); break;
    }

    delay(5000);  // Wait before retry
  }
}



float readBatteryVoltage() {
  int adcValue = analogRead(BATTERY_ADC_PIN);
  float voltage = adcValue * (3.3 / 4095.0); // if 3.3V ADC ref
  voltage *= 2.0; // If voltage divider is used (1:2)
  return voltage;
}

int calculateBatteryPercentage(float voltage) {
  float minVolt = 3.0;
  float maxVolt = 4.2;
  voltage = constrain(voltage, minVolt, maxVolt);
  return (int)(((voltage - minVolt) / (maxVolt - minVolt)) * 100);
}


