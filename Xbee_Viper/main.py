# Default template for XBee MicroPython projects

# import urequests
# import umqtt
# import uftp
# import remotemanager
# import hdc1080
# import ds1621
import socket
# from machine import UART
import time

# from machine import I2C

import usocket
import network
import sys

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

'''

'''

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

# uart = UART(1, 115200)
data = 0
sms = 0
ident = 0
t = 0
allowed = ["2524126262", "9198860812", "5179451531", "6157148918", "9194233837", "9192301472",
           "9196019412"]
prev_sender = ""


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


def create_time(array):
    """creates time for VIPER post

           Parameters
           ----------
          array- datetime object

           Returns
           -------
           time in string format
           """
    # (year, month, day, hour, second, day of week , day of year)
    # 2011-08-19T15:31:08-04:00 is style we want to create
    # t1 = datetime.datetime.now()
    # print("ti is: ", t1)

    year = array[0]
    month = array[1]
    day = array[2]
    hour = array[3]
    minute = array[4]
    second = array[5]
    yearday = array[6]
    total_time = "" + str(year) + "-" + str(month) + "-" + str(day) + "T" + str(hour) +":"+ str(minute)+":"+str(second)+"-05:00"
    print("Total Time: ", total_time)
    return total_time


def read_serial():
    global data
    global ident
    global t
    serial = sys.stdin.read()
    if serial:
        print(serial)
        types = check_serial_type(serial)
        if types is 0:
            return 0
        elif types is 1:  # post to viper
            serial = serial[1:]
            t = (time.localtime())  # (year, month, day, hour, second, day, yearday)
            t = create_time(t)
            ident = ident + 1
            ssend(serial, ident, t)

        elif types is 2:  # send to users
            serial = serial[1:]
            send_text(serial)
        elif types is 3:  # send to all users

            serial = serial[1:]
            send_text_all(serial)
        elif types is 4:  # post to VIPER
            t = (time.localtime())  # (year, month, day, hour, second, day, yearday)
            t = create_time(t)
            comma = serial.find(",")
            ident = serial[:comma]
            serial = serial[comma+1:]
            ssend(serial,ident,t)
    else:
        return 0


def send_serial(message):
    sys.stdout.write(message)


def check_serial_type(msg):
    if msg:
        first = msg[0:1]
        first = first.lower()
        if first.isnumeric():
            return 4
        switcher = {
            "p": 1,
            "c": 2,
            "a": 3
        }
        return switcher.get(first, 0)


'''
HTTP example
FORMAT TO USE % s 'POST /CAP/post HTTP/1.1\r\nHost: %s\r\n\r\n' % host, 'utf16'

post = bytes('POST /CAP/post HTTP/1.1\r\nHost: viper.response.epa.gov\r\nAuthorization: '
                     'Basic Y29sbGllci5qYW1lc0BlcGEuZ292Ok1pbmlzdHJ5bWFuMSE=\r\nContent-Length: 547\r\nConnection: '
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
        length = get_post_length(body)
        s.connect((host, 443))
        posthead = bytes("POST /CAP/post HTTP/1.1\r\nHost: viper.response.epa.gov\r\nAuthorization: "
                         "Y29sbGllci5qYW1lc0BlcGEuZ292OldldGJvYXJkdGVhbTEh\r\nContent-Length: %i\r\nConnection: "
                         "Keep-Alive\r\n\r\n, utf8" % length)
        post = bytes(posthead + body)
        print("Requesting from host")
        s.send(post)
        while True:
            print(str(s.recv(500), 'utf8'), end='')
    finally:
        s.close()


def ssend(body, ident, time):
    """Sends a message to socket connection on VIPER server

           Parameters
           ----------
          msg: message we want to send to phone
          sms: sms object that xbee uses to store sender information
           Returns
           -------
           None
           """
    print("\n Starting Response \n")
    print("Body is: ", body)
    post = bytes('<?xml version="1.0" encoding="utf-8"?>\n'
                 '<alert xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance"\n'
                 'xmlns:xsd="http://www.w3.org/2001/XMLSchema"\n'
                 'xmlns="urn:oasis:names:tc:emergency:cap:1.1">\n'
                 '<identifier>' + str(ident) + '</identifier>\n'
                 '<sender>EPA_WET_BOARD_test 10-28-2020</sender>\n'
                 '<sent>' + str(time) + '</sent>\n'
                 '<source>Board 0,BoardTest1,'
                 '1,2</source>\n'
                 '<info>\n'
                 '<headline>' + str(body) + '</headline>\n'
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


# i2c = I2C(1, freq=400000)  # I2c Module
c = network.Cellular()


def command_read(comm):
    switcher = {
        1: "C1",
        2: "C2",
        3: "C3",
        4: "C4",
        5: "C5",
        6: "C6",
        7: "C7",
        8: "C8",
        9: "C9",
        10: "C10"
    }
    print("Received: ", comm)
    return switcher.get(comm, "Invalid command")


def i2c_read():
    """placeholder

           Parameters
           ----------
          msg: message we want to send to phone
          sms: sms object that xbee uses to store sender information

           Returns
           -------
           None
           """
    global i2c
    data_i2c = i2c.readfrom(40, 4)
    data_i2c = int.from_bytes(data_i2c, byteorder='big')
    return data_i2c


def check_txt():
    """   Checks Text Message make sure sender is allowed and message is a valid command

           Parameters
           ----------
          None

           Returns
           -------
           0: no message or invalid
           else: a message and sms object
           """
    valid = 0
    if c.isconnected():
        sms_txt = c.sms_receive()
        if sms_txt:
            print("Sender is: ", sms_txt['sender'])
            sender = sms_txt['sender']
            valid = check_number(sender)
        if sms_txt and valid:
            if command_read(sms_txt['message']) is "Invalid command":
                return ["", sms_txt]
            else:
                return [sms_txt['message'], sms_txt]
        else:
            return ["", sms_txt]


def create_msg(msg):
    """Creates a message to send to Teensy

        Parameters
        ----------
        msg: String


        Returns
        -------
       String: message C#
        """
    print("Message is ", msg)
    msg1 = "C" + str(msg)
    return msg1


def send_text(msg, sms):
    """Sends a text message back to most recent user

        Parameters
        ----------
       msg: message we want to send to phone
       sms: sms object that xbee uses to store sender information

        Returns
        -------
        None
        """
    if c.isconnected():
        c.sms_send(sms['sender'], msg)

def send_text_all(msg):
    """Sends a text message back to most recent user

            Parameters
            ----------
           msg: message we want to send to phone
           sms: sms object that xbee uses to store sender information

            Returns
            -------
            None
         """
    if c.isconnected():
        for each in allowed:
            c.sms_send(each, msg)


def get_post_length(post):
    """collects length of post to socket connection

           Parameters
           ----------
         post: total post length

           Returns
           -------
           total_l length of post
           """
    total_l = len(post)
    return total_l


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


def check_number(number):
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
    print("Number valid: ", number, " is ", ok_num)
    return ok_num


while True:
    """Runs while loop functionality of xbee. 
            ---check for texts
            ---is message is valid, create a new message to send to Teensy as a command
            ---read Serial from Teensy and process
            ---sleep for 5 seconds

           """
    # sms_sender = ''

    serial_type = 0
    if c.isconnected():
        msg_sms = check_txt()  # check for text*
    # tuple_msg = check_txt()
        if len(msg_sms[0]) > 0:
            msg = msg_sms[0]
        else:
            msg = None
        sms = msg_sms[1]
        if sms:
            prev_sender = sms['sender']
        if msg is not None:
            msg = create_msg(msg)
            send_serial(msg)  # if true: interface with Teensy and send Teensy C#

    read_serial()

    # if true: interface with Teensy and send Teensy C#
    # check timer for VIPER POST

    time.sleep(5)