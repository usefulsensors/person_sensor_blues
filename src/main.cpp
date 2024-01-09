#include <Arduino.h>

#include <Notecard.h>

#include "person_sensor.h"

#define usbSerial Serial

#define productUID "com.usefulsensors.pete:person_sensor"

#define TRANSMIT_DELAY_MS (15 * 1000)
#define SAMPLE_DELAY_MS (500)

Notecard notecard;

// the setup function runs once when you press reset or power the board
void setup()
{
  delay(2500);
  usbSerial.begin(115200);
  
  notecard.begin();
  notecard.setDebugOutputStream(usbSerial);

  J *req = notecard.newRequest("hub.set");
  JAddStringToObject(req, "product", productUID);
  JAddStringToObject(req, "mode", "continuous");
  notecard.sendRequest(req);

  // You need to make sure you call Wire.begin() in setup, or the I2C access
  // below will fail.
  Wire.begin();
}

// the loop function runs over and over again forever
void loop()
{
  // Sample twice a second, but only transmit the averaged results
  // every 15 seconds.
  static int faces_total = 0;
  static int facing_faces_total = 0;
  static int sensor_samples_total = 0;
  static int32_t time_since_transmit = 0;

  person_sensor_results_t results = {};
  if (!person_sensor_read(&results)) {
    Serial.println("No person sensor results found on the i2c bus");
    delay(SAMPLE_DELAY_MS);
    return;
  }

  faces_total += results.num_faces;
  for (int i = 0; i < results.num_faces; ++i) {
    const person_sensor_face_t* face = &results.faces[i];
    if (face->is_facing) {
      facing_faces_total += 1;
    }
  }
  sensor_samples_total += 1;

  time_since_transmit += SAMPLE_DELAY_MS;
  if (time_since_transmit >= TRANSMIT_DELAY_MS) {
    J *req = notecard.newRequest("note.add");
    if (req != NULL)
    {
      JAddStringToObject(req, "file", "sensors.qo");
      JAddBoolToObject(req, "sync", true);
      J *body = JAddObjectToObject(req, "body");
      if (body)
      {
        const float num_faces = (float)(faces_total) / sensor_samples_total;
        const float num_facing_faces = (float)(facing_faces_total) / sensor_samples_total;
        JAddNumberToObject(body, "num_faces", num_faces);
        JAddNumberToObject(body, "num_facing_faces", num_facing_faces);
      }
      notecard.sendRequest(req);
    }
    faces_total = 0;
    facing_faces_total = 0;
    sensor_samples_total = 0;
    time_since_transmit = 0;
 }

  delay(SAMPLE_DELAY_MS);
}