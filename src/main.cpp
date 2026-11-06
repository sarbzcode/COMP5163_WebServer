/*
 * COMP 5163 - Lab: Web Server 1
 *
 * Statement of Authorship:
 * I, Sarbjot Singh (0307838), wrote this program myself.
 *
 * Description:
 * The WeMos D1 Mini acts as an HTTP web server. When a browser connects, it
 * receives a well-formed HTML page showing the DS18B20 temperature, the
 * device's IP address and the time the device has been running (HH:MM:SS).
 */

#include <Arduino.h>
#include <ESP8266WiFi.h>

// 1-Wire sensor communication library
#include <OneWire.h>
// DS18B20 sensor library
#include <DallasTemperature.h>

// access credentials for WiFi network.
const char *ssid = "***************";
const char *password = "***********";


// WiFi server. Listen on port 80, which is the default TCP port for HTTP
WiFiServer server(80);

// Pin that the DS18B20 is connected to
const int oneWireBus = D3;

// Setup a oneWire instance to communicate with any OneWire devices
OneWire oneWire(oneWireBus);

// Pass our oneWire reference to Dallas Temperature sensor
DallasTemperature DS18B20(&oneWire);

// Unique 8-byte address of the sensor
DeviceAddress sensorAddress;

// Convert a temperature in Celsius to a judgement string
String judgeTemperature(float fTemp)
{
  if (fTemp < 10.0)
    return "Cold!";
  else if (fTemp < 15.0)
    return "Cool";
  else if (fTemp < 25.0)
    return "Perfect";
  else if (fTemp < 30.0)
    return "Warm";
  else if (fTemp < 35.0)
    return "Hot";
  else
    return "Too Hot!";
}

// Convert a temperature in Celsius to a page background color (CSS hex).
// The ranges match judgeTemperature() so the color and the text agree.
String getBackgroundColor(float fTemp)
{
  if (fTemp == DEVICE_DISCONNECTED_C)
    return "#cccccc"; // grey: no valid reading
  else if (fTemp < 10.0)
    return "#5dade2"; // blue: cold
  else if (fTemp < 15.0)
    return "#aed6f1"; // light blue: cool
  else if (fTemp < 25.0)
    return "#abebc6"; // green: perfect
  else if (fTemp < 30.0)
    return "#f9e79f"; // yellow: warm
  else if (fTemp < 35.0)
    return "#f5b041"; // orange: hot
  else
    return "#ec7063"; // red: too hot
}

// Convert the time since boot (millis) into an HH:MM:SS string
String getUptime()
{
  unsigned long totalSeconds = millis() / 1000;
  unsigned long hours = totalSeconds / 3600;
  unsigned long minutes = (totalSeconds % 3600) / 60;
  unsigned long seconds = totalSeconds % 60;

  char buffer[16];
  snprintf(buffer, sizeof(buffer), "%02lu:%02lu:%02lu", hours, minutes, seconds);
  return String(buffer);
}

// Build the complete HTTP response (headers + well-formed HTML page)
String buildResponse(float fTemp)
{
  String temperatureText;
  if (fTemp == DEVICE_DISCONNECTED_C)
  {
    // sensor was unplugged after startup
    temperatureText = "Sensor disconnected!";
  }
  else
  {
    temperatureText = String(fTemp, 1) + " &deg;C (" + judgeTemperature(fTemp) + ")";
  }

  // HTTP headers: status line, content type, close the connection, blank line
  String html = "HTTP/1.1 200 OK\r\n";
  html += "Content-Type: text/html\r\n";
  html += "Connection: close\r\n";
  html += "\r\n";

  // HTML body
  html += "<!DOCTYPE html>\n";
  html += "<html lang=\"en\">\n";
  html += "<head>\n";
  html += "<meta charset=\"UTF-8\">\n";
  html += "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">\n";
  // refresh the page automatically every 5 seconds
  html += "<meta http-equiv=\"refresh\" content=\"5\">\n";
  html += "<title>D1 Mini Web Server</title>\n";
  html += "<style>\n";
  // the whole page background changes color with the temperature
  html += "body { font-family: Arial, sans-serif; background: " + getBackgroundColor(fTemp) + "; text-align: center; }\n";
  html += ".card { background: white; max-width: 420px; margin: 30px auto; padding: 20px;";
  html += " border-radius: 12px; box-shadow: 0 2px 8px rgba(0,0,0,0.2); }\n";
  html += "h1 { color: #004080; }\n";
  html += ".temp { font-size: 2em; color: #c0392b; font-weight: bold; }\n";
  html += "td { padding: 6px 10px; text-align: left; }\n";
  html += "</style>\n";
  html += "</head>\n";
  html += "<body>\n";
  html += "<div class=\"card\">\n";
  html += "<h1>WeMos D1 Mini Web Server</h1>\n";
  html += "<p class=\"temp\">" + temperatureText + "</p>\n";
  html += "<table>\n";
  html += "<tr><td><b>Temperature:</b></td><td>" + temperatureText + "</td></tr>\n";
  html += "<tr><td><b>IP Address:</b></td><td>" + WiFi.localIP().toString() + "</td></tr>\n";
  html += "<tr><td><b>Uptime (HH:MM:SS):</b></td><td>" + getUptime() + "</td></tr>\n";
  html += "</table>\n";
  html += "<hr>\n";
  html += "<p>Created by <b>Sarbjot Singh(0307838)</b></p>\n";
  html += "</div>\n";
  html += "</body>\n";
  html += "</html>\n";

  return html;
}

void setup()
{
  // configure the USB serial monitor (only needs to be done once)
  Serial.begin(115200);
  Serial.println("\nWeb Server Demo - Temperature Application");

  // Start the DS18B20 sensor
  DS18B20.begin();

  // Check that at least one sensor is on the bus and that we can read its address
  if (DS18B20.getDeviceCount() == 0 || !DS18B20.getAddress(sensorAddress, 0))
  {
    Serial.println("No DS18B20 temperature sensors are installed!");
    // terminate gracefully: nothing more to do, so halt here
    while (true)
    {
      delay(1000);
    }
  }

  // Print the sensor's unique address
  Serial.print("Found DS18B20 sensor with address: ");
  for (uint8_t i = 0; i < 8; i++)
  {
    if (sensorAddress[i] < 16)
      Serial.print("0");
    Serial.print(sensorAddress[i], HEX);
  }
  Serial.println();

  // Connect to the WiFi network
  Serial.printf("\nConnecting to %s ", ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println(" connected");

  // Start the web server
  server.begin();
  Serial.printf("Web server started, open %s in a web browser\n",
                WiFi.localIP().toString().c_str());
}

void loop()
{
  // wait for a client (web browser) to connect
  WiFiClient client = server.available();

  if (client)
  {
    Serial.println("\n>> Client connected");
    Serial.println(">> Client request:");

    while (client.connected())
    {
      // read line by line what the client (web browser) is requesting
      if (client.available())
      {
        // print information about the request (browser, OS, etc)
        String line = client.readStringUntil('\r');
        Serial.print(line);

        // a blank line marks the end of the client's request headers
        if (line.length() == 1 && line[0] == '\n')
        {
          // read a fresh temperature only when a client asks for it
          // (we only have 1 sensor, so the index is 0)
          DS18B20.requestTemperatures();
          float fTemp = DS18B20.getTempCByIndex(0);

          // log the reading to the USB serial monitor
          if (fTemp == DEVICE_DISCONNECTED_C)
          {
            Serial.println("Error: DS18B20 sensor disconnected!");
          }
          else
          {
            Serial.println("Current temperature is: " + String(fTemp) + "\xC2\xB0 C or " + judgeTemperature(fTemp));
          }

          // send the HTTP headers and HTML page back to the browser
          client.print(buildResponse(fTemp));
          Serial.println(">> Response sent to client");
          break;
        }
      }
    }

    // read and discard any remaining request data, then terminate
    while (client.available())
    {
      client.read();
    }

    // close the connection:
    client.stop();
    Serial.println(">> Client disconnected");
  }
}
