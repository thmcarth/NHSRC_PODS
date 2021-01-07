**This is the Readme to document Hawes's notes about the ANCOR/ISCO Project v2 for Anne Mikelonis, Katherine Ratliff, Garrett Wiley and anyone else who needs to know more about this project**

Firstly Setting up this project is well documented to a large extent in a manual called the HSRP_Monitoring Word Document that is in EPA hands with Anne or Katherine.
The rest of the data here will be for software or hardware engineers

INSTALL THESE SOFTWARE:
 1. XCTU for Xbee
 2. Arduino IDE.  Bill M has older versions if new one breaks everything.
 3. Pycharm Commununity edition
     a. Install the XBee Micropython Plugin


***********
A few important notes for any developer coming up.  
1. Hydraprobe code relies upon the very particular SDI-12 library that we have a copy of in the libraries folder called Arduino-SDI-12!!!!
2. Xbee Serial Functions are limited to Sys.stdin and Sys.std out:  effectively read and write.  To fix this will require a small hardware change to which pins we are accessing.  Check the Xbee3 LTE CAt 1 manuals for more information on the UART pins.  There are alternative pins we can access that will let us use the UART class in python.  To figure out which pins these are you need to check the pin assignments with the XCTU software, look for UART Alternative options on some of the digital pins.
3. XML Basic format is in a manual you can get at this website: https://response.epa.gov/site/site_profile.aspx?site_id=5033  YOU WILL NEED TO MAKE AN ACCOUNT SO YOU CAN ACCESS THE RESOURCES.  Also all the VIPER links can be accessed here.
4: Link to ORD Deployments: https://viper.ert.org/DeploymentManager/default.aspx
*************


Firstly, the Custom Libraries Folder contains most of if not all of the Libraries needed to compile code for the NHSRC_v_1.0x code.  The Latest version of that is 1.07, 1.04 was before I started work on the project.  I will also be attaching a folder of EVERY library I am using currently on my version of the Arduino IDE, just copy and paste that into your Arduino library folder:: Documents->Arduino->libaries. The Viper Notes folder Contains the basic XML Schema needed to send data to VIPER, more examples can be found in the Manual and in the Micropython code in the  Xbee_Viper Folder main.py file.  
Basic Runtime of Code:INCLUDE all needed libraries, initialize global values that need to stay constant between loops, set up addresses, instantiate Serial ports, digital lines.
Next: 1. Setup {...} sets up SD CARD, serial comms, intervals for sampling, instantiate comms with moisture probes
2. Loop
a. Set rec (text message received flag) to false, Check for text messages from Xbee by pinging Xbee with "?"
b. If ISCORail is true (assumed it is on restart) check to see if ISCOserial is available and read in, check texts a few times.
c. grab battery Voltage
d. if enough time has passed, post data to VIPER
e.if davis is connected, grab data from the davis rainbucket
f. if enough time has passed save all data to the SD Card, then print the statys if which sampling mode is being used
g. Check water level and sample if in auto mode
h. **Save ident for VIPER identifier every 2 hours***
i. Check texts again and return to (a)



