# -*- coding: utf-8 -*-

import serial
import logging
import struct

log = logging.getLogger(__name__)

class Protocol(object):
    BOARD_ARDUINO = 0
    BOARD_MOTOR = 1
    BOARD_SERVO = 2

    MAGIC_RECV_A = 0xbe
    MAGIC_RECV_B = 0xef
    MAGIC_SEND_A = 0x13
    MAGIC_SEND_B = 0x37

    MOTOR_SIDE_LEFT = 0
    MOTOR_SIDE_RIGHT = 1

    MOTOR_DIR_FWD = 0
    MOTOR_DIR_BWD = 1
    MOTOR_DIR_STOP = 2

    def __init__(self, device):
        self.device = device
        self.port = serial.Serial(self.device, 9600)
        self.buffer = []
        log.info('Opened serial device %s', self.port.name)

    def writePacket(self, board, device, arg1, arg2):
        self.port.write(struct.pack(
            'BBBBHH',
            self.MAGIC_SEND_A,
            self.MAGIC_SEND_B,
            board,
            device,
            arg1,
            arg2
        ))

    def setMotor(self, motor, direction, value):
        self.writePacket(self.BOARD_MOTOR, motor, direction, value)
        log.debug('Setting motor %d to direction %d with speed %d', motor, direction, value)

    def setServo(self, servo, value):
        self.writePacket(self.BOARD_SERVO, servo, value, 0);
        log.debug('Setting servo %d to position %d', servo, value)

    def getCS(self):
        self.writePacket(self.BOARD_MOTOR, 100, 0, 0);
        log.debug('Get CS')

    def getEN(self):
        self.writePacket(self.BOARD_MOTOR, 101, 0, 0);
        log.debug('Get EN')

    def handle(self):
        self.port.flush()
        while self.port.in_waiting >= 8:
            m = self.port.read(8)
            magic_a, magic_b, board, device, arg1, arg2 = struct.unpack('BBBBHH', m)
            if magic_a == self.MAGIC_RECV_A and magic_b == self.MAGIC_RECV_B:
                data = (board, device, arg1, arg2)
                self.buffer.insert(0, data)

    def read(self):
        try:
            return self.buffer.pop()
        except IndexError:
            return None

    def close(self):
        self.port.close()
        log.info('Closed serial device %s', self.port.name)
