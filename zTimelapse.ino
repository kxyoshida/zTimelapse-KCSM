#define baudRate 9600 // or try 115200

#define encoder0PinA  2  // from encoder Pin A
#define encoder0PinB  4  // from encoder Pin B

#define OutputPin 3  // output to Vcc pin of TA7291P
#define ta5 10    //  output to 5-pin of TA7291P
#define ta6 11    //  output to 6-pin of TA7291P

#define uBound  7  // boundary safety switch, upper side
#define lBound  8  // boundary safety switch, lower side

#define trgPin 12
#define enablePin 13
#define ledA 5  // 62500 Hz
#define ledB 6  // 62500 Hz
#define ledC 9  // 31250 Hz

#define OUTMAX 255

#define MOTORPAUSE 200   //  in microseconds, required to suppress penetrating current
#define MOTORPULSEWIDTH 800   

#define TRGDUR 250    // trigger pulse for camera shutter in microseconds

#define STEPLIMIT 1000   // forbid movement of a large step

int zTolerance = 2;
int servoRate = 30;  // larger value may produce succseeful results for very small steps

int durA=1000;
int durB=1000;
int durC=1000;

int ampA=255;
int ampB=255;
int ampC=255;

// Parameters reserving target z-positions
int zPosG=0;
int zPosH=100;
int zPosI=-100;
int zPosJ=200;
int zPosK=-200;

volatile long encoderPos = 0;  // hidden absolute z-coordinate
long offset = 0; // will be used to reset new origin
// 1 unit equals approx 0.04 micro meter(?)

long Setpoint;

int incomingByte = 0;	// for incoming serial data
boolean locked=false;
int zPosNew=0;
int par=0;    // a variable for choosing command
char str[16];  // string buffer to temporally stpre the incoming number
int c=0;       // index for string buffer

void setup() { 
  pinMode(encoder0PinA, INPUT); 
  digitalWrite(encoder0PinA, HIGH);       // turn on pullup resistor
  pinMode(encoder0PinB, INPUT); 
  digitalWrite(encoder0PinB, HIGH);       // turn on pullup resistor

  attachInterrupt(0, doEncoder, CHANGE);  // encoder pin on interrupt 0 - pin 2

  pinMode(uBound, INPUT);
  digitalWrite(uBound, HIGH);
  pinMode(lBound, INPUT);  
  digitalWrite(lBound, LOW);

  pinMode(OutputPin, OUTPUT);
  pinMode(ta5, OUTPUT);
  digitalWrite(ta5, LOW);  
  pinMode(ta6, OUTPUT);
  digitalWrite(ta6, LOW);

  pinMode(trgPin, OUTPUT);
  digitalWrite(trgPin, HIGH);
  pinMode(enablePin, OUTPUT);
  digitalWrite(trgPin, LOW);  

  str[0] = '\0'; 
  offset = -encoderPos;
  Setpoint = encoderPos+offset;   

  Serial.begin (baudRate);
} 

int calcFeedback(long current, long target, float rate, int zerror, int outmax) {
  long gap = target - current;	// read input from interrupt
  if (abs(gap) <= zerror) {    
     return 0;
  } else {
     int output = int(rate * gap);
     if (output >= outmax) return outmax;
     else if (output <= -outmax) return -outmax;
     else return output;
  }
}

void loop(){
  int Output = calcFeedback(encoderPos+offset, Setpoint, servoRate, zTolerance, OUTMAX);
  if (locked)
    toServo(Output, MOTORPULSEWIDTH, MOTORPAUSE);

  if (Serial.available() > 0) {
    // read the incoming byte:
    incomingByte = Serial.read();

    if (par!=0) { // if commands reserving parameters were previously called
      // store the incoming chars into string buffer to be used as a parameter later
      str[c] = incomingByte;
      c++;
      if (c>15) {  // terminating "parameter" command mode by overflow
        str[0]='\0';
        c=0;
        par=0;
      } else {  // terminating "parameter" command mode by special characters
        switch (incomingByte) {
        case '\n':
          setPar(par);
          par=0;
          break;        
        case '\0':
          setPar(par);
          par=0;
          break;
        case '/':
          setPar(par);
          par=0;
          break;       
        }
      }
    } else { // if "par" commands had not been called
      switch (incomingByte) { // accept the following commands any time
      case 'l':  //  lock the focus
        locked=true;      
        break;        
      case 'u':  //  unlock the focus
        locked=false;
        Serial.print(encoderPos+offset,DEC);
        Serial.print('\t');
        Serial.println(Setpoint,DEC);                
        break;
      case 'r':  // reset Setpoint to current position
        Setpoint=encoderPos+offset;
        break;
      case 'R':  // reset z-origin to current position
        offset=-encoderPos;
        Setpoint=encoderPos+offset;
        break;
      case 'v':  //  getCurrentZValue
        Serial.println(encoderPos+offset,DEC);
        break;        
      case 'V':  //  getCurrentZValue
        Serial.print(encoderPos+offset,DEC);
        Serial.print('\t');
        Serial.println(Setpoint,DEC);        
        break;
      }

        switch (incomingByte) {
        case 'a':  // continuously light A ON
        //      Serial.println("chA ON");
          analogWrite(ledA, 255-ampA);   // set the LED on
          break;
        case 'b':  // continuously light B ON
          //      Serial.println("chB ON");
          analogWrite(ledB, 255-ampB);   // set the LED on      
          break;
        case 'c':  // continuously light C ON
          //      Serial.println("chC ON");
          analogWrite(ledC, 255-ampC);   // set the LED on      
          break;
        case 'o':  // turn off all LEDs
          //      Serial.println("LIGHT OFF");
          digitalWrite(ledA, HIGH);  // set the LED off      
          digitalWrite(ledB, HIGH);  // set the LED off    
          digitalWrite(ledC, HIGH);  // set the LED off   
          break; 
        case 't':  // manually trigger the camera shutter
          trigCamera();
          break;
        case 'A':  // trigger camera and flash LED A
          //      Serial.println("chA");
          trigCamera();        
          analogWrite(ledA, 255-ampA);   // set the LED on
          delay(durA);              // wait for a second
          digitalWrite(ledA, HIGH);    // set the LED off
          break;
        case 'B':  // trigger camera and flash LED B
          //      Serial.println("chB");
          trigCamera();
          analogWrite(ledB, 255-ampB);   // set the LED on
          delay(durB);              // wait for a second
          digitalWrite(ledB, HIGH);    // set the LED off
          break;
        case 'C':  // trigger camera and flash LED C
          //      Serial.println("chC");
          trigCamera();
          analogWrite(ledC, 255-ampC);   // set the LED on
          delay(durC);              // wait for a second
          digitalWrite(ledC, HIGH);    // set the LED off
          break;          
        case 'g':  //  set position to zPosG
          setNewPosition(zPosG);      
          break;              
        case 'h':  //  set position to zPosH
          setNewPosition(zPosH);      
          break;              
        case 'i':  //  set position to zPosI
          setNewPosition(zPosI);      
          break;              
        case 'j':  //  set position to zPosJ
          setNewPosition(zPosJ);      
          break;              
        case 'k':  //  set position to zPosK
          setNewPosition(zPosK);      
          break;                
          // The following commands invoke parameter-setting modes 
          // which will exit after receiving "\n".
        case 'd':  //  reserve the incoming number for durA
          par='d';
          break;
        case 'e':  //  reserve the incoming number for durB
          par='e';      
          break;
        case 'f':  //  reserve the incoming number for durC
          par='f';      
          break;      
        case 'D':  //  reserve the incoming number for ampA
          par='D';
          break;
        case 'E':  //  reserve the incoming number for ampB
          par='E';      
          break;
        case 'F':  //  reserve the incoming number for ampC
          par='F';      
          break;  
        case 'G':  //  reserve the incoming number for zPosG
          par='G';      
          break;              
        case 'H':  //  reserve the incoming number for zPosH
          par='H';      
          break;      
        case 'I':  //  reserve the incoming number for zPosI
          par='I';      
          break;      
        case 'J':  //  reserve the incoming number for zPosJ
          par='J';      
          break;      
        case 'K':  //  reserve the incoming number for zPosK
          par='K';      
          break;        
        case 'z':  // reserve the incoming number for immediate z-positioning
          par='z';      
          break;
        case 'Z':  // reserve the incoming number for setting z-tolerance
          par='Z';      
          break;          
        case 'S':  // reserve the incoming number for setting servoRate
          par='S';      
          break;          
        }
    }
  }
}

void doEncoder() {
  /* If pinA and pinB are both high or both low, it is spinning
   * forward. If they're different, it's going backward.
   *
   * For more information on speeding up this process, see
   * [Reference/PortManipulation], specifically the PIND register.
   */
  int statusA = digitalRead(encoder0PinA);
  int statusB = digitalRead(encoder0PinB);  

  if (statusA==statusB) {
    encoderPos--;
  } 
  else {
    encoderPos++;
  }
}

void toServo(int vel, int duration, int pause) {
  digitalWrite(ta5, LOW);
  digitalWrite(ta6, LOW);
  analogWrite(OutputPin,0); 
  delayMicroseconds(pause);    
  
  if (vel>0 && digitalRead(uBound)==HIGH) {   
    digitalWrite(ta5, HIGH);
    digitalWrite(ta6, LOW);
    analogWrite(OutputPin,vel);
  } 
  else if (vel<0 && digitalRead(lBound)==HIGH) { 
    digitalWrite(ta5, LOW);
    digitalWrite(ta6, HIGH);
    analogWrite(OutputPin,-vel); 
  }
  delayMicroseconds(duration);
  
  digitalWrite(ta5, LOW);
  digitalWrite(ta6, LOW);
  analogWrite(OutputPin,0); 
//  delayMicroseconds(pause);  
}

void setPar(int par) {
  // Once called, translate the string buffer content into an integer 
  // and assign it to the reserved parameter
  str[c] = '\0';
  switch (par) {
  case 'd':
    durA=atoi(str);
    /*
    Serial.print("Duration of LED A has been set to: ");  
     Serial.println(durA,DEC);
     */
    break;
  case 'e':
    durB=atoi(str);
    /*
    Serial.print("Duration of LED B has been set to: ");  
     Serial.println(durB,DEC);
     */
    break;
  case 'f':
    durC=atoi(str);
    /*
    Serial.print("Duration of LED C has been set to: ");  
     Serial.println(durC,DEC);
     */
    break;    
  case 'D':
    ampA=atoi(str);
    /*
    Serial.print("Amplitude of LED A has been set to: ");  
     Serial.println(ampA,DEC);
     */
    break;
  case 'E':
    ampB=atoi(str);
    /*
    Serial.print("Amplitude of LED B has been set to: ");  
     Serial.println(ampB,DEC);
     */
    break;
  case 'F':
    ampC=atoi(str);
    /*
    Serial.print("Amplitude of LED C has been set to: ");  
     Serial.println(ampC,DEC);
     */
    break;
  case 'G':
    zPosG=atoi(str);
    /*
    Serial.print("Reserved Target Position G has been set to: ");  
     Serial.println(zPosG,DEC);
     */
    break;        
  case 'H':
    zPosH=atoi(str);
    /*
    Serial.print("Reserved Target Position H has been set to: ");  
     Serial.println(zPosH,DEC);
     */
    break;        
  case 'I':
    zPosI=atoi(str);
    /*
    Serial.print("Reserved Target Position I has been set to: ");  
     Serial.println(zPosI,DEC);
     */
    break;        
  case 'J':
    zPosJ=atoi(str);
    /*
    Serial.print("Reserved Target Position J has been set to: ");  
     Serial.println(zPosJ,DEC);
     */
    break;        
  case 'K':
    zPosK=atoi(str);
    /*
    Serial.print("Reserved Target Position K has been set to: ");  
     Serial.println(zPosK,DEC);
     */
    break;        

  case 'z':
    zPosNew=atoi(str);
    setNewPosition(zPosNew);
    /*
    Serial.print("z-Position has been set to: ");  
     Serial.print(zPosNew,DEC);
     Serial.print('\t');
     Serial.println(Setpoint,DEC);  
     */
    break; 
  case 'Z':
    zTolerance=atoi(str);
    Serial.print("z-Tolerance has been set to: ");  
     Serial.println(zTolerance,DEC);
    break; 
  case 'S':
    servoRate = atoi(str);
    Serial.print("servoRate has been set to: ");  
     Serial.println(servoRate,DEC);
    break; 
    
  } 
  c=0;
}

void setNewPosition(int newPos) {
  if (abs(newPos-Setpoint) <= STEPLIMIT)
    Setpoint = newPos;
}

void trigCamera() {
  digitalWrite(enablePin, HIGH);
  delayMicroseconds(TRGDUR);  
  digitalWrite(trgPin, LOW);
  delayMicroseconds(TRGDUR);
  digitalWrite(trgPin, HIGH);
  digitalWrite(enablePin, LOW);             
}

