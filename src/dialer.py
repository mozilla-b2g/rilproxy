import array
import socket
import select
import sys
import struct

class Phone:
    RADIO_STATE = [
        "RADIO_STATE_OFF",
        "RADIO_STATE_UNAVAILABLE",
        "RADIO_STATE_SIM_NOT_READY",
        "RADIO_STATE_SIM_LOCKED_OR_ABSENT",
        "RADIO_STATE_SIM_READY",
        "RADIO_STATE_RUIM_NOT_READY",
        "RADIO_STATE_RUIM_READY", 
        "RADIO_STATE_RUIM_LOCKED_OR_ABSENT",        
        "RADIO_STATE_NV_NOT_READY",
        "RADIO_STATE_NV_READY"
    ]

    NETWORK_STATE = []
    
    def __init__(self):
        self._parcels = []
        self.socket = None
        self.poll = select.poll()
        self._packets = {}

    def open(self):
        self.socket = socket.create_connection(("localhost", 6555))
        self.poll.register(self.socket.fileno(), select.POLLIN)

    def waitForAllReturns(self):
        print "Waiting on %s" % self._packets.keys()
        while len(self._packets) > 0:
            self.receiveParcel()            

    def close(self):
        self.setScreenState(0)
        self.waitForAllReturns()
        self.socket.close()

    def hasData(self):
        return len(self.poll.poll(10)) > 0

    def sendParcel(self, p):
        self._packets[p.pid] = p
        s = p.prepareParcelForWrite()
        # print ["0x%.02x" % ord(x) for x in s]
        self.socket.send(struct.pack("I", socket.htonl(len(s))))
        self.socket.send(s)

    def receiveParcel(self):
        data = Parcel()
        buf = self.socket.recv(8192)
        data.setBuffer(buf)
        print ["0x%.02x" % ord(x) for x in buf]
        while data.dataLeft() > 0:
            size = socket.ntohl(struct.unpack_from("I", data.readRaw(4))[0])
            print "Parcel size: %d Data Left: %d" % (size, data.dataLeft())
            # Since packet size and packets are sent as seperate
            # writes, sometimes this requires two fetches
            if data.dataLeft() == 0:
                buf = self.socket.recv(8192)
                data = Parcel()
                data.setBuffer(buf)
            p = CommandParcel()
            b = data.readRaw(size)
            p.setBuffer(b)
            p.parseParcel(self._packets)
            print p.parcelToString()
            print ["0x%.02x" % ord(x) for x in b]
            if p.pid in self._packets.keys():
                self._packets.pop(p.pid)
            if p.request_type in (CommandParcel.REQUEST_TYPES["RIL_REQUEST_OPERATOR"], CommandParcel.REQUEST_TYPES["RIL_REQUEST_REGISTRATION_STATE"], CommandParcel.REQUEST_TYPES["RIL_REQUEST_GPRS_REGISTRATION_STATE"]):
                p.readInt()
                self.outputStrings(p)
            elif p.request_type in (CommandParcel.REQUEST_TYPES["RIL_REQUEST_GET_IMEI"], CommandParcel.REQUEST_TYPES["RIL_REQUEST_GET_IMEISV"], CommandParcel.REQUEST_TYPES["RIL_REQUEST_BASEBAND_VERSION"]):
                p.readInt()
                self.outputString(p)                
            elif p.request_type == CommandParcel.REQUEST_TYPES["RIL_UNSOL_RESPONSE_RADIO_STATE_CHANGED"]:
                self.unsolRadioStateChanged(p)
            elif p.request_type == CommandParcel.REQUEST_TYPES["RIL_UNSOL_RESPONSE_NETWORK_STATE_CHANGED"]:
                self.unsolNetworkStateChanged(p)

    def outputString(self, p):
        string_length = p.readInt()
        if string_length == 4294967295:
            print "NULL STRING"
            return
        string_length = string_length + 1
        if string_length % 2 == 1:
            string_length = string_length + 1
        string = p.readRaw(string_length*2)    
        print "%d : %s" % (string_length, string.decode('utf-16')) #["0x%.02x" % ord(x) for x in string]) 

    def outputStrings(self, p):        
        string_count = p.readInt()
        for i in range(0, string_count):            
            self.outputString(p)
            
    def unsolRadioStateChanged(self, p):
        state = p.readInt()
        print "RADIO STATE: %s" % (self.RADIO_STATE[state])

    def unsolNetworkStateChanged(self, p):
        self.sendParcel(CommandParcel(CommandParcel.REQUEST_TYPES["RIL_REQUEST_REGISTRATION_STATE"]))
        self.sendParcel(CommandParcel(CommandParcel.REQUEST_TYPES["RIL_REQUEST_GPRS_REGISTRATION_STATE"]))
        self.sendParcel(CommandParcel(CommandParcel.REQUEST_TYPES["RIL_REQUEST_OPERATOR"]))

    def setScreenState(self, state):
        p = CommandParcel(CommandParcel.REQUEST_TYPES["RIL_REQUEST_SCREEN_STATE"])
        p.writeInt(1)
        p.writeInt(state)
        self.sendParcel(p)

    def setRadioPower(self, state):
        p = CommandParcel(CommandParcel.REQUEST_TYPES["RIL_REQUEST_RADIO_POWER"])
        p.writeInt(1)
        p.writeInt(state)
        self.sendParcel(p)

    def setNetworkType(self, network_type):
        p = CommandParcel(CommandParcel.REQUEST_TYPES["RIL_REQUEST_SET_PREFERRED_NETWORK_TYPE"])
        p.writeInt(1)
        p.writeInt(network_type)
        self.sendParcel(p)

    def getPhoneIdentity(self):        
        self.sendParcel(CommandParcel(CommandParcel.REQUEST_TYPES["RIL_REQUEST_GET_IMEI"]))
        self.sendParcel(CommandParcel(CommandParcel.REQUEST_TYPES["RIL_REQUEST_GET_IMEISV"]))
        self.sendParcel(CommandParcel(CommandParcel.REQUEST_TYPES["RIL_REQUEST_BASEBAND_VERSION"]))

    def initializePhone(self):
        self.setScreenState(1)
        self.setRadioPower(1)
        self.setNetworkType(1) # GSM only
        self.getPhoneIdentity()
        self.sendParcel(CommandParcel(CommandParcel.REQUEST_TYPES["RIL_REQUEST_QUERY_NETWORK_SELECTION_MODE"]))

    def dialPhone(self):
        pass

class Parcel:
    def __init__(self):
        self._buffer = ""
        self._position = 0

    def dataLeft(self):
        return len(self._buffer) - self._position
        
    def setPosition(self, pos):
        self._position = pos

    def setBuffer(self, data):
        self._buffer = data

    def writeInt(self, i):
        self._buffer = self._buffer + struct.pack("I", i)

    def readInt(self):
        i = struct.unpack_from("I", self._buffer[0+self._position:4+self._position])[0]
        self._position = self._position + 4
        return i

    def readRaw(self, size):
        data = self._buffer[0+self._position:size+self._position]
        self._position += size
        return data

class CommandParcel(Parcel):
    serial = 1
    REQUEST_TYPES = {
        "RIL_REQUEST_GET_SIM_STATUS":1,
        "RIL_REQUEST_ENTER_SIM_PIN":2,
        "RIL_REQUEST_ENTER_SIM_PUK":3,
        "RIL_REQUEST_ENTER_SIM_PIN2":4,
        "RIL_REQUEST_ENTER_SIM_PUK2":5,
        "RIL_REQUEST_CHANGE_SIM_PIN":6,
        "RIL_REQUEST_CHANGE_SIM_PIN2":7,
        "RIL_REQUEST_ENTER_NETWORK_DEPERSONALIZATION":8,
        "RIL_REQUEST_GET_CURRENT_CALLS":9,
        "RIL_REQUEST_DIAL":10,
        "RIL_REQUEST_GET_IMSI":11,
        "RIL_REQUEST_HANGUP":12,
        "RIL_REQUEST_HANGUP_WAITING_OR_BACKGROUND":13,
        "RIL_REQUEST_HANGUP_FOREGROUND_RESUME_BACKGROUND":14,
        "RIL_REQUEST_SWITCH_WAITING_OR_HOLDING_AND_ACTIVE":15,
        "RIL_REQUEST_SWITCH_HOLDING_AND_ACTIVE":15,
        "RIL_REQUEST_CONFERENCE":16,
        "RIL_REQUEST_UDUB":17,
        "RIL_REQUEST_LAST_CALL_FAIL_CAUSE":18,
        "RIL_REQUEST_SIGNAL_STRENGTH":19,
        "RIL_REQUEST_REGISTRATION_STATE":20,
        "RIL_REQUEST_GPRS_REGISTRATION_STATE":21,
        "RIL_REQUEST_OPERATOR":22,
        "RIL_REQUEST_RADIO_POWER":23,
        "RIL_REQUEST_DTMF":24,
        "RIL_REQUEST_SEND_SMS":25,
        "RIL_REQUEST_SEND_SMS_EXPECT_MORE":26,
        "RIL_REQUEST_SETUP_DATA_CALL":27,
        "RIL_REQUEST_SIM_IO":28,
        "RIL_REQUEST_SEND_USSD":29,
        "RIL_REQUEST_CANCEL_USSD":30,
        "RIL_REQUEST_GET_CLIR":31,
        "RIL_REQUEST_SET_CLIR":32,
        "RIL_REQUEST_QUERY_CALL_FORWARD_STATUS":33,
        "RIL_REQUEST_SET_CALL_FORWARD":34,
        "RIL_REQUEST_QUERY_CALL_WAITING":35,
        "RIL_REQUEST_SET_CALL_WAITING":36,
        "RIL_REQUEST_SMS_ACKNOWLEDGE":37,
        "RIL_REQUEST_GET_IMEI":38,
        "RIL_REQUEST_GET_IMEISV":39,
        "RIL_REQUEST_ANSWER":40,
        "RIL_REQUEST_DEACTIVATE_DATA_CALL":41,
        "RIL_REQUEST_QUERY_FACILITY_LOCK":42,
        "RIL_REQUEST_SET_FACILITY_LOCK":43,
        "RIL_REQUEST_CHANGE_BARRING_PASSWORD":44,
        "RIL_REQUEST_QUERY_NETWORK_SELECTION_MODE":45,
        "RIL_REQUEST_SET_NETWORK_SELECTION_AUTOMATIC":46,
        "RIL_REQUEST_SET_NETWORK_SELECTION_MANUAL":47,
        "RIL_REQUEST_QUERY_AVAILABLE_NETWORKS":48,
        "RIL_REQUEST_DTMF_START":49,
        "RIL_REQUEST_DTMF_STOP":50,
        "RIL_REQUEST_BASEBAND_VERSION":51,
        "RIL_REQUEST_SEPARATE_CONNECTION":52,
        "RIL_REQUEST_SET_MUTE":53,
        "RIL_REQUEST_GET_MUTE":54,
        "RIL_REQUEST_QUERY_CLIP":55,
        "RIL_REQUEST_LAST_DATA_CALL_FAIL_CAUSE":56,
        "RIL_REQUEST_DATA_CALL_LIST":57,
        "RIL_REQUEST_RESET_RADIO":58,
        "RIL_REQUEST_OEM_HOOK_RAW":59,
        "RIL_REQUEST_OEM_HOOK_STRINGS":60,
        "RIL_REQUEST_SCREEN_STATE":61,
        "RIL_REQUEST_SET_SUPP_SVC_NOTIFICATION":62,
        "RIL_REQUEST_WRITE_SMS_TO_SIM":63,
        "RIL_REQUEST_DELETE_SMS_ON_SIM":64,
        "RIL_REQUEST_SET_BAND_MODE":65,
        "RIL_REQUEST_QUERY_AVAILABLE_BAND_MODE":66,
        "RIL_REQUEST_STK_GET_PROFILE":67,
        "RIL_REQUEST_STK_SET_PROFILE":68,
        "RIL_REQUEST_STK_SEND_ENVELOPE_COMMAND":69,
        "RIL_REQUEST_STK_SEND_TERMINAL_RESPONSE":70,
        "RIL_REQUEST_STK_HANDLE_CALL_SETUP_REQUESTED_FROM_SIM":71,
        "RIL_REQUEST_EXPLICIT_CALL_TRANSFER":72,
        "RIL_REQUEST_SET_PREFERRED_NETWORK_TYPE":73,
        "RIL_REQUEST_GET_PREFERRED_NETWORK_TYPE":74,
        "RIL_REQUEST_GET_NEIGHBORING_CELL_IDS":75,
        "RIL_REQUEST_SET_LOCATION_UPDATES":76,
        "RIL_REQUEST_CDMA_SET_SUBSCRIPTION":77,
        "RIL_REQUEST_CDMA_SET_ROAMING_PREFERENCE":78,
        "RIL_REQUEST_CDMA_QUERY_ROAMING_PREFERENCE":79,
        "RIL_REQUEST_SET_TTY_MODE":80,
        "RIL_REQUEST_QUERY_TTY_MODE":81,
        "RIL_REQUEST_CDMA_SET_PREFERRED_VOICE_PRIVACY_MODE":82,
        "RIL_REQUEST_CDMA_QUERY_PREFERRED_VOICE_PRIVACY_MODE":83,
        "RIL_REQUEST_CDMA_FLASH":84,
        "RIL_REQUEST_CDMA_BURST_DTMF":85,
        "RIL_REQUEST_CDMA_VALIDATE_AND_WRITE_AKEY":86,
        "RIL_REQUEST_CDMA_SEND_SMS":87,
        "RIL_REQUEST_CDMA_SMS_ACKNOWLEDGE":88,
        "RIL_REQUEST_GSM_GET_BROADCAST_SMS_CONFIG":89,
        "RIL_REQUEST_GSM_SET_BROADCAST_SMS_CONFIG":90,
        "RIL_REQUEST_GSM_SMS_BROADCAST_ACTIVATION":91,
        "RIL_REQUEST_CDMA_GET_BROADCAST_SMS_CONFIG":92,
        "RIL_REQUEST_CDMA_SET_BROADCAST_SMS_CONFIG":93,
        "RIL_REQUEST_CDMA_SMS_BROADCAST_ACTIVATION":94,
        "RIL_REQUEST_CDMA_SUBSCRIPTION":95,
        "RIL_REQUEST_CDMA_WRITE_SMS_TO_RUIM":96,
        "RIL_REQUEST_CDMA_DELETE_SMS_ON_RUIM":97,
        "RIL_REQUEST_DEVICE_IDENTITY":98,
        "RIL_REQUEST_EXIT_EMERGENCY_CALLBACK_MODE":99,
        "RIL_REQUEST_GET_SMSC_ADDRESS":100,
        "RIL_REQUEST_SET_SMSC_ADDRESS":101,
        "RIL_REQUEST_REPORT_SMS_MEMORY_STATUS":102,
        "RIL_REQUEST_REPORT_STK_SERVICE_IS_RUNNING":103,
        "RIL_UNSOL_RESPONSE_RADIO_STATE_CHANGED":1000,
        "RIL_UNSOL_RESPONSE_CALL_STATE_CHANGED":1001,
        "RIL_UNSOL_RESPONSE_NETWORK_STATE_CHANGED":1002,
        "RIL_UNSOL_RESPONSE_NEW_SMS":1003,
        "RIL_UNSOL_RESPONSE_NEW_SMS_STATUS_REPORT":1004,
        "RIL_UNSOL_RESPONSE_NEW_SMS_ON_SIM":1005,
        "RIL_UNSOL_ON_USSD":1006,
        "RIL_UNSOL_ON_USSD_REQUEST":1007,
        "RIL_UNSOL_NITZ_TIME_RECEIVED":1008,
        "RIL_UNSOL_SIGNAL_STRENGTH":1009,
        "RIL_UNSOL_DATA_CALL_LIST_CHANGED":1010,
        "RIL_UNSOL_SUPP_SVC_NOTIFICATION":1011,
        "RIL_UNSOL_STK_SESSION_END":1012,
        "RIL_UNSOL_STK_PROACTIVE_COMMAND":1013,
        "RIL_UNSOL_STK_EVENT_NOTIFY":1014,
        "RIL_UNSOL_STK_CALL_SETUP":1015,
        "RIL_UNSOL_SIM_SMS_STORAGE_FULL":1016,
        "RIL_UNSOL_SIM_REFRESH":1017,
        "RIL_UNSOL_CALL_RING":1018,
        "RIL_UNSOL_RESPONSE_SIM_STATUS_CHANGED":1019,
        "RIL_UNSOL_RESPONSE_CDMA_NEW_SMS":1020,
        "RIL_UNSOL_RESPONSE_NEW_BROADCAST_SMS":1021,
        "RIL_UNSOL_CDMA_RUIM_SMS_STORAGE_FULL":1022,
        "RIL_UNSOL_RESTRICTED_STATE_CHANGED":1023,
        "RIL_UNSOL_ENTER_EMERGENCY_CALLBACK_MODE":1024,
        "RIL_UNSOL_CDMA_CALL_WAITING":1025,
        "RIL_UNSOL_CDMA_OTA_PROVISION_STATUS":1026,
        "RIL_UNSOL_CDMA_INFO_REC":1027,
        "RIL_UNSOL_OEM_HOOK_RAW":1028,
        "RIL_UNSOL_RINGBACK_TONE":1029,
        "RIL_UNSOL_RESEND_INCALL_MUTE":1030,
    }


    def __init__(self, command = None):
        Parcel.__init__(self)
        if command is not None:
            self.response_type = 0
            self.pid = CommandParcel.serial
            CommandParcel.serial = CommandParcel.serial + 1
            self.request_type = command
            print "%d: %s" % (self.pid, self.parcelToString())
        else:
            self.response_type = None
            self.request_type = None
            self.pid = None

    def prepareParcelForWrite(self):
        b = struct.pack("II", self.request_type, self.pid)
        return b + self._buffer        

    def parseParcel(self, packet_matcher):
        self.response_type = self.readInt()

        if self.response_type == 0:            
            self.pid = self.readInt()
            print "Response Type: %d Response To: %d" % (self.response_type, self.pid) 
            if self.pid in packet_matcher.keys():
                self.request_type = packet_matcher[self.pid].request_type
        else:
            self.request_type = self.readInt()

    def parcelToString(self):
        l = [x[0] for x in CommandParcel.REQUEST_TYPES.items() if x[1] == self.request_type]
        if len(l) == 0 and self.request_type is not None:
            return "CANNOT FIND %d" % (self.request_type)
        elif self.request_type is None:
            return "CONTINUATION OF %d" % (self.pid)
        return "%s" % l[0]

def main():
    has_init = 0
    p = Phone()
    p.open()
    try:
        p.receiveParcel()
        if has_init is 0:
            print "Initializing!"
            p.initializePhone()
            has_init = 1
        while 1:
            if not p.hasData():
                continue
            p.receiveParcel()
    except KeyboardInterrupt:
        print "Closing socket"
        p.close()
    return 0

if __name__ == '__main__':
    sys.exit(main())