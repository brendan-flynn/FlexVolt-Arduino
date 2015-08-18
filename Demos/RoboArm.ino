/*  Author:  Brendan Flynn
    Date : last mod = August 18, 2015
    Summary : Arduino driver for FlexVolt Shield.  
    Controls a servo and led based on inputs from a flexvolt muscle sensor
    
    for details see:
    www.flexvoltbiosensor.com
    
    License:
    released under the Apache 2.0 Software License
    http://www.apache.org/licenses/LICENSE-2.0

    Inputs : 6 analog inputs, reading raw and filtered signals from FlexVolt Shield (need 2CH Shield + 4CH Shield to read 6 signals)
*/

#include <Servo.h> 
 
Servo myservo;  // create servo object to control a servo 
                // a maximum of eight servo objects can be created 

 
///////////  Port Mapping //////////////
int analogPin0 = A0;
int analogPin1 = A1;
int analogPin2 = A2;
int analogPin3 = A3;
int analogPin4 = A4;
int analogPin5 = A5;

int LEDpin = 11; // attach an LED to pin 11 for visualization of threholding signals

///////////  Variables  ///////////
long updatetime = 0;  // sampling timer variable
long delaytime = 1000;// delay between sampling data.  in microseconds.  20000us => 50Hz, 1000us => 1kHz
long servotime = 0; // servo command timer variable
int servodelay = 10; // delay between servo commands
long changepostime = 0; // pos recalculate timer variable
int posdelay = 20; // delay between calculating position

int pos = 0;    // variable to store the servo position 

int movethresh = 550;  // threshold for deciding how to respond.  Muscle signals > 550 do something different than signals <= 550.  Signal max is 1024

// servo limits
int servotop = 170;
int servobottom = 70;

// measured value variables
int val1 = 0, val2 = 0, val3 = 0, val4 = 0; // measurement variables
long smoothFilterRegister1 = 0, smoothFilterRegister2 = 0, smoothFilterRegister3 = 0, smoothFilterRegister4 = 0; // long storage of the averaging filter value
int smoothed1 = 0, smoothed2 = 0, smoothed3 = 0, smoothed4 = 0; // smoothed variables
int Filter_Shift = 8;  // play with this number to change the filtering/averaging level.  Higher numbers smooth more

void setup(void){
  myservo.write(170);
  myservo.attach(9);  // attaches the servo on pin 9 to the servo object 
  myservo.write(170);
  pinMode(LEDpin, OUTPUT);
}

void loop(){
    // this loop runs continuously
    
    // sampling is on a microsecond timer, to get into the ~100Hz sampling rate range.  Don't have to sample very fast here.
    if (micros() > updatetime){  // check if it's time to send next sample
      updatetime = micros()+delaytime;  // reset timer for next read time
      val3 = analogRead(analogPin2); // read signal - you can use any number of the inputs.  Here I use just 1 signal
      
      // we need to smooth the input, which is very noisy and bounces evenly above and below 
      // 512 (which is the body reference level), to have a more useable signal for thresholding
      // so signals ranging from 0-512 are negative and 512-1024 are positive
      // subtracting 512, then taking the absolute value, converts to a 0-1024 scale of positive signals
      // adding back 512 just recenters, not really necessary.
      // the filter is recursive, so we do not need to store an array of values and keep taking the mean value
      // each sample, the new measurement is added, and a 1/2^Filter_Shift fraction of the register is subtracted
      // So, if Filter_Shift is 8, then each sample, we add the sample val and subtract 1/256th of the register value
      // When we want a value from the smooth filter register, we also take 1/256th value (again using Filter_Shift)
      // The result is a simple smoothing filter without much memory or processing overhead!
      smoothFilterRegister3 = smoothFilterRegister3 - (smoothFilterRegister3 >> Filter_Shift) + long((abs(val3-512))+512);
      smoothed3 = int(smoothFilterRegister3 >> Filter_Shift); // Filter_Shift can be adjusted!
    }
    
    // servo control signals are on a separate timer, in the 10 millisecond range
    if (millis() > servotime){
      servotime = millis()+servodelay;
      myservo.write(pos); // pos is updated continuously by the code below based on the signal in from flexvolt
    }
    
    // update the pos setting for the servo, on a third timer, also in the millisecond range (~20ms)
    if (millis() > changepostime){
      changepostime = millis()+posdelay;
      // thresholding - if the signal in from the flexvolt is greater than movethresh, do something, otherwise, do something else
      if (smoothed3 > movethresh){
        pos--;
        digitalWrite(LEDpin, HIGH); // signaling on the led
        if (pos < servobottom){pos = servobottom;} // handling position limits so we don't try to rotate the servo too far
      }
      else if (smoothed3 <= movethresh){
        pos++;
        digitalWrite(LEDpin, LOW); // signaling on the led
        if (pos > servotop){pos = servotop;} // handling position limits so we don't try to rotate the servo too far
        
      }
    }
}
