#include<String.h>
#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>
RF24 radio(8, 7); // CE, CSN
const byte address[6] = "00222";

byte z;
unsigned long t,td;
int Vrx0, Vry0;
int Vrx,Vry;
byte przycisk;
String koddowyslania;

void setup() {
  
  radio.begin();
  radio.openWritingPipe(address);
  radio.setPALevel(RF24_PA_MIN);
  radio.stopListening();
  delay(10);

  pinMode(9,OUTPUT);
  
  Serial.begin(9600);
  Vry0=analogRead(A5);
  Serial.print("Vry0 = ");Serial.println(Vry0);
  Vrx0=analogRead(A4);
  Serial.print("Vrx0 = ");Serial.println(Vrx0);
  
  t=td=millis();
  z=0;
}

void loop() {
  
  if(millis()-t>10){
    t=millis();
    
    Vry=map(analogRead(A5),0,2*Vry0,16,63);		// os y
    Vrx=map(analogRead(A4),0,2*Vry0,16,63);		// os x
    
    if(digitalRead(2)==0) przycisk=1; else przycisk =0;	// przycisk
    //Serial.println(digitalRead(2));
  koddowyslania=(String)Vry+(String)Vrx+(String)przycisk;	//kodowanie
  char wysylka[32]="";
  for(int i=0; i<=4;i++){
    wysylka[i]=koddowyslania[i];
    Serial.print(wysylka[i]);
  } Serial.print("\n");
  radio.write(&wysylka, sizeof(wysylka));	//wysylanie
  }
  if(millis()-td>1000){
    td=millis();
    z=!z;
    digitalWrite(9,z);	
  }
}
