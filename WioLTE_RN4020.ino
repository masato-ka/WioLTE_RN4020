#include <WioLTEforArduino.h>
#include <stdio.h>

#define ASCIIA 65
#define ASCIIF 70
#define ASCII0 48
#define ASCII9 57
#define BAUDRATE 115200
//#define PRINT_DEBUG

WioLTE Wio;
char buffer[42];


void trashSerialBuffer(){
  while(Serial.available()){
    Serial.read(); 
  }
}

void copySerialBuffer(char* buffer){
  while(Serial.available()){
    *buffer = Serial.read();
    ++buffer;
  }
}


int hex_format_char_to_decimal(const char* hex_char){
  #ifdef PRINT_DEBUG
    SerialUSB.println(hex_char);
    SerialUSB.println(sizeof(hex_char));
  #endif
  if(sizeof(hex_char) != sizeof(char)*4){return -1;}
  int result = 0;
  int base = 0;
  for(int i=0; i < 2; ++i){
    if(65 <= hex_char[i] && hex_char[i] <=70){
     result += hex_char[i]-55;
    }
    if(48 <= hex_char[i] && hex_char[i] <= 57){
     result += hex_char[i]-48;
    }
    base = (16*(1-i)==0)? 1 : 16*(1-i);
//    if(16*(1-i) == 0){base=1;}
    result *= base;
  }
  #ifdef PRINT_DEBUG
    SerialUSB.println(result);
  #endif
  return result;
}


int hex_format_str_to_decimal(String str){
  #ifdef PRINT_DEBUG
    SerialUSB.println("raw:" + str);
    SerialUSB.println(str.length());
  #endif
  if(str.length() != 4){
    SerialUSB.println("break");
    return -1;
  }

  int lsb = hex_format_char_to_decimal(str.substring(0,2).c_str());
  int msb = hex_format_char_to_decimal(str.substring(2,4).c_str());
  if(msb == -1 || lsb == -1){return -1;}
  return (msb<<8) + lsb;
}


double templature(String raw){
  return hex_format_str_to_decimal(raw.substring(2,6).c_str()) * 0.01;
}

double humidity(String raw){
  return hex_format_str_to_decimal(raw.substring(6,10).c_str()) * 0.01;
}

double lumix(String raw){
  return hex_format_str_to_decimal(raw.substring(10,14).c_str());
}

double uvi(String raw){
  return hex_format_str_to_decimal(raw.substring(14,18).c_str()) * 0.01;
}

double pressure(String raw){
  return hex_format_str_to_decimal(raw.substring(18,22).c_str()) * 0.1;
}

double noise(String raw){
  return hex_format_str_to_decimal(raw.substring(22,26).c_str()) * 0.01;
}

double disco(String raw){
  return hex_format_str_to_decimal(raw.substring(26,30).c_str()) * 0.01;
}

double heat(String raw){
  return hex_format_str_to_decimal(raw.substring(30,34).c_str()) * 0.01;
}

double battery(String raw){
  return hex_format_str_to_decimal(raw.substring(34,38).c_str()) * 0.001;
}

void setup() {
  delay(200);

  SerialUSB.println("");
  SerialUSB.println("---START INIT---------------------");

  SerialUSB.println("### Initialize of IO");
  Wio.Init();
  Wio.PowerSupplyGrove(true);

  SerialUSB.println("### Initialize RN4020");
  Serial.begin(115200);
  
  SerialUSB.println("### LTE Port supply ON.");
  Wio.PowerSupplyLTE(true);
  delay(5000);
  SerialUSB.println("### Turn on or reset.");
  if (!Wio.TurnOnOrReset()) {
    SerialUSB.println(F("### ERROR! ###"));
    return;
  }
  SerialUSB.println("### Connecting to \"soracom.io\".");
  delay(5000);
  if (!Wio.Activate("soracom.io", "sora", "sora")) {
    SerialUSB.println("### ERROR! ###");
    return;
  }
  delay(3000L);
  SerialUSB.println("---Complete INIT----------------");

}

String get_wx2beacon_data(){
  SerialUSB.println("### Connect to periferal");
  Serial.println("E,1,E6E4B1DA83BC");
  delay(2000);
  trashSerialBuffer();
  SerialUSB.println("#### Read data");
  Serial.println("CHR,0019");
  delay(500);
  copySerialBuffer(buffer);
  SerialUSB.println("#### Disconnect");
  Serial.println("K");
  delay(500);
  trashSerialBuffer();
  return String(buffer);
}

void send_to_harvest(char *data){
  int connectId;
  SerialUSB.println("### Open.");
  connectId = Wio.SocketOpen("harvest.soracom.io", 8514, WIOLTE_UDP);
  if (connectId < 0) {
    SerialUSB.println("### ERROR! ###");
    return;  
  }

  SerialUSB.println("### Send.");
  SerialUSB.print("Send:");
  SerialUSB.print(data);
  SerialUSB.println("");
  if (!Wio.SocketSend(connectId, data)) {
    SerialUSB.println("### ERROR! ###");
    return;
  }

  SerialUSB.println("### Receive.");
  int length;
  do {
    length = Wio.SocketReceive(connectId, data, sizeof (data));
    if (length < 0) {
      SerialUSB.println("### ERROR! ###");
      return;
    }
  } while (length == 0);
  SerialUSB.print("Receive:");
  SerialUSB.print(data);
  SerialUSB.println("");

  SerialUSB.println("### Close.");
  if (!Wio.SocketClose(connectId)) {
    SerialUSB.println("### ERROR! ###");
    return;
  }
}

void loop() {
  char data[200];
  String raw_data_str = get_wx2beacon_data();
  SerialUSB.println(raw_data_str);
  String data_str = raw_data_str.substring(2,40);


  sprintf(data, "{\"temp\":%.2f, \"humid\":%.2f,\"lumix\":%.2f, \"uvi\":%.2f,\"pressure\":%.2f, \"noise\":%.1f,\"disco\":%.1f, \"heat\":%.1f,\"batt\":%.3f}",
                  templature(data_str), humidity(data_str),
                  lumix(data_str), uvi(data_str),
                  pressure(data_str), noise(data_str),
                  disco(data_str), heat(data_str),
                  battery(data_str));
  SerialUSB.println(data);

  send_to_harvest(data);
  
  delay(1800000L);
}

