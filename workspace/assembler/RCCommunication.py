import struct # for packing integers
import zmq

IDrunControlData = 1 
IDencoderData = 2

ID = {IDrunControlData:"Run Control Data:", IDencoderData:"Encoder Data:"}

class RCCommunication(object):
    """
    frame 0: message identifier
    frame 1: ControlStep 
    """
    ID = 99
    ControlStep = 0

    def __init__(self, ID=99,ControlStep=0):
        assert isinstance(ControlStep, int)
        assert isinstance(ID, int)
        
        self.ID = ID
        self.ControlStep = ControlStep

    def sendData(self, socket):
        """ """
        ID_string = struct.pack('!i', self.ID)
        ControlStep_string = struct.pack('!l', self.ControlStep)
        socket.send_multipart([ID_string, ControlStep_string])

    @classmethod
    def recvData(cls, socket):
        """123"""
        ID_string, ControlStep_string = socket.recv_multipart()
        ID = struct.unpack('!i', ID_string)[0]
        ControlStep = struct.unpack('!l',ControlStep_string)[0]

        return cls(ID=ID,ControlStep=ControlStep)

    def sendAck(self, socket):
        socket.send("OK")

    def recvAck(self, socket):
        msg = socket.recv()
        if msg == "OK":
            return msg
        else:
            return "Fail"

class LaserData(object):
    def __init__(self,laserid=-1,pos_rot=-8888.,pos_lin=-9999.,pos_att=-8888.,pos_iris=-9999.,
        time=0,count_trigger=-1,count_run=-1,count_laser=-1,
        pos_tomg_1_axis1=-66666,pos_tomg_1_axis2=-77777,pos_tomg_2_axis1=-88888,pos_tomg_2_axis2=-99999):
        
        self.laserid = laserid          # which laser system: 1 or 2
        self.pos_rot = pos_rot          # Position Rotary Heidenhain Encoder 
        self.pos_lin = pos_lin          # Position Linear Heidenhain Encoder 
        self.pos_att = pos_att          # Position Attenuator Watt Pilot 
        self.pos_iris = pos_iris        # Position Iris Standa 
        self.time = time                # System Time of Laser Server 
        self.count_trigger = count_trigger          # Trigger Counter by Heidenhain Encoder
        self.count_run = count_run                  # Run Counter of step in calibration run 
        self.count_laser = count_laser              # Number of pulses shot with UV laser
        self.pos_tomg_1_axis1 = pos_tomg_1_axis1    # Motorized Mirror Zaber T-OMG at box, axis 1
        self.pos_tomg_1_axis2 = pos_tomg_1_axis2    # Motorized Mirror Zaber T-OMG at box, axis 2
        self.pos_tomg_2_axis1 = pos_tomg_2_axis1    # Motorized Mirror Zaber T-OMG at flange, axis 1
        self.pos_tomg_2_axis2 = pos_tomg_2_axis2    # Motorized Mirror Zaber T-OMG at flange, axis 2

    def __str__(self):
        mstr = """DATA INFO
 Record Time: {time}\n Laser ID: {laserid}\n Rotary Position: {pos_rot}\n Linear Position: {pos_lin}\n Attenuator Position: {pos_att}
 Iris Position: {pos_iris}\n Encoder Trigger Count: {count_trigger}\n Run Control Counter: {count_run}\n Laser Shot Count: {count_laser}
 Laser Box Mirror Position: x={pos_tomg_1_axis1} y={pos_tomg_1_axis2}\n Feedtrough Mirror Position: x={pos_tomg_2_axis1} y={pos_tomg_2_axis2}\n""".format(
                    time=self.time,
                    laserid=self.laserid,
                    pos_rot=self.pos_rot,
                    pos_lin=self.pos_lin,
                    pos_att=self.pos_att,
                    pos_iris=self.pos_iris,
                    count_trigger=self.count_trigger,
                    count_run=self.count_run,
                    count_laser=self.count_laser,
                    pos_tomg_1_axis1=self.pos_tomg_1_axis1,
                    pos_tomg_1_axis2=self.pos_tomg_1_axis2,
                    pos_tomg_2_axis1=self.pos_tomg_2_axis1,
                    pos_tomg_2_axis2=self.pos_tomg_2_axis2
                    )
        return mstr

    def writeBinary(self,fileID):
        f = open(fileID, 'w')
        BinaryData = bytearray(self.laserid)
        f.write(BinaryData)

