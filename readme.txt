ESP32 now works in as access point. You can connect to it by WiFi and search in browser for IP 192.168.4.22. There you can see current temperature graph that is updated every 5 seconds. What you need to do is:
1.	Add humidity graph
2.	Add co2 graph
3.	Store historic data about temperature, humidity, co2
4.	To each graph add buttons (24h, 12h, 6h, 3h, 1h) which edit graph view

File you should edit is only index.html that is in /data folder. Some instructions you can find here: https://randomnerdtutorials.com/esp32-esp8266-plot-chart-web-server/
https://randomnerdtutorials.com/esp32-plot-readings-charts-multiple/
To run the code you will need to add libraries that are in the /main folder as well others that will be neccessery by the Arduino IDE. Also you will need to use SPIFFS files. That means you have to use older version of Arduino IDE (below 2.0.0). Instruction here: https://www.instructables.com/Using-ESP8266-SPIFFS/ SPIFFS is used to store in the ESP32 website html information and libraries for the graphs.
You have to upload to ESP32 /data folder by SPIFFS and then just the code by the upload button.	
About point 1 and 2 you just need to more or less copy idea from temperature graph. Every 5 seconds ESP32 sends current sensor data as JSON string. This string is received in Event called "new_readings". I send data from server in JSON format as follows:
{
"temperature":	
	[some number],
	[some number],
[some number],
	...
"co2":
	[some number],
	[some number],
[some number],
...
"humidity":
	[some number],
	[some number],
	[some number]
	...
}
About point 3. After established connection to the server it sends JSON string with all collected sensor readings. Number of values is unknown. After connection you have to store incoming history data, then display it on the graph. It is needed to display for the user historic information. You have to calculate timedates simply by substracting for each point 5 seconds from actual time. Function that is called after connection is function getReadings().

About point 4 we need buttons to select displayed hours on the graph. You need to store incoming sensor reading coming from the ESP32 every 5 seconds and depending on the pressed button graph timeline changes.