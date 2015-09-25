/******************************************************************************
* File Name          : UF_uArm.cpp
* Author             : Evan
* Updated            : Evan
* Version            : V0.0.2 (BATE)
* Date               : 12 Dec, 2014
* Modified Date      : 17 Jan, 2015
* Description        :
* License            : 
* Copyright(C) 2014 UFactory Team. All right reserved.
*******************************************************************************/
#include "UF_uArm_Metal_new.h"

int angleR;
int angleL;
int angleBottom;
UF_uArm::UF_uArm()
{
	heightLst				= 0;
	height					= 0;
	stretch					= 0;
	rotation				= 0;
	handRot					= 0;
    lstTime					= 0;
	delay_loop			    = 0;
	servoSpdR				= 0;
	servoSpdL				= 0;
	servoSpdRot			    = 100;
	servoSpdHand		    = 0;
	servoSpdHandRot	        = 0;
	leftServoLast		    = 110;
    rightServoLast	        = 100;
	rotServoLast		    = 90;
	handrotLast             = 90;
	sampleDelay			    = 10;
	playFlag				= false;
	recordFlag			    = true;
	firstFlag				= true;
	gripperRst			    = true;
	griperState[14]	        = 0;
	data[5] = 0;  // 0: L  1: R  2: Rotation 3: hand rotation 4:gripper
}

void UF_uArm::init()
{
    // read offset data
    offsetL = EEPROM.read(1);
    offsetR = EEPROM.read(2);
    // initialization the pin
    pinMode(LIMIT_SW, INPUT_PULLUP);  digitalWrite(LIMIT_SW, HIGH);
    pinMode(BTN_D4,   INPUT_PULLUP);  digitalWrite(BTN_D4,   HIGH);
    pinMode(BTN_D7,   INPUT_PULLUP);  digitalWrite(BTN_D7,   HIGH);
    pinMode(BUZZER,   OUTPUT); digitalWrite(BUZZER,   LOW);
    pinMode(PUMP_EN,  OUTPUT); digitalWrite(PUMP_EN,  LOW);
    pinMode(VALVE_EN, OUTPUT); digitalWrite(VALVE_EN, LOW);
	if (EEPROM.read(0) == CALIBRATION_FLAG) // read of offset flag
    {
		// attaches the servo on pin to the servo object
		servoL.attach(SERVO_L, D090M_SERVO_MIN_PUL, D090M_SERVO_MAX_PUL);
		servoR.attach(SERVO_R, D090M_SERVO_MIN_PUL, D090M_SERVO_MAX_PUL);
		servoRot.attach(SERVO_ROT, D090M_SERVO_MIN_PUL, D090M_SERVO_MAX_PUL);
		servoHand.attach(SERVO_HAND, D009A_SERVO_MIN_PUL, D009A_SERVO_MAX_PUL);
		servoHandRot.attach(SERVO_HAND_ROT, D009A_SERVO_MIN_PUL, D009A_SERVO_MAX_PUL);
		servoHand.write(HAND_ANGLE_OPEN, 0, true);
		servoHand.detach();

		servoL.write(map(readAngle(SERVO_L), SERVO_MIN, SERVO_MAX, 0, 180));
		servoR.write(map(readAngle(SERVO_R), SERVO_MIN, SERVO_MAX, 0, 180));
		servoRot.write(map(readAngle(SERVO_ROT), SERVO_MIN, SERVO_MAX, 0, 180));
		// initialization postion
		setServoSpeed(SERVO_R,   15);  // 0=full speed, 1-255 slower to faster
		setServoSpeed(SERVO_L,   15);  // 0=full speed, 1-255 slower to faster
		setServoSpeed(SERVO_ROT, 13);  // 0=full speed, 1-255 slower to faster
		setPosition(stretch, height, rotation, handRot);
    }
    else
    {	// buzzer alert if calibration needed
		alert(3, 200, 200);
    }
    /************************do the test**************/
    /*while(1)
    {
    	Serial.println("\tstartPLd");Serial.print(map(readAngle(SERVO_L), SERVO_MIN, SERVO_MAX, 0, 180));
    	Serial.println("\tstartPRd");Serial.print(map(readAngle(SERVO_R), SERVO_MIN, SERVO_MAX, 0, 180));
    	Serial.println("\tstartPL");Serial.print(readAngle(SERVO_L));
    	Serial.println("\tstartPR");Serial.print(readAngle(SERVO_R));
    }*/
}

void UF_uArm::calibration()
{
	int initPosL = INIT_POS_L - 12; // Added 30 degrees here to start at reasonable point
	int initPosR = INIT_POS_R - 12; // Added 30 degrees here to start at reasonable point

	if(!digitalRead(BTN_D7))
	{
		delay(20);
		// buzzer alert
		alert(1, 20, 0);
	}

	lstTime = millis();
  while(!digitalRead(BTN_D7))
  {
    if(millis() - lstTime > BTN_TIMEOUT_3000)
    {
      // buzzer alert
      alert(2, 50, 100);
      while(!digitalRead(BTN_D7))
      {
 		/*servoR.write(INIT_POS_R, 15, true); // 0=full speed, 1-255 slower to faster
		servoL.write(INIT_POS_L, 15, true); // 0=full speed, 1-255 slower to faster 
		while(digitalRead(BTN_D7));

		alert(1, 20, 0);*/
        servoL.detach();
        servoR.detach();
        delay(500);
      }

			while(1)
			{
				// SG-> Commentary: While user adjusts for minimum angles, keep track of angle and add
				//                  margin of analog reading value of 12, which is about 3 degrees.
				int minAngle_L = readAngle(SERVO_L) - 16;
				int minAngle_R = readAngle(SERVO_R) - 16;
				//Serial.println("\toffsetL");Serial.print(map(readAngle(SERVO_L), SERVO_MIN, SERVO_MAX, 0, 180));
				//Serial.println("\toffsetR");Serial.print(map(readAngle(SERVO_R), SERVO_MIN, SERVO_MAX, 0, 180));
		        
				if(!digitalRead(BTN_D7))
				{
					
					// buzzer alert
					alert(1, 20, 0);
					delay(200); //SG-> Added to delay for user to remove hand
					// buzzer alert
					servoL.attach(SERVO_L, D090M_SERVO_MIN_PUL, D090M_SERVO_MAX_PUL);
					servoR.attach(SERVO_R, D090M_SERVO_MIN_PUL, D090M_SERVO_MAX_PUL);
					//servoL.write(map(readAngle(SERVO_L), SERVO_MIN, SERVO_MAX, 0, 180));
					//servoR.write(map(readAngle(SERVO_R), SERVO_MIN, SERVO_MAX, 0, 180));
					servoR.write(initPosR, 10, true); // 0=full speed, 1-255 slower to faster
					servoL.write(initPosL, 10, true); // 0=full speed, 1-255 slower to faster

					//while(1);
					delay(100);
					while(readAngle(SERVO_R) < minAngle_R - SAMPLING_DEADZONE)
					{
						servoR.write(++initPosR);//Serial.println("\tR+");
						delay(50);
					}
					while(readAngle(SERVO_R) > minAngle_R + SAMPLING_DEADZONE)
					{
						servoR.write(--initPosR);//Serial.println("\tR-");
						delay(50);
					}
					while(readAngle(SERVO_L) < minAngle_L - SAMPLING_DEADZONE)
					{
						servoL.write(++initPosL);
						delay(50);
					}
					while(readAngle(SERVO_L) > minAngle_L + SAMPLING_DEADZONE)
					{
						servoL.write(--initPosL);
						delay(50);
					}
					offsetL = initPosL - INIT_POS_L+3;
					offsetR = initPosR - INIT_POS_R+3;
					//Serial.println("\toffsetL");Serial.print(initPosL);Serial.print(INIT_POS_L);
					//Serial.println("\toffsetR");Serial.print(initPosR);Serial.print(INIT_POS_R);
		
					EEPROM.write(0, CALIBRATION_FLAG);  // Added flag to know if offset is done
					EEPROM.write(1, offsetL);						// offsetL
					EEPROM.write(2, offsetR);						// offsetR
					// buzzer alert
					alert(1, 500, 0);
					// reset postion
					init();
					break;
				}
			}
		}
	}
}


void UF_uArm::recordingMode(unsigned char _sampleDelay)
{
	sampleDelay = _sampleDelay;
  // D4 button - Recording mode
  if(!digitalRead(BTN_D4))
  {
    alert(2, 200, 100);
    while(!digitalRead(BTN_D4));
    delay(50);
		servoL.detach();
		servoR.detach();
		servoRot.detach();
		servoHandRot.detach();
    while(1)
		{
            // D4 button - recording
            if(!digitalRead(BTN_D4))
	        {
		       recordFlag = true;
               alert(1, 50, 0);
               lstTime = millis();
               while(!digitalRead(BTN_D4))
               {
                  if(millis() - lstTime > BTN_TIMEOUT_1000)//make the new data to the old data for long time storage
		          {
                     recordFlag = false;
		             unsigned char dat[2];
                     readExternalEeprom(0x0000, &dat[0],1);
                     delay(5);
                     readExternalEeprom(0x8000, &dat[1],1);
                     delay(5);
                     if((dat[0]==new_data)&&(dat[1]==old_data))
                     {
                        //addr = 1;
                        dat[0] = old_data;
                        writeExternalEeprom(0x0000, dat,1);
                        delay(5);
                        dat[0] = new_data;                        
                        writeExternalEeprom(0x8000, dat,1);
                        delay(5);
                        dat[0] = 255;                        
                        writeExternalEeprom(0x8001, dat,1);
                        delay(5);                      
                     }
                     else if((dat[0]==old_data)&&(dat[1]==new_data))
                     {
                        dat[0] = new_data;
                        writeExternalEeprom(0x0000, dat,1);
                        delay(5);
                        dat[0] = 255;
                        writeExternalEeprom(0x0001, dat,1); 
                        delay(5);                      
                        dat[0] = old_data;                        
                        writeExternalEeprom(0x8000, dat,1); 
                        delay(5);
                     }
                     else
                     {
  	                    //addr = 1;
  	                    dat[0] = new_data;
  	                    writeExternalEeprom(0x0000, dat,1);	
  	                    delay(5);
  	                    dat[0] = 255;
  	                    writeExternalEeprom(0x0001, dat,1);	
  	                    delay(5);
  	                    dat[0] = old_data;
  	                    writeExternalEeprom(0x8000, dat,1);	
  	                    delay(5);
  	                    dat[0] = 255;
  	                    writeExternalEeprom(0x8001, dat,1);	
  	                    delay(5);
                     }   
			         alert(1, 300, 0);
			         //Serial.println("done");
                     while(!digitalRead(BTN_D4));
		          }
		       }
               delay(20);
               if(recordFlag)
                 record(BTN_D4, BTN_D7);
	        }

      // D7 button - play
      if(!digitalRead(BTN_D7))
      {
        playFlag = false;
        alert(1, 100, 0); 
        //if(firstFlag)
		//		{
        //  readEEPROM();
        //  firstFlag = false;
		//		}
        lstTime = millis();
        while(!digitalRead(BTN_D7))
		{
          if(millis() - lstTime > BTN_TIMEOUT_1000)
		  { 
            playFlag = true;
            alert(1, 300, 0);
            while(!digitalRead(BTN_D7));
          }
		}
        delay(20);
        play(BTN_D7);
	  }
	}
  }
}

void UF_uArm::setPosition(double _stretch, double _height, int _armRot, int _handRot)
{
	_armRot = -_armRot;
	if(!digitalRead(LIMIT_SW))//limit switch protection
	{
		alert(1, 10, 0);
	    if(_height < heightLst)
	    {
	    	_height = heightLst;
		}
    }
	// input limit
	_stretch = constrain(_stretch, ARM_STRETCH_MIN,   ARM_STRETCH_MAX) + 68;		// +68, set zero -stretch 
	_height  = constrain(_height,  ARM_HEIGHT_MIN,    ARM_HEIGHT_MAX);
	_armRot  = constrain(_armRot,  ARM_ROTATION_MIN,  ARM_ROTATION_MAX) + 90;		// +90, change -90~90 to 0~180
	_handRot = constrain(_handRot, HAND_ROTATION_MIN, HAND_ROTATION_MAX) + 90;	// +90, change -90~90 to 0~180
	// angle calculation
/*	double stretch2height2 = _stretch * _stretch + _height * _height;              // 
	double angleA = (acos( (ARM_A2B2 - stretch2height2) / ARM_2AB )) * RAD_TO_DEG; // angle between the upper and the lower
	double angleB = (atan(_height/_stretch)) * RAD_TO_DEG;                         // 
	double angleC = (acos((ARM_A2 + stretch2height2 -ARM_B2)/(2 * ARM_A * sqrt(stretch2height2)))) * RAD_TO_DEG; // 
	angleR = 180 - angleA - angleB - angleC + FIXED_OFFSET_R + offsetR;        // 
	angleL = angleB + angleC + FIXED_OFFSET_L + offsetL;                       // */
/**************/
double xx =_stretch*_stretch+_height*_height;
double xxx=ARM_B2-ARM_A2+xx;
double angleB =acos((_stretch*xxx+_height*sqrt(4.0*ARM_B2*xx-xxx*xxx))/(xx*2.0*ARM_B))* RAD_TO_DEG;
xxx=ARM_A2-ARM_B2+xx;
double angleA =acos((_stretch*xxx-_height*sqrt(4.0*ARM_A2*xx-xxx*xxx))/(xx*2.0*ARM_A))* RAD_TO_DEG;
int angleR =angleB + offsetR - 4;//int angleR =angleB + 40 + offsetR;
int angleL =angleA + offsetL + 16;//int angleL =25 + angleA + offsetL;
//Serial.write(angleR>>8);
//Serial.write(angleR);
//Serial.write(angleL>>8);
//Serial.write(angleL);
/**************/
	// angle limit
	angleL = constrain(angleL, 5 + offsetL, 145 + offsetL);
//	angleR = constrain(angleR, 0 + offsetR, 180 + offsetR);
//	angleR = constrain(angleR, angleL - 110 + offsetR, angleR);	// behind  -120+30 = -90
//	if(angleL < 25+offsetL)
//	angleR = constrain(angleR, 80 + offsetR, angleR);			// front down
	// set servo position
	servoR.write(angleR, servoSpdR, false);///angleR=angleR;//servoR.write(angleR, servoSpdR, false);
	servoL.write(angleL, servoSpdL, false);///angleL=angleL;//servoL.write(angleL, servoSpdL, false);
	servoRot.write(_armRot, servoSpdRot, false);///angleBottom=_armRot;//servoRot.write(_armRot, servoSpdRot, false);
	servoHandRot.write(_handRot, servoSpdHandRot, false);
	heightLst = _height;
}

void UF_uArm::setServoSpeed(char _servoNum, unsigned char _servoSpeed) // 0=full speed, 1-255 slower to faster
{
	switch(_servoNum)
	{
		case SERVO_L:
			servoSpdR = _servoSpeed;
			break;
		case SERVO_R:
			servoSpdL = _servoSpeed;
			break;
		case SERVO_ROT:
			servoSpdRot = _servoSpeed;
			break;
		case SERVO_HAND_ROT:
			servoSpdHand = _servoSpeed;
			break;
		case SERVO_HAND:
			servoSpdHandRot = _servoSpeed;
			break;
		default: break;
	}
}

int UF_uArm::readAngle(char _servoNum)
{
	int portAd;
	switch(_servoNum)
	{
		case SERVO_L:
			portAd = A0;
			break;
		case SERVO_R:
			portAd = A1;
			break;
		case SERVO_ROT:
			portAd = A2;
			break;
		case SERVO_HAND_ROT:
			portAd = A3;
			break;
		case SERVO_HAND:
			portAd = A6;
			break;
		default: return 0; break;
	}
	int adAdd = 0;
	for(char i=0; i<5; i++)
	{
		adAdd += analogRead(portAd);
	}
	return adAdd/5;
}

void UF_uArm::gripperCatch()
{
	servoHand.attach(SERVO_HAND, D009A_SERVO_MIN_PUL, D009A_SERVO_MAX_PUL);
  servoHand.write(HAND_ANGLE_CLOSE, 0, false);
  digitalWrite(VALVE_EN, LOW); // valve disnable
  digitalWrite(PUMP_EN, HIGH); // pump enable
  gripperRst = true;
}

void UF_uArm::gripperRelease()
{
	if(gripperRst)
	{
    servoHand.attach(SERVO_HAND, D009A_SERVO_MIN_PUL, D009A_SERVO_MAX_PUL);
    servoHand.write(HAND_ANGLE_OPEN, 0, false);
    digitalWrite(VALVE_EN, HIGH); // valve enable, decompression
    digitalWrite(PUMP_EN, LOW);   // pump disnable
    gripperRst = false;
    delay_loop = 0;
  }
}

void UF_uArm::gripperDetach()
{
	if(++delay_loop > 300000)        // delay release valve
	{
    servoHand.detach();
    digitalWrite(VALVE_EN, LOW); // valve disnable
    delay_loop=0;
  }
}

void UF_uArm::gripperDirectDetach()
{
	servoHand.detach();
	digitalWrite(PUMP_EN, LOW);   // pump disnable
	digitalWrite(VALVE_EN, LOW); // valve disnable
}

void UF_uArm::pumpOn()
{
	digitalWrite(PUMP_EN, HIGH);    // pump enable
}

void UF_uArm::pumpOff()
{
	digitalWrite(PUMP_EN, LOW);     // pump disnable
}

void UF_uArm::valveOn()
{
	digitalWrite(VALVE_EN, HIGH);   // valve enable, decompression
}

void UF_uArm::valveOff()
{
	digitalWrite(VALVE_EN, LOW);    // valve disnable
}

void UF_uArm::detachServo(char _servoNum)
{
	switch(_servoNum)
	{
		case SERVO_L:
		servoL.detach();
		break;
		case SERVO_R:
		servoR.detach();
		break;
		case SERVO_ROT:
		servoRot.detach();
		break;
		case SERVO_HAND_ROT:
		servoHandRot.detach();
		break;
		case SERVO_HAND:
		servoHand.detach();
		break;
		default: break;
	}
}

void UF_uArm::sendData(byte _dataAdd, int _dataIn)
{
	Serial.write(0xFF); Serial.write(0xAA); // send data head
	Serial.write(_dataAdd);
	Serial.write(*((char *)(&_dataIn) + 1));
	Serial.write(*((char *)(&_dataIn)));
}

void UF_uArm::alert(int _times, int _runTime, int _stopTime)
{
	for(int _ct=0; _ct < _times; _ct++)
	{
		delay(_stopTime);
		digitalWrite(BUZZER, HIGH);
		delay(_runTime);
		digitalWrite(BUZZER, LOW);
	}
}

void UF_uArm::play(unsigned char buttonPin)
{
  unsigned char dat[2];
  readExternalEeprom(0x8000, &dat[0],1); 
  delay(5);
  //readExternalEeprom(0x0000, &dat[0],1);
  if(record_status==have_done_record_since_power)
  {
    if(dat[0]==new_data)
    {
      addr = 0x8001;//Serial.println("playnew1");
    }
    else 
    {
      addr = 1;//Serial.println("playnew0");
    }
  }
  else
  {
    if(dat[0]==old_data)
    {
      addr = 0x8001;//Serial.println("playold1");
    }
    else 
    {
      addr = 1;//Serial.println("playold0");
    }  	
  }
  
  //unsigned char addrC = 0;
  //Serial.println("Playing");
  // attaches the servo on pin to the servo object
  servoL.attach(SERVO_L, D090M_SERVO_MIN_PUL, D090M_SERVO_MAX_PUL);
  servoR.attach(SERVO_R, D090M_SERVO_MIN_PUL, D090M_SERVO_MAX_PUL);
  servoRot.attach(SERVO_ROT, D090M_SERVO_MIN_PUL, D090M_SERVO_MAX_PUL);
  servoHandRot.attach(SERVO_HAND_ROT, D090M_SERVO_MIN_PUL, D090M_SERVO_MAX_PUL);
  while(digitalRead(buttonPin))
  {
    //Serial.println("Doing1");
  	readExternalEeprom(addr, data, 5);
  	//Serial.print("\tdata[0]=");  Serial.print(data[0]);Serial.print('\t');
  	//readExternalEeprom(0x00,0x7d, data,5);
    if(data[0] == DATA_FLAG)
	{
		if(playFlag)
		{
			//Serial.print("do");
            readExternalEeprom(0x8000, &dat[0],1); 
            if(record_status==have_done_record_since_power)
            {
              if(dat[0]==new_data)
              {
                addr = 0x8001;
              }
              else 
              {
                addr = 1;
              }
            }
            else
            {
              if(dat[0]==old_data)
              {
                addr = 0x8001;
              }
              else 
              {
                addr = 1;
              }  	
            }
            readExternalEeprom(addr, data, 5);
            delay(5);	
		}
		else break;

	}
 	//Serial.println("Doing2");
    unsigned char leftServo  = data[0];
    unsigned char rightServo = data[1];
    unsigned char rotServo   = data[2];
    unsigned char handrot    = data[3];

		if(data[4]==0x01)
			gripperCatch();
		else
			gripperRelease();

		//Serial.print("addr=");Serial.print(addr);
		/*Serial.print("\tleft: ");  Serial.print(leftServo);
		Serial.print("\tright: "); Serial.print(rightServo);
		Serial.print("\trot: ");   Serial.println(rotServo);
		Serial.print("\tHANDrot: ");   Serial.println(handrot);
        Serial.print("\tGRIPPER: ");   Serial.println(data[4]);*/
    servoBufOutL(leftServoLast,  leftServo);
    servoBufOutR(rightServoLast, rightServo);
    servoBufOutRot(rotServoLast, rotServo);
    servoBufOutHandRot(handrotLast, handrot);
    //Serial.print("***");

    leftServoLast  = leftServo;
    rightServoLast = rightServo;
    rotServoLast   = rotServo;
    handrotLast    = handrot;
    delay(sampleDelay);
    addr+=5;
      //Serial.println("Doing3");
  }
  //Serial.println("Done");
  servoL.detach();
  servoR.detach();
  servoRot.detach();
  servoHandRot.detach();
  gripperDirectDetach();
  alert(1, 250, 100);
  delay(250);
}

void UF_uArm::record(unsigned char buttonPin, unsigned char buttonPinC)
{
  unsigned char dat[2];
  readExternalEeprom(0x0000, &dat[0],1);//Serial.println(dat[0]);
  delay(5);
  readExternalEeprom(0x8000, &dat[1],1);//Serial.println(dat[1]);
  delay(5);
  if((dat[0]==new_data)&&(dat[1]==old_data))
  {
    addr = 1;
    dat[0] = new_data;
    writeExternalEeprom(0x0000, dat,1); //Serial.println("recordnew0");
    delay(5);
  }
  else if((dat[0]==old_data)&&(dat[1]==new_data))
  {
  	addr = 0x8001;
  	dat[0] = new_data;
  	writeExternalEeprom(0x8000, dat,1); //Serial.println("recordnew1");
  	delay(5);
  }
  else
  {
  	addr = 1;
  	dat[0] = new_data;
  	writeExternalEeprom(0x0000, dat,1);	//Serial.println("recordnew0none");
  	dat[0] = old_data;
  	delay(5);
  	writeExternalEeprom(0x8000, dat,1);	
  	delay(5);
  }

  record_status = have_done_record_since_power;
  //unsigned char addrC = 0; 
  boolean btnSt = false;
  //memset(griperState,0,14); // reset griperState array
  gripperDirectDetach();
  //Serial.println("Recording");
	while(digitalRead(buttonPin))
    {
		unsigned int leftServo  = map(constrain(readAngle(SERVO_L), SERVO_MIN, SERVO_MAX), SERVO_MIN, SERVO_MAX, 0, 180);
		unsigned int rightServo = map(constrain(readAngle(SERVO_R), SERVO_MIN, SERVO_MAX), SERVO_MIN, SERVO_MAX, 0, 180);
		unsigned int rotServo   = map(constrain(readAngle(SERVO_ROT), SERVO_MIN, SERVO_MAX), SERVO_MIN, SERVO_MAX, 0, 180);
        unsigned int handrot    = map(constrain(readAngle(SERVO_HAND_ROT), SERVO_MIN , SERVO_MAX), SERVO_MIN, SERVO_MAX, 0, 180);
		data[0] = leftServo;
		data[1] = rightServo;
		data[2] = rotServo;
		data[3] = handrot;
		//Serial.print(addr);Serial.print("\tleft: ");  Serial.print(data[0]);
		//Serial.print("\tright: "); Serial.print(data[1]);
		//Serial.print("\trot: ");   Serial.println(data[2]);
     
		if(!digitalRead(buttonPinC))
		{
			while(!digitalRead(buttonPinC));
			delay(10);
			if(btnSt)
			{
				gripperRelease();
				alert(2, 50, 50);
			}
			else
			{
				gripperCatch();
				alert(1, 50, 0);
			}
			btnSt = !btnSt;
		}
		//if(addr%3==0) // 3 steps, save griper state to array
		/*{
			if(btnSt)
				griperState[addrC / 8] |= (1 << (addrC % 8)); // 14byte = 112bit, every bit is a kind of state 
			else
				griperState[addrC / 8] |= (0 << (addrC % 8)); 
			addrC++;
		}*/
		data[4] =(btnSt==false)?0:1;

		if((addr%32768) >= MEMORY_SERVO_PER)
		{
			//data[5] = DATA_FLAG;
			
			//data[0]=255;
			//writeExternalEeprom(byte(addr>>8),byte(addr%256), data,1);
			//addr+=5;//avoid +=5 make addr =or> MEMORY_SERVO_PER
			alert(3, 50, 100);
			break;
		}

		writeExternalEeprom(addr, data, 5);
		addr+=5;
		delay(sampleDelay-2);
		//readExternalEeprom(0x00,0xff, data,5);
		//Serial.print("\tRleft: ");  Serial.println(data[0]);
		//Serial.print("\tRright: "); Serial.print(data[1]);
		//Serial.print("\tRrot: ");  Serial.print(data[2]);
    }
    //if(addr <= MEMORY_SERVO_PER)
    //{
    //data[5] = DATA_FLAG;
    data[0]=255;
    //data[1]=
	writeExternalEeprom(addr, data,1);
    //}
    gripperDirectDetach();
    while(digitalRead(buttonPin));
//  Serial.println("Done");
    alert(2, 50, 100);
  //writeEEPROM();
    firstFlag = false;
} 

/*void UF_uArm::writeEEPROM()
{
	int eepAddr = 0;
	while(data[2][eepAddr] != DATA_FLAG)
	{
		EEPROM.write(3 + eepAddr + MEMORY_SERVO_PER + MEMORY_SERVO_PER, data[2][eepAddr]);
		EEPROM.write(3 + eepAddr + MEMORY_SERVO_PER, data[1][eepAddr]);
		EEPROM.write(3 + eepAddr, data[0][eepAddr]);
		eepAddr++;
	}
	EEPROM.write(3 + eepAddr + MEMORY_SERVO_PER + MEMORY_SERVO_PER, DATA_FLAG);
	for(char i=0; i<14; i++)
	{
		EEPROM.write(3 + MEMORY_SERVO_PER*3 + 1 + i, griperState[i]);
	}
}*/

void UF_uArm::writeExternalEeprom(unsigned int address, unsigned char * data_array, int num)
{
  unsigned int i=0,j=0;
  //Serial.println("*****" );Serial.println(address_h);Serial.println(address_l);Serial.println("*****" );
  j=(address%128);
  if((j>=124)&&(num==5))
  {
  	j=128-j;
  	Wire.beginTransmission(0xa0>>1); // transmit to device #c0
  	Wire.write(byte(address>>8));
  	Wire.write(byte(address%256));//byte(first_byte_address&&0x00ff));//first_byte_address&&0xFF);
  	for(i=0;i<j;i++)
  	{
  	  Wire.write(*(data_array+i));              // sends one byte
  	}
  	Wire.endTransmission();    // stop transmitting
  	address+=j;
  	delay(5);
  	Wire.beginTransmission(0xa0>>1); // transmit to device #c0
  	Wire.write(byte(address>>8));
  	Wire.write(address%256);//byte(first_byte_address&&0x00ff));//first_byte_address&&0xFF);
  	for(i=0;i<num-j;i++)
  	{
  	  Wire.write(*(data_array+j+i));              // sends one byte
  	}
  	Wire.endTransmission();    // stop transmitting
  	//Serial.println("Done2");
  }
  else
  {
  	Wire.beginTransmission(0xa0>>1); // transmit to device #c0
  	Wire.write(byte(address>>8));
  	Wire.write(byte(address%256));//byte(first_byte_address&&0x00ff));//first_byte_address&&0xFF);
  	for(i=0;i<num;i++)
  	{
  	  Wire.write(*(data_array+i));              // sends one byte
  	  //if(num==1)Serial.println(byte(address>>8));
  	}
    
  	Wire.endTransmission();    // stop transmitting
  }
}

/*void UF_uArm::readEEPROM()
{
	int eepAddr = 0;
	while(EEPROM.read(3 + eepAddr + MEMORY_SERVO_PER + MEMORY_SERVO_PER) != DATA_FLAG)
	{
		data[2][eepAddr] = EEPROM.read(3 + eepAddr + MEMORY_SERVO_PER + MEMORY_SERVO_PER);
		data[1][eepAddr] = EEPROM.read(3 + eepAddr + MEMORY_SERVO_PER);
		data[0][eepAddr] = EEPROM.read(3 + eepAddr);
		eepAddr++;
	}
	data[2][eepAddr] = DATA_FLAG;
	for(char i=0; i<14; i++)
	{
		griperState[i] = EEPROM.read(3 + MEMORY_SERVO_PER*3 + 1 + i);
	}
}*/

void UF_uArm::readExternalEeprom(unsigned int address, unsigned char * data_array, int num)
{
  int i=0;
  
  	Wire.beginTransmission(0xa0>>1); // transmit to device #c0
  	Wire.write(byte(address>>8));
  	Wire.write(byte(address%256));//byte(first_byte_address&&0x00ff));//first_byte_address&&0xFF);
  	Wire.endTransmission(); 
  	Wire.requestFrom(0xa0>>1,num);    // request 6 bytes from slave device #2
  	for(i=0;i<num;i++)//while (Wire.available())   // slave may send less than requested
  	{
    	*(data_array+i) = Wire.read(); // receive a byte as character
    //i++;
  	}
  
}

void UF_uArm::servoBufOutL(unsigned char _lastDt, unsigned char _dt)
{
	if(abs(_dt - _lastDt) > BUFFER_OUTPUT)     // goes from Min degrees to Max degrees 
	  servoL.write(_dt, 40, true);
	else 
	servoL.write(_dt); 
}

void UF_uArm::servoBufOutR(unsigned char _lastDt, unsigned char _dt)
{
	if(abs(_dt - _lastDt) > BUFFER_OUTPUT)
		servoR.write(_dt, 40, true);
	else 
	servoR.write(_dt);
}

void UF_uArm::servoBufOutRot(unsigned char _lastDt, unsigned char _dt)
{
	if(abs(_dt - _lastDt) > BUFFER_OUTPUT)
		servoRot.write(_dt, 40, true);
	else 
	servoRot.write(_dt);
}
void UF_uArm::servoBufOutHandRot(unsigned char _lastDt, unsigned char _dt)
{
	if(abs(_dt - _lastDt) > BUFFER_OUTPUT)
		servoHandRot.write(_dt, 15, true);
	else 
	servoHandRot.write(_dt);
}
//******************************************************************
void UF_uArm::run(int x, int y, int z)
{
servoR.write(x, servoSpdR, false);
servoL.write(y, servoSpdL, false);
servoRot.write(z, servoSpdRot, false);
}