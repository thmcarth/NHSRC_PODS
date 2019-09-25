// Author-Hawes Collier \\

**********************
Instructions for Installing and running the ISCO_pod_sampler code for the first time
**********************

1.Download arduino installer-->https://www.arduino.cc/en/Main/Donate
2.Download Teensyduino that matches your version of Arduino (most likely 1.8.x where x is a number) 
(Note: This link is for 1.8.10 beta--> https://forum.pjrc.com/threads/57609-Teensyduino-1-48-Beta-1)
--This link is for the main site download-->https://www.pjrc.com/teensy/td_download.html
3.Clone or download Github NHSRC_PODS repository as ZIP
--unzip the Repository where it is easy to access for you (Desktop)
4.Download and Keep custom libraries (Ubidots_FONATIM.zip, Timezone.zip) as zip files (because easier to add to your project)
--On github "view raw" will automatically download the zip file to your computer
5. Plug in arduino to usb on PC to actually program the device and flash new directions to the arduino 
- On the Arduino IDE: select Tools-->Board-->Teensy 3.5.  
- Open most recent Sketch 1.03 or 1.04 of Pod_ISCO code example: (C:\Users\Amikelon\Desktop\NHSRC_PODS-master\NHSRC_PODS-master\_94_Pod_ISCOv1.04)
- It will ask you to make a folder, just click OK
- Click Sketch--> Include Library--> add ZIP Library--> select Ubidots_FONATim.zip  (Repeat steps for Timezone.zip and Adafruit Sleepydog.zip)
- Sketch --> Include Library--> Manage Libraries--> install Adafruit and Ubidots FONA Libraries.
6. Click Verify (Check Mark) and see if there are any errors.
******
Likely Errors and solutions
******
1. Multiple libraries were found for "TimeLib.h" compilation terminated.
- Likely solution, need to make sure you install using Sketch--> include Library--> Manage Library to install FONA Libraries

