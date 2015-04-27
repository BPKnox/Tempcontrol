
#include <OneWire.h>
#include <LiquidCrystal.h>
// DS18S20 Temperature chip i/o

#define ledPin 2


OneWire ds(1);  // on pin 1
LiquidCrystal lcd(7, 8, 9, 10, 11, 12);

long int prev_time_6 = millis();
int prev_state_6 = HIGH;
int unit = 0;


long int timer1_counter;


byte data[12];

byte smiley[8] = {
  0b00000,
  0b00000,
  0b01010,
  0b00000,
  0b00000,
  0b10001,
  0b01110,
  0b00000
};

byte degree[8] = {
  0b00110,
  0b01001,
  0b00110,
  0b00000,
  0b00000,
  0b00000,
  0b00000,
  0b00000
};

void setup(void) {
  pinMode(6, INPUT_PULLUP);  // PB to switch unit
  pinMode(ledPin, OUTPUT);
  
  lcd.begin(16, 2);          // set up the LCD's number of columns and rows: 
  lcd.createChar(1, degree);
  lcd.createChar(2, smiley);
  lcd.setCursor(0, 0);
  lcd.print("Temp: ");
  
  noInterrupts();
  TCCR1A = 0;
  TCCR1B = 0;
  timer1_counter = 3037;
  TCNT1 = timer1_counter;
  TCCR1B |= (1<<CS12);
  TIMSK1 |= (1<<TOIE1);
  interrupts();
  
  byte addr[8];
  ds.reset_search();
  if ( !ds.search(addr)) {    
      ds.reset_search();
      return;
  }
  
  ds.reset();
  ds.select(addr);
  ds.write(0x44,1);         // start conversion, with parasite power on at the end
}

void loop(void) {
  //byte i;
  byte present = 0;
  //byte addr[8];

  /*search for all temp sensors
  ds.reset_search();
  if ( !ds.search(addr)) {    
      //Serial.print("No more addresses.\n");
      ds.reset_search();
      return;
  }
  // addr[0] == 0x28 means Device is a DS18B20
  // addr[0] == 0x10 means Device is a DS1820
  */

  ///////////////////
  //delay(1000);     //!!!! maybe 750ms is enough, maybe not
  
  //present = ds.reset();  // we might do a ds.depower() here, but the reset will take care of it.

  //Convert to a temperature we can read
  int HighByte, LowByte, SignBit, Whole, Fract = 0;
  long int TReading, Tc_100, Tf_100 = 0;
  LowByte = data[0];
  HighByte = data[1];
  
  TReading = (HighByte << 8) + LowByte;
  if (Check_PB(6)){
    unit = !unit;
  }
  DisplayLCD(TReading, unit);
 
}


void DisplayLCD (long int RawTemp, int ForC){
  int SignBit, Whole, Fract = 0;
  long int Tc_100, Tf_100 = 0;
  
  SignBit = RawTemp & 0x8000;  // test most sig bit, if it is 1 then it is negative
  if (SignBit){ // If negative
    RawTemp = (RawTemp ^ 0xffff) + 1; // 2's comp
  }
  
  Tc_100 = (6 * RawTemp) + RawTemp / 4;    // multiply by (100 * 0.0625) or 6.25
  if (ForC == 1){
    Tf_100 = ((9* Tc_100) / 5);
    Whole = (Tf_100 / 100) + 32;  // separate off the whole and fractional portions
    Fract = Tf_100 % 100;
  }else{
    Whole = Tc_100 / 100;  // separate off the whole and fractional portions
    Fract = Tc_100 % 100;
  }
 
  lcd.setCursor(6,0);    // (note: line 1 is the second row, since counting begins with 0):
  if (SignBit){ // If its negative
     lcd.print("-");
  }

  lcd.print(Whole);
  lcd.print(".");
  lcd.print(Fract);
  lcd.write(1);

  if (ForC == 1){lcd.print("F");
  }
  else {lcd.print("C");
  }
  lcd.print(" ");
  lcd.setCursor(0,1);
  lcd.print(RawTemp);
}

boolean Check_PB(int pin){
  if(digitalRead(pin) == HIGH){
    if(((millis()-prev_time_6) > 50) && (prev_state_6 == LOW)){
      prev_state_6 = HIGH;
      //lcd.clear();
      //delay(1000);
      return true;
    }
    prev_state_6 = HIGH;
   //prev_state_6 = HIGH;
  }
  if(digitalRead(pin) == LOW){
    if(prev_state_6 == HIGH){
      prev_time_6 = millis();
    }
    prev_state_6 = LOW;
  }
  return false;
}   
  
ISR(TIMER1_OVF_vect) {      // interrupt service routine 
  TCNT1 = timer1_counter;   // preload timer
  digitalWrite(ledPin, digitalRead(ledPin) ^ 1); // blink LED
  
  byte addr[8];
  ds.reset_search();
  if ( !ds.search(addr)) {    
      ds.reset_search();
      return;
  }
  
  //ds.reset();          // Reset for temp??
  //ds.select(addr);     // choose specific probe to address??   
  ds.write(0xBE);      // Read Scratchpad (from last conversion)
  for (byte i = 0; i < 9; i++) {           // Save the 9 bytes (from Scratchpad)
    data[i] = ds.read();
  }
  
  ds.reset();
  ds.select(addr);
  ds.write(0x44,1);         // start conversion for next ISR, with parasite power on at the end
}


