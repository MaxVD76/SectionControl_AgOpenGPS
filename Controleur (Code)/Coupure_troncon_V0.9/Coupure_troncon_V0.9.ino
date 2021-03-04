    /* V0.9 - 04/03/2020 - Maxime Viaud    10kohm resistors(without internal pulldown)
    *  Adapted from "machine USB code", implemandted with my personnal code
    *  Section control + button control (mode auto and each section button control) + main switch to switch on/off the whole boom (all sections)
    *  Connected to the Relay Port in AgOpenGPS
    *  If you find any mistakes or have an idea to improove the code, feel free to contact me. N'hésitez pas à me contacter en cas de problème ou si vous avez une idée d'amelioration.
    */
    
  //loop time variables in microseconds

  #include <EEPROM.h> 
  #define EEP_Ident 0x4310  

    //Program counter reset
    void(* resetFunc) (void) = 0;

  //Variables for config - 0 is false  
  struct Config {
  byte raiseTime = 2;
  byte lowerTime = 4;
  byte enableToolLift = 0;
  byte isRelayActiveHigh = 0; //if zero, active low (default)
  
  };  Config aogConfig;   //4 bytes
  
  const byte LOOP_TIME = 200; //5hz
  unsigned long lastTime = LOOP_TIME;
  unsigned long currentTime = LOOP_TIME;
  unsigned long fifthTime = 0;
  unsigned int count = 0;

  //Comm checks
  byte watchdogTimer = 0; //make sure we are talking to AOG
  byte serialResetTimer = 0; //if serial buffer is getting full, empty it
  
   //Communication with AgOpenGPS
  bool isDataFound = false, isDataFound2 = false, isSettingFound = false, isAogConfigFound = false, isRelayActiveHigh = true;
  int header = 0, tempHeader = 0, temp, EEread = 0;

  //The variables used for storage
  byte relayHi=0, relayHiT=0, relayLo = 0, relayLoT, gpsSpeed = 0, tramline = 0, empty =0, tree = 0, uTurn = 0, hydLift = 0, SectSWOffToAOGHi = 0, SectSWOffToAOGLo = 0, SectMainToAOG = 0;

  byte raiseTimer = 0, lowerTimer = 0, lastTrigger = 0;

  byte emptyR=0, relayHiR=0, relayLoR=0, SectSWOffFromAOGHi = 0, SectSWOffFromAOGLo = 0, SectMainFromAOG = 0;  //bytes storing data received from PGN 32 761

  byte mainon=0b00000000, mainoff=0b00000000, sect=0b00000000; //on or off all the boom

  
  //Raise and lower as D4 and D3
  #define RAISE 4
  #define LOWER 3

  int startTime =0;
//////////////////////////////////////////////////
const int  button1Pin = A0;
const int  button2Pin = A1;
const int  button3Pin = A2;
const int  button4Pin = A3;
const int  button5Pin = A4;
const int  button6Pin = A5;
const int  button7Pin = 2;
const int  button8Pin = 3;
const int  buttonmodePin = 5;// mode mean auto/manual (physical button which switch the software button)
const int  buttonboomoffPin = 4;
const int  buttonboomonPin = 3;

const int sect1Pin = 6;
const int sect2Pin = 7;
const int sect3Pin = 8;
const int sect4Pin = 9;
const int sect5Pin = 10;
const int sect6Pin = 11;
const int sect7Pin = 12;
const int sect8Pin = 13;

// Variables will change:
int button1PushCounter = 0;   
int button1State = 0;         
int lastButton1State = 0;

int button2PushCounter = 0;   
int button2State = 0;         
int lastButton2State = 0; 

int button3PushCounter = 0;   
int button3State = 0;         
int lastButton3State = 0; 

int button4PushCounter = 0;   
int button4State = 0;         
int lastButton4State = 0; 

int button5PushCounter = 0;   
int button5State = 0;         
int lastButton5State = 0; 

int button6PushCounter = 0;   
int button6State = 0;         
int lastButton6State = 0; 

int button7PushCounter = 0;   
int button7State = 0;         
int lastButton7State = 0; 

int button8PushCounter = 0;   
int button8State = 0;         
int lastButton8State = 0; 

int buttonmodePushCounter = 0;   
int buttonmodeState = 0;         
int lastButtonmodeState = 0;

int buttonboomonswPushCounter = 0;   
int buttonboomonswState = 0;         
int lastButtonboomonswState = 0;

int buttonboomoffswPushCounter = 0;   
int buttonboomoffswState = 0;         
int lastButtonboomoffswState = 0;

bool modeauto = false;
///////////////////////////////////////////////////////////
void setup()
  {
    //set the baud rate
     Serial.begin(38400);  
     while (!Serial) { ; } // wait for serial port to connect. Needed for native USB
     
     EEPROM.get(0, EEread);              // read identifier
    
  if (EEread != EEP_Ident)   // check on first start and write EEPROM
  {           
    EEPROM.put(0, EEP_Ident);
    EEPROM.put(6, aogConfig);
  }
  else 
  { 
    EEPROM.get(6, aogConfig);
  }

  //set the pins to be outputs (pin numbers)
  pinMode(LOWER, OUTPUT);
  pinMode(RAISE, OUTPUT);

  // initialize the button pin as a input:
  pinMode(button1Pin, INPUT);
  pinMode(button2Pin, INPUT);
  pinMode(button3Pin, INPUT);
  pinMode(button4Pin, INPUT);
  pinMode(button5Pin, INPUT);
  pinMode(button6Pin, INPUT);
  pinMode(button7Pin, INPUT);
  pinMode(button8Pin, INPUT);
  
  pinMode(buttonmodePin, INPUT);
  pinMode(buttonboomoffPin, INPUT);
  pinMode(buttonboomonPin, INPUT);

  // initialize the LED/relay as an output:
  pinMode(sect1Pin, OUTPUT);
  pinMode(sect2Pin, OUTPUT);
  pinMode(sect3Pin, OUTPUT);
  pinMode(sect4Pin, OUTPUT);
  pinMode(sect5Pin, OUTPUT);
  pinMode(sect6Pin, OUTPUT);
  pinMode(sect7Pin, OUTPUT);
  pinMode(sect8Pin, OUTPUT);


}

void loop()
{
  //Loop triggers every 200 msec and sends back gyro heading, and roll, steer angle etc

  currentTime = millis();
  unsigned int time = currentTime;

  if (currentTime - lastTime >= LOOP_TIME)
  {
    lastTime = currentTime;

    //If connection lost to AgOpenGPS, the watchdog will count up 
    if (watchdogTimer++ > 250) watchdogTimer = 12;

    //clean out serial buffer to prevent buffer overflow
    if (serialResetTimer++ > 20)
    {
      while (Serial.available() > 0) char t = Serial.read();
      serialResetTimer = 0;
    }

    if (watchdogTimer > 10) 
    {
      relayLo = 0;
      relayHi = 0;
    }

    //countdown if not zero, make sure up only
    if (raiseTimer) 
    {
      raiseTimer--;
      lowerTimer = 0;
    }
    if (lowerTimer) lowerTimer--; 

    //section relays
    SetRelays();

    Buttons();
    
    //Send data back to agopenGPS, renvoi des données à Ag Open GPS

    Serial.print(127);//V4: PGN 127 249 (32 761) for section control
    Serial.print(",");
    Serial.print(249); 
    Serial.print(",");
    Serial.print(empty); // not interesting for us 
    Serial.print(",");
    Serial.print(empty); // not interesting for us 
    Serial.print(",");
    Serial.print(empty); // not interesting for us 
    Serial.print(",");
    Serial.print(relayHiT); 
    Serial.print(",");
    Serial.print(relayLoT);
    Serial.print(",");
    Serial.print(SectSWOffToAOGHi);
    Serial.print(",");
    Serial.print(SectSWOffToAOGLo);
    Serial.print(",");
    Serial.println(SectMainToAOG);

     
   Serial.flush();   // flush out buffer
   
  } //end of timed loop

  //****************************************************************************************
  //This runs continuously, outside of the timed loop, keeps checking UART for new data
        // PGN - 32762 - 127.250 0x7FFA
        //public int mdHeaderHi, mdHeaderLo = 1, mdSectionControlByteHi = 2, mdSectionControlByteLo = 3,
        //mdSpeedXFour = 4, mdUTurn = 5, mdTree = 6, mdHydLift = 7, md8 = 8, md9 = 9;
  if (Serial.available() > 0 && !isDataFound && !isAogConfigFound) //find the header, 127H + 254L = 32766
  {
    int temp = Serial.read();
    header = tempHeader << 8 | temp;                //high,low bytes to make int
    tempHeader = temp;                              //save for next time
    if (header == 32762) isDataFound = true;        //Do we have a match?
    if (header == 32760) isAogConfigFound = true;     //Do we have a match?
  }

  //Data Header has been found, so the next 6 bytes are the data -- 127H + 250L = 32762
  if (Serial.available() > 7 && isDataFound) //si on à plus de 7 octetes après l'en-tête, on met chaque octet dans une variable (octet 1 = relayHi, etc.).
  {
    isDataFound = false;
    relayHi = Serial.read();
    relayLo = Serial.read();          // read relay control from AgOpenGPS
    gpsSpeed = Serial.read() >> 2;  //actual speed times 4, single byte
    uTurn = Serial.read();  
    tree = Serial.read();
    hydLift = Serial.read();
    
    //just get the rest of bytes, les deux octets restants sont lus mais non assignés à une variable
    Serial.read();   //high,low bytes   
    Serial.read();  

    //reset watchdog
    watchdogTimer = 0;

    //Reset serial Watchdog  
    serialResetTimer = 0;
  }

   //Arduino Config Header has been found, so the next 8 bytes are the data
  if (Serial.available() > 7 && isAogConfigFound)
  {
    isAogConfigFound = false;

    aogConfig.raiseTime = Serial.read();
    aogConfig.lowerTime = Serial.read();    
    aogConfig.enableToolLift = Serial.read();
    
    //set1 
    byte sett = Serial.read();  //setting0     
    if (bitRead(sett,0)) aogConfig.isRelayActiveHigh = 1; else aogConfig.isRelayActiveHigh = 0;
    
    Serial.read();
    Serial.read();
    Serial.read();
    Serial.read();

    //save in EEPROM and restart
    EEPROM.put(6, aogConfig);
    resetFunc();
  }
}

//*************************FUNCTIONS*************************

//Here are the functions called previously in the void loop. Vous trouverez ici les fonctions appelées dans la boucle principale qui permettent le contrôle via es boutons.
//    * SetRealys : says which section must be open. Read bits from PGN 32762. A partir des octets du PGN 32762, dit quel tronçon dit être ouvert (via les relais)
//    * buttons : write the value of the button on the right bit of the right bytes. /!\ Pushbuttons only (count the state of changes), permanent switch will not work. 

  void SetRelays(void)
 { 
    if (aogConfig.isRelayActiveHigh)
    {
      //active low
      if (lowerTimer) bitClear(PORTD, LOWER); //Digital Pin D3
      else bitSet(PORTD, LOWER); 
      if (raiseTimer) bitClear(PORTD, RAISE); //Digital Pin D4
      else bitSet(PORTD, RAISE); 
      
      if (bitRead(relayLo,0)) bitClear(PORTD, 6); //Digital Pin 5
      else bitSet(PORTD, 6); 
      if (bitRead(relayLo,1)) bitClear(PORTD, 7); //Digital Pin 6
      else bitSet(PORTD, 7); 
      if (bitRead(relayLo,2)) bitClear(PORTB, 0); //Digital Pin 7
      else bitSet(PORTB, 0); 
      if (bitRead(relayLo,3)) bitClear(PORTB, 1); //Digital Pin 8
      else bitSet(PORTB, 1); 
      if (bitRead(relayLo,4)) bitClear(PORTB, 2); //Digital Pin 9
      else bitSet(PORTB, 2); 
      if (bitRead(relayLo,5)) bitClear(PORTB, 3); //Digital Pin 10
      else bitSet(PORTB, 3); 
      if (bitRead(relayLo,6)) bitClear(PORTB, 4); //Digital Pin 11
      else bitSet(PORTB, 4); 
      if (bitRead(relayLo,7)) bitClear(PORTB, 5); //Digital Pin 12
      else bitSet(PORTB, 5); 
    }
    else //active high
    {
      if (lowerTimer) bitSet(PORTD, LOWER); //Digital Pin D3
      else bitClear(PORTD, LOWER); 
      if (raiseTimer) bitSet(PORTD, RAISE); //Digital Pin D4
      else bitClear(PORTD, RAISE); 
      
      if (bitRead(relayLo,0)) bitSet(PORTD, 6); //Digital Pin 5
      else bitClear(PORTD, 6); 
      if (bitRead(relayLo,1)) bitSet(PORTD, 7); //Digital Pin 6
      else bitClear(PORTD, 7); 
      if (bitRead(relayLo,2)) bitSet(PORTB, 0); //Digital Pin 7
      else bitClear(PORTB, 0); 
      if (bitRead(relayLo,3)) bitSet(PORTB, 1); //Digital Pin 8
      else bitClear(PORTB, 1); 
      if (bitRead(relayLo,4)) bitSet(PORTB, 2); //Digital Pin 9
      else bitClear(PORTB, 2); 
      if (bitRead(relayLo,5)) bitSet(PORTB, 3); //Digital Pin 10
      else bitClear(PORTB, 3); 
      if (bitRead(relayLo,6)) bitSet(PORTB, 4); //Digital Pin 11
      else bitClear(PORTB, 4); 
      if (bitRead(relayLo,7)) bitSet(PORTB, 5); //Digital Pin 12
      else bitClear(PORTB, 5); 
    }
 }


  void Buttons(void)
 {
  // read the pushbutton input pin:
  button1State = digitalRead(button1Pin);
  button2State = digitalRead(button2Pin);
  button3State = digitalRead(button3Pin);
  button4State = digitalRead(button4Pin);
  button5State = digitalRead(button5Pin);
  button6State = digitalRead(button6Pin);
  button7State = digitalRead(button7Pin);
  button8State = digitalRead(button8Pin);
  buttonmodeState = digitalRead(buttonmodePin);
  buttonboomonswState = digitalRead(buttonboomonPin);
  buttonboomoffswState = digitalRead(buttonboomoffPin);

      
//_______________________BUTTON__________________//
//Read push buttons status and increment a counter
  
  if (button1State != lastButton1State) {
    if (button1State == HIGH) {
      button1PushCounter++;
      bitClear(mainon,0);
      bitClear(mainoff,0);
      }
    delay(50);
  }
  lastButton1State = button1State;
    
  if (button2State != lastButton2State) {
    if (button2State == HIGH) {
      button2PushCounter++;
      bitClear(mainon,1);
      bitClear(mainoff,1);
      }
    delay(50);
  }
  lastButton2State = button2State;
 
  if (button3State != lastButton3State) {
    if (button3State == HIGH) {
      button3PushCounter++;
      bitClear(mainon,2);
      bitClear(mainoff,2);
      }
    delay(50);
  }
  lastButton3State = button3State;

  if (button4State != lastButton4State) {
    if (button4State == HIGH) {
      button4PushCounter++;
      bitClear(mainon,3);
      bitClear(mainoff,3);
      }
    delay(50);
  }
  lastButton4State = button4State;

  if (button5State != lastButton5State) {
    if (button5State == HIGH) {
      button5PushCounter++;
      bitClear(mainon,4);
      bitClear(mainoff,4);
      }
    delay(50);
  }
  lastButton5State = button5State;

  if (button6State != lastButton6State) {
    if (button6State == HIGH) {
      button6PushCounter++;
      bitClear(mainon,5);
      bitClear(mainoff,5);
      }
    delay(50);
  }
  lastButton6State = button6State;

  if (button7State != lastButton7State) {
    if (button7State == HIGH) {
      button7PushCounter++;
      bitClear(mainon,6);
      bitClear(mainoff,6);
      }
    delay(50);
  }
  lastButton7State = button7State;

  if (button8State != lastButton8State) {
    if (button8State == HIGH) {
      button8PushCounter++;
      bitClear(mainon,7);
      bitClear(mainoff,7);
      }
    delay(50);
  }
  lastButton8State = button8State;  
  
  if (buttonmodeState != lastButtonmodeState) {
    if (buttonmodeState == HIGH) {
      buttonmodePushCounter++;
      }
    delay(50);
  }
  lastButtonmodeState = buttonmodeState;

  if (buttonboomonswState == HIGH) {
    buttonboomoffswPushCounter++;
    mainon=255;
    }
  delay(50);

  if (buttonboomoffswState == HIGH) {
    buttonboomonswPushCounter++;
    mainoff=255;
    }
  delay(50);


//_______________MODE______________________//

  if (buttonmodePushCounter % 2 == 0) {
    modeauto = true;
    } else {
    modeauto = false;
    }

  if (button1PushCounter % 2 == 0) {
    bitSet(sect, 0);
  } else {
    bitClear(sect, 0);
  }
 
  if (button2PushCounter % 2 == 0) {
    bitSet(sect, 1);
  } else {
    bitClear(sect, 1);
  }

  if (button3PushCounter % 2 == 0) {
    bitSet(sect, 2);
  } else {
    bitClear(sect, 2);
  }
  if (button4PushCounter % 2 == 0) {
    bitSet(sect, 3);
  } else {
    bitClear(sect, 3);
  }
  if (button5PushCounter % 2 == 0) {
    bitSet(sect, 4);
  } else {
    bitClear(sect, 4);
  }
  if (button6PushCounter % 2 == 0) {
    bitSet(sect, 5);
  } else {
    bitClear(sect, 5);
  }
  if (button7PushCounter % 2 == 0) {
    bitSet(sect, 6);
  } else {
    bitClear(sect, 6);
  }
  if (button8PushCounter % 2 == 0) {
    bitSet(sect, 7);
  } else {
    bitClear(sect, 7);
  }


//_____________WRITE__________________//  

  bitWrite(SectMainToAOG,0,modeauto);
  bitWrite(SectMainToAOG,1,!modeauto);


  if (bitRead(SectMainToAOG,0)){            //if the section control is in automatic mode (byte SectMainToAOG = 00000001), then....       si la coupure tronçon est en mode automatique, alors.... 
             
    if(bitRead(relayLoT, 0)){                 // first it "cleans" the bits if these ones are filled ... d'abord on "nettoie" les bits qui sont activés/remplis
      bitClear(relayLoT, 0);
      bitSet(SectSWOffToAOGLo, 0);      
    }if (bitRead(relayLoT, 1)){             // if the "relayLoT" byte, bit1(T mean transmitted) = 1 ... si le l'octet "relayLoT", bit1(T pour Transmis) =1
      bitClear(relayLoT, 1);                  //byte "relayLoT", bit1 =0
      bitSet(SectSWOffToAOGLo, 1);            //byte "SectSWOffToAOGLo", bit1 =1
    }if (bitRead(relayLoT, 2)){
      bitClear(relayLoT, 2);
      bitSet(SectSWOffToAOGLo, 2);
    }if (bitRead(relayLoT, 3)){
      bitClear(relayLoT, 3);
      bitSet(SectSWOffToAOGLo, 3);
    }if (bitRead(relayLoT, 4)){
      bitClear(relayLoT, 4);
      bitSet(SectSWOffToAOGLo, 4);
    }if (bitRead(relayLoT, 5)){
      bitClear(relayLoT, 5);
      bitSet(SectSWOffToAOGLo, 5);
    }if (bitRead(relayLoT, 6)){
      bitClear(relayLoT, 6);
      bitSet(SectSWOffToAOGLo, 6);  
    }if (bitRead(relayLoT, 7)){
      bitClear(relayLoT, 7);
      bitSet(SectSWOffToAOGLo, 7);
    } 
                                      //now we write in the bits we cleaned previously .... on réécrit aussitot sur les bit que l'on vient de "nettoyer"
    if(bitRead(sect, 0)){                       
      bitClear(mainon,0);               // explanation just below .... explications ci dessous
      if(bitRead(mainoff, 0)){                           
        bitSet(SectSWOffToAOGLo, 0);
        button1PushCounter++;                                         
      }else{                         
        bitClear(SectSWOffToAOGLo, 0);
      }               
    } else{
      bitClear(mainoff,0);
      if (bitRead(mainon, 0)){                         
        bitClear(SectSWOffToAOGLo, 0); 
        button1PushCounter++;                                        
       } else{                            
        bitSet(SectSWOffToAOGLo, 0);
          
       }                    
    }
    if(bitRead(sect, 1)){
      bitClear(mainon,1);
      if(bitRead(mainoff, 1)){                           
        bitSet(SectSWOffToAOGLo, 1);
        button2PushCounter++;                                         
      }else{                         
        bitClear(SectSWOffToAOGLo, 1);
      }               
    } else{
      bitClear(mainoff,1);
      if (bitRead(mainon, 1)){                         
        bitClear(SectSWOffToAOGLo, 1); 
        button2PushCounter++;                                        
       } else{                            
        bitSet(SectSWOffToAOGLo, 1);
          
       }                    
    }
    if(bitRead(sect, 2)){
      bitClear(mainon,2);
      if(bitRead(mainoff, 2)){                           
        bitSet(SectSWOffToAOGLo, 2);
        button3PushCounter++;                                         
      }else{                         
        bitClear(SectSWOffToAOGLo, 2);
      }               
    } else{
      bitClear(mainoff,2);
      if (bitRead(mainon, 2)){                         
        bitClear(SectSWOffToAOGLo, 2); 
        button3PushCounter++;                                        
       } else{                            
        bitSet(SectSWOffToAOGLo, 2);
          
       }                    
    }
    if(bitRead(sect, 3)){
      bitClear(mainon,3);
      if(bitRead(mainoff, 3)){                           
        bitSet(SectSWOffToAOGLo, 3);
        button4PushCounter++;                                         
      }else{                         
        bitClear(SectSWOffToAOGLo, 3);
      }               
    } else{
      bitClear(mainoff,3);
      if (bitRead(mainon, 3)){                         
        bitClear(SectSWOffToAOGLo, 3); 
        button4PushCounter++;                                        
       } else{                            
        bitSet(SectSWOffToAOGLo, 3);
          
       }                    
    }    
    if(bitRead(sect, 4)){
      bitClear(mainon,4);
      if(bitRead(mainoff, 4)){                           
        bitSet(SectSWOffToAOGLo, 4);
        button5PushCounter++;                                         
      }else{                         
        bitClear(SectSWOffToAOGLo, 4);
      }               
    } else{
      bitClear(mainoff,4);
      if (bitRead(mainon, 4)){                         
        bitClear(SectSWOffToAOGLo, 4); 
        button5PushCounter++;                                        
       } else{                            
        bitSet(SectSWOffToAOGLo, 4);
          
       }                    
    }
    if(bitRead(sect, 5)){
      bitClear(mainon,5);
      if(bitRead(mainoff, 5)){                           
        bitSet(SectSWOffToAOGLo, 5);
        button6PushCounter++;                                         
      }else{                         
        bitClear(SectSWOffToAOGLo, 5);
      }               
    } else{
      bitClear(mainoff,5);
      if (bitRead(mainon, 5)){                         
        bitClear(SectSWOffToAOGLo, 5); 
        button6PushCounter++;                                        
       } else{                            
        bitSet(SectSWOffToAOGLo, 5);
          
       }                    
    }
    if(bitRead(sect, 6)){
      bitClear(mainon,6);
      if(bitRead(mainoff, 6)){                           
        bitSet(SectSWOffToAOGLo, 6);
        button7PushCounter++;                                         
      }else{                         
        bitClear(SectSWOffToAOGLo, 6);
      }               
    } else{
      bitClear(mainoff,6);
      if (bitRead(mainon, 6)){                         
        bitClear(SectSWOffToAOGLo, 6); 
        button7PushCounter++;                                        
       } else{                            
        bitSet(SectSWOffToAOGLo, 6);
          
       }                    
    }
    if(bitRead(sect, 7)){
      bitClear(mainon,7);
      if(bitRead(mainoff, 7)){                           
        bitSet(SectSWOffToAOGLo, 7);
        button8PushCounter++;                                         
      }else{                         
        bitClear(SectSWOffToAOGLo, 7);
      }               
    } else{
      bitClear(mainoff,7);
      if (bitRead(mainon, 7)){                         
        bitClear(SectSWOffToAOGLo, 7); 
        button8PushCounter++;                                        
       } else{                            
        bitSet(SectSWOffToAOGLo, 7);
          
       }                    
    }
  }else if (bitRead (SectMainToAOG,1)){     //if the section control is in manual mode (Byte SectMainToAOG, bit1 =1, or SectMainToAOG = 00000010) .... si AOG est en mode manuel (octet SectMainToAOG = 00000010)
    if(bitRead(sect, 0)){                     //if bit0 from byte "sect" =1 (means that section 1 is activated)     .... si le bit0 de l'octet "sect"=1 (qui signifie que la section 1 est en marche)
      bitClear(mainon,0);                     // we "clean" the bit0 of the byte mainon to mae sure that there is any issue .... on "nettoie"le bit0 de l'octet mainon pour pas qu'il y ai d'interférences 
      if(bitRead(mainoff, 0)){                //if bit0 from mainoff =1 (means wich pushed the "off" button  ... si le bit0 de l'octet mainoff =1 (on a appuyé sur le boutton "off")
        bitClear(relayLoT, 0);                  //set bit0, byte relayLoT to 0 (switch off the relay for section 1)  ... on ecrit octet relayLoT, bit0 =0 (on éteint le relay de la section 1)            
        bitSet(SectSWOffToAOGLo, 0);            // set bit0, byte SectSWOffToAOGLo to 1 (switch off in the software) ... on ecrit octet SectSWOffToAOGLo, bit0 =1 (on "active" l'exctinction de la section dans le logiciel)
        button1PushCounter++;                   //add to the button push cunter 1 (section 1), like if we pushed button 1   .... on incrémente le compteur du boutton (comme si on avait appuyé dessus pour l'éteindre)                    
      }else{                                  //else (mainoff is not activated)... sinon (mainoff n'est pas activé) --> we do like if nothing appened
        bitSet(relayLoT, 0);                    //switchoff relay 1 ... on eteint le relay 1     
        bitClear(SectSWOffToAOGLo, 0);          //"activate" the sofwtware switchoff .... on "active" l'exctinction de la section dans le logiciel
      }               
    } else{
      bitClear(mainoff,0);
      if (bitRead(mainon, 0)){
        bitSet(relayLoT, 0);                         
        bitClear(SectSWOffToAOGLo, 0);
        button1PushCounter++;                                   
       } else{
        bitClear(relayLoT, 0);                             
        bitSet(SectSWOffToAOGLo, 0);
           
       }                    
    }
    if(bitRead(sect, 1)){
      bitClear(mainon,1);
      if(bitRead(mainoff, 1)){
        bitClear(relayLoT, 1);                             
        bitSet(SectSWOffToAOGLo, 1);
        button2PushCounter++;                                         
      }else{
        bitSet(relayLoT, 1);                         
        bitClear(SectSWOffToAOGLo, 1);
      }               
    } else{
      bitClear(mainoff,1);
      if (bitRead(mainon, 1)){
        bitSet(relayLoT, 1);                         
        bitClear(SectSWOffToAOGLo, 1); 
        button2PushCounter++;                                        
       } else{
        bitClear(relayLoT, 1);                             
        bitSet(SectSWOffToAOGLo, 1);
          
       }                    
    }
    if(bitRead(sect, 2)){
      bitClear(mainon,2);
      if(bitRead(mainoff, 2)){
        bitClear(relayLoT, 2);                             
        bitSet(SectSWOffToAOGLo, 2);
        button3PushCounter++;                                         
      }else{
        bitSet(relayLoT, 2);                         
        bitClear(SectSWOffToAOGLo, 2);
      }               
    } else{
      bitClear(mainoff,2);
      if (bitRead(mainon, 2)){
        bitSet(relayLoT, 2);                         
        bitClear(SectSWOffToAOGLo, 2); 
        button3PushCounter++;                                      
       } else{
        bitClear(relayLoT, 2);                             
        bitSet(SectSWOffToAOGLo, 2);
          
       }                    
    }
    if(bitRead(sect, 3)){
      bitClear(mainon,3);
      if(bitRead(mainoff, 3)){
        bitClear(relayLoT, 3);                             
        bitSet(SectSWOffToAOGLo, 3);
        button4PushCounter++;                                         
      }else{
        bitSet(relayLoT, 3);                         
        bitClear(SectSWOffToAOGLo, 3);
      }               
    } else{
      bitClear(mainoff,3);
      if (bitRead(mainon, 3)){
        bitSet(relayLoT, 3);                         
        bitClear(SectSWOffToAOGLo, 3); 
        button4PushCounter++;                                      
       } else{
        bitClear(relayLoT, 3);                             
        bitSet(SectSWOffToAOGLo, 3);
          
       }                    
    }
    if(bitRead(sect, 4)){
      bitClear(mainon,4);
      if(bitRead(mainoff, 4)){
        bitClear(relayLoT, 4);                             
        bitSet(SectSWOffToAOGLo, 4);
        button5PushCounter++;                                         
      }else{
        bitSet(relayLoT, 4);                         
        bitClear(SectSWOffToAOGLo, 4);
      }               
    } else{
      bitClear(mainoff, 4);
      if (bitRead(mainon, 4)){
        bitSet(relayLoT, 4);                         
        bitClear(SectSWOffToAOGLo, 4); 
        button5PushCounter++;                                      
       } else{
        bitClear(relayLoT, 4);                             
        bitSet(SectSWOffToAOGLo, 4);
          
       }                    
    }
    if(bitRead(sect, 5)){
      bitClear(mainon,5);
      if(bitRead(mainoff, 5)){
        bitClear(relayLoT, 5);                             
        bitSet(SectSWOffToAOGLo, 5);
        button6PushCounter++;                                         
      }else{
        bitSet(relayLoT, 5);                         
        bitClear(SectSWOffToAOGLo, 5);
      }               
    } else{
      bitClear(mainoff,5);
      if (bitRead(mainon, 5)){
        bitSet(relayLoT, 5);                         
        bitClear(SectSWOffToAOGLo, 5); 
        button6PushCounter++;                                      
       } else{
        bitClear(relayLoT, 5);                             
        bitSet(SectSWOffToAOGLo, 5);
          
       }                    
    }
  } 
  if(bitRead(sect, 6)){
      bitClear(mainon,6);
      if(bitRead(mainoff, 6)){
        bitClear(relayLoT, 6);                             
        bitSet(SectSWOffToAOGLo, 6);
        button7PushCounter++;                                         
      }else{
        bitSet(relayLoT, 6);                         
        bitClear(SectSWOffToAOGLo, 6);
      }               
    } else{
      bitClear(mainoff,6);
      if (bitRead(mainon, 6)){
        bitSet(relayLoT, 6);                         
        bitClear(SectSWOffToAOGLo, 6); 
        button7PushCounter++;                                      
       } else{
        bitClear(relayLoT, 6);                             
        bitSet(SectSWOffToAOGLo, 6);
          
       }                    
    }
    if(bitRead(sect, 7)){
      bitClear(mainon,7);
      if(bitRead(mainoff, 7)){
        bitClear(relayLoT, 7);                             
        bitSet(SectSWOffToAOGLo, 7);
        button8PushCounter++;                                         
      }else{
        bitSet(relayLoT, 7);                         
        bitClear(SectSWOffToAOGLo, 7);
      }               
    } else{
      bitClear(mainoff,7);
      if (bitRead(mainon, 7)){
        bitSet(relayLoT, 7);                         
        bitClear(SectSWOffToAOGLo, 7); 
        button8PushCounter++;                                      
       } else{
        bitClear(relayLoT, 7);                             
        bitSet(SectSWOffToAOGLo, 7);
          
       }                    
    }
}

 
