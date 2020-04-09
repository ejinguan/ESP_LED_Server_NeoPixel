// Wemons D1 mini: Lolin (Wemos) D1 mini, CPU 80 MHz, Flash 4 MB, Upload 962100
// ESP-01 (ESP8266EX): Generic ESP8266 Module, CPU 80 MHz, Flash 1MB, Crystal 26 MHz, Reset Method dtr (aka NodeMcu), Upload 115200

#include <ESP8266WiFi.h>
#include <WiFiManager.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <Adafruit_NeoPixel.h>

#define hex_2_int(x) (((x)<='9') ?((x)-'0'): (((x)<='F') ?((x)-'A'+10):((x)-'a'+10)))

// -------- WIFI -------------------------------
// Get passwords.h
#ifndef STASSID
#include "passwords.h"
#endif

// Wifi connection credentials
const char* ssid = STASSID;
const char* password = STAPSK;

// Define ESP8266 Web Server
ESP8266WebServer server(80);

// -------- LED --------------------------------
// This is for maintaining current LED state
String  led_rgb = "110000";
int     led_colors[3];
float   led_hsi[3];

// Neopixel definition
#define NUM_PIXELS 24
#define PIN_NEOPIXEL 2
Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUM_PIXELS, PIN_NEOPIXEL, NEO_GRB + NEO_KHZ800);
void handleHomePage () {

  String buf;
  
  buf = buf + "<!DOCTYPE html><html><head>";
  buf = buf + "<meta charset=\"utf-8\">";
  buf = buf + "<meta http-equiv=\"X-UA-Compatible\" content=\"IE=edge\">";
  buf = buf + "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">";
  buf = buf + "<link rel=\"stylesheet\" href=\"https://maxcdn.bootstrapcdn.com/bootstrap/3.3.6/css/bootstrap.min.css\">";
  buf = buf + "<script src=\"https://maxcdn.bootstrapcdn.com/bootstrap/3.3.6/js/bootstrap.min.js\"></script>";
  buf = buf + "<script src=\"https://cdnjs.cloudflare.com/ajax/libs/jscolor/2.0.4/jscolor.min.js\"></script>";
  buf = buf + "</head><body><div class=\"container\"><div class=\"row\"><h1>ESP Color Picker</h1>";
  buf = buf + "<a type=\"submit\" id=\"change_color\" type=\"button\" class=\"btn btn-primary\">Change Color</a> ";
  buf = buf + "<input class=\"jscolor {onFineChange:'update(this)'}\" id=\"rgb\" value=\"" + led_rgb + "\"></div></div>";
  buf = buf + "<script>function update(picker) {document.getElementById('rgb').innerHTML = Math.round(picker.rgb[0]) + ', ' +  Math.round(picker.rgb[1]) + ', ' + Math.round(picker.rgb[2]);";
  buf = buf + "document.getElementById(\"change_color\").href=document.getElementById('rgb').value;}</script></body></html>";
  
  server.send(200, "text/HTML", buf);  
}

void handleRoot() {
  server.sendHeader("Location","/LED/");        // Add a header to respond with a new location for the browser to go to the home page again
  server.send(303);                         // Send it back to the browser with an HTTP status 303 (See Other) to redirect
  
  
  //digitalWrite(led, 0);
  //server.send(200, "text/plain", "hello from esp8266!");
  //digitalWrite(led, 1);
}

void handleNotFound() {
  // Check if it's a request for LED
  String my_url;
  my_url = server.uri();
  my_url.toUpperCase();
  
  if (my_url.startsWith("/LED")) {
    handleLED();
    
  } else {
    String message = "File Not Found\n\n";
    
    message += "URI: ";
    message += my_url;
    message += "\nMethod: ";
    message += (server.method() == HTTP_GET) ? "GET" : "POST";
    message += "\nArguments: ";
    message += server.args();
    message += "\n";
    for (uint8_t i = 0; i < server.args(); i++) {
      message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
    }
    server.send(404, "text/plain", message);
  }
}

void handleLED() {

  String my_url;
  my_url = server.uri();
  my_url.toUpperCase();
  
  String message = "LED Control\n";
  message += "\nURI: ";
  message += server.uri();
  
  // url will look like "/LED/xxxxxx"
  String led_param = my_url.substring(5);
  message += "\nLED:" + led_param;

  // if LED not specified then no change
  if (led_param.length() == 0) led_param = led_rgb;

  // Process the url if it is 6 char (assume hex string)
  if (led_param.length() == 6) {

    setColor(led_param);
  
    handleHomePage();
  } else {
    server.send(200, "text/HTML", message);  
  }
}

void setColor(String rgb) {
  int red_value   = (rgb.charAt(0) - '0')*16 + (rgb.charAt(1) - '0');
  int green_value = (rgb.charAt(2) - '0')*16 + (rgb.charAt(3) - '0');
  int blue_value  = (rgb.charAt(4) - '0')*16 + (rgb.charAt(5) - '0');

  for(uint16_t i=0; i< strip.numPixels(); i++) {
    strip.setPixelColor(i, strip.Color(red_value, green_value, blue_value));
    //delay(50);
    strip.show();
  }

  led_rgb = rgb;
}


void setup(void) {
  pinMode(PIN_NEOPIXEL, OUTPUT);
  
  setColor("110000"); //red
  Serial.begin(115200);

  delay(1000); // Prevent crashing on boot
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.println("");

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  
  setColor("000011"); //blue
  delay(1000);
  
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  if (MDNS.begin("esp8266")) {
    Serial.println("MDNS responder started");
  }
  
  setColor("001100"); //green

  server.on("/", handleRoot);
  
  server.on("/LED", handleLED);

  server.on("/inline", []() {
    server.send(200, "text/plain", "this works as well");
  });

  server.on("/gif", []() {
    static const uint8_t gif[] PROGMEM = {
      0x47, 0x49, 0x46, 0x38, 0x37, 0x61, 0x10, 0x00, 0x10, 0x00, 0x80, 0x01,
      0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0x2c, 0x00, 0x00, 0x00, 0x00,
      0x10, 0x00, 0x10, 0x00, 0x00, 0x02, 0x19, 0x8c, 0x8f, 0xa9, 0xcb, 0x9d,
      0x00, 0x5f, 0x74, 0xb4, 0x56, 0xb0, 0xb0, 0xd2, 0xf2, 0x35, 0x1e, 0x4c,
      0x0c, 0x24, 0x5a, 0xe6, 0x89, 0xa6, 0x4d, 0x01, 0x00, 0x3b
    };
    char gif_colored[sizeof(gif)];
    memcpy_P(gif_colored, gif, sizeof(gif));
    // Set the background to a random set of colors
    gif_colored[16] = millis() % 256;
    gif_colored[17] = millis() % 256;
    gif_colored[18] = millis() % 256;
    server.send(200, "image/gif", gif_colored, sizeof(gif_colored));
  });

  server.onNotFound(handleNotFound);

  server.begin();
  Serial.println("HTTP server started");
}

void loop(void) {
  server.handleClient();
  MDNS.update();
// From: http://blog.saikoled.com/post/43693602826/why-every-led-light-should-be-using-hsi
//
// Function example takes H, S, I, and a pointer to the 
// returned RGB colorspace converted vector. It should
// be initialized with:
//
// int rgb[3];
//
// in the calling function. After calling hsi2rgb
// the vector rgb will contain red, green, and blue
// calculated values.
// H: 0-360 deg
// S: 0-1
// I: 0-1
void hsi2rgb(float H, float S, float I, int* rgb) {
  int r, g, b;
  while(H < 0) {
    H += 360; // Add to positive if H < 0
  }
  H = fmod(H,360); // cycle H around to 0-360 degrees
  H = 3.14159*H/(float)180; // Convert to radians.
  S = S>0?(S<1?S:1):0; // clamp S and I to interval [0,1]
  I = I>0?(I<1?I:1):0;
    
  // Math! Thanks in part to Kyle Miller.
  if(H < 2.09439) {
    r = 255*I/3*(1+S*cos(H)/cos(1.047196667-H));
    g = 255*I/3*(1+S*(1-cos(H)/cos(1.047196667-H)));
    b = 255*I/3*(1-S);
  } else if(H < 4.188787) {
    H = H - 2.09439;
    g = 255*I/3*(1+S*cos(H)/cos(1.047196667-H));
    b = 255*I/3*(1+S*(1-cos(H)/cos(1.047196667-H)));
    r = 255*I/3*(1-S);
  } else {
    H = H - 4.188787;
    b = 255*I/3*(1+S*cos(H)/cos(1.047196667-H));
    r = 255*I/3*(1+S*(1-cos(H)/cos(1.047196667-H)));
    g = 255*I/3*(1-S);
  }
  rgb[0]=r;
  rgb[1]=g;
  rgb[2]=b;
}
// RGB to HSI
// Formulas on https://www.imageeprocessing.com/2013/05/converting-rgb-image-to-hsi.html
// Adapted from https://github.com/saikoLED/saiko5/blob/master/firmware/arduino-sketchbook/LightBrick_wip/SaikoColor.h
// H: 0-360 deg
// S: 0-1
// I: 0-1
void rgb2hsi(float R, float G, float B, float* hsi) {
  float H;
  float S;
  float I;

  float minv = R;
  if (G < minv) minv = G;
  if (B < minv) minv = B;

  I = (R+G+B)/3.0;
  S = 1 - minv/I;
  if (S == 0.0) {
    H = 0.0;
  } else  {
    H = ((R-G)+(R-B))/2.0;
    H = H/sqrt((R-G)*(R-G) + (R-B)*(G-B));
    H = acos(H);
    if (B > G) {
      H = 2.0*M_PI - H;
    }
    H = H/(2.0*M_PI) * 360;
  }

  hsi[0] = H;
  hsi[1] = S;
  hsi[2] = I/255.0;
}
