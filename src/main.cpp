#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

#define DHTPIN 14 // Digital pin connected to the DHT sensor

// Uncomment the type of sensor in use:
// #define DHTTYPE    DHT11     // DHT 11
#define DHTTYPE DHT22 // DHT 22 (AM2302)
// #define DHTTYPE    DHT21     // DHT 21 (AM2301)

DHT dht(DHTPIN, DHTTYPE);

// Button setup
#define BUTTON_PIN 33
int lastState = HIGH;
int currentState;

// Screen state which is the resource to be shared between tasks
bool isScreenOn = true;

SemaphoreHandle_t xMutex = NULL; // Declare a mutex object

void readButton();
void btnTask(void *pvParameters);
void displayScreen();
void screenTask(void *pvParameters);

void setup()
{
  Serial.begin(115200);

  dht.begin();

  pinMode(BUTTON_PIN, INPUT_PULLUP);

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C))
  {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;)
      ;
  }
  delay(2000);
  display.clearDisplay();
  display.setTextColor(WHITE);

  xMutex = xSemaphoreCreateMutex(); // crete a mutex object

  xTaskCreatePinnedToCore(
      btnTask,   /* Task function. */
      "btnTask", /* name of task. */
      10000,     /* Stack size of task */
      NULL,      /* parameter of the task */
      2,         /* priority of the task */
      NULL,      /* Task handle to keep track of created task */
      1);        /* pin task to core 0 */

  xTaskCreatePinnedToCore(
      screenTask,   /* Task function. */
      "screenTask", /* name of task. */
      10000,        /* Stack size of task */
      NULL,         /* parameter of the task */
      1,            /* priority of the task */
      NULL,         /* Task handle to keep track of created task */
      1);           /* pin task to core 1 */
}

void loop()
{
}

void readButton()
{
  // read the state of the button
  currentState = digitalRead(BUTTON_PIN);

  if (lastState == HIGH && currentState == LOW)
  {
    Serial.println("The button is pressed");
    isScreenOn = !isScreenOn;
  }

  // save the the last state
  lastState = currentState;
}

void btnTask(void *pvParameters)
{
  while (1)
  {
    if (xSemaphoreTake(xMutex, portMAX_DELAY)) // take the mutex
    {
      readButton();
      xSemaphoreGive(xMutex);
      delay(100);
    }
  }
}

void displayScreen()
{
  if (isScreenOn)
  {
    Serial.println("Screen is on");
    // read temperature and humidity
    float t = dht.readTemperature();
    float h = dht.readHumidity();
    if (isnan(h) || isnan(t))
    {
      Serial.println("Failed to read from DHT sensor!");
    }

    // clear display
    display.clearDisplay();

    // display temperature
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.print("Temperature: ");
    display.setTextSize(2);
    display.setCursor(0, 10);
    display.print(t);
    display.print(" ");
    display.setTextSize(1);
    display.cp437(true);
    display.write(167);
    display.setTextSize(2);
    display.print("C");

    // display humidity
    display.setTextSize(1);
    display.setCursor(0, 35);
    display.print("Humidity: ");
    display.setTextSize(2);
    display.setCursor(0, 45);
    display.print(h);
    display.print(" %");

    display.display();
  }
  else
  {
    Serial.println("Screen is off");
    display.clearDisplay();
    display.display();
  }
}

void screenTask(void *pvParameters)
{
  while (1)
  {
    if (xSemaphoreTake(xMutex, (200 * portTICK_PERIOD_MS))) // try to acquire the mutex
    {
      bool screenState = isScreenOn;
      displayScreen();
      xSemaphoreGive(xMutex); // release the mutex
      for (int16_t i = 0; i < 50 && screenState == isScreenOn; i++)
      {
        delay(100);
      }
    }
  }
}