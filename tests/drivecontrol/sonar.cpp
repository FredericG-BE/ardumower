#include "sonar.h"
#include "drivers.h"


SonarControl Sonar;

#define pinSonarCenterTrigger 24   // ultrasonic sensor pins
#define pinSonarCenterEcho 22
#define pinSonarRightTrigger 30    
#define pinSonarRightEcho 32
#define pinSonarLeftTrigger 34         
#define pinSonarLeftEcho 36


#define NO_ECHO 0
// ultrasonic sensor max echo time (WARNING: do not set too high, it consumes CPU time!)
#define MAX_ECHO_TIME 3000         


SonarControl::SonarControl(){  
  nextSonarTime = 0;
  sonarLeftUse      = 1;
  sonarRightUse     = 1;
  sonarCenterUse    = 0;
  sonarTriggerBelow = 900;    // ultrasonic sensor trigger distance
  sonarDistCenter = sonarDistLeft = sonarDistRight = 0;
  sonarDistCounter  = 0;             
}

void SonarControl::setup(){
  Console.println("SonarControl::setup");
  // sonar
  pinMode(pinSonarCenterTrigger, OUTPUT); 
  pinMode(pinSonarCenterEcho, INPUT); 
  pinMode(pinSonarLeftTrigger, OUTPUT); 
  pinMode(pinSonarLeftEcho, INPUT); 
  pinMode(pinSonarRightTrigger, OUTPUT); 
  pinMode(pinSonarRightEcho, INPUT); 
}

// call this in main loop
void SonarControl::run(){
  if (millis() < nextSonarTime) return; 
  nextSonarTime = millis() + 1000;
  if (sonarRightUse)  sonarDistRight  = readHCSR04(pinSonarRightTrigger,  pinSonarRightEcho);
  if (sonarLeftUse)   sonarDistLeft   = readHCSR04(pinSonarLeftTrigger,   pinSonarLeftEcho);
  if (sonarCenterUse) sonarDistCenter = readHCSR04(pinSonarCenterTrigger, pinSonarCenterEcho);
  
  if ((sonarDistCenter != NO_ECHO) && (sonarDistCenter < sonarTriggerBelow))
    sonarDistCounter++;    

  if ((sonarDistRight != NO_ECHO) && (sonarDistRight < sonarTriggerBelow))
    sonarDistCounter++;    

  if ((sonarDistLeft != NO_ECHO) && (sonarDistLeft < sonarTriggerBelow))
    sonarDistCounter++;     
}


// HC-SR04 ultrasonic sensor driver
unsigned int SonarControl::readHCSR04(int triggerPin, int echoPin){
  unsigned int uS;  
  digitalWrite(triggerPin, LOW); 
  delayMicroseconds(2); 
  digitalWrite(triggerPin, HIGH);
  delayMicroseconds(10); 
  digitalWrite(triggerPin, LOW);
  uS = pulseIn(echoPin, HIGH, MAX_ECHO_TIME + 1000);  
  if (uS > MAX_ECHO_TIME) uS = NO_ECHO;
  return uS;
}

void SonarControl::print(){
  Console.print("sonar L,C,R=");
  Console.print(sonarDistLeft);
  Console.print(sonarDistCenter);
  Console.print(sonarDistRight);  
  Console.println();
}  

  