const int venstreKlokke = 25;
const int hojreKlokke = 18;
const int pulsePin = 26;
const int afbryderPin = 27;
const int ringPin = 5;

String inputString;

int gammelVenstreTilstand;
int gammelHojreTilstand;
int gammelPulseTilstand;
int gammelRingTilstand;
int gammelAfbryderTilstand;

long startPulsTid;
long slutPulsTid;
long tidSidenPuls;
long pulseTid;

int count;
String stringTal = "ATD+ +45";
byte udregn;
byte tal[8];

bool stringComplete = false;

void setup() {
  Serial.begin(115200);
  Serial2.begin(115200);

  pinMode(venstreKlokke, OUTPUT);
  pinMode(hojreKlokke, OUTPUT);
  pinMode(afbryderPin, INPUT_PULLUP);
  pinMode(ringPin, INPUT_PULLUP);
  pinMode(pulsePin, INPUT_PULLUP);
  
  inputString.reserve(200);

  Serial2.println("AT"); //start kommounikation med sim800l på Serial2
  delay(100);
  Serial2.println("AT+CMIC=0,15"); //sæt mic gain til +22.5db på main audio. Juster efter behov 
  delay(100);
  Serial2.println("AT+CLVL=100"); //skru højtaler op på 100%. juster efter behov
  delay(100);
  Serial2.println("AT+SNDLEVEL=0,100"); //skru helt op for klartone

//uncomment for at afprøve ringeklokken
  /*for (int i = 0; i < 30; i++) {
    ding(30, 10);
    dong(30, 10);
  }*/
}


void loop() {
  //tag telefonen og laeg paa
  int ringTilstand = digitalRead(ringPin);
  if (ringTilstand == LOW && gammelRingTilstand == HIGH) {
    delay(100);
    while (digitalRead(afbryderPin) == HIGH && digitalRead(ringPin) == LOW) {
      dingDong(2000);
      long tid = millis();
      while (tid + 2000 > millis() && digitalRead(afbryderPin) == HIGH && digitalRead(ringPin) == LOW) {}
    }
    Serial2.println("ATA");
    delay(500);
    while (digitalRead(afbryderPin) == HIGH) {}
    delay(500);
    Serial2.println("ATH");
  }
  gammelRingTilstand = ringTilstand;

  //tone naar roret loftes
  int afbryderTilstand = digitalRead(afbryderPin);
  if (afbryderTilstand == LOW && gammelAfbryderTilstand == HIGH) {
    Serial2.println("AT+CPAS"); //spørg om der er forbindelse
    delay(100);
    String streng;
    long tidSidenAfbrydning = millis();
    while (streng.indexOf("CPAS") < 0) { //svaret på "AT+CPAS" indeholder altid "CPAS" men vi skal lige læse serial2 først
      while (stringComplete == false) {
        serialEvent();
        if (tidSidenAfbrydning + 1000 < millis()) {//spørg om der er forbindelse
          Serial2.println("AT+CPAS");
          tidSidenAfbrydning = millis();
        }
      }
      streng = inputString;
      inputString = "";
      stringComplete = false;
    }
    //hvis svaret er "+CPAS: 0" er der forbindelse, så aktiver klartonen. Ellers bipper vi for at indikere mangel på forbindelse
    if (streng.indexOf("+CPAS: 0") > 0) {
      Serial2.println("AT+STTONE=1,1,60000");
    } else {
      Serial2.println("AT+STTONE=1,5,60000");
    }
  }
  
  //hvis der bliver lagt på stopper vi klartonen ved at aktivere en anden tone i 1 sekund. Weird 
  if (afbryderTilstand == HIGH && gammelAfbryderTilstand == LOW) {
    Serial2.println("AT+STTONE=1,3,400");
    count = 0;
    udregn = 0;
  }
  gammelAfbryderTilstand = afbryderTilstand;

  //ring op
  if (afbryderTilstand == LOW) {
    //mål antal pulse
    int pulseTilstand = digitalRead(pulsePin);
    if (pulseTilstand == HIGH && gammelPulseTilstand == LOW) {
      startPulsTid = millis();
    }
    if (pulseTilstand == LOW && gammelPulseTilstand == HIGH) {
      slutPulsTid = millis();
      pulseTid = slutPulsTid - startPulsTid;
      //debounce ved at sikre at pulsen er over 5 millis
      if (pulseTid > 5) {
        count++;
      }
    }
    tidSidenPuls = millis() - slutPulsTid;
    
    //pulsen er længere end 300 millis så vi ved den er stoppet med at pulse
    if (tidSidenPuls > 300 && count > 0) {
      count = count * (count != 10);//smart måde at lave 10 pulse om til tallet 0
      udregn++;
      stringTal += count; //tilføj nummeret på en string der holder telefonnummeret
      count = 0;
      if (udregn > 7) {//hvis der er 8 tal ringer vi op
        udregn = 0;
        stringTal += ";";
        Serial2.println("AT+STTONE=1,3,400");//stop klartonen
        delay(100);
        Serial2.println(stringTal);//ring op til telefonnummeret
        stringTal = "ATD+ +45";
        delay(100);
        while (digitalRead(afbryderPin) == LOW) {}//bliv på linjen så længe røret er løftet
        delay(500);
        Serial2.println("ATH");//læg på
      }
    }
    gammelPulseTilstand = pulseTilstand;
  }
}

void ding(int mellemrum, int aktiveringsTid) {
  digitalWrite(venstreKlokke, HIGH);
  delay(aktiveringsTid);
  digitalWrite(venstreKlokke, LOW);
  delay(mellemrum);
}

void dong(int mellemrum, int aktiveringsTid) {
  digitalWrite(hojreKlokke, HIGH);
  delay(aktiveringsTid);
  digitalWrite(hojreKlokke, LOW);
  delay(mellemrum);
}

void serialEvent() {
  while (Serial2.available()) {
    // get the new byte:
    char inChar = (char)Serial2.read();
    // add it to the inputString:
    inputString += inChar;
    // if the incoming character is a newline, set a flag so the main loop can
    // do something about it:
    if (inChar == '\n') {
      stringComplete = true;
      Serial.println(inputString);
    }
  }
}

void dingDong(int ringTid) {
  long tid = millis();
  while (tid + ringTid > millis() && digitalRead(ringPin) == LOW && digitalRead(afbryderPin) == HIGH) {
    ding(30, 10);
    dong(30, 10);
  }
}
