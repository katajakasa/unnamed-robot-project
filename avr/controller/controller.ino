#include <Adafruit_PWMServoDriver.h>

int inApin[2] = {7, 4}; // INA: Clockwise input
int inBpin[2] = {8, 9}; // INB: Counter-clockwise input
int pwmpin[2] = {5, 6}; // PWM input
int cspin[2] = {2, 3};  // CS: Current sense ANALOG input
int enpin[2] = {0, 1};  // EN: Status of switches output (Analog pin)

#define SERVO_MIN  150
#define SERVO_MAX  600

#define MOTOR_CW   0
#define MOTOR_CCW  1
#define MOTOR_STOP 2

#define PACKET_SIZE 8

enum {
  BOARD_ARDUINO = 0,
  BOARD_MOTOR,
  BOARD_SERVO,
};

enum {
  ERRCODE_BAD_CMD = 0,
  ERRCODE_UNKNOWN_BOARD,
  ERRCODE_BAD_ARG1_RANGE,
  ERRCODE_BAD_ARG2_RANGE,
};

Adafruit_PWMServoDriver servo = Adafruit_PWMServoDriver();

void motorOff(int motor) {
  digitalWrite(inApin[motor], LOW);
  digitalWrite(inBpin[motor], LOW);
  analogWrite(pwmpin[motor], 0);
}

void motorOn(uint8_t motor, uint8_t dir, uint8_t pwm) {
  if(motor <= 1) {
    if(dir <= 1) {
      // Set inA[motor]
      if(dir == MOTOR_CW) {
        digitalWrite(inApin[motor], HIGH);
      } else {
        digitalWrite(inApin[motor], LOW);
      }

      // Set inB[motor]
      if(dir == MOTOR_CCW) {
        digitalWrite(inBpin[motor], HIGH);
      } else {
        digitalWrite(inBpin[motor], LOW);
      }

      analogWrite(pwmpin[motor], pwm);
    }
  }
}

/**
 * Write error to computer vial serial
 * Board:
 * 0 = Arduino itself
 * 1 = Motor controller
 * 2 = Servo controller
 * 
 * board: See above
 * dev: Device number on the board
 * type: Error type
 */
void writeError(uint8_t board, uint8_t dev, uint8_t type) {
  char out_buffer[PACKET_SIZE] = {0xbe, 0xef, board, dev, 0, 0, type, 0};
  Serial.write(out_buffer, PACKET_SIZE);
}

void writeSuccess(uint8_t board, uint8_t dev) {
  char out_buffer[PACKET_SIZE] = {0xbe, 0xef, board, dev, 1, 0, 0, 0};
  Serial.write(out_buffer, PACKET_SIZE);
}

void writeValue(uint8_t board, uint8_t dev, uint16_t arg1, uint16_t arg2) {
  char out_buffer[PACKET_SIZE] = {0xbe, 0xef, board, dev, lowByte(arg1), highByte(arg1), lowByte(arg2), highByte(arg2)};
  Serial.write(out_buffer, PACKET_SIZE);
}

void setup() {
  Serial.begin(9600);

  for(int i = 0; i < 2; i++) {
    pinMode(inApin[i], OUTPUT);
    pinMode(inBpin[i], OUTPUT);
    pinMode(pwmpin[i], OUTPUT);
  }

  // Set motors off on init
  motorOff(0);
  motorOff(1);

  // Servo controller init
  servo.begin();
  servo.setPWMFreq(60);

  // Write ok
  writeSuccess(0xFF, 0);
}

void loop() {
  if(Serial.available() >= 8) {
    char in_buffer[PACKET_SIZE];
    Serial.readBytes(in_buffer, PACKET_SIZE);
    if(in_buffer[0] == 0x13 && in_buffer[1] == 0x37) {
      uint8_t board = in_buffer[2];
      uint8_t device = in_buffer[3];
      uint16_t arg1 = word(in_buffer[5], in_buffer[4]);
      uint16_t arg2 = word(in_buffer[7], in_buffer[6]);
      switch(board) {
        case BOARD_ARDUINO:
          break;
        case BOARD_MOTOR:
          if(device == 100) {
            writeValue(BOARD_ARDUINO, device, analogRead(cspin[0]), analogRead(cspin[1]));
          } else if(device == 101) {
            writeValue(BOARD_ARDUINO, device, analogRead(enpin[0]), analogRead(enpin[1]));
          } else {
            if(arg1 >= 3) {
              writeError(board, device, ERRCODE_BAD_ARG1_RANGE);
            } else if(arg2 >= 256) {
              writeError(board, device, ERRCODE_BAD_ARG2_RANGE);
            } else {
              if(arg1 == MOTOR_STOP) {
                motorOff(device); // dev
              } else {
                motorOn(device, arg1, arg2); // dev, cw/ccw, pwm
              }
              writeSuccess(board, device);
            }
          }
          break;
        case BOARD_SERVO:
          if(arg1 >= SERVO_MIN && arg1 <= SERVO_MAX) {
            servo.setPWM(device, 0, arg1);
            writeSuccess(board, device);
          } else {
            writeError(board, device, ERRCODE_BAD_ARG1_RANGE);
          }
          break;
        default:
          writeError(board, device, ERRCODE_UNKNOWN_BOARD);
          break;
      }
    } else {
      writeError(BOARD_ARDUINO, 0, ERRCODE_BAD_CMD);
    }
  }

  delay(10);
}
