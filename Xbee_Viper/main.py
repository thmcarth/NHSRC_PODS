# Default template for XBee MicroPython projects

import urequests
import umqtt
import uftp
import remotemanager
import hdc1080
import ds1621
import socket
from machine import UART
import time
from machine import I2C
import usocket
import network

'''
Commands we are going to send/receive to TEENSY

SEND_____
C1 (ask for Send voltage rail values to senderNum)
C2 (ask for Sample, and start timer to send sample status back to user)
C3 Turn ISCO on or off (not sure why we do this)
C4 Changes the sample mode (auto/manual)
C5 Change auto sample interval
_________
***
***
RECEIVE______
Cx where x = value asked
Cx where  x = string status
Cx where x is status
Cx where x is status
Cx where x is status
_____________

Plan of attack for development!:::

1. We set up a loop in the xbee that waits for a cell-phone text or message from teensy
1a. Let's rely on XBEE timing over teensy clock (this has internet connection)
2. If we receive a text message to the user we want to make sure we save that cell  # so we don't run
into the issue of dealing with texts from multiple users.
2a.  If we receive a text, lets put it into a buffer/variable array that expands or contracts based on how many text 
messages there are.  Basically make it a Dictionary that starts off as empty (this may be hard without interrupts (im looking into this))
3.
'''

# socket notes
'''

# Import the socket module.  
# This allows the creation/use of socket objects.
 
import usocket
# Create a TCP socket that can communicate over the internet.
socketObject = usocket.socket(usocket.AF_INET, usocket.SOCK_STREAM)
# Create a "request" string, which is how we "ask" the web server for data.
request = "GET /ks/test.html HTTP/1.1\r\nHost: www.micropython.org\r\n\r\n"
# Connect the socket object to the web server
socketObject.connect(("www.micropython.org", 80))
# Send the "GET" request to the MicroPython web server.  
# A "GET" request asks the server for the web page data.
bytessent = socketObject.send(request)
print("\r\nSent %d byte GET request to the web server." % bytessent)
 
print("Printing first 3 lines of server's response: \r\n")
# Single lines can be read from the socket, 
# useful for separating headers or
# reading other data line-by-line.
# Use the "readline" call to do this.  
# Calling it a few times will show the
# first few lines from the server's response.
socketObject.readline()
socketObject.readline()
socketObject.readline()
# The first 3 lines of the server's response 
# will be received and output to the terminal.
 
print("\nPrinting the remainder of the server's response: \n")
# Use a "standard" receive call, "recv", 
# to receive a specified number of
# bytes from the server, or as many bytes as are available.
# Receive and output the remainder of the page data.
socketObject.recv(512)
 
# Close the socket's current connection now that we are finished.
socketObject.close()
print("Socket closed.")
 

'''

'''
THESE ARE THE PHONE NUMBER WE ACCEPT!
char* hawesPhone = "+12524126262";  
Numbers to accept SMS commands from: (from the old teensy code)
char* annesPhone1 = "+19198860812";
char* annesPhone2 = "+15179451531";
char* katherinesPhone = "+16157148918";
char* worthsPhone = "+19194233837";
char* googlePhone = "+19192301472";
char* garrettPhone = "+19196019412";
'''
# Set up HTTP COMMS with VIPER


'''VIPER - CAP via HTTPS - Notes:

----------------------------------------
HTTPS = HTTP over TLS/SSL
TLS - Transport Layer Security
 -rVIPER requires v1.2
HTTP Basic Auth
 -HTTP Authorization header
 -"Basic" scheme
 -base64-encoded bytes of "[username]:[password]"
 -e.g., "Authorization: Basic QWxhZGRpbjpPcGVuU2VzYW1l"
Content-Length header required
 -size of message body (i.e. CAP XML), in bytes
CAP XML in request body'''
'''
POST /CAP/post HTTP/1.1
Host: viper.response.epa.gov
Authorization: Basic QWxhZGRpbjpPcGVuU2VzYW1l
Content-Length: 547
Connection: Keep-Alive

 Our Authentication: Y29sbGllci5qYW1lc0BlcGEuZ292OldldGJvYXJkdGVhbTEh

<?xml version="1.0" encoding="utf-16"?>
<alert xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance"
       xmlns:xsd="http://www.w3.org/2001/XMLSchema"
       xmlns="urn:oasis:names:tc:emergency:cap:1.1">
<identifier>281005951_634498074648864996</identifier>
<sender>My Device</sender>
<sent>2011-08-19T15:31:08-04:00</sent>>
<source>Acme Particulate Monitor,APM S/N 123456,0,0</source>
<info>
  <headline>ConcRT;0.001;mg/m3;Green;ConcHr;0;mg/m3;Green;</headline>
  <area>
    <circle>38.904722, -77.016389 0</circle>
  </area>
</info>
</alert>

This ^^ is all the body
'''

uart = UART(1, 9600)
data = 0
sms = 0
ident = 0
t = 0
'''
char* hawesPhone = "+12524126262";
//Numbers to accept SMS commands from:
char* annesPhone1 = "+19198860812";
char* annesPhone2 = "+15179451531";
char* katherinesPhone = "+16157148918";
char* worthsPhone = "+19194233837";
char* googlePhone = "+19192301472";
char* garrettPhone = "+19196019412";
'''
allowed_number = [12524126262, 19198860812, 15179451531, 16157148918, 19194233837, 19192301472, 19196019412]


def time_counter(seconds):
    """Pauses program for a user-specified time.

    Parameters
    ----------
    seconds : int
        number of seconds to pause program

    Returns
    -------
    ``None``
    """
    start = time.time()
    elapsed = 0
    while elapsed < seconds:
        elapsed = time.time() - start
    #    print('done counting!')
    return None


def check_number(number, allowed):
    """Checks to see if a number is in allowed list.

    Parameters
    ----------
    number : number to check
    allowed : list of allowed numbers

    Returns
    -------
    boolean
        `True` if number is in allowed list. Otherwise, `False`
    """
    ok_num = 0
    if number in allowed:
        ok_num = 1
    return ok_num


def create_time(array):
    # (year, month, day, hour, second, day of week , day of year)
    # 2011-08-19T15:31:08-04:00 is style we want to create
    year = array[0]
    month = array[1]
    day = array[2]
    hour = array[3]
    seconds = array[4]
    minutes = seconds/60
    yearday = array[6]
    total_time = "" + year + "-" + month + "-" + day + "T" + hour + " " + minutes + " " + seconds
    return total_time


def read_serial():
    global data
    global ident
    global t

    if uart.any() > 0:
        serial = uart.read(uart.any())
        types = check_serial_type(serial)
        if types is 0:
            return 0
        elif types is 1:  # post to viper
            serial = serial[1:]
            t = (time.localtime())  # (year, month, day, hour, second, day, yearday)
            t = create_time(t)
            ident = ident + 1
            ssend(serial, ident, time)
            return 0
        elif types is 2:  # send to users
            serial = serial[1:]
            return serial
    else:
        return 0


def send_serial(message):
    uart.write(message)


def check_serial_type(msg):
    if msg:
        first = msg[0]
        first = first.lower()
        switcher = {
            "p": 1,
            "c": 2
        }
        return switcher.get(first, 0)


'''
HTTP example
FORMAT TO USE % s 'POST /CAP/post HTTP/1.1\r\nHost: %s\r\n\r\n' % host, 'utf16'
 
post = bytes('POST /CAP/post HTTP/1.1\r\nHost: viper.response.epa.gov\r\nAuthorization: '
                     'Basic Y29sbGllci5qYW1lc0BlcGEuZ292OldldGJvYXJkdGVhbTEh\r\nContent-Length: 547\r\nConnection: '
                     'Keep-Alive\r\n<?xml version="1.0" encoding="utf-16"?>
<alert xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance"
       xmlns:xsd="http://www.w3.org/2001/XMLSchema"
       xmlns="urn:oasis:names:tc:emergency:cap:1.1">
<identifier>281005951_634498074648864996</identifier>
<sender>My Device</sender>
<sent>2011-08-19T15:31:08-04:00</sent>>
<source>Acme Particulate Monitor,APM S/N 123456,0,0</source>
<info>
  <headline>ConcRT;0.001;mg/m3;Green;ConcHr;0;mg/m3;Green;</headline>
  <area>
    <circle>38.904722, -77.016389 0</circle>
  </area>
</info>
</alert>\r\n', 'utf16')

<?xml version="1.0" encoding="utf-16"?>
<alert xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance"
       xmlns:xsd="http://www.w3.org/2001/XMLSchema"
       xmlns="urn:oasis:names:tc:emergency:cap:1.1">
<identifier>281005951_634498074648864996</identifier>
<sender>My Device</sender>
<sent>2011-08-19T15:31:08-04:00</sent>>
<source>Acme Particulate Monitor,APM S/N 123456,0,0</source>
<info>
  <headline>ConcRT;0.001;mg/m3;Green;ConcHr;0;mg/m3;Green;</headline>
  <area>
    <circle>38.904722, -77.016389 0</circle>
  </area>
</info>
</alert>
                     
'''


def http_post(host, body):
    s = socket.socket()
    try:
        s.connect((host, 443))
        # post = bytes('POST /CAP/post HTTP/1.1\r\nHost: viper.response.epa.gov\r\nAuthorization: '
        #             'Y29sbGllci5qYW1lc0BlcGEuZ292OldldGJvYXJkdGVhbTEh\r\nContent-Length: %s\r\nConnection: '
        #             'Keep-Alive\r\n\r\n', 'utf8')
        post = bytes(body)
        print("Requesting from host")
        s.send(post)
        while True:
            print(str(s.recv(500), 'utf8'), end='')
    finally:
        s.close()


def http_test(post, date, ident):
    st = socket.socket()
    try:
        st.connect(("https://viper.response.epa.gov/CAP/post", 443))
        post = bytes('POST /CAP/post HTTP/1.1\r\nHost: viper.response.epa.gov\r\nAuthorization: '
                     'Basic Y29sbGllci5qYW1lc0BlcGEuZ292OldldGJvYXJkdGVhbTEh\r\nContent-Length: 547\r\nConnection: '
                     'Keep-Alive\r\n'
                     '<?xml version="1.0" encoding="utf-16"?>'
                     '<alert xmlns: xsi = "http://www.w3.org/2001/XMLSchema-instance"'
                     'xmlns: xsd = "http://www.w3.org/2001/XMLSchema"'
                     'xmlns = "urn:oasis:names:tc:emergency:cap:1.1">'
                     '<identifier> ' + ident + ' </identifier '
                                               '<sender> EPA_WET_BOARD </sender>'
                                               '<sent>' + date + '</sent>>'
                                                                 '<source>Acme Particulate Monitor,APM S/N 123456,0,0</source>'
                                                                 '<info>'
                                                                 '<headline> ' + post + '</headline>'
                                                                                        '<area>'
                                                                                        '<circle>38.904722, -77.016389 0</circle>'
                                                                                        '</area>'
                                                                                        '</info>'
                                                                                        '</alert>\r\n\r\n')
        st.send(post)
    finally:
        st.close()


def ssend(body, ident, time):
    print("\n Starting Response \n")
    post = bytes('<?xml version="1.0" encoding="utf-8"?>\n'
                 '<alert xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance"\n'
                 'xmlns:xsd="http://www.w3.org/2001/XMLSchema"\n'
                 'xmlns="urn:oasis:names:tc:emergency:cap:1.1">\n'
                 '<identifier>' + ident + '</identifier>\n'
                 '<sender>EPA_WET_BOARD</sender>\n'
                 '<sent>' + time + '</sent>\n'
                 '<source>Acme Particulate Monitor,APM S/N 123456,0,0</source>\n'
                 '<info>\n'
                 '<headline>' + body + '</headline>\n'
                 '<area>\n'
                 '<circle>38.904722, -77.016389 0</circle>\n'
                 '</area>\n'
                 '</info>\n'
                 '</alert>\n', 'utf-8')
    socketObject = usocket.socket(usocket.AF_INET, usocket.SOCK_STREAM)
    socketObject.connect(("remote.ertviper.org", 8038))
    print(" Sending \n")
    socketObject.send(post)
    print(socketObject.readline())

    print("Printing the remainder of the server's response: \n")
    # Use a "standard" receive call, "recv",
    # to receive a specified number of
    # bytes from the server, or as many bytes as are available.
    # Receive and output the remainder of the page data.
    socketObject.close()
    print("Socket closed.")


i2c = I2C(1, freq=400000)  # I2c Module
c = network.Cellular()


def command_read(comm):
    comm = comm[0]
    switcher = {
        1: "C1",
        2: "C2",
        3: "C3",
        4: "C4",
        5: "C5"
    }
    return switcher.get(comm, "Invalid command")


def i2c_read():
    global i2c
    data_i2c = i2c.readfrom(40, 4)
    data_i2c = int.from_bytes(data_i2c, byteorder='big')
    return data_i2c


def check_txt():
    if c.isconnected():
        sms = c.sms_receive()
        first_char = (sms['message'][0])

        if sms:
            if check_number(sms['sender'], allowed_number):

                txt = command_read(sms['message'])
                if command_read(sms['message']) is "Invalid command":
                    return 0
                if first_char is "5":
                    return sms['message'], sms
                else:
                    return sms['message'], sms


def create_msg(msg):
    msg = "C" + msg
    return msg


def change_time(string):
    string = "5_" + string


def send_text(msg, sms):
    c.sms_send(sms['sender'], msg)


while True:
    serial_type = 0
    msg, sms = check_txt()  # check for text* see if it is a message
    if msg is not 0:
        msg = create_msg(msg)
        send_serial(msg)  # if true: interface with Teensy and send Teensy C#
    else:
        send_text("Invalid Command", sms)
    read = read_serial()
    if read is not 0:
        send_text(read, sms)

    time.sleep(5)
