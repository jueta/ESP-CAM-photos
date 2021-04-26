#include <AutoConnect.h>
#include <HTTPClient.h>
#include <WiFi.h>
#include <WebServer.h>

WebServer Server;
AutoConnect Portal(Server);

//String systemURL = "http://bombastesteback.herokuapp.com/data/teste";
//String systemURL = "http://192.168.1.105:3000/teste";

void rootPage() {
  char content[] = "Hello, world";
  Server.send(200, "text/plain", content);
}
void setup() {
  delay(1000);
  Serial.begin(115200);
  Serial.println("Começou");
  Server.on("/", rootPage);
  if (Portal.begin()) {
    Serial.println("HTTP server:" + WiFi.localIP().toString());
    
  }
}
void loop() {
  if(WiFi.status()== WL_CONNECTED){   //Check WiFi connection status
   
   HTTPClient http;   
  
   http.begin(systemURL);  //Specify destination for HTTP request
   http.addHeader("Content-Type", "application/json");             //Specify content-type header
   String body = "{\"api_key\":\"tPmAT5Ab3j7F9\",\"sensor\":\"BME280\",\"value1\":\"24.25\",\"value2\":\"49.54\",\"value3\":\"1005.14\"}";
   int httpResponseCode = http.POST(body);   //Send the actual POST request
  
   if(httpResponseCode>0){
    String response = http.getString();                       //Get the response to the request
  
    Serial.println(httpResponseCode);   //Print return code
    Serial.println(response);           //Print request answer
  
   }else{
  
    Serial.print("Error on sending POST: ");
    Serial.println(httpResponseCode);
  
   }
  
   http.end();  //Free resources
  
 }else{
  
    Serial.println("Error in WiFi connection");   
  
 }
  
  delay(10000); 
  Portal.handleClient();
}
