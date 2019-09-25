Tim's Custom Library
-----------------------
name:UBIDOTS_FONA

Notes on functions
** Function section
* Function Name



*Ubidots (constructor)
-This sets up the server that will communicate with VIPER, this is called near line 118 in the main code _94_ISCO.ino
-TOKEN defines the special key provided by VIPER to connect to our data page.
-Server is set up in the header file ( char * SERVER = "remote.ertviper.org";)  This shows us where we are connecting

**GPRS FUNCTIONS

*setAPN
- This function sets up the cellular hotspot/ Access point for communication to continue.
- prints commands to the Cell modem to set up username and password for access point

*manageData
- compares strings from FONA
- Resets IP, checks IP, Connects to device, then manages data sending and connection

** FUNCTIONS TO RETRIEVE DATA

* getValuewithDevice
- sends request of data to FONA

** FUNCTIONS TO SEND DATA

*addInt
-sends integer value from ISCO with associated name of value to VIPER

*addFloat (similar to Int but there's a decimal point)

*addString
-sends a string similarly to previous Send Functions

*clearData
-clears out current data string

*setDeviceName
-sets name that we can visualize 

*sendALL
-sends all saved Variables

**Auxillary Functions

*init
- turns on serial communication for FONA

*powerUporDown
-turns on/off GPRS 

*setDebug
- Turns on or off debug messages

*readData
-reads data from GPRS











