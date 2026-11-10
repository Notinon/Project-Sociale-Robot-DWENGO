#include <Servo.h>
#include <LiquidCrystal.h>
#include <EEPROM.h>
#include <Wire.h>
#include <Dwenguino.h>
#include <LedController.hpp>

// Defines
#define BUTTON_PIN_SW_S SW_S
#define BUTTON_PIN_SW_N SW_N
#define BUTTON_PIN_SW_E SW_E
#define BUTTON_PIN_SW_C SW_C
#define BUTTON_PIN_SW_W SW_W

// Timers!!
unsigned long millisec = 0;
unsigned int sec = 0;
unsigned int mins = 0;
unsigned int hours = 0;
unsigned long offsetMillis = 0; // voor als de robot wordt opgestart we handmatig kunnen instellen wat het uur is

// Knoppen
const int Button_Down = SW_S;
const int Button_Up = SW_N;
const int Button_Settings = SW_C;
const int Button_Right = SW_E;
const int Button_Left = SW_W;

// Object Componenten
Servo hand_1;
Servo hand_2;

// POS STAAT VOOR POSITION CHRISTIAN.
int pos = 0;

// Selection initialisation
String selection = "";

// Pointer bullshit
String *selected = &selection; // Wordt gelezen als: Pointer selected point naar HET MEMORY ADRESS VAN(&) Selection
unsigned int *current_hour = &hours;
unsigned int *current_minute = &mins;

/*

  Wat is een pointer?:

  een pointer laat ons toe om rechstreeks een variabele
  die in memory staat aan te passen

  Waarom?

  Pointers laten ons veel gemakkelijker functies gebruiken
  omdat we rechtstreeks zeggen tegen de functie om te kijken op het memory adress waar de pointer
  naar point

*/

// String Collectiven
const String NL_Settings[5] = {"Taal", "Med/Dag", "Uur voor med", "Slaapstand", "Sportmode"};
const String EN_Settings[5] = {"Language", "Meds/Day", "Hour for meds", "Sleepmode", "Sportmode"};
const String FR_Settings[5] = {"Langue", "Medi/Jour", "Heure de medi", "Mode Veille", "Mode Sport"};
enum Taal {English, Nederlands, Francais};
const String Talen[3] = {"Eng", "Ned", "Fra"};
const String EN_Exercises[4] = {"Wave your hands", "Jump Jump!", "Turn around!", "Squat!"};
const String FR_Exercises[4] = {"Leve tes mains", "Saute Saute!", "Tournez-vous!", "Squat!"}; 
const String NL_Exercises[4] = {"Zwaai met armen", "Spring!", "Draai rond!", "Squat!"};

// Belangrijke Booleans
bool button_S_State = false;
bool Start_hasRun = false;
bool medsDone = false; 

// EEPROM adresses (Verander ze niet, ik meen het)
int AdressTaal = 0;
int AdressMeds = 0 + sizeof(Taal);
int AdressUur = AdressMeds + sizeof(int);
int AdressSleep = AdressUur + sizeof(int);
int AdressExer = AdressSleep + sizeof(bool);

// LED Matrix
LedController<2,1> LM = LedController<2,1>();
ByteBlock Gear[2] = {
  {
    0b00000000,
    0b00000000,
    0b11000000,
    0b11000000,
    0b11101000,
    0b11110000,
    0b00011110,
    0b00001110
  },
  {
    0b00001110,
    0b00011110,
    0b11110000,
    0b11101000,
    0b11000000,
    0b11000000,
    0b00000000,
    0b00000000
  }
};

ByteBlock Smile[2] = {
   {
    0b00000000,
    0b00000000,
    0b00000000,
    0b00000000,
    0b01111100,
    0b01111100,
    0b00000000,
    0b00000000
  },
  {
    0b00000000,
    0b00000000,
    0b00111110,
    0b00111110,
    0b00000000,
    0b00000000,
    0b00000000,
    0b00000000
  }
};

ByteBlock Gewicht[2] = {
  {
    0b10000000,
    0b10000000,
    0b10000000,
    0b11110000,
    0b11110000,
    0b11100000,
    0b11000000,
    0b11000000
  },
  {
    0b00000011,
    0b00000011,
    0b00000111,
    0b00001111,
    0b00001111,
    0b00000001,
    0b00000001,
    0b00000001
  }
};


// Settings Variables
String selectTaal;
int setTaal;
uint8_t Meds = 0;
unsigned int Uur;
int SlaapStand;
bool Exercise;
bool Periodic_Exer;

// Afstandsensor variabelen
const int Trig = 13;
const int Echo = 12;
const float Speed = 58;
long Duration = 0;
float Distance1 = 0;
float lastDistance = 0;
unsigned long lastMoveTime = 0;
const float threshold = 5.0; // Tolerantie
const unsigned long inactivityLimit = 30 * 60 * 1000UL; 

// Functies
float Calculate_distance_Sonar();
void Selection(String *selection);
void Wave_Attention();


void setup() {
  initDwenguino();

  auto conf = controller_configuration<2,1>();
  conf.useHardwareSpi = false;
  conf.SPI_CLK = 13;
  conf.SPI_MOSI = 2;
  conf.SPI_CS = 10;
  LM.init(conf);
  LM.activateAllSegments();
  LM.setIntensity(8);
  LM.clearMatrix();

  hand_1.attach(40);
  hand_2.attach(41);
  Serial.begin(9600);
  pinMode(Trig, OUTPUT);
  pinMode(Echo, INPUT);  
  Distance1 = round(Calculate_distance_Sonar());
  Serial.begin(9600);

  EEPROM.get(AdressTaal, setTaal);
  EEPROM.get(AdressMeds, Meds);
  EEPROM.get(AdressUur, Uur);
  EEPROM.get(AdressSleep, SlaapStand);
  EEPROM.get(AdressExer, Exercise);
}

void Startup() {
  hand_2.write(0);
  dwenguinoLCD.backlightOn();
  
  dwenguinoLCD.setCursor(0, 0);
  switch(setTaal){
    case 1: dwenguinoLCD.print("Goeiedag!");
    case 2: dwenguinoLCD.print("Bonjour!");
    default: dwenguinoLCD.print("Hello!");
  }
  delay(3000);

  bool confirm = false;
  
  while (!confirm) {
    dwenguinoLCD.clear();
    dwenguinoLCD.setCursor(0, 0);
    switch(setTaal){
      case 1: dwenguinoLCD.print("Stel uur in:");
      case 2: dwenguinoLCD.print("Quelle heure est-il: ");
      default: dwenguinoLCD.print("Please set the hour: ");
    }
    dwenguinoLCD.setCursor(0, 1);
    dwenguinoLCD.print(*current_hour); 
    dwenguinoLCD.print(" : ");
    dwenguinoLCD.print(*current_minute);

    if (digitalRead(Button_Up) == PRESSED) {
      *current_hour = (*current_hour + 1) % 24; 
      delay(200);
    }
    if (digitalRead(Button_Down) == PRESSED) {
      if (*current_hour == 0) *current_hour = 23; else (*current_hour)--;
      delay(200);
    }
    if (digitalRead(Button_Settings) == PRESSED) {
      confirm = true;
      delay(500);
    }
  }

  confirm = false;

  while (!confirm) {
    dwenguinoLCD.clear();
    dwenguinoLCD.setCursor(0, 0);
  switch(setTaal){
      case 1: dwenguinoLCD.print("Stel minuten in:");
      case 2: dwenguinoLCD.print("Quelle minute est-il: ");
      default: dwenguinoLCD.print("Please set the minutes: ");
    }
    dwenguinoLCD.setCursor(0, 1);
    dwenguinoLCD.print(*current_hour);
    dwenguinoLCD.print(" : ");
    dwenguinoLCD.print(*current_minute);

    if (digitalRead(Button_Up) == PRESSED) {
      *current_minute = (*current_minute + 1) % 60;
      delay(200);
    }
    if (digitalRead(Button_Down) == PRESSED) {
      if (*current_minute == 0) *current_minute = 59; else (*current_minute)--;
      delay(200);
    }
    if (digitalRead(Button_Settings) == PRESSED) {
      confirm = true;
      delay(500);
    }
  }

  offsetMillis = (*current_hour * 3600000UL) + (*current_minute * 60000UL) - millis();
  
  dwenguinoLCD.clear();
  dwenguinoLCD.print("Succes");
  delay(1000);
  for(int i = 0; i < 2; i++){
    LM.displayOnSegment(i, Smile[i]);
  }
}

void clock() {
  unsigned long currentMillis = millis() + offsetMillis;
  sec = (currentMillis / 1000) % 60;
  mins = (currentMillis / 60000) % 60;
  hours = (currentMillis / 3600000) % 24;
  
  if (hours == 0 && mins == 0 && sec == 0) medsDone = false; 
}

void settings() {
  int choice[5] = {0, 1, 2, 3, 4};
  int i = 0;
  bool updateNeeded = true; 

  for(int i = 0; i < 2; i++){
    LM.displayOnSegment(i, Gear[i]);
  }

  while (button_S_State == true) {
    clock();
    if (updateNeeded) {
      dwenguinoLCD.clear();
      dwenguinoLCD.setCursor(0, 0);
      switch(setTaal){
        case 1: dwenguinoLCD.print(NL_Settings[choice[i]]); break;
        case 2: dwenguinoLCD.print(FR_Settings[choice[i]]); break;
        default: dwenguinoLCD.print(EN_Settings[choice[i]]); break;
      }
      updateNeeded = false;
    }

    if (digitalRead(Button_Right) == PRESSED) {
      i++;
      if (i > 4) i = 0; 
      updateNeeded = true;
      delay(200);
    }

    if (digitalRead(Button_Left) == PRESSED) {
      i--;
      if (i < 0) i = 4;
      updateNeeded = true;
      delay(200);
    }

    if (digitalRead(Button_Up) == PRESSED) {
      switch(setTaal){
        case 1: selection = NL_Settings[choice[i]]; break;
        case 2: selection = FR_Settings[choice[i]]; break;
        default: selection = EN_Settings[choice[i]]; break;
      }
      delay(200);
    }

    if (*selected != "") {
      Selection(selected);
      updateNeeded = true;
    }

    if (digitalRead(Button_Settings) == PRESSED) {
      button_S_State = false;
      dwenguinoLCD.clear();
      delay(200);
    }
  }
}

void Selection(String *selection) {
  int j = 0;
  bool updateSub = true;

  if (*selection == "Taal" || *selection == "Langue" || *selection == "Language") {
    while (digitalRead(Button_Down) != PRESSED) {
      clock();
      if (updateSub) {
        dwenguinoLCD.clear();
        dwenguinoLCD.setCursor(0, 0);
        dwenguinoLCD.print(*selection);
        dwenguinoLCD.setCursor(0, 1);
        dwenguinoLCD.print(Talen[j]);
        updateSub = false;
      }

      if (digitalRead(Button_Right) == PRESSED) {
        j++; if (j > 2) j = 0;
        updateSub = true;
        delay(200);
      }
      if (digitalRead(Button_Left) == PRESSED) {
        j--; if (j < 0) j = 2;
        updateSub = true;
        delay(200);
      }
      if (digitalRead(Button_Up) == PRESSED) {
        EEPROM.update(AdressTaal, (uint8_t)j);
        setTaal = (uint8_t)j;
        dwenguinoLCD.clear();
        switch (setTaal) {
          case 1: dwenguinoLCD.print("Succesvol"); break;
          case 2: dwenguinoLCD.print("Succès"); break;
          default: dwenguinoLCD.print("Succes"); break;
        }
        delay(1000);
        break;
      }
    }
  }

  if (*selection == "Med/Dag" || *selection == "Medi/Jour" || *selection == "Meds/Day") {
    while (digitalRead(Button_Down) != PRESSED) {
      clock();
      if (updateSub) {
        dwenguinoLCD.clear();
        dwenguinoLCD.setCursor(0, 0);
        dwenguinoLCD.print(*selection);
        dwenguinoLCD.setCursor(0, 1);
        dwenguinoLCD.print(Meds);
        updateSub = false;
      }
      if (digitalRead(Button_Right) == PRESSED) {
        Meds++; updateSub = true;
        delay(200);
      }
      if (digitalRead(Button_Left) == PRESSED) {
        Meds--; updateSub = true;
        delay(200);
      }
      if (digitalRead(Button_Up) == PRESSED) {
        EEPROM.update(AdressMeds, Meds);
        dwenguinoLCD.clear();
        dwenguinoLCD.print("OK");
        delay(1000);
        break;
      }
    }
  }

  if (*selection == "Uur voor med" || *selection == "Heure de medi" || *selection == "Hour for meds") {
    while (digitalRead(Button_Down) != PRESSED) {
      if (updateSub) {
        dwenguinoLCD.clear();
        dwenguinoLCD.setCursor(0, 0);
        dwenguinoLCD.print(*selection);
        dwenguinoLCD.setCursor(0, 1);
        dwenguinoLCD.print(Uur);
        updateSub = false;
      }
      if (digitalRead(Button_Right) == PRESSED) {
        Uur = (Uur + 1) % 24; updateSub = true;
        delay(200);
      }
      if (digitalRead(Button_Left) == PRESSED) {
        if (Uur == 0) Uur = 23; else Uur--; updateSub = true;
        delay(200);
      }
      if (digitalRead(Button_Up) == PRESSED) {
        EEPROM.update(AdressUur, Uur);
        dwenguinoLCD.clear();
        dwenguinoLCD.print("OK");
        delay(1000);
        break;
      }
    }
  }

  if (*selection == "Slaapstand" || *selection == "Mode Veille" || *selection == "Sleepmode" ) {
    while (digitalRead(Button_Down) != PRESSED) {
      clock();
      if (updateSub) {
        dwenguinoLCD.clear();
        dwenguinoLCD.setCursor(0, 0);
        dwenguinoLCD.print(*selection);
        dwenguinoLCD.setCursor(0, 1);
        switch(setTaal){
          case 1: dwenguinoLCD.print(SlaapStand == 1 ? "Aan" : "Uit"); break;
          case 2: dwenguinoLCD.print(SlaapStand == 1 ? "Oui" : "Non"); break;
          default: dwenguinoLCD.print(SlaapStand == 1 ? "On" : "Off"); break;
        }
        updateSub = false;
      }
      if (digitalRead(Button_Right) == PRESSED) {
        SlaapStand = 1; updateSub = true;
        delay(200);
      }
      if (digitalRead(Button_Left) == PRESSED) {
        SlaapStand = 0; updateSub = true;
        delay(200);
      }
      if (digitalRead(Button_Up) == PRESSED) {
        EEPROM.update(AdressSleep, SlaapStand);
        dwenguinoLCD.clear();
        dwenguinoLCD.print("OK");
        delay(1000);
        break;
      }
    }
  }

  if (*selection == "Sportmode" || *selection == "Mode Sport" || *selection == "Sportmode") {
    while (digitalRead(Button_Down) != PRESSED) {
      clock();
      if (updateSub) {
        dwenguinoLCD.clear();
        dwenguinoLCD.setCursor(0, 0);
        dwenguinoLCD.print(*selection);
        dwenguinoLCD.setCursor(0, 1);
        switch(setTaal){
          case 1: dwenguinoLCD.print(Exercise ? "AAN" : "UIT"); break;
          case 2: dwenguinoLCD.print(Exercise ? "ALLUMÉ" : "ÉTEINT"); break;
          default: dwenguinoLCD.print(Exercise ? "ON" : "OFF"); break;
        }
        updateSub = false;
      }
      if (digitalRead(Button_Right) == PRESSED) {
        Exercise = true; updateSub = true;
        delay(200);
      }
      if (digitalRead(Button_Left) == PRESSED) {
        Exercise = false; updateSub = true;
        delay(200);
      }
      if (digitalRead(Button_Up) == PRESSED) {
        EEPROM.update(AdressExer, Exercise);
        dwenguinoLCD.clear();
        dwenguinoLCD.print("OK");
        delay(1000);
        break;
      }
    }
  }

  *selection = "";
  dwenguinoLCD.clear();
  dwenguinoLCD.print("Back...");
  delay(500);
}

void Medicine_taken() {
  bool taken = false;
  clock();
  Wave_Attention();
  dwenguinoLCD.clear();
  dwenguinoLCD.setCursor(0, 0);
  while (taken == false) {
    dwenguinoLCD.print("Medicine time!");
    dwenguinoLCD.setCursor(0, 1);
    dwenguinoLCD.print("Taken: " + (String)Meds + " y/n");
    if (digitalRead(Button_Down) == PRESSED) {
      dwenguinoLCD.clear();
      dwenguinoLCD.print("Please take.");
      delay(500);
    }
    if (digitalRead(Button_Up) == PRESSED) {
      dwenguinoLCD.clear();
      dwenguinoLCD.print("Thank you!");
      delay(500);
      taken = true;
      medsDone = true; 
      return;
    }
  }
}

void Wave_Attention() {
  clock();
  for (int w = 0; w < 3; w++) {
    for (pos = 70; pos <= 110; pos++) {
      hand_1.write(pos);
      hand_2.write(180 - pos);
      delay(10);
    }
    for (pos = 110; pos >= 70; pos--) {
      hand_1.write(pos);
      hand_2.write(180 - pos);
      delay(10);
    }
  }
}


void Come_on_lets_exercise(bool exercise) {
  clock();
  if (exercise == false) return;
  LM.clearMatrix();
  for(int j = 0; j < 2; j++){
    LM.displayOnSegment(j, Gewicht[j]);
  }
  for (int i = 0; i < 4; i++) {
    dwenguinoLCD.clear();
    dwenguinoLCD.setCursor(0, 0);
    switch(setTaal){
      case 1: dwenguinoLCD.print(NL_Exercises[i]); break;
      case 2: dwenguinoLCD.print(FR_Exercises[i]); break;
      default: dwenguinoLCD.print(EN_Exercises[i]); break;
    }
    delay(1000);
    for (pos = 0; pos < 180; pos++) {
      hand_1.write(pos);
      hand_2.write(pos);
      delay(10);
    }
    for (int k = 180; k >= 0; k--) { 
      hand_1.write(k);
      hand_2.write(k);
      delay(10);
    }
  }
} 



float Calculate_distance_Sonar() {
  digitalWrite(Trig, LOW);
  delayMicroseconds(2);
  digitalWrite(Trig, HIGH);
  delayMicroseconds(10);
  digitalWrite(Trig, LOW);
  Duration = pulseIn(Echo, HIGH, 30000);
  if (Duration == 0) return 400; // Geest signalen van sonar preventeren.
  return (Duration / Speed);
}

bool is_Inactivity(){
  float currentDistance = Calculate_distance_Sonar();
  if(abs(lastDistance - currentDistance) > threshold){
    lastDistance = currentDistance;
    lastMoveTime = millis();
  }
  return (millis() - lastMoveTime >= inactivityLimit);
}

void loop() {
  for(int i = 0; i < 2; i++){
    LM.displayOnSegment(i, Smile[i]);
  }

  if (Start_hasRun == false) {
    Startup();
    Start_hasRun = true;
  }

  Serial.write(Distance1);
  clock();

  if (Uur == hours && medsDone == false) {
    Medicine_taken();
  }
  if (digitalRead(Button_Settings) == PRESSED) {
    button_S_State = true;
    delay(200);
    settings();
  }
  
  if (Exercise == true && is_Inactivity() && (Calculate_distance_Sonar() <= 150)) {
    int input = -1;
    dwenguinoLCD.clear();
    switch(setTaal){
      case 1:
        dwenguinoLCD.print("Wil je sporten?");
        dwenguinoLCD.setCursor(0,1);
        dwenguinoLCD.print("Ja/Nee");
        break;
      case 2:
        dwenguinoLCD.print("Un peu d'exercise?");
        dwenguinoLCD.setCursor(0,1);
        dwenguinoLCD.print("Oui/Non");
        break;
      default:
        dwenguinoLCD.print("Wish to exercise?");
        dwenguinoLCD.setCursor(0, 1);
        dwenguinoLCD.print("Yes/No");
        break;
    }
    delay(1000);
    while (input == -1){
      if (digitalRead(Button_Left) == PRESSED) input = 1;
      if (digitalRead(Button_Right) == PRESSED) input = 0;
    } 
    Periodic_Exer = (bool)input;
    Come_on_lets_exercise(Periodic_Exer);
  }

  if (SlaapStand == 1 && (Calculate_distance_Sonar() >= 200) && is_Inactivity()){ 
    dwenguinoLCD.clear();
    dwenguinoLCD.backlightOff();
    Wave_Attention();
    while(digitalRead(Button_Settings) != PRESSED && Calculate_distance_Sonar() >= 150){
      clock();
      if(Uur == hours && medsDone == false){
        Medicine_taken();
      }
      delay(500);
    }
    dwenguinoLCD.backlightOn();
    dwenguinoLCD.setCursor(0,0);
    switch(setTaal){
      case 1: dwenguinoLCD.print("O! Welkom terug!"); break;
      case 2: dwenguinoLCD.print("Ou! Bon retour!"); break;
      default: dwenguinoLCD.print("Oh! Welcome back!"); break;
    }
    delay(2000);
    lastMoveTime = millis(); 
  }
}