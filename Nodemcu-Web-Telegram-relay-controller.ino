/*******************************************************************
    this is a basic example how to program a Telegram Bot
    using TelegramBOT library on ESP8266
 *                                                                 *
    Open a conversation with the bot, you can command via Telegram
    a led from ESP8266 GPIO
    https://web.telegram.org/#/im?p=@PattoonlineBot
 *                                                                 *
    written by Federico Adreani
 *******************************************************************/

#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <ESP8266TelegramBOT.h>
#include <SPI.h>
#include <ESP8266HTTPClient.h>


#include <DHT.h>
#define DHTTYPE DHT11   // DHT 11
#define DHT11_PIN 15

DHT dht(DHT11_PIN, DHTTYPE);





//Ethernet Port
WiFiServer server = WiFiServer(80); //default html port 80

//The number of outputs going to be switched.
int outputQuantity = 10;  //should not exceed 10

boolean outputInverted = true; //true or false

// Write the text description of the output channel
String buttonText[10] = {"22. Settore 1", "24. Settore 2", "26. Settore 3", "28. Settore 4", "30. Settore 5", "32. Luci Esterne", "34. na", "36. na", "38. na", "40. na"};
int outp = 0;
boolean printLastCommandOnce = false;
boolean printButtonMenuOnce = false;
boolean initialPrint = true;
String allOn = "";
String allOff = "";
boolean reading = false;
boolean outputStatus[10]; //Create a boolean array for the maximum ammount.
String rev = "V5.01";
int switchOnAllPinsButton = false; //true or false

int outputAddress[10] = { 16, 5, 4, 0, 2, 14, 12, 13, 10, 10}; //Allocate 10 spaces and name the output pin address.   // !!!!!!!!!!! cambiare 99 con 14 !DHT
unsigned long timeConnectedAt;
int refreshPage = 30; //default is 10sec.
// Set the output to retain the last status after power recycle.
int retainOutputStatus[10] = {1, 1, 1, 1, 1, 1, 1, 1, 1, 1}; //1-retain the last status. 0-will be off after power cut.


const int hygrometer = 03;  //Hygrometer sensor analog pin output at pin A0 of Arduino
int umid;


float tempOutDeg = 0.0;
float umid_aria = 0.0;


const char* ssid = "wifi_3";
const char* password = "adreaniWifi";


// Initialize Telegram BOT

#define BOTtoken "398215908:AAFzqHU8zwpp2zuj1iMcPCLWsY0CLGLEsfw"  //"425354869:AAFc9Z6HVVm3EJHonQoEqscSNLwMBQTSvyY"       //token of FlashledBOT
#define BOTname "AdreaniSpruzziniController"      //"AdreaniSpruzziniController"
#define BOTusername "AdreaniSpruzziniControllerBot"//"AdreaniSpruzziniControllerBot"

const String id_proprietario = "404952185";

String id_scrittore_messaggio = "404952185";


TelegramBOT bot(BOTtoken, BOTname, BOTusername);

int Bot_mtbs = 1000; //mean time between scan messages
long Bot_lasttime;   //last time messages' scan has been done
bool Start = false;

String message_read;
String wellcome = "Bentornato Fede";
String help_msg = "Comandi: /start per avviare il sistema /Hn per accendere il pin n /Ln per spegnere il led n /cicloXX per avviare un ciclo intero di XX minuti (per stazione)   /stato per sapere lo stato generale dei pin   /Hall per accendere tutto e /Lall per spegnere tutto";

String ip_esterno;



bool web = false;
bool isCiclo = false;

/********************************************
   EchoMessages - function to Echo messages
 ********************************************/
void Bot_ExecMessages() {
  for (int i = 1; i < bot.message[0][0].toInt() + 1; i++)      {
    message_read = bot.message[i][5].substring(1, bot.message[i][5].length());

    id_scrittore_messaggio = bot.message[i][4];


    if (id_proprietario != id_scrittore_messaggio) {

      bot.sendMessage(id_proprietario, "!!! ATTENZIONE FEDERICO !!!", "");
      bot.sendMessage(id_proprietario, "Accesso sospetto da parte di una persona sconosciuta", "");
      bot.sendMessage(id_proprietario, id_scrittore_messaggio, "");

    }

    WiFiClient client = server.available();

    //Serial.println(message_send);


    const char *c = message_read.c_str();

    int output = 3;

    String num_pin_str(c[1]);// numero del pin da attivare scritto come stringa x i messaggi

    isCiclo = false;

    if (message_read == "start") {

      writer(wellcome);
      writer("Premi /help per sapere i comandi oppure /stato per sapere lo stato generale dei pin");

      Start = true;

    } else if (message_read == "stato") {
      writer("Stato:");
      writer("Temperatura: " + String(tempOutDeg));
      writer("Umidità aria: " + String(umid_aria));
      writer("Umidità terreno: " + String(umid));
      bool any_pin_on = false;
      for (int var = 0; var < outputQuantity; var++)  {
        if (outputStatus[var] == true ) {                                                           //If Output is ON
          if (outputInverted == false) {                                                            //and if output is not inverted
            writer("PIN " + String(var) + " is ON ... Spegnilo con /L" + String(var));
            any_pin_on = true;
          } else {                                                                                   //else output is inverted then
            //Print html for OFF LED
          }
        } else {
          if (outputInverted == false) {                                                          //and if output is not inverted
            //Print html for OFF LED
          }
          else {                                                                                  //else output is inverted then
            writer("PIN " + String(var) + " is ON ... Spegnilo con /L" + String(var)); //Print html for ON LED
            any_pin_on = true;
          }
        }
      }

      if (any_pin_on == false) {
        writer("Nessun pin acceso");
      }
      writer("Ip esterno: " + ip_esterno);



    } else if (message_read == "help") {
      writer(help_msg);


    } else if (c[0] == 'c' && c[1] == 'i' && c[2] == 'c' && c[3] == 'l' && c[4] == 'o') {

      int minuti;

      if ( c[5] >= '0' && c[5] <= '9' ) {
        if ( c[6] >= '0' && c[6] <= '9' ) {
          char arr_minuti[] = {c[5], c[6]};
          //numero a due cifre
          minuti = atoi(arr_minuti);

        } else {
          char arr_minuti[] = {c[5]};
          minuti = atoi(arr_minuti);
        }


        writer("ciclo per i seq minuti ---> ");
        writer(String(minuti));
        WiFiClient client = server.available();
        cicloSpruzzini(client, minuti);


      } else {
        writer("Errore formato ciclo  /help");

      }


    } else if (c[0] == 'H' || c[0] == 'h') {
      if ( c[1] >= '0' && c[1] <= '9' ) {
        output = 1;
      } else if (c[1] == 'a' && c[2] == 'l' && c[3] == 'l') {
        for (int var = 0; var < outputQuantity; var++) {
          triggerPin(outputAddress[var], client, 1);
        }

      } else {
        writer("Errore formato richiesta accensione, /help");
      }

    } else if (c[0] == 'L' || c[0] == 'l') {
      if ( c[1] >= '0' && c[1] <= '9' ) {
        output = 0;
      } else if (c[1] == 'a' && c[2] == 'l' && c[3] == 'l') {
        for (int var = 0; var < outputQuantity; var++) {
          triggerPin(outputAddress[var], client, 0);
        }

      } else {
        writer("Errore formato richiesta spegnimento, /help");
      }


    } else {
      writer("Richiesta sconosciuta, consulta /help");

    }

    if (output == 0 || output == 1) {
      int pin = (int)c[1] - '0';
      web = false;
      triggerPin(outputAddress[pin], client, output);
    }

    bot.message[0][0] = "";   // All messages have been replied - reset new messages
  }

}



void setup() {

  for (int var = 0; var < outputQuantity; var++) {
    pinMode(outputAddress[var], OUTPUT);
    digitalWrite(outputAddress[var], HIGH);
  }

  pinMode(15, INPUT);

  readOutputStatuses();


  Serial.begin(115200);
  delay(500);

  dht.begin();


  // Mac address should be different for each device in your LAN
  byte arduino_mac[] = { 0xDE, 0xED, 0xBA, 0xFE, 0xFE, 0xED };
  IPAddress arduino_ip ( 192,  168,   1,  170);
  IPAddress gateway_ip ( 192,  168,   1,   1);
  IPAddress subnet_mask(255, 255, 255,   0);

  WiFi.mode(WIFI_STA);//spegne l'hotspot, ma consente connessione

  WiFi.config(arduino_ip, gateway_ip, subnet_mask);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }


  Serial.println("");

  server.begin();

  Serial.println("WiFi connected");

  IPAddress ip = WiFi.localIP();


  bot.begin();      // launch Bot functionalities

  get_ip_esterno();

  writer("Server started at: " + String(ip[0]) + "." + String(ip[1]) + "." + String(ip[2]) + "." + String(ip[3])); //  http://192.168.1.175/
  writer("IP esterno -> " + ip_esterno);
  writer("Premi /start per iniziare");
}



void loop() {

  tempOutDeg = dht.readHumidity();
  umid_aria = dht.readTemperature();

  umid = analogRead(hygrometer);    //Read analog value
  umid = constrain(umid, 400, 1023); //Keep the ranges!
  umid = map(umid, 400, 1023, 100, 0); //Map value : 400 will be 100 and 1023 will be 0


  checkForClient();

  if (millis() > Bot_lasttime + Bot_mtbs)  {
    bot.getUpdates(bot.message[0][1]);   // launch API GetUpdates up to xxx message
    Bot_ExecMessages();   // reply to message with Echo
    Bot_lasttime = millis();
  }

}







////////////////////////////////////////////////////////////////////////
//printLoginTitle Function
////////////////////////////////////////////////////////////////////////
//Prints html button title
void printLoginTitle(WiFiClient client) {
  //    client.println("<div  class=\"group-wrapper\">");
  client.println("    <h2>Please enter the user data to login.</h2>");
  client.println();
}





////////////////////////////////////////////////////////////////////////
//htmlHeader Function
////////////////////////////////////////////////////////////////////////
//Prints html header
void printHtmlHeader(WiFiClient client) {
  Serial.print("Serving html Headers at ms -");
  timeConnectedAt = millis(); //Record the time when last page was served.
  Serial.print(timeConnectedAt); // Print time for debbugging purposes

  // send a standard http response header
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: text/html");
  client.println("Connnection: close");
  client.println();
  client.println("<!DOCTYPE HTML>");
  client.println("<head>");

  // add page title
  client.println("<title>Ethernet Switching</title>");
  client.println("<meta name=\"description\" content=\"Ethernet Switching\"/>");

  // add a meta refresh tag, so the browser pulls again every x seconds:
  client.print("<meta http-equiv=\"refresh\" content=\"");
  client.print(refreshPage);
  client.println("; url=/\">");

  // add other browser configuration
  client.println("<meta name=\"apple-mobile-web-app-capable\" content=\"yes\">");
  client.println("<meta name=\"apple-mobile-web-app-status-bar-style\" content=\"default\">");
  client.println("<meta name=\"viewport\" content=\"width=device-width, user-scalable=no\">");

  //inserting the styles data, usually found in CSS files.
  client.println("<style type=\"text/css\">");
  client.println("");

  //This will set how the page will look graphically
  client.println("html { height:100%; }");

  client.println("  body {");
  client.println("    height: 100%;");
  client.println("    margin: 0;");
  client.println("    font-family: helvetica, sans-serif;");
  client.println("    -webkit-text-size-adjust: none;");
  client.println("   }");
  client.println("");
  client.println("body {");
  client.println("    -webkit-background-size: 100% 21px;");
  client.println("    background-color: #c5ccd3;");
  client.println("    background-image:");
  client.println("    -webkit-gradient(linear, left top, right top,");
  client.println("    color-stop(.75, transparent),");
  client.println("    color-stop(.75, rgba(255,255,255,.1)) );");
  client.println("    -webkit-background-size: 7px;");
  client.println("   }");
  client.println("");
  client.println(".view {");
  client.println("    min-height: 100%;");
  client.println("    overflow: auto;");
  client.println("   }");
  client.println("");
  client.println(".header-wrapper {");
  client.println("    height: 44px;");
  client.println("    font-weight: bold;");
  client.println("    text-shadow: rgba(0,0,0,0.7) 0 -1px 0;");
  client.println("    border-top: solid 1px rgba(255,255,255,0.6);");
  client.println("    border-bottom: solid 1px rgba(0,0,0,0.6);");
  client.println("    color: #fff;");
  client.println("    background-color: #8195af;");
  client.println("    background-image:");
  client.println("    -webkit-gradient(linear, left top, left bottom,");
  client.println("    from(rgba(255,255,255,.4)),");
  client.println("    to(rgba(255,255,255,.05)) ),");
  client.println("    -webkit-gradient(linear, left top, left bottom,");
  client.println("    from(transparent),");
  client.println("    to(rgba(0,0,64,.1)) );");
  client.println("    background-repeat: no-repeat;");
  client.println("    background-position: top left, bottom left;");
  client.println("    -webkit-background-size: 100% 21px, 100% 22px;");
  client.println("    -webkit-box-sizing: border-box;");
  client.println("   }");
  client.println("");
  client.println(".header-wrapper h1 {");
  client.println("    text-align: center;");
  client.println("    font-size: 20px;");
  client.println("    line-height: 44px;");
  client.println("    margin: 0;");
  client.println("   }");
  client.println("");
  client.println(".group-wrapper {");
  client.println("    margin: 9px;");
  client.println("    }");
  client.println("");
  client.println(".group-wrapper h2 {");
  client.println("    color: #4c566c;");
  client.println("    font-size: 17px;");
  client.println("    line-height: 0.8;");
  client.println("    font-weight: bold;");
  client.println("    text-shadow: #fff 0 1px 0;");
  client.println("    margin: 20px 10px 12px;");
  client.println("   }");
  client.println("");
  client.println(".group-wrapper h3 {");
  client.println("    color: #4c566c;");
  client.println("    font-size: 12px;");
  client.println("    line-height: 1;");
  client.println("    font-weight: bold;");
  client.println("    text-shadow: #fff 0 1px 0;");
  client.println("    margin: 20px 10px 12px;");
  client.println("   }");
  client.println("");
  client.println(".group-wrapper h4 {");  //Text for description
  client.println("    color: #212121;");
  client.println("    font-size: 14px;");
  client.println("    line-height: 1;");
  client.println("    font-weight: bold;");
  client.println("    text-shadow: #aaa 1px 1px 3px;");
  client.println("    margin: 5px 5px 5px;");
  client.println("   }");
  client.println("");
  client.println(".group-wrapper table {");
  client.println("    background-color: #fff;");
  client.println("    -webkit-border-radius: 10px;");

  client.println("    -moz-border-radius: 10px;");
  client.println("    -khtml-border-radius: 10px;");
  client.println("    border-radius: 10px;");

  client.println("    font-size: 17px;");
  client.println("    line-height: 20px;");
  client.println("    margin: 9px 0 20px;");
  client.println("    border: solid 1px #a9abae;");
  client.println("    padding: 11px 3px 12px 3px;");
  client.println("    margin-left:auto;");
  client.println("    margin-right:auto;");

  client.println("    -moz-transform :scale(1);"); //Code for Mozilla Firefox
  client.println("    -moz-transform-origin: 0 0;");

  client.println("   }");
  client.println("");

  //how the green (ON) LED will look
  client.println(".green-circle {");
  client.println("    display: block;");
  client.println("    height: 23px;");
  client.println("    width: 23px;");
  client.println("    background-color: #0f0;");
  //client.println("    background-color: rgba(60, 132, 198, 0.8);");
  client.println("    -moz-border-radius: 11px;");
  client.println("    -webkit-border-radius: 11px;");
  client.println("    -khtml-border-radius: 11px;");
  client.println("    border-radius: 11px;");
  client.println("    margin-left: 1px;");

  client.println("    background-image: -webkit-gradient(linear, 0% 0%, 0% 90%, from(rgba(46, 184, 0, 0.8)), to(rgba(148, 255, 112, .9)));@");
  client.println("    border: 2px solid #ccc;");
  client.println("    -webkit-box-shadow: rgba(11, 140, 27, 0.5) 0px 10px 16px;");
  client.println("    -moz-box-shadow: rgba(11, 140, 27, 0.5) 0px 10px 16px; /* FF 3.5+ */");
  client.println("    box-shadow: rgba(11, 140, 27, 0.5) 0px 10px 16px; /* FF 3.5+ */");

  client.println("    }");
  client.println("");

  //how the black (off)LED will look
  client.println(".black-circle {");
  client.println("    display: block;");
  client.println("    height: 23px;");
  client.println("    width: 23px;");
  client.println("    background-color: #040;");
  client.println("    -moz-border-radius: 11px;");
  client.println("    -webkit-border-radius: 11px;");
  client.println("    -khtml-border-radius: 11px;");
  client.println("    border-radius: 11px;");
  client.println("    margin-left: 1px;");
  client.println("    -webkit-box-shadow: rgba(11, 140, 27, 0.5) 0px 10px 16px;");
  client.println("    -moz-box-shadow: rgba(11, 140, 27, 0.5) 0px 10px 16px; /* FF 3.5+ */");
  client.println("    box-shadow: rgba(11, 140, 27, 0.5) 0px 10px 16px; /* FF 3.5+ */");
  client.println("    }");
  client.println("");

  //this will add the glare to both of the LEDs
  client.println("   .glare {");
  client.println("      position: relative;");
  client.println("      top: 1;");
  client.println("      left: 5px;");
  client.println("      -webkit-border-radius: 10px;");
  client.println("      -moz-border-radius: 10px;");
  client.println("      -khtml-border-radius: 10px;");
  client.println("      border-radius: 10px;");
  client.println("      height: 1px;");
  client.println("      width: 13px;");
  client.println("      padding: 5px 0;");
  client.println("      background-color: rgba(200, 200, 200, 0.25);");
  client.println("      background-image: -webkit-gradient(linear, 0% 0%, 0% 95%, from(rgba(255, 255, 255, 0.7)), to(rgba(255, 255, 255, 0)));");
  client.println("    }");
  client.println("");


  //and finally this is the end of the style data and header
  client.println("</style>");
  client.println("</head>");

  //now printing the page itself
  client.println("<body>");
  client.println("<div class=\"view\">");
  client.println("    <div class=\"header-wrapper\">");
  client.println("      <h1>Ethernet Switching</h1>");
  client.println("    </div>");

  //////

} //end of htmlHeader



////////////////////////////////////////////////////////////////////////
//printHtmlButtons Function
////////////////////////////////////////////////////////////////////////
//print the html buttons to switch on/off channels
void printHtmlButtons(WiFiClient client) {

  //Start to create the html table
  client.println("");
  //client.println("<p>");
  client.println("<FORM>");
  client.println("<table border=\"0\" align=\"center\">");


  //Printing the Temperature
  client.print("<tr>\n");

  client.print("<td><h4>");
  client.print("Temperatura: ");
  client.print("</h4></td>\n");
  client.print("<td></td>");
  client.print("<td>");
  client.print("<h3>");
  client.print(tempOutDeg);
  client.print(" &deg;C</h3></td>\n");


  client.print("<td></td>");
  client.print("</tr>");


  /////////////////////////////////////////////////////////
  //umidita aria
  /////////////////////////////////////////////////////////

  client.print("<tr>\n");

  client.print("<td><h4>");
  client.print("Umidita aria:");
  client.print("</h4></td>\n");
  client.print("<td></td>");
  client.print("<td>");
  client.print("<h3>");
  client.print(umid_aria);
  client.print(" %</h3></td>\n");


  client.print("<td></td>");
  client.print("</tr>");



  /////////////////////////////////////////////////////////
  //umidita terreno
  /////////////////////////////////////////////////////////

  client.print("<tr>\n");

  client.print("<td><h4>");
  client.print("Umidita del terreno:");
  client.print("</h4></td>\n");
  client.print("<td></td>");
  client.print("<td>");
  client.print("<h3>");
  client.print(umid);
  client.print(" %</h3></td>\n");


  client.print("<td></td>");
  client.print("</tr>");



  //Start printing button by button
  for (int var = 0; var < outputQuantity; var++)  {

    //set command for all on/off
    /*    allOn += "H";
        allOn += outputAddress[var];
        allOff += "L";
        allOff += outputAddress[var];
    */

    //Print begining of row
    client.print("<tr>\n");

    //Prints the button Text
    client.print("<td><h4>");
    client.print(buttonText[var]);
    client.print("</h4></td>\n");

    //Prints the ON Buttons
    client.print("<td>");
    //client.print(buttonText[var]);
    client.print("<INPUT TYPE=\"button\" VALUE=\"ON ");
    //client.print(buttonText[var]);
    client.print("\" onClick=\"parent.location='/?H");
    client.print(var);
    client.print("'\"></td>\n");

    //Prints the OFF Buttons
    client.print(" <td><INPUT TYPE=\"button\" VALUE=\"OFF");
    //client.print(var);
    client.print("\" onClick=\"parent.location='/?L");
    client.print(var);
    client.print("'\"></td>\n");


    //Print first part of the Circles or the LEDs

    //Invert the LED display if output is inverted.

    if (outputStatus[var] == true ) {                                                           //If Output is ON
      if (outputInverted == false) {                                                            //and if output is not inverted
        client.print(" <td><div class='green-circle'><div class='glare'></div></div></td>\n"); //Print html for ON LED
      }
      else {                                                                                   //else output is inverted then
        client.print(" <td><div class='black-circle'><div class='glare'></div></div></td>\n"); //Print html for OFF LED
      }
    }
    else                                                                                      //If Output is Off
    {
      if (outputInverted == false) {                                                          //and if output is not inverted
        client.print(" <td><div class='black-circle'><div class='glare'></div></div></td>\n"); //Print html for OFF LED
      }
      else {                                                                                  //else output is inverted then
        client.print(" <td><div class='green-circle'><div class='glare'></div></div></td>\n"); //Print html for ON LED
      }
    }

    //Print end of row
    client.print("</tr>\n");

  }




  //Prints the CICLO INTERO
  client.print("<tr>\n<td><INPUT TYPE=\"button\" VALUE=\"CICLO INTERO");
  client.print("\" onClick=\"parent.location='/?Hx' + document.getElementById('durata').value");
  client.print("\"></td>\n<td>    <label>Durata: <input type='number' id='durata' min='0' max='100' step='1' value='8'>  </label>         </td></tr>");


  //Display or hide the Print all on Pins Button
  if (switchOnAllPinsButton == true ) {

    //Prints the ON All Pins Button
    client.print("<tr>\n<td><INPUT TYPE=\"button\" VALUE=\"Switch ON All Pins");
    client.print("\" onClick=\"parent.location='/?");
    //client.print(allOn);
    client.print("'\"></td>\n");

    //Prints the OFF All Pins Button
    client.print("<td><INPUT TYPE=\"button\" VALUE=\"Switch OFF All Pins");
    client.print("\" onClick=\"parent.location='/?");
    client.print(allOff);
    client.print("'\"></td>\n<td></td>\n<td></td>\n</tr>\n");
  }

  //Closing the table and form
  client.println("</table>");
  client.println("</FORM>");
  //client.println("</p>");

}


////////////////////////////////////////////////////////////////////////
//htmlFooter Function
////////////////////////////////////////////////////////////////////////
//Prints html footer
void printHtmlFooter(WiFiClient client) {
  //Set Variables Before Exiting
  printLastCommandOnce = false;
  printButtonMenuOnce = false;
  allOn = "";
  allOff = "";

  //printing last part of the html
  client.println("\n<h3 align=\"center\">&copy; Author - Adreani Federico <br> " + ip_esterno + " - 01/06/2018 - ");
  client.println(rev);
  client.println("</h3></div>\n</div>\n</body>\n</html>");
  //client.println("<iframe src='http://www.xxxxxx.com/arduino/arduino.php'></iframe>");

  delay(1); // give the web browser time to receive the data

  client.stop(); // close the connection:

  Serial.println(" - Done, Closing Connection.");

  delay (2); //delay so that it will give time for client buffer to clear and does not repeat multiple pages.

} //end of htmlFooter




////////////////////////////////////////////////////////////////////////
//readOutputStatuses Function
////////////////////////////////////////////////////////////////////////
//Reading the Output Statuses
void readOutputStatuses() {
  for (int var = 0; var < outputQuantity; var++)  {
    outputStatus[var] = digitalRead(outputAddress[var]);
    //Serial.print(outputStatus[var]);
  }

}


////////////////////////////////////////////////////////////////////////
//printHtmlButtonTitle Function
////////////////////////////////////////////////////////////////////////
//Prints html button title
void printHtmlButtonTitle(WiFiClient client) {
  client.println("<div  class=\"group-wrapper\">");
  client.println("    <h2>Switch the required output.</h2>");
  client.println();
}






size_t FindIndex( const int a[], size_t size, int value ) {
  size_t index = 0;
  while ( index < size && a[index] != value ) ++index;
  return ( index == size ? -1 : index );
}



void triggerPin(int pin, WiFiClient client, int outp) {



  //Switching on or off outputs, reads the outputs and prints the buttons
  if (web && !isCiclo) {
    writer("Rilevata richiesta web: ");
  }
  web = false;


  int pin_index = FindIndex(outputAddress, outputQuantity, pin);

  //Setting Outputs
  if (pin != 777) {

    if (outp == 1) {
      if (outputInverted == true) {
        if (!isCiclo) {
          writer("Accendo il pin " + String(pin_index));
          writer("Premi /L" + String(pin_index) + " per rispegnerlo");
        }
        digitalWrite(pin, LOW);
      } else {
        if (!isCiclo) {
          writer("Spengo il pin " + String(pin_index));
          writer("Premi /H" + String(pin_index) + " per riaccenderlo");
        }
        digitalWrite(pin, HIGH);
      }
    }
    if (outp == 0) {
      if (outputInverted == true) {
        if (!isCiclo) {
          writer("Spengo il pin " + String(pin_index));
          writer("Premi /H" + String(pin_index) + " per riaccenderlo");
        }
        digitalWrite(pin, HIGH);
      } else {
        if (!isCiclo) {
          writer("Accendo il pin " + String(pin_index));
          writer("Premi /L" + String(pin_index) + " per rispegnerlo");
        }
        digitalWrite(pin, LOW);
      }
    }


  }
  //Refresh the reading of outputs
  readOutputStatuses();


  //Prints the buttons
  if (printButtonMenuOnce == true) {
    printHtmlButtons(client);
    printButtonMenuOnce = false;
  }

}




void checkForClient() {

  WiFiClient client = server.available();

  if (client) {

    // an http request ends with a blank line

    boolean sentHeader = false;
    boolean currentLineIsBlank = true;

    isCiclo = false;

    while (client.connected()) {

      char c = client.read();

      if (c == '*') {
        printHtmlHeader(client); //call for html header and css
        printLoginTitle(client);
        printHtmlFooter(client);
        //sentHeader = true;
        break;
      }


      //if header was not set send it
      if (!sentHeader) {

        printHtmlHeader(client); //call for html header and css
        printHtmlButtonTitle(client); //print the button title

        //This is for the arduino to construct the page on the fly.
        sentHeader = true;
      }

      if (reading && c == ' ') {
        reading = false;
      }


      if (c == '?') {
        reading = true; //found the ?, begin reading the info
      }

      if (reading) {


        //if user input is H set output to 1
        if (c == 'H') {
          outp = 1;
          web = true;
        }

        //if user input is L set output to 0
        if (c == 'L') {
          outp = 0;
          web = true;
        }

        //          Serial.print(c);   //print the value of c to serial communication

        switch (c) {

          case '0':
            //add code here to trigger on 0
            triggerPin(outputAddress[0], client, outp);
            break;
          case '1':
            //add code here to trigger on 1
            triggerPin(outputAddress[1], client, outp);
            break;
          case '2':
            //add code here to trigger on 2
            triggerPin(outputAddress[2], client, outp);
            break;
          case '3':
            //add code here to trigger on 3
            triggerPin(outputAddress[3], client, outp);
            break;
          case '4':
            //add code here to trigger on 4
            triggerPin(outputAddress[4], client, outp);
            break;
          case '5':
            //add code here to trigger on 5
            triggerPin(outputAddress[5], client, outp);
            //printHtml(client);
            break;
          case '6':
            //add code here to trigger on 6
            triggerPin(outputAddress[6], client, outp);
            break;
          case '7':
            //add code here to trigger on 7
            triggerPin(outputAddress[7], client, outp);
            break;
          case '8':
            //add code here to trigger on 8
            triggerPin(outputAddress[8], client, outp);
            break;
          case '9':
            //add code here to trigger on 9
            triggerPin(outputAddress[9], client, outp);
            break;
          case 'x':
            char arr[2] = {char(client.read()), char(client.read())};
            int durata =  atoi(arr);
            cicloSpruzzini(client, durata);
            break;
        } //end of switch case

      }//end of switch switch the relevant output

      //if user input was blank
      if (c == '\n' && currentLineIsBlank) {
        printLastCommandOnce = true;
        printButtonMenuOnce = true;
        triggerPin(777, client, outp); //Call to read input and print menu. 777 is used not to update any outputs
        break;
      }

      //for (int i = 0 ; i < 16 ; i++) {writer(String((char)buffer[i]));}

    }


    printHtmlFooter(client); //Prints the html footer

  }

}




////////////////////////////////////////////////////////////////////////
//cicloSpruzzini Function
////////////////////////////////////////////////////////////////////////
//
void cicloSpruzzini(WiFiClient client, int durata) {


  writer("inizio ciclo spruzzini --> " + String(durata));



  for ( unsigned int a = 0; a < 5; a++ )
  {
    writer("Inizio settore " + String(a));

    isCiclo = true;

    triggerPin(outputAddress[a], client, 1);
    aspetta(durata);
    triggerPin(outputAddress[a], client, 0);

    writer("Fine settore " + String(a));
  }

  writer("Ciclo terminato");

  client.print("<meta http-equiv='refresh' content='1; url=/'>");
  client.stop();
}





void aspetta(int durata) {
  for (int i = 0; i < 600; i++) {
    for (int b = 0; b < durata; b++) {

      
      delay(100);
    }
  }
}






void writer(String stringa) {

  Serial.println(stringa);


  bot.sendMessage(id_scrittore_messaggio, stringa, "");

  Serial.println("nessuna chat su telegram");


}


void get_ip_esterno() {
  HTTPClient http;
  http.begin("http://www.pattoonline.com/arduino/arduino.php");
//  http.addHeader("Content-Type", "application/x-www-form-urlencoded");
 // http.POST("title=foo&body=bar&userId=1");
  ip_esterno = http.getString();
  http.end();
}
