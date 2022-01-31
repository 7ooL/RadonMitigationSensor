# RadonMitigationSensor

I built a pressure reading IoT device, using a ESP8266 clone D1 mini, an adafruit MPRLS pressure sensor, a RED/GREEN led panel light, a buzzard, and a switch. 

video of working version:
https://youtu.be/t_ssRsPznq8



It uses a running average of the pressure from inside the radon mitigation tube and checks it against a defined trigger value to detect if the average pressure is greater than the trigger value, therefore sounding the alarm.
