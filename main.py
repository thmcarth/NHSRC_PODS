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
import utime
# from machine import I2C

import usocket
import network
import sys

deployed = 1  # Deployed should be 1 whenever you are going to place the XBEE back into the field.  Otherwise, you will likely cause issues with the Teensy to Xbee communications
test = 1
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
receive = 0
resend_data = 0
"""""2524126262", "9198860812", "5179451531", "6157148918", "9194233837", "9192301472"""
allowed = ["9196019412", "9199204318","5135718738","9198860812","5133171308","2102515470","9196675239","9195362223"]
prev_sender = ""
prev_msg = None


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

    year = array[0]
    month = array[1]
    month = str(month)
    if len(month) is 1:
        month = "0" + month
    day = array[2]
    day = str(day)
    if len(day) is 1:
        day = "0" + day
    hour = array[3]
    hour = str(hour)
    if len(hour) is 1:
        hour = "0" + hour
    minute = array[4]
    minute = str(minute)
    if len(minute) is 1:
        minute = "0" + minute
    second = array[5]
    second = str(second)
    if len(second) is 1:
        second = "0" + second
    yearday = array[6]
    total_time = "" + str(year) + "-" + str(month) + "-" + str(day) + "T" + str(hour) + ":" + str(minute) + ":" + str(
        second) + "-05:00"
    if not deployed:
        print("Total Time: ", total_time)
    return total_time


def serial_dialogue():
    """
    Wait for special Character.
    Send signal to Teensy
    :return:
    """


def read_serial():
    """
    No params
    Reads in 3 sets of serial data and combines them to ensure all data is received.  Because Serial BUS is slow
    Take all serial data and look at the first character to run through check serial type function
    Decides what to do with serial data: text, mass text, or post to viper
    TODO: Read in whole serial string and check for a "signal or end" character.  Not implemeented (with secondary UART)
            READ LAST CHARACTER OF STRING
            if not !, then return 0 or do nothing
    """
    global prev_sender
    global prev_msg
    global data
    global ident
    global t

    serial1 = sys.stdin.read()  # UART library doesn't work here!
    utime.sleep_ms(15)
    serial2 = sys.stdin.read()
    utime.sleep_ms(15)
    serial3 = sys.stdin.read()
    if serial1 is None and serial2 is None and serial3 is None:
        return 0
    """
    if test:
        if c.isconnected() and serial1:2w
            c.sms_send(2524126262, serial1[:2])
        if c.isconnected() and serial2:
            c.sms_send(2524126262, serial2[:2])
        if c.isconnected() and serial3:
            c.sms_send(2524126262, serial3[:2])
    """
    serial = None
    if serial1 and serial2 and serial3:  ## 1 2 3
        serial = str(serial1) + str(serial2) + str(serial3)
        if serial[-1] is not "!":
            if c.isconnected() and test:
                c.sms_send(9199204318, "not !, 1 2 3")
                # c.sms_send(2524126262, serial[-10:])
            return 0
        else:
            serial = serial[:-1]
        if not deployed:
            print(serial1)
            print(serial2)
            print(serial3)
            print("Therefore serial(3) is ")
            print(serial)

    elif serial1 and serial2 and not serial3:  # 1 2
        serial = str(serial1) + str(serial2)
        if serial[-1] is not "!":
            if c.isconnected() and test:
                c.sms_send(9199204318, "not !, 1 2")
                # c.sms_send(2524126262, serial[-10:])
            return 0
        else:
            serial = serial[:-1]
        if not deployed:
            print(serial2)
            print("Therefore serial2 is ")
            print(serial)
    elif serial1 and not serial2 and not serial3:  # 1
        serial = serial1
        if serial[-1] is not "!":
            if c.isconnected() and test:
                c.sms_send(9199204318, "not !, 1")
                # c.sms_send(2524126262, serial[-10:])
            return 0
        else:
            serial = serial[:-1]
        if not deployed:
            print(serial)
    elif not serial1 and serial2 and serial3:  # 2 3
        serial = str(serial2) + str(serial3)
        if serial[-1] is not "!":
            if c.isconnected() and test:
                c.sms_send(9199204318, "not !, 2 3 ")
                # c.sms_send(2524126262, serial[-10:])
            return 0
        else:
            serial = serial[:-1]
        if not deployed:
            print(serial)
    elif not serial1 and not serial2 and serial3:  # 3
        serial = serial3
        if serial[-1] is not "!":
            if c.isconnected() and test:
                c.sms_send(9199204318, "not !, 3")
                # c.sms_send(2524126262, serial[-10:])
            return 0
        else:
            serial = serial[:-1]
        if not deployed:
            print(serial)
    elif not serial1 and serial2 and not serial3:  # 2
        serial = serial2
        if serial[-1] is not "!":
            if c.isconnected() and test:
                c.sms_send(9199204318, "not !, 2")
                # c.sms_send(2524126262, serial[-10:])
            return 0
        else:
            serial = serial[:-1]
        if not deployed:
            print(serial)
    elif serial1 and not serial2 and serial3:  # 3 1
        serial = str(serial1) + str(serial3)
        if serial[-1] is not "!":
            if c.isconnected() and test:
                c.sms_send(9199204318, "not !, 1 3")
                # c.sms_send(2524126262, serial[-10:])
            return 0
        else:
            serial = serial[:-1]
        if not deployed:
            print(serial)
    else:
        return 0

    if serial:
        types = check_serial_type(serial)
        if types is 0:
            return 0
        elif types is 1:  # post to viper DEPRECATED.  Checks for IDent now in teensy string instead of using Xbee to update Idenmt
            serial = serial[1:]
            t = (time.localtime())  # (year, month, day, hour, second, day, yearday)
            t = create_time(t)
            ident = ident + 1
            ssend(serial, ident, t)

        elif types is 2:  # send to users
            serial = serial[1:]
            # c.sms_send(2524126262, "Got a C command")
            send_text(serial)
            # prev_sender = None
        elif types is 3:  # send to all users
            serial = serial[1:]
            send_text_all(serial)
        elif types is 4:  # post to VIPER!!!! accounts for EEPROM Identifier
            if c.isconnected():
                c.sms_send(9199204318, "Posting")
            t = (time.localtime())  # (year, month, day, hour, second, day, yearday)
            t = create_time(t)
            comma = serial.find("<")
            if comma is -1:
                if c.isconnected():
                    c.sms_send(9199204318, "No <")
                return 0
            ident = serial[:comma]
            serial = serial[comma + 2:]
            ssend(serial, ident, t)
        elif types is 6:
            """
            When receiving ?, check for texts
            """

            if c.isconnected():
                msg_sms_receive = check_txt()  # check for text
                # [sms_txt['message'], sms_txt] returns or "", None
                if msg_sms_receive:
                    if len(msg_sms_receive['message']) > 0:
                        msg = msg_sms_receive['message']
                    else:
                        msg = None
                    sms_sender = msg_sms_receive['sender']
                    if sms_sender:
                        prev_sender = msg_sms_receive['sender']
                    if msg is not None and sms_sender is not None:
                        print(msg)  # if true: interface with Teensy and send Teensy C#
    else:
        return 0


# def parse_message(msg):
#     while True:


def check_serial_type(msg):
    if msg:
        first = msg[:1]
        if not deployed:
            print(first)
        first = first.lower()
        id = first.isdigit()
        if id is True:
            return 4
        switcher = {
            # "p": 1,
            "c": 2,
            "a": 3,
            # "k": 5,  # deprecate not used anymore
            "?": 6
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
        if not deployed:
            print("Requesting from host")
        s.send(post)
        while True:
            if not deployed:
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
    if c.isconnected():
        if test:
            if c.isconnected():
                c.sms_send(9199204318, ident)
                # c.sms_send(2524126262, time)
        if not deployed:
            print("\n Starting Response \n")
            print("Body is:" + str(body))
        post = bytes('<?xml version="1.0" encoding="utf-8"?>\n'
                     '<alert xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance"\n'
                     'xmlns:xsd="http://www.w3.org/2001/XMLSchema"\n'
                     'xmlns="urn:oasis:names:tc:emergency:cap:1.1">\n'
                     '<identifier>' + str(ident) + '</identifier>\n'
                                                   '<sender>EPA_WET_BOARD_2 12-17-2020</sender>\n'
                                                   '<sent>' + str(time) + '</sent>\n'
                                                                          '<source>Board3,BoardTest3,'
                                                                          '1,2</source>\n'
                                                                          '<info>\n'
                                                                          '<headline>' + str(body) + '</headline>\n'
                                                                                                     '<area>\n'
                                                                                                     '<circle>38.904722, -77.016389 0</circle>\n'  # !!!!!!!!! Change coords for each deployment 
                                                                                                     '</area>\n'
                                                                                                     '</info>\n'
                                                                                                     '</alert>\n',
                     'utf-8')
        socketObject = usocket.socket(usocket.AF_INET, usocket.SOCK_STREAM)
        socketObject.connect(("remote.ertviper.org", 8038))
        if not deployed:
            print(" Sending \n")
        socketObject.write(post)
        if not deployed:
            print(socketObject.readline())
            print("Printing the remainder of the server's response: \n")
        # Use a "standard" receive call, "recv",
        # to receive a specified number of
        # bytes from the server, or as many bytes as are available.
        # Receive and output the remainder of the page data.
        socketObject.close()
        if not deployed:
            print("Socket closed.")
    else:
        if not deployed:
            print("No connection")


# i2c = I2C(1, freq=400000)  # I2c Module
c = network.Cellular()


def command_read(comm):
    switcher = {
        1: "1",
        2: "2",
        3: "3",
        4: "4",
        5: "5",
        6: "6",
        7: "7",
        8: "8",
        9: "9",
        10: "10"
    }
    if not deployed:
        print("Received: ", comm)
    return switcher.get(comm, "Invalid command")


"""def i2c_read():
    placeholder
           Parameters
           ----------
          msg: message we want to send to phone
          sms: sms object that xbee uses to store sender information
           Returns
           -------
           None

    global i2c
    data_i2c = i2c.readfrom(40, 4)
    data_i2c = int.from_bytes(data_i2c, byteorder='big')
    return data_i2c
"""


def check_txt():
    global receive
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
            if not deployed:
                print("Sender is: ", sms_txt['sender'])
            sender = sms_txt['sender']
            valid = check_number(sender)
            if valid:
                # if command_read(sms_txt['message']) is "Invalid command":
                # return ["", sms_txt]
                # else:
                return sms_txt  # msg, object
            else:
                return None  # blank and None
        else:
            return None  # blank and None
    else:
        return None  # blank and None


def create_msg(msg):
    """Creates a message to send to Teensy
        Parameters
        ----------
        msg: String
        Returns
        -------
       String: message C#
        """
    if not deployed:
        print("Message is ", msg)
    #  msg1 = "C" + str(msg)
    return msg


def send_text(msg):
    global prev_msg
    global prev_sender
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
        c.sms_send(prev_sender, msg)
    prev_msg = None


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
    ok_num = None
    if number in allowed:
        ok_num = 1
    if not deployed:
        print("Number valid: ", number, " is ", ok_num)
    return ok_num


elapsed = 0
start = time.time()
while True:

    """Runs while loop functionality of xbee. 
            ---check for texts
            ---is message is valid, create a new message to send to Teensy as a command
            ---read Serial from Teensy and process
            ---sleep for 5 seconds
           """

    if c.isconnected():
        read_serial()

    # if true: interface with Teensy and send Teensy C#
    # check timer for VIPER POST

    utime.sleep_ms(250)