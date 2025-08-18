import serial

arduino = serial.Serial(port='COM3', baudrate=115200, timeout=.2)

def flush():
    arduino.flushInput()

def write_to_serial_3_parms(code, motor, parameter): # "parameter" is a general purpose input whose meaning depends on the context in which the function is called.
    # Flush before and after to prevent garbage
    flush()
    arduino.write(str.encode(str(code) + "," + str(motor) + "," + str(parameter) + ";"))
    flush()

def write_to_serial_2_parms(code, parameter):
    # Flush before and after to prevent garbage\
    flush()
    arduino.write(str.encode(str(code) + "," + str(parameter) + ";"))
    flush()

def get_imu_ave_data():
    write_to_serial_2_parms(27, "") # GET_IMU_AVE_DATA

def get_IMUDat():
    return str(arduino.read(arduino.inWaiting()))