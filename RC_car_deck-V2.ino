#include <String.h>
#include <Servo.h>
#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>
RF24 radio(8, 7); // CE, CSN
const byte address[6] = "00222";

//DEKLARACJA PINOW
const int lista_pin_pwm[4] = {9,6,3,5};				//piny PWM - sterowanie napieciem DC
const int lista_pin_zwrot[4] = {14,4,2,15};			//		     sterowanie kieurnkiem obrotow

//STRUKTURA STEROWANIA SILNIKIEM
const float lista_korekta[4]= {1.0,1.0,1.0,1.0};
struct jednap{
  int pin_pwm;
  int pin_zwrot; 
  int idepwm;
  float pwm;
  byte zwrot;
  float korekta;
};
struct jednap silnik[4];

//ZMIENNE CZASOWE
unsigned long t,td,ts; 
const unsigned long okres_prob=1;
const float wsp_zmiany=0.06, wsp_skretu=50;
byte autko_stop;
byte autko_zwrot;
byte autko_skret;
byte autko_czolg;

//ZAKRESY
const int max_pwm=255, min_pwm=80, 
max_pwm_skret=255, min_pwm_skret=60, max_pwm_czolg=150;

//DEKLARACJA SERVA
Servo sg90;

void setup() {
  
//inicjalizacja serva
  pinMode(10,OUTPUT);
  sg90.attach(10);
  
//przypisanie stałych dla silnik[i]
  for(int i=0;i<=3;i++){
    silnik[i].pin_pwm=lista_pin_pwm[i];
    silnik[i].pin_zwrot=lista_pin_zwrot[i];
    silnik[i].korekta=lista_korekta[i];
    }
  delay(10);

//komunikacja
  Serial.begin(9600);
  radio.begin();
  radio.openReadingPipe(0, address);
  radio.setPALevel(RF24_PA_MIN);
  radio.startListening(); 
  delay(10);  

//zadeklarowanie pinow
  for (int i=0; i<=3;i++){
    pinMode(silnik[i].pin_pwm,OUTPUT);
    pinMode(silnik[i].pin_zwrot,OUTPUT);
  }

//ustawieni startowych wartosci
  for (int i=0; i<=3;i++){
    analogWrite(silnik[i].pwm,0);
    digitalWrite(silnik[i].zwrot, 0);
    silnik[i].idepwm=0;
    silnik[i].pwm=0;
    silnik[i].zwrot=0;
  }
  autko_stop=0;
  t=td=ts=millis();
  Serial.println("START");
}

//deklaracja zmiennych komunikacji
char odbior[32]="";
int pwm_bazowe, pwm_skret;
byte odbior_czeka;



void loop(){

//odbior danych od nrf
  if(radio.available()){
    odbior_czeka=1;
    radio.read(&odbior,sizeof(odbior));
    //Serial.println(odbior);
  }else odbior_czeka=0;
 
//odszyfrowanie 
  if(odbior_czeka==1){
    pwm_skret=map((odbior[2]-'0')*10+(odbior[3]-'0'),16,63,(180),(60));		//servo
    sg90.write(pwm_skret);
    //Serial.print(pwm_bazowe); Serial.print("|");
    pwm_bazowe=map((odbior[0]-'0')*10+(odbior[1]-'0'),16,63,-max_pwm,max_pwm);	//DC pwm
    //Serial.println(pwm_skret);
  }

  
  /*if(millis()-t>100){
    t=millis();
    pwm_bazowe=random(-max_pwm,max_pwm);
    pwm_skret=random(-max_pwm_skret,max_pwm_skret);
    odbior_czeka=1;
  } else odbior_czeka=0;*/


//limity pwm 
  if(odbior_czeka==1){
    pwm_bazowe=(pwm_bazowe<-min_pwm)?pwm_bazowe:(
      (pwm_bazowe<=min_pwm)?0:pwm_bazowe );
  
    /*pwm_skret=(pwm_skret<-min_pwm_skret)?pwm_skret:(
      (pwm_skret<=min_pwm_skret)?0:pwm_skret );*/
  }

//servo


//AKTUALIZACJA STANU
  if (odbior_czeka==1){	
    //t=millis();
    
    autko_stop=((odbior[4]-'0')==1)?0:1;
    autko_zwrot=(pwm_bazowe>-15)?0:1;
    autko_skret=(pwm_skret>0)?1:0;

    int przypadek= autko_zwrot*10 + autko_skret;
    
    pwm_bazowe=abs(pwm_bazowe); //pwm_skret=abs(pwm_skret);

      
    //if(autko_stop==0&&autko_czolg==0){
      for(int i=0;i<=3;i++)
            silnik[i].zwrot=autko_zwrot;


      if(autko_skret==1){
        silnik[0].idepwm=silnik[2].idepwm=pwm_bazowe; // *(max_pwm_skret-pwm_skret+wsp_skretu)/(max_pwm_skret+wsp_skretu);
        silnik[1].idepwm=silnik[3].idepwm=pwm_bazowe;  
      } 
      else {
        silnik[1].idepwm=silnik[3].idepwm=pwm_bazowe;// *(max_pwm_skret-pwm_skret+wsp_skretu)/(max_pwm_skret+wsp_skretu);
        silnik[0].idepwm=silnik[2].idepwm=pwm_bazowe;        
      }

      /*}else{
        for(int i=0;i<=3;i++)
        silnik[i].idepwm=0;   
      }*/
    //Serial.print(silnik[1].idepwm); Serial.print("\t"); Serial.println(silnik[1].pwm);
  }
  

//kontrola idepwm

  for  (int i=0;i<=3;i++){
    silnik[i].idepwm=(silnik[i].idepwm<min_pwm)?0:silnik[i].idepwm;
    }
    
//regulacja
  if(millis()-td>=okres_prob){
    td=millis();
    for(int i=0;i<=3;i++)
    {
    silnik[i].pwm=silnik[i].pwm+wsp_zmiany*(silnik[i].idepwm-silnik[i].pwm);
    }
  }

//pozwolenie
  /*if(autko_stop==1)
    for(int i=0;i<=3;i++){
      silnik[i].pwm=0;
    }*/

//wyprowadzenie 
  for (int i=0;i<=3;i++){
    digitalWrite(silnik[i].pin_zwrot, silnik[i].zwrot); 
    analogWrite(silnik[i].pin_pwm, silnik[i].pwm);
  }

}
