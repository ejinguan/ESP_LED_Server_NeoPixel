// H: 0-360 deg,  S: 0-1,  I: 0-1
// R,G,B: 0-255
void hsi2rgb(float H, float S, float I, int* rgb) ;
void rgb2hsi(float R, float G, float B, float* hsi);

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
