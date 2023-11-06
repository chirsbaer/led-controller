#include <Arduino.h>
#include <Adafruit_NeoPixel.h>

// Define the GPIO pins for the MOSFETs controlling the LED strips
const int WHITE_LED_PIN = 25;  // White LED strip pin (Used for both white and yellow)
const int RED_LED_PIN = 26;    // RGB - Red pin (Used for both red and orange)
const int GREEN_LED_PIN = 27;  // RGB - Green pin
const int BLUE_LED_PIN = 14;   // RGB - Blue pin

// Define the pin where the NeoPixel stick is connected
#define NEOPIXEL_PIN    15
// Define the number of pixels in the NeoPixel stick
#define NUMPIXELS       8
// Create the NeoPixel object
Adafruit_NeoPixel pixels(NUMPIXELS, NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800);


// Define the LED Channel for the ESP32 (each color should have its own channel)
const int CHANNEL_WHITE = 0;
const int CHANNEL_RED = 1;
const int CHANNEL_GREEN = 2;
const int CHANNEL_BLUE = 3;

// Define the PWM frequency and resolution
const int PWM_FREQ = 5000;      // Frequency in Hz
const int PWM_RESOLUTION = 16;  // 16-bit resolution

// Define the GPIO pins for the buttons
const int BTN_MORE_LIGHT = 32;
const int BTN_LESS_LIGHT = 33;
const int BTN_ON_OFF = 34;
const int BTN_TOGGLE_COLOR = 35;

// Define the color profiles for yellow/orange and blue in terms of PWM duty cycle (0 - 65535)
const uint16_t COLOR_YELLOW_ORANGE[3] = { 65535, 32768, 0 };  // Example values: Full red, half green, no blue
const uint16_t COLOR_BLUE[3] = { 0, 0, 65535 };               // Example values: No red, no green, full blue
// Define the color for the white mode on the RGB strip
const uint16_t COLOR_WHITE[3] = {65535, 65535, 65535};

// Dimmer levels array (8 levels)
const uint16_t DIMMER_LEVELS[8] = { 0, 8191, 16383, 24575, 32768, 40960, 49152, 65535 };

// Variables to keep track of the state
bool ledOn = false;   // LED on/off state
int dimmerIndex = 4;  // Initial dimmer level index
int colorMode = 0;    // Color mode (0: Yellow/Orange, 1: Blue)

// Debounce variables
unsigned long lastDebounceTime = 0;  // the last time the output pin was toggled
unsigned long debounceDelay = 50;    // the debounce time; increase if the output flickers

// Function prototypes
void setLEDs();
void updateBrightness(bool increase);
void togglePower();
void changeColor();

void setup() {
  // Set the LED pins as outputs
  pinMode(WHITE_LED_PIN, OUTPUT);
  pinMode(RED_LED_PIN, OUTPUT);
  pinMode(GREEN_LED_PIN, OUTPUT);
  pinMode(BLUE_LED_PIN, OUTPUT);

  // Initialize LED channels
  ledcSetup(CHANNEL_WHITE, PWM_FREQ, PWM_RESOLUTION);
  ledcSetup(CHANNEL_RED, PWM_FREQ, PWM_RESOLUTION);
  ledcSetup(CHANNEL_GREEN, PWM_FREQ, PWM_RESOLUTION);
  ledcSetup(CHANNEL_BLUE, PWM_FREQ, PWM_RESOLUTION);

  // Attach the channel to the GPIO to be controlled
  ledcAttachPin(WHITE_LED_PIN, CHANNEL_WHITE);
  ledcAttachPin(RED_LED_PIN, CHANNEL_RED);
  ledcAttachPin(GREEN_LED_PIN, CHANNEL_GREEN);
  ledcAttachPin(BLUE_LED_PIN, CHANNEL_BLUE);

  // Set the button pins as inputs with internal pull-up resistors
  pinMode(BTN_MORE_LIGHT, INPUT_PULLUP);
  pinMode(BTN_LESS_LIGHT, INPUT_PULLUP);
  pinMode(BTN_ON_OFF, INPUT_PULLUP);
  pinMode(BTN_TOGGLE_COLOR, INPUT_PULLUP);

  // Initialize the NeoPixel stick
  pixels.begin();
  pixels.show(); // Initialize all pixels to 'off'

  // Initialize all LEDs to off
  setLEDs();
}

void loop() {
  // Read the state of the buttons
  int moreLightState = digitalRead(BTN_MORE_LIGHT);
  int lessLightState = digitalRead(BTN_LESS_LIGHT);
  int onOffState = digitalRead(BTN_ON_OFF);
  int toggleColorState = digitalRead(BTN_TOGGLE_COLOR);

  // Check if any button was pressed
  if ((millis() - lastDebounceTime) > debounceDelay) {
    if (moreLightState == LOW) {
      updateBrightness(true);  // Increase brightness
      lastDebounceTime = millis();
    } else if (lessLightState == LOW) {
      updateBrightness(false);  // Decrease brightness
      lastDebounceTime = millis();
    } else if (onOffState == LOW) {
      togglePower();  // Toggle the power state
      lastDebounceTime = millis();
    } else if (toggleColorState == LOW) {
      changeColor();  // Change color mode
      lastDebounceTime = millis();
    }
  }

  // Other code...
}

void setLEDs() {
  if (ledOn) {
    uint16_t white, red, green, blue;

    if (colorMode == 0) {  // Yellow/orange
      white = COLOR_YELLOW_ORANGE[0] * DIMMER_LEVELS[dimmerIndex] / 65535;
      red = COLOR_YELLOW_ORANGE[1] * DIMMER_LEVELS[dimmerIndex] / 65535;
      green = COLOR_YELLOW_ORANGE[2] * DIMMER_LEVELS[dimmerIndex] / 65535;
      blue = 0;   // Not used for yellow/orange
    } else {      // Blue
      white = 0;  // Not used for blue
      red = 0;    // Not used for blue
      green = 0;  // Not used for blue
      blue = COLOR_BLUE[2] * DIMMER_LEVELS[dimmerIndex] / 65535;
    }

    // Set the brightness for each color channel
    ledcWrite(CHANNEL_WHITE, white);
    ledcWrite(CHANNEL_RED, red);
    ledcWrite(CHANNEL_GREEN, green);
    ledcWrite(CHANNEL_BLUE, blue);
  } else {
    // Turn off all LEDs
    ledcWrite(CHANNEL_WHITE, 0);
    ledcWrite(CHANNEL_RED, 0);
    ledcWrite(CHANNEL_GREEN, 0);
    ledcWrite(CHANNEL_BLUE, 0);
  }
}

void updateBrightness(bool increase) {
  // Increase or decrease dimmer index within bounds
  if (increase && dimmerIndex < 7) {
    dimmerIndex++;
  } else if (!increase && dimmerIndex > 0) {
    dimmerIndex--;
  }
  setLEDs();  // Update the LEDs
}

void togglePower() {
  ledOn = !ledOn;  // Toggle the power state
  setLEDs();       // Update the LEDs
}


void smoothDimming(uint16_t targetLevel) {
  uint16_t currentLevel = DIMMER_LEVELS[dimmerIndex]; // Get the current brightness level
  int stepDelay = 2000 / abs(int(currentLevel) - int(targetLevel)); // Calculate the delay per step for a 2 second transition

  while (currentLevel != targetLevel) {
    if (currentLevel < targetLevel) {
      currentLevel++;
    } else if (currentLevel > targetLevel) {
      currentLevel--;
    }
    // Apply the new brightness level
    ledcWrite(CHANNEL_WHITE, (colorMode == 2) ? currentLevel : 0); // Assuming white mode is represented by 2
    ledcWrite(CHANNEL_RED, (colorMode == 0) ? currentLevel : 0);
    ledcWrite(CHANNEL_GREEN, (colorMode == 0) ? currentLevel / 2 : 0); // Assuming yellow has half green
    ledcWrite(CHANNEL_BLUE, (colorMode == 1) ? currentLevel : 0);
    // Reflect this change on the RGB LED stick as well
    updateRGBStick(currentLevel);
    delay(stepDelay);
  }
}

void updateBrightness(bool increase) {
  // Increase or decrease dimmer index within bounds
  if (increase && dimmerIndex < 7) {
    dimmerIndex++;
  } else if (!increase && dimmerIndex > 0) {
    dimmerIndex--;
  }
  smoothDimming(DIMMER_LEVELS[dimmerIndex]); // Use smooth dimming to reach the new brightness level
}

void changeColor() {
  colorMode = (colorMode + 1) % 3; // Now we have three color modes (0: Yellow/Orange, 1: Blue, 2: White)
  smoothDimming(DIMMER_LEVELS[dimmerIndex]); // Update the LEDs with smooth transition
}

// Function to update the RGB LED stick to reflect the current dimming level and color
void updateRGBStick(uint16_t brightness) {
  uint32_t color;

  // Choose color based on the current color mode
  if (colorMode == 0) { // Yellow/orange color
    color = pixels.Color(brightness >> 8, brightness >> 9, 0); // Scaling down the 16-bit brightness to 8-bit
  } else if (colorMode == 1) { // Blue color
    color = pixels.Color(0, 0, brightness >> 8);
  } else { // White color (assuming colorMode == 2)
    color = pixels.Color(brightness >> 8, brightness >> 8, brightness >> 8);
  }

  // Set the color for each pixel in the stick
  for(int i = 0; i < NUMPIXELS; i++) {
    pixels.setPixelColor(i, color);
  }

  // Apply the changes to the stick
  pixels.show();
}