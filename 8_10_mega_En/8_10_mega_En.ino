#include <U8g2lib.h>//u8g2 library
#include <Servo.h>
#include "Stepper.h"
U8G2_SSD1306_128X64_NONAME_F_SW_I2C u8g2(U8G2_R0,SCL,SDA); //Configure the constructor      

int set_tilt=35;//Define the initial tilt angle of the servo
float set_speed=1.50;  
int set_cycles=2;
int blt_sd_up = 45;//Defines the servo tilt angle increased
int blt_sd_down = 43;//Defines the servo tilt angle decreased
int blt_sc_up = 41;//Defines the servo tilt angle increased
int blt_sc_down = 39;//Defines the servo tilt angle decreased
int blt_sp_up = 37;//Defines the servo tilt angle increased
int blt_sp_down = 33;//Defines the servo tilt angle decreased
int blt_start = 47;////Defines Define the master switch
int PinInterrupt = 3;//Define the interrupt pin
int START1;//Servo intermediate variables
int led1 = 31;
int led2 = 35;
int stop_flag = 0;
Servo myDuoJi_L;//Define the left servo

#define stepsPerRevolution 100  //How many steps it takes to set the stepper motor to rotate one revolution requires 100 steps
//Define the corresponding pins for stepper motors
#define IN1 6      
#define IN2 5
#define IN3 4
#define IN4 2

Stepper stepper(stepsPerRevolution, IN1, IN2, IN3, IN4);  //Create a stepper motor object and initialize it
String NodeClient_Buff = "";//An initialization string that is used to receive data from Node
unsigned int NodeClient_BuffIndex = 0;
void(* resetFunc) (void) = 0; //Manufacture the restart command

void setup() 
{
  STEER(136,15); // The steering gear rotation angle is set_tilt
  u8g2.begin(); //Start the U8G2 driver 
  u8g2.clearBuffer();//Clear the display cache
  myDuoJi_L.attach(9,500,2500);//Left servo initialization
  pinMode(blt_sd_up, INPUT_PULLUP);
  pinMode(blt_sd_down, INPUT_PULLUP);
  pinMode(blt_sc_up, INPUT_PULLUP);
  pinMode(blt_sc_down, INPUT_PULLUP);
  pinMode(blt_sp_up, INPUT_PULLUP);
  pinMode(blt_sp_down, INPUT_PULLUP);
  pinMode(blt_start, INPUT_PULLUP);  
  pinMode(PinInterrupt, INPUT_PULLUP);  
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);
  pinMode(led1, OUTPUT);
  pinMode(led2, OUTPUT);
  Serial.begin(9600);
  Serial1.begin(9600);
  attachInterrupt(digitalPinToInterrupt(PinInterrupt),resetFunc,FALLING);//Set the pin interrupt
}

void serialEvent1()
{
  if(Serial1.available())
  {
  String input = Serial1.readString();
  Serial.println(input);
  String getTemp = "";
  int tempIndex = 0; //Assign an initial value to the temp position
  if((input.indexOf("N") == 0))//receive turn off command
  {
  Serial.print("receive on");
  Serial1.print("N");
  servos_stepper_working();
  }
  //if((input.indexOf("F") ==0))
  //{
  //Serial.print("receive off");
  //Serial1.print("F");
  //stop_working();
  //}
  if(tempIndex = input.indexOf("?")==0){//C language string lookup, find '?' Location If with '?' indicates that it is the set tilt angle
  getTemp = input.substring(10);
  Serial.println(getTemp);  
  set_tilt=getTemp.toInt();
  //real_tilt = set_tilt+140;
  Serial.print("It is a number\n");
  Serial.print(set_tilt); 
  }
  if(tempIndex = input.indexOf("!")==0){//C language string lookup, find '!' Location If with '!' indicates that it is the set rotary speed
  getTemp = input.substring(11);
  Serial.println(getTemp);  
  set_speed=getTemp.toInt()*1.00/100;
  Serial.print("It is a number\n");
  Serial.print(set_speed);  
  }
  if(tempIndex = input.indexOf("*")==0){//C language string lookup, find '*' Location If with '*' indicates that it is the set rotary cycles
  getTemp = input.substring(12);
  Serial.println(getTemp); 
  set_cycles=getTemp.toInt();
  Serial.print("It is a number\n");
  Serial.print(set_cycles); 
  }
  input="";
  }  
}

void loop() 
{
  display_initial();
  if(digitalRead(blt_sd_up) == 0) {set_tilt++; Serial.println(set_tilt); }
  if(digitalRead(blt_sd_down) == 0) {set_tilt--;Serial.println(set_tilt);}
  if(digitalRead(blt_sc_up) == 0) {set_cycles++; Serial.println(set_cycles); }
  if(digitalRead(blt_sc_down) == 0) {set_cycles--;Serial.println(set_cycles);}
  if(digitalRead(blt_sp_up) == 0) {set_speed=set_speed+0.1; Serial.println(set_speed); }
  if(digitalRead(blt_sp_down) == 0) {set_speed=set_speed-0.1;Serial.println(set_speed);}
  if(digitalRead(blt_start) == 0) 
  {
    servos_stepper_working();
  }
}


void display_initial()
{
  u8g2.clearBuffer();//Clear the display cache
  u8g2.setFont(u8g2_font_ncenB08_tr);//Set the font
  u8g2.drawUTF8(5,15,"ROTARY PLATFORM");//Displays English, the lower left position coordinates are (78,30)  
  u8g2.setCursor(0,25);//Sets the lower-left coordinates of the variable that will be printed
  u8g2.print("angle:");//print 'angle' message
  u8g2.setCursor(50,25);                                
  u8g2.print(set_tilt);//Print "set_tilt"variables
  u8g2.setCursor(70,25);                                 
  u8g2.print("degree");                                      

  u8g2.setCursor(0,40);                                
  u8g2.print("speed:");                                 
  u8g2.setCursor(50,40);                                
  u8g2.print(set_speed);                                      
  u8g2.setCursor(80,40);                                
  u8g2.print("");                                      

  u8g2.setCursor(0,55);                             
  u8g2.print("cycles:");                               
  u8g2.setCursor(55,55);                               
  u8g2.print(set_cycles);                                 
  u8g2.setCursor(80,55);                                 
  u8g2.print("");                                   
  u8g2.sendBuffer();                                   
}

//Servo speed control 
void STEER(int STEER1, int SPEED1)
{
	int step=0,count_ci=0;
	int flag6=0;
	while(step==0)
  {
  count_ci++;
	if(count_ci%SPEED1==0&&START1>STEER1&&STEER1!=0) START1--;
	if(count_ci%SPEED1==0&&START1<STEER1&&STEER1!=0) START1++;
	
	if(START1==STEER1&&STEER1!=0) {START1=STEER1;flag6=1;}
	
	if(STEER1==0||SPEED1==0) flag6=1;
	
	myDuoJi_L.write(START1);
	if(flag6==1){ step=99;}
	delay(2);
  }
}

void servos_stepper_working()
{
  int i;
  u8g2.clearBuffer();                                    
  u8g2.drawUTF8(5,15,"Servos working");                
  u8g2.sendBuffer();                                    
  STEER(set_tilt+130,10); // Add initial values
  delay(2000); //
  u8g2.clearBuffer();                                    
  u8g2.drawUTF8(5,15,"rotarying working");                
  u8g2.sendBuffer();//Load the above
  stepper.setSpeed(set_speed*21);//Set the speed of the stepper motor to SEt_speed steps/minute
  for(i=0;i<set_cycles;i++)
  {
  stepper.step(2048);  //Spin for a week
  if(stop_flag == 1)
   {
     break;
     stop_flag = 0;
   }
  //delay(1000);
  }
  u8g2.clearBuffer();                                   
  u8g2.drawUTF8(5,15,"rotarying finishing");              
  u8g2.sendBuffer();  
  STEER(130,10); 
  delay(2000); 
  digitalWrite(led1, HIGH);
  digitalWrite(led2, HIGH);
  delay(2000);
  delay(2000);
  digitalWrite(led1, LOW);
  digitalWrite(led2, LOW);
  u8g2.clearBuffer();                                    
  u8g2.drawUTF8(5,15,"working down");                
  u8g2.sendBuffer();   
  delay(2000); //
}
//define stop functno
void stop_working()
{
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, LOW);
  //STEER(125,10); //
  delay(2000); //
  Serial.println("stop");
}