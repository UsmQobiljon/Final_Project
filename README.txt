Final project is about movement detector with photo ESP32 camera. 
ESP32 Camera stays in deep sleep mode. 
Whenever PIR motion sensor detects movement red LED light is turned on and after that it sends signal to transistor. 
After this transistor wakes up the ESP32 camera. 
ESP32 Camera turns on, flashlight lights then camera takes a picture. 
After that it saves the picture in microSD.
After that it sends the last taken picture to the Telegram bot. 
In order to see the taken picture you have to connect microSD to computer or go to the Telegram bot.  
If PIR motion sensor does not detect any movement it stays idle for 5 seconds. 
After 5 seconds ESP32 camera goes back to deep sleep mode. If movement detected the process repeats. 

PIR detector connects to the ESP32 camera with GPIO 13 pin.
5v supply goes to 5V pin and GND. 