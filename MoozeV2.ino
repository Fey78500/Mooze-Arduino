#include <FirebaseArduino.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>

#include "../generic/common.h"

#define BUTTON_PIN D5     // the number of the pushbutton pin

int buttonState = 1;         // variable for reading the pushbutton status
int lastButtonState = 1;         // variable for reading the pushbutton status

#define FIREBASE_HOST "mooze-1dacd.firebaseio.com"
#define FIREBASE_AUTH "08wi7hLSmtuvF9nYlB34gQbpDHD09b8DVRbd14Dv"

String biper = "fef";
String order = "";
bool isReady = 0;
bool isReturned = 0;

void setup() {
  pinMode(BUTTON_PIN, INPUT);
  Serial.begin(115200);
  WiFiManager wifiManager;
  wifiManager.autoConnect("Mooze WiFi Configuration");
  beep(300, 80);
  beep(400, 80);
  beep(500, 80);
  //4. Setup Firebase credential in setup()
  Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);
  Firebase.stream("/orders");
  delay(1000);

}
void(* resetFunc) (void) = 0; //declare reset function @ address 0

void beep(int frequence, int timer) {
  tone(D1, frequence, timer);
  delay(timer);
  noTone(D1);
}

void loop() {
    if (Firebase.failed()) {
      Serial.println("streaming error");
      Serial.println(Firebase.error());
      Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);
      Firebase.stream("/orders");
    }
    
    if (Firebase.available()) {
     //On recupere l'evenement reçu de firebase
     FirebaseObject event = Firebase.readEvent();
     String eventType = event.getString("type");
     eventType.toLowerCase();
     Serial.println(eventType);
     Serial.println(event.getString("path"));
     //Si c'est une modification
     if(eventType == "patch"){
       CreateField(event);
     }
     if(eventType == "put"){
       UpdateField(event);
     }
  }
  // Si la commande est prete et pas encore recuperer
  if(isReady == 1 && isReturned == 0){
    //on beep
    beep(100, 1000);
    delay(100);
  }
  if(isReady == 1){
    //Si le button est appuyé
    buttonState = digitalRead(BUTTON_PIN);
    if (buttonState != lastButtonState) {
      if(buttonState == LOW){
        // la commande a été recuperer
        isReturned = 1;
        
        Firebase.setBool("/orders/" + order + "/delivered", 1);
        Firebase.setString("/orders/" + order + "/biper", "");
        order="";
        isReady=0;
        isReturned=0;
        resetFunc();
      }
    }
  }
  lastButtonState = buttonState;
}
void CreateField(FirebaseObject event){
  //On recupere le chemin
  String path = event.getString("path");
  //On recupere l'action grace au chemin (dernier mot de l'url)
  int actionPosition = path.indexOf('/');
  String getOrder = path.substring(actionPosition + 1);
  //On recupere la data
  String getBiper = Firebase.getString("/orders/" + getOrder + "/biper");
  if(getBiper == biper){
    order = getOrder;
    Serial.println(order);
    isReady = Firebase.getBool("/orders/" + order + "/checkedOut"); 
    Serial.println(isReady); 
  }
}
void UpdateField(FirebaseObject event){
  //On recupere le chemin
  String path = event.getString("path");
  //On recupere l'action grace au chemin (dernier mot de l'url)
  int actionPosition = path.indexOf('/', 1 );
  String getAction = path.substring(actionPosition + 1);
  //Si c'est biper
  if(getAction == "biper"){
    //On recupere la data
    String data = event.getString("data");
    //et on verifie si la data corespond a ce biper
    if(data == biper){
      //Si oui on change la commande courante
      Serial.print(order);
      order = path.substring(1,actionPosition);
    }
  }
  //Si l'action est checkedOut et que c'est bien la commande en cours
  if(getAction == "checkedOut" && order == path.substring(1,actionPosition)){
    //On recupere la valeurs et on l'attribue a is ready
    bool data = event.getBool("data");
    isReady = data;     
  }
}

