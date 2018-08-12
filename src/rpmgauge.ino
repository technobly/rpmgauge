/*
 * Project RPM gauge
 * Description: A Car Heads-up RPM Gauge built with Carloop, Particle Photon and Neopixel sticks.
 * Author: Brett Walach <technobly@gmail.com>
 * Date: August 11th 2018
 */

#include "Particle.h"
#include <carloop.h>
#include <neopixel.h>
#include <math.h>

// Connect your 32 pixel Neopixel stick(s) to VIN, GND and TX on the Carloop (be sure to use CARLOOP_CAN mode to disable GPS support)
#define PIXEL_PIN   (TX)
#define PIXEL_COUNT (32)
#define PIXEL_SHIFT (0) // was originally driving the display from a Particle Internet button, thus needed to set this to 11
#define PIXEL_TYPE  (WS2812B)

Adafruit_NeoPixel gauge(PIXEL_COUNT+PIXEL_SHIFT, PIXEL_PIN, PIXEL_TYPE);

// Don't block the main program while connecting to WiFi/cellular.
// This way the main program runs on the Carloop even outside of WiFi range.
SYSTEM_THREAD(ENABLED);

// Tell the program which revision of Carloop you are using.
Carloop<CarloopRevision2> carloop;

unsigned int CAN_ID = 0x7e0;
unsigned int CAN_MSG[8] = { 0x02, 0x01, 0x0c, 0, 0, 0, 0, 0 };
int rpm = 0;
int last_rpm = 0;
volatile bool updating_can = false;

// Go ahead and play with these values...
#define DECAY_VALUE           (5.0f)
#define MARKER_BRIGHTNESS     (0.2)
#define BACKGROUND_BRIGHTNESS (0.01)
#define NEEDLE_BRIGHTNESS     (0.2)
#define MAX_RPM               (7000)
#define SHIFT_POINT_RPM       (6000)

int scalePixel(int rpm) {
    // was playing around with exponential decay values, it's not quite what I wanted though, linear is fine for now...
    // return (double(PIXEL_COUNT) * (1.0f - exp(-(rpm * PIXEL_COUNT / MAX_RPM)/DECAY_VALUE))) + double(PIXEL_SHIFT);
    return (rpm * PIXEL_COUNT / MAX_RPM) + PIXEL_SHIFT - 1;
}

void updateGauge(int rpm) {
    int x;
    int y = scalePixel(rpm);
    static float needle_brightness = NEEDLE_BRIGHTNESS;
    static int needle_color = 0;
    // Serial.printlnf("rpm: %6d y: %3d", rpm, y);

    needle_brightness = (rpm >= SHIFT_POINT_RPM) ? 1.0:NEEDLE_BRIGHTNESS;
    needle_color = (rpm >= SHIFT_POINT_RPM) ? 1:0;

    // clear all
    for(x=0; x<PIXEL_COUNT+PIXEL_SHIFT; x++) {
        gauge.setPixelColor(x, 0);
    }
    // paint the background
    for(x=0+PIXEL_SHIFT; x<=14+PIXEL_SHIFT; x++) {
        gauge.setPixelColor(x, gauge.Color(0, 255*BACKGROUND_BRIGHTNESS, 0) );
    }
    for(; x<=25+PIXEL_SHIFT; x++) {
        gauge.setPixelColor(x, gauge.Color(255*BACKGROUND_BRIGHTNESS/2, 100*BACKGROUND_BRIGHTNESS/2, 0) );
    }
    for(; x<=31+PIXEL_SHIFT; x++) {
        gauge.setPixelColor(x, gauge.Color(255*BACKGROUND_BRIGHTNESS/2, 0, 0) );
    }
    // enable markers
    gauge.setPixelColor(scalePixel(1000), gauge.Color(0, 255*MARKER_BRIGHTNESS, 0) );
    gauge.setPixelColor(scalePixel(2000), gauge.Color(0, 255*MARKER_BRIGHTNESS, 0) );
    gauge.setPixelColor(scalePixel(3000), gauge.Color(0, 255*MARKER_BRIGHTNESS, 0) );
    gauge.setPixelColor(scalePixel(4000), gauge.Color(255*MARKER_BRIGHTNESS, 100*MARKER_BRIGHTNESS, 0) );
    gauge.setPixelColor(scalePixel(5000), gauge.Color(255*MARKER_BRIGHTNESS, 100*MARKER_BRIGHTNESS, 0) );
    gauge.setPixelColor(scalePixel(6000), gauge.Color(255*MARKER_BRIGHTNESS, 0, 0) );
    gauge.setPixelColor(scalePixel(7000), gauge.Color(255*MARKER_BRIGHTNESS, 0, 0) );
    for(x=0; x<=y; x++) {
        if (x == y) {
            // paint the needle
            if (needle_color == 0) {
                gauge.setPixelColor(x-1, gauge.Color(0, 0, 255*needle_brightness));
                gauge.setPixelColor(x, gauge.Color(0, 255*needle_brightness, 255*needle_brightness));
                gauge.setPixelColor(x+1, gauge.Color(0, 0, 255*needle_brightness));
            } else {
                gauge.setPixelColor(x-1, gauge.Color(255*needle_brightness, 0, 255*needle_brightness));
                gauge.setPixelColor(x, gauge.Color(255*needle_brightness, 0, 255*needle_brightness));
                gauge.setPixelColor(x+1, gauge.Color(255*needle_brightness, 0, 255*needle_brightness));
            }
            x++;
        }
    }
    gauge.show();
}

// Send a message at a regular time interval
void sendMessage() {
    if (updating_can) {
        return;
    }

    static uint32_t lastTransmitTime = 0;
    uint32_t transmitInterval = 200; /* ms */
    uint32_t now = millis();
    if (now - lastTransmitTime > transmitInterval) {
        digitalWrite(D7, !digitalRead(D7));
        CANMessage message;

        // A CAN message has an ID that identifies the content inside
        message.id = CAN_ID;

        // It can have from 0 to 8 data bytes
        message.len = 8;

        // Pass the data to be transmitted in the data array
        message.data[0] = CAN_MSG[0];
        message.data[1] = CAN_MSG[1];
        message.data[2] = CAN_MSG[2];
        message.data[3] = CAN_MSG[3];
        message.data[4] = CAN_MSG[4];
        message.data[5] = CAN_MSG[5];
        message.data[6] = CAN_MSG[6];
        message.data[7] = CAN_MSG[7];

        // Send the message on the bus!
        carloop.can().transmit(message);

        lastTransmitTime = now;

        // Serial.printlnf("sent for message id: %03x", MSG_ID);
    }
}

void recvMessage() {
    if (updating_can) {
        return;
    }

    uint32_t startRecv = millis();
    const uint32_t RECV_TIMEOUT = 50UL;
    CANMessage message;
    while (millis() - startRecv <= RECV_TIMEOUT) {
        if (carloop.can().receive(message)) {
            if (message.id == 0x7e8) {
                // String data;
                // data.reserve(256);
                // data = String::format("time:%f,id:0x%03x,data:", millis() / 1000.0, message.id);
                // for (int i = 0; i < message.len; i++) {
                //     if (i == 0) {
                //         data += "0x";
                //     }
                //     data += String::format("%02x", message.data[i]);
                // }

                rpm = ((message.data[3]*256)+message.data[4])/4;

                if (rpm > (last_rpm + 100) || rpm < (last_rpm - 100) ) {
                    updateGauge(rpm);
                    last_rpm = rpm;
                    // Serial.printlnf("%d %d", rpm, last_rpm);

                    // if (Particle.connected()) {
                    //     Particle.publish("rpm", String(rpm), PRIVATE);
                    // }
                }
            }
        }
    }
}

void setup() {
    pinMode(D7, OUTPUT);
    carloop.begin(CARLOOP_CAN); // canbus only so we can use TX for neopixel output

    gauge.begin();
    gauge.show(); // Initialize all pixels to 'off'
    updateGauge(0);
}

void loop() {

    // Send every 1 second
    sendMessage();

    // Receive as fast as possible
    recvMessage();

    // rpm demo visualization (comment out the CAN stuff above when enabling this for proper speed)
    // fakeRPMdemo();

/*
    // It really needs a Knight Rider mode!!
    knightRider(3, 16, 6, 0xFF1000); // Cycles, Speed, Width, RGB Color (original orange-red)
    knightRider(3, 16, 6, 0xFF00FF); // Cycles, Speed, Width, RGB Color (purple)
    knightRider(3, 16, 6, 0x0000FF); // Cycles, Speed, Width, RGB Color (blue)
    knightRider(3, 16, 6, 0xFF0000); // Cycles, Speed, Width, RGB Color (red)
    knightRider(3, 16, 6, 0x00FF00); // Cycles, Speed, Width, RGB Color (green)
    knightRider(3, 16, 6, 0xFFFF00); // Cycles, Speed, Width, RGB Color (yellow)
    knightRider(3, 16, 6, 0x00FFFF); // Cycles, Speed, Width, RGB Color (cyan)
    knightRider(3, 16, 6, 0xFFFFFF); // Cycles, Speed, Width, RGB Color (white)
    clearStrip();
    delay(2000);

    // Iterate through a whole rainbow of colors
    for(byte j=0; j<252; j+=7) {
        knightRider(1, 16, 4, colorWheel(j)); // Cycles, Speed, Width, RGB Color
    }
    clearStrip();
    delay(2000);
*/
}

void fakeRPMdemo() {
    static int count = 0;
    static int count2 = 0;
    static int dir = 1;
    updateGauge(count);
    if (dir) {
        if (count > 7000) {
            count = 7000;
            count2++;
            if (count2 > 10) {
                dir = 0;
                count2 = 0;
            }
        } else {
            count = (count + 100) + (count * 0.1);
        }
    } else {
        if (count < 0) {
            count2++;
            if (count2 > 30) {
                dir = 1;
                count2 = 0;
            }
        } else {
            count -= 130;
        }
    }
    updateGauge(count);
    delay(25);
}

// Cycles - one cycle is scanning through all pixels left then right (or right then left)
// Speed - how fast one cycle is (32 with 16 pixels is default KnightRider speed)
// Width - how wide the trail effect is on the fading out LEDs.  The original display used
//         light bulbs, so they have a persistance when turning off.  This creates a trail.
//         Effective range is 2 - 8, 4 is default for 16 pixels.  Play with this.
// Color - 32-bit packed RGB color value.  All pixels will be this color.
// knightRider(cycles, speed, width, color);
void knightRider(uint16_t cycles, uint16_t speed, uint8_t width, uint32_t color) {
  uint32_t old_val[PIXEL_COUNT+PIXEL_SHIFT]; // up to 256 lights!
  // Larson time baby!
  for(int i = 0; i < cycles; i++){
    for (int count = PIXEL_SHIFT; count<PIXEL_COUNT+PIXEL_SHIFT; count++) {
      if (count<PIXEL_SHIFT) {
        gauge.setPixelColor(count, 0);
      } else {
        gauge.setPixelColor(count, color);
      }
      old_val[count] = color;
      for(int x = count; x>0+PIXEL_SHIFT; x--) {
        old_val[x-1] = dimColor(old_val[x-1], width);
        if (x<PIXEL_SHIFT) {
          gauge.setPixelColor(count, 0);
        } else {
          gauge.setPixelColor(x-1, old_val[x-1]);
        }
      }
      gauge.show();
      delay(speed);
    }
    for (int count = PIXEL_COUNT-1+PIXEL_SHIFT; count>=0+PIXEL_SHIFT; count--) {
      if (count<PIXEL_SHIFT) {
        gauge.setPixelColor(count, 0);
      } else {
        gauge.setPixelColor(count, color);
      }
      old_val[count] = color;
      for(int x = count; x<=PIXEL_COUNT+PIXEL_SHIFT ;x++) {
        old_val[x-1] = dimColor(old_val[x-1], width);
        if (x<PIXEL_SHIFT) {
          gauge.setPixelColor(count, 0);
        } else {
          gauge.setPixelColor(x+1, old_val[x+1]);
        }
      }
      gauge.show();
      delay(speed);
    }
  }
}

void clearStrip() {
  for( int i = 0; i<PIXEL_COUNT+PIXEL_SHIFT; i++){
    gauge.setPixelColor(i, 0x000000); gauge.show();
  }
}

uint32_t dimColor(uint32_t color, uint8_t width) {
   return (((color&0xFF0000)/width)&0xFF0000) + (((color&0x00FF00)/width)&0x00FF00) + (((color&0x0000FF)/width)&0x0000FF);
}

// Using a counter and for() loop, input a value 0 to 251 to get a color value.
// The colors transition like: red - org - ylw - grn - cyn - blue - vio - mag - back to red.
// Entering 255 will give you white, if you need it.
uint32_t colorWheel(byte WheelPos) {
  byte state = WheelPos / 21;
  switch(state) {
    case 0: return gauge.Color(255, 0, 255 - ((((WheelPos % 21) + 1) * 6) + 127)); break;
    case 1: return gauge.Color(255, ((WheelPos % 21) + 1) * 6, 0); break;
    case 2: return gauge.Color(255, (((WheelPos % 21) + 1) * 6) + 127, 0); break;
    case 3: return gauge.Color(255 - (((WheelPos % 21) + 1) * 6), 255, 0); break;
    case 4: return gauge.Color(255 - (((WheelPos % 21) + 1) * 6) + 127, 255, 0); break;
    case 5: return gauge.Color(0, 255, ((WheelPos % 21) + 1) * 6); break;
    case 6: return gauge.Color(0, 255, (((WheelPos % 21) + 1) * 6) + 127); break;
    case 7: return gauge.Color(0, 255 - (((WheelPos % 21) + 1) * 6), 255); break;
    case 8: return gauge.Color(0, 255 - ((((WheelPos % 21) + 1) * 6) + 127), 255); break;
    case 9: return gauge.Color(((WheelPos % 21) + 1) * 6, 0, 255); break;
    case 10: return gauge.Color((((WheelPos % 21) + 1) * 6) + 127, 0, 255); break;
    case 11: return gauge.Color(255, 0, 255 - (((WheelPos % 21) + 1) * 6)); break;
    default: return gauge.Color(0, 0, 0); break;
  }
}