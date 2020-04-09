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

// -------- LED --------------------------------
// -------- LED Animations ---------------------
#define anim_frame_ms 2
unsigned long anim_last_frame = 0;  // Hold millis of last frame
int   anim_direction = 1;           // 1 = left, 2 = right
int   anim_speed = 5;               // 0-10
int   anim_frame_count = 0;         // Frame counter for use
bool  anim_fade = false;            // On/off for Fade
bool  anim_wave = false;            // On/off for Wave             
bool  anim_RGBwave = false;         // On/off for RGB Wave
int   anim_brightness;              // not done yet


// ---------------------------------------------
// --- Handlers for various endpoints ----------
// ---------------------------------------------

// --- Generic handler to generate homepage ----
void handleHomePage () {

  String buf;
  
  buf = buf + "<!DOCTYPE html><html><head>";
  buf = buf + "<meta charset=\"utf-8\">";
  buf = buf + "<meta http-equiv=\"X-UA-Compatible\" content=\"IE=edge\">";
  buf = buf + "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">";
  buf = buf + "<link rel=\"stylesheet\" href=\"https://maxcdn.bootstrapcdn.com/bootstrap/3.3.6/css/bootstrap.min.css\">"; // 3.3.6
  buf = buf + "<script src=\"https://maxcdn.bootstrapcdn.com/bootstrap/3.3.6/js/bootstrap.min.js\"></script>";
  buf = buf + "<script src=\"https://cdnjs.cloudflare.com/ajax/libs/jscolor/2.0.4/jscolor.min.js\"></script>";
  buf = buf + "</head><body><div class=\"container\"><div class=\"row\"><h1>ESP Color Picker</h1>";
  buf = buf + "<div class=\"input-group\">";
  buf = buf + "<a type=\"submit\" id=\"change_color\" type=\"button\" class=\"btn btn-primary input-group-addon\">Change Color</a>";
  buf = buf + "<input class=\"form-control jscolor {onFineChange:'update(this)'} form-control\" id=\"rgb\" value=\"" + led_rgb + "\"></div></div>";
  buf = buf + "<script>function update(picker) {document.getElementById('rgb').innerHTML = Math.round(picker.rgb[0]) + ', ' +  Math.round(picker.rgb[1]) + ', ' + Math.round(picker.rgb[2]);";
  buf = buf + "document.getElementById(\"change_color\").href=document.getElementById('rgb').value;}</script>";
  buf = buf + "<div class=\"row\">HSI: H " + led_hsi[0] + " S " + led_hsi[1] + " I " + led_hsi[2] + "</div>";
  buf = buf + "<div class=\"row\">&nbsp;</div>";
  buf = buf + "<div class=\"row\">Fade: " + (anim_fade? "<b>On</b> | <a href=\"../fade/0\">Off</a>" : "<a href=\"../fade/1\">On</a> | <b>Off</b>") + "</div>";
  buf = buf + "<div class=\"row\">RGB Wave: " + (anim_RGBwave? "<b>On</b> | <a href=\"../rgbwave/0\">Off</a>" : "<a href=\"../rgbwave/1\">On</a> | <b>Off</b>") + "</div>";
  buf = buf + "<div class=\"row\">Speed: ";
  for (int i = 0; i<=10; i++) {
    if (i==anim_speed){
      buf = buf + "<b>" + i + "</b> ";
    } else {
      buf = buf + "<a href=\"../speed/" + i + "\">" + i + "</a> ";
    }
  }
  buf = buf + "</div>";
  buf = buf + "<div class=\"row\">Direction: ";
  for (int i = 1; i<=4; i++) {
    if (i==anim_direction){
      buf = buf + "<b>" + i + "</b> ";
    } else {
      buf = buf + "<a href=\"../dir/" + i + "\">" + i + "</a> ";
    }
  }
  buf = buf + "</div>";
  buf = buf + "<div class=\"row\">";
  buf = buf +   "Anim_Frame_Count: " + anim_frame_count;
  buf = buf + "</div>";
  buf = buf + "</div></body></html>";
  
  server.send(200, "text/HTML", buf);
}


// --- Handle root directory -------------------
void handleRoot() {
  
  // Force a redirect to /LED
  server.sendHeader("Location","/LED/");    // Add a header to respond with a new location for the browser to go to the home page again
  server.send(303);                         // Send it back to the browser with an HTTP status 303 (See Other) to redirect
  
  // Optionally can send HTTP response
  //server.send(200, "text/plain", "hello from esp8266!");
}


// --- Handle where path was not configured ----
// Here catch if it's a case where RGB code is specified
void handleNotFound() {
  
  // Check if it's a request for LED
  String my_url;
  my_url = server.uri();
  my_url.toUpperCase();
  
  if (my_url.startsWith("/LED")) {
    handleLED();
    
  } else if (my_url.startsWith("/FADE")) {
    handleFade();

  } else if (my_url.startsWith("/RGBWAVE")) {
    handleRGBWave();

  } else if (my_url.startsWith("/SPEED")) {
    handleSpeed();

  } else if (my_url.startsWith("/DIR")) {
    handleDirection();
    
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


// --- Handle LED RGB input --------------------
// Here catch if it's a case where RGB code is specified
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

    // If different then set colour
    if (led_rgb != led_param) setColor(led_param);
  
    handleHomePage();
  } else {
    server.send(200, "text/HTML", message);  
  }
}


void setColor(String rgb) {
  int red_value   = hex_2_int(rgb.charAt(0))*16 + hex_2_int(rgb.charAt(1));
  int green_value = hex_2_int(rgb.charAt(2))*16 + hex_2_int(rgb.charAt(3));
  int blue_value  = hex_2_int(rgb.charAt(4))*16 + hex_2_int(rgb.charAt(5));
  
  led_colors[0] = red_value;
  led_colors[1] = green_value;
  led_colors[2] = blue_value;

  rgb2hsi(red_value, green_value, blue_value, led_hsi);

  for(uint16_t i=0; i < NUM_PIXELS; i++) {
    strip.setPixelColor(i, strip.Color(red_value, green_value, blue_value));
    //delay(50);
    strip.show();
  }

  led_rgb = rgb;
}


void handleSpeed() {

  String my_url;
  my_url = server.uri();
  my_url.toUpperCase();  
  // url will look like "/SPEED/xxxxxx"
  anim_speed = my_url.substring(7).toInt();
  
  if (anim_speed < 0) anim_speed = 0;
  if (anim_speed > 10) anim_speed = 10;

  // Cause a redirect back to /LED/
  handleRoot(); 
  
}

void handleDirection() {

  String my_url;
  my_url = server.uri();
  my_url.toUpperCase();  
  // url will look like "/DIR/xxxxxx"
  anim_direction = my_url.substring(5).toInt();
  
  if (anim_direction < 1) anim_direction = 1;
  if (anim_direction > 4) anim_direction = 4;

  // Cause a redirect back to /LED/
  handleRoot(); 
  
}



// Fade in and out 
void handleFade() {

  String my_url;
  my_url = server.uri();
  my_url.toUpperCase();  
  // url will look like "/fade/xxxxxx"
  String my_string = my_url.substring(6);

  if (my_string.length() == 0) {
    anim_fade = !anim_fade;
  } else {
    anim_fade = my_string.toInt();
  }
  if (anim_fade) {
    anim_frame_count = 0;
  } else {
    setColor(led_rgb);
  }
  
  // Cause a redirect back to /LED/
  handleRoot(); 
}
void FadeFrame() {
  if (anim_speed==0) return;
  
  int red_value   = led_colors[0];
  int green_value = led_colors[1];
  int blue_value  = led_colors[2];

  int breakpoint = 2 * 1000 / anim_frame_ms / anim_speed;
  int anim_frame = anim_frame_count % breakpoint;

  if ((anim_frame_count / breakpoint) % 2 == 1) {
    // increasing brightness
    red_value   = (red_value * anim_frame) / breakpoint;
    green_value = (green_value * anim_frame) / breakpoint;
    blue_value  = (blue_value * anim_frame) / breakpoint;
  } else {
    // decreasing brightness
    red_value   = red_value - (red_value * anim_frame) / breakpoint;
    green_value = green_value - (green_value * anim_frame) / breakpoint;
    blue_value  = blue_value - (blue_value * anim_frame) / breakpoint;
  }
  
  Serial.print(red_value);
  Serial.print(" "); 
  Serial.print(green_value); 
  Serial.print(" ");
  Serial.println(blue_value);
  
  for(uint16_t i=0; i< strip.numPixels(); i++) {
    strip.setPixelColor(i, strip.Color(red_value, green_value, blue_value));
    strip.show();
  }
}

// Fade wave across the board
void handleWave() {

  
}
void WaveFrame() {
  
}

// RGB wave across the board
void handleRGBWave() {

  String my_url;
  my_url = server.uri();
  my_url.toUpperCase();  
  // url will look like "/rgbwave/xxxxxx"
  String my_string = my_url.substring(9);

  if (my_string.length() == 0) {
    anim_RGBwave = !anim_RGBwave;
  } else {
    anim_RGBwave = my_string.toInt();
  }
  if (anim_RGBwave) {
    anim_frame_count = 0;
  } else {
    setColor(led_rgb);
  }
  
  // Cause a redirect back to /LED/
  handleRoot(); 
  
}
void RGBWaveFrame() {
  if (anim_speed==0) return;

  // To store temporary RGB results
  int led_temp[3];

  uint16_t hue_increment = 360 / NUM_PIXELS;

  int breakpoint = 2 * 1000 / anim_frame_ms / anim_speed;
  int anim_frame = anim_frame_count % breakpoint;
  

  for(uint16_t i=0; i < NUM_PIXELS; i++) {
    // Add hue increment to each LED

    
    hsi2rgb(led_hsi[0] + (anim_direction%2 == 1?-1:1) * i*hue_increment                    // 1,2 directions -1, 3,4 directions 1
                       + (anim_direction <= 2?-1:1) * (360.0 * anim_frame/breakpoint), // odd directions -1, even directions 1
            led_hsi[1], 
            led_hsi[2], 
            led_temp);
    
    strip.setPixelColor(i, strip.Color(led_temp[0], led_temp[1], led_temp[2]));
    //delay(50);
    strip.show();
  }

  
}


void setup(void) {
  // Set Neopixel pin as output
  pinMode(PIN_NEOPIXEL, OUTPUT);
  
  setColor("110000"); //red
  Serial.begin(115200);

  delay(1000); // Prevent drawing too much current with LED and crash on boot
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

  // Check if current millis is more than 1 frame from last
  if (millis() > anim_last_frame + anim_frame_ms) {
    // Do a frame update
    if (anim_fade) {
      FadeFrame();
    }
    if (anim_wave) {
      WaveFrame();
    }
    if (anim_RGBwave) {
      RGBWaveFrame();
    }

    anim_last_frame = millis();
    if (anim_speed > 0) anim_frame_count++;
  }
}



// From: http://blog.saikoled.com/post/43693602826/why-every-led-light-should-be-using-hsi
// Code also  from: http://fourier.eng.hmc.edu/e161/lectures/ColorProcessing/node3.html
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
  float r, g, b;
  while(H < 0) {
    H += 360; // Add to positive if H < 0
  }
  H = fmod(H,360); // cycle H around to 0-360 degrees
  H = 3.14159*H/(float)180; // Convert to radians.
  S = S>0?(S<1?S:1):0; // clamp S and I to interval [0,1]
  I = I>0?(I<1?I:1):0;

  if (S==0)
    rgb[0]=rgb[1]=rgb[2]=I*255;
  else {
    // Math! Thanks in part to Kyle Miller.
    if(H < 2.09439) { // H < 2/3 PI
      b = (1-S)/3;
      r = (1+S*cos(H)/cos(1.047196667 - H))/3;
      g = 1-r-b;
    } else if(H < 4.188787) { // H < 4/3 PI
      H = H - 2.09439;
      r = (1-S)/3;
      g = (1+S*cos(H)/cos(1.047196667 - H))/3;
      b = 1-r-g;
    } else {
      H = H - 4.188787;
      g = (1-S)/3;
      b = (1+S*cos(H)/cos(1.047196667 - H))/3;
      r = 1-g-b;
    }
    if (r < 0) r = 0; r = 3*I*r*255; if (r > 255) r = 255;
    if (g < 0) g = 0; g = 3*I*g*255; if (g > 255) g = 255;
    if (b < 0) b = 0; b = 3*I*b*255; if (b > 255) b = 255;
    
    rgb[0]=r;
    rgb[1]=g;
    rgb[2]=b;
  }
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
