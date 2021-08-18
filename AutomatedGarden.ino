// When a command is entered in to the serial monitor on the computer 
// the Arduino will relay it to the ESP8266
//#include <elapsedMillis.h>//just for arduino ide 2.0, remove when using teensy

//NETWORK DETAILS
const String NetworkName = "ATT449";
const String NetworkPassword = "5916889444";
const String ServerIP  = "192.168.1.68";
const String ServerPort  = "1234";
//NETWORK DETAILS


//const String DefaultPacket = "DefaultPacket:0;DefaultPacketData";//is now just a number: 3;2:1
//const String SamplePacket = "DataTypeTest;DataHere=";
//Packet capacity: 01;123;123

//TIME DELAYS
const long measuringTimeDelay =  10000;//1800000;//30 minutes
const long serialTimeOut =  50000;//50 seconds
//TIME DELAYS

//these are combined into one ID aftering being sent
const int DEVICE_ID_1 = 00;//suggested use: device number within a group
const int DEVICE_ID_2 = 00;//suggested use: group number

enum PacketType {//I decided on each packet containing a packet type and 2 values
  DeviceID,//The name of this device
  MoistAndWater,//Moitsure and if it has been watered
  TempAndHumid 
};


//PIN VALUES
const int ledPin = 13;

const int ExtraSensorCount = 0;
//const int TemperaturePin = X;
//const int HumidityPin = Y;

const int PlantInputCount = 2;//each plant gets one valve
const int ValveOutputPins[PlantInputCount] = {0,1};//BIG TODO change pin nums
const int PlantInputPins[PlantInputCount] = {17,18};//BIG TODO change pin nums
//PIN VALUES


//SENSOR VALUES
int PlantInputValues[PlantInputCount];
int TemperatureValue = 0;
int HumidityValue = 0;
//SENSOR VALUES

//BUFFERS
int DataBuffer[PlantInputCount + ExtraSensorCount][3];//stores the data for all packets (type;A:B)
char PacketBuffer[10];//stores the built packet for sending
//BUFFERS

elapsedMillis timeElapsed = 0;

//STATUS VALUES
bool ledON = false;

bool SerialConnected = true;//was serial detected at startup
bool WifiSerialConnected = true;//was serial2 detected at startup

bool WifiConnection = false;
//STATUS VALUES

 
void setup() 
{
    Serial.begin(9600); // communication with the host computer
    timeElapsed = 0;
    while(!Serial){
      if(timeElapsed > serialTimeOut) {
        SerialConnected = false;
        Serial.end(); } 
    }//checks serial connection
    
    Serial2.begin(9600);  // Start the software serial for communication with the ESP8266 //, SERIAL_8E2);
    timeElapsed = 0;
    while(!Serial2){
      if(timeElapsed > serialTimeOut) {
        WifiSerialConnected = false;
        Serial2.end(); } 
    }//checks serial2 connection


    pinMode(ledPin, OUTPUT);
    //pinmode(TemperaturePin, OUTPUT);
    //pinmode(HumidityPin, OUTPUT);
    for (int k = 0; k < PlantInputCount; ++k){//i++ and ++i are the same here (same as i=i+1)
      pinMode(ValveOutputPins[k], OUTPUT);
      pinMode(PlantInputPins[k], INPUT);
    }//Pin Modes

    if(SerialConnected){
      Serial.println("");
      Serial.println("Remember to to set Both NL & CR in the serial monitor.");
      Serial.println("\n"); 
    }//start sequence
      
    digitalWrite(ledPin, HIGH);
    delay(50);
    
    InitWifiModule();

    delay(50);
    WifiStatus();
    
    if(SerialConnected){
      Serial.print("Ready. ");
      Serial.println(WifiConnection ? "Wifi Conncected" : "Wifi not conncected"); 
    }
      
    digitalWrite(ledPin, LOW);
    timeElapsed = 0;
}

void WifiStatus(){//checks if wifi status is 2, only checked at startup because sending data can change this
  if(WifiSerialConnected){//todo use a different check (like AT+CIFSR)
    String cmd = SendCommandLine("AT+CIPSTATUS", 1500);
    WifiConnection = (cmd.indexOf("2") > 0);
    if(SerialConnected && WifiConnection){
      Serial.println("Wifi Connection ok");
    }
  }
  else
    WifiConnection = false;
}

bool EspOK(){//checks if ESP returns the OK response (ready for commands)
  if(WifiSerialConnected){
    String cmd = SendCommandLine("AT", 500);
    bool isOK = (cmd.indexOf("OK") > 0);
    if(SerialConnected && isOK){
      Serial.println("Esp Connection Ok");
    }
    return isOK;
  }
  else
    return false;
}
 
void loop() 
{
    //Serial.println(analogRead(A9));
    //delay(10);
  
    // listen for communication from the ESP8266 and then write it to the serial monitor
    if ( Serial2.available() )    {  
      Serial.write( Serial2.read() ); 
      ledON = !ledON;
      digitalWrite(ledPin, ledON);
    }

    if(Serial.available() ){
      Serial2.write(Serial.read());
      
      //String avar = Serial.readString();
      //String test = avar;
      //if(test.trim() == 'a'){ 
      //}
      //else{
      //  Serial2.print(avar); 
    }


    
    if (timeElapsed > measuringTimeDelay) 
    {  
      //read stuff here
      //do stuff here
      //write to packet buffer
      
      //may need to check connection status

      if(WifiConnection && EspOK()){
        //String arr[] = {DefaultPacket, SamplePacket + String(analogRead(A9)), "Special THIRD packet"};//TODO change so these are made as they are sent?
        SendPackets();//sizeof is a compile time function which gets the size in bytes (and strings argie reference types(?) so their size is the same)
      }
      
      timeElapsed = 0;//set this after because this still increases while the above code is running
    }
  }

void ReadSensors(){
//TODO TODO TODO TODO TODO
}



void SendPackets(){
  if(SerialConnected) Serial.println("starting tcp connection");

  SendCommandLine("AT+CIPSTART=\"TCP\",\"" + ServerIP + "\"," + ServerPort, 4000);//start TCP connection
  delay(1000);

  //either in the send loop, or before here listen for incoming packets

  int packetCount = sizeof(DataBuffer) / sizeof(DataBuffer[0]);
  bool successfulConnect = false;
  for (int k = 0; k < packetCount; ++k)//i++ and ++i are the same here (same as i=i+1)
  {      
    Serial2.println("AT+CIPSEND=10");//"00;001:001" //each cipsend can send 1 packet before it automatically returns to command mode
    delay(200);
    
    if(Serial2.find(">") || successfulConnect)//only the first cipsend returns a '>'
    {
      successfulConnect = true;
      //Serial.println("Response '>' found, sending data:");//debug
  
      int index = 0;
      index += sprintf(PacketBuffer+index, "%02d", DataBuffer[k][0]);
      index += sprintf(PacketBuffer+index, ";");
      index += sprintf(PacketBuffer+index, "%03d", DataBuffer[k][1]);
      index += sprintf(PacketBuffer+index, ":");
      index += sprintf(PacketBuffer+index, "%03d", k);//DataBuffer[k][2]);
  
      if(SerialConnected) Serial.println(PacketBuffer);//debug
  
      Serial2.println(PacketBuffer);
      delay(100);//datasheet says a 20ms minimum between packets
    }
    else
    {
      if(SerialConnected) Serial.println("Response '>' not found, closing connection");
      Serial2.println("AT+CIPCLOSE");
      break;
    }
  }

    //Serial.println("Ending Packets");//debug
    //Serial2.print("+++");//ends packet sending and returns to normal command mode //not needed since it returns to command mode after every send
    //delay(2000);//datasheet says wait at least one second after returning to normal command mode //see above

    delay(150);
    Serial2.println("AT+CIPCLOSE");    
}

void InitWifiModule()      
{      
  SendCommandLine("AT+RST", 5000);  //reset the ESP8266 module.      
  
  delay(2000);      
  //SendCommand("AT+CWJAP=\"" + NetworkName + "\",\"" + NetworkPassword + "\"\r\n", 2000, DEBUG);        //connect to the WiFi network. //disabled since it auto-connects to this network  
  
  //delay (2000);      
  SendCommandLine("AT+CWMODE=1", 1500); //set the ESP8266 WiFi mode to station mode. (1: Station. 2: Access Point. 3: Station & AP.
  
  delay (500);      
  SendCommandLine("AT+CIFSR", 1500);  //Show IP Address, and the MAC Address.   
  
  delay (500);      //TODO investigate
  SendCommandLine("AT+CIPMUX=0", 1500);// 0 is single, 1 is multi       //Multiple conections.      
  
  //delay (500);      //TODO see if server is auto-started or not
  //SendCommandLine("AT+CIPSERVER=1,80", 1500, DEBUG);          //start the communication at port 80, port 80 used to communicate with the web servers through the http requests.      
}

String SendCommandLine(String command, const int timeout)
{
  return SendCommand(command + "\r\n", timeout);
}

String SendCommand(String command, const int timeout)      
{      
    String response = "";                                             //initialize a String variable named "response". we will use it later.      
          
    Serial2.print(command);                                           //send the AT command to the esp8266 (from ARDUINO to ESP8266).      
    long int time = millis();                                         //get the operating time at this specific moment and save it inside the "time" variable.      
    while( ((unsigned long)(time+timeout)) > millis())                                 //excute only whitin 1 second.      
    {            
      while(Serial2.available())                                      //is there any response came from the ESP8266 and saved in the Arduino input buffer?      
      {      
        char c = Serial2.read();                                      //if yes, read the next character from the input buffer and save it in the "response" String variable.      
        response += c;                                                  //append the next character to the response variable. at the end we will get a string(array of characters) contains the response.      
      }        
    }          
    if(SerialConnected)                                                         //if the "debug" variable value is TRUE, print the response on the Serial monitor.      
    {      
      Serial.println(response);   
    }          
    return response;                                                  //return the String response.      
}      
