import time
from logic import hex_to_float
from serial_interface import get_imu_ave_data, get_IMUDat

def ReadIMUdataSet():
    Response = []
    get_imu_ave_data()
    time.sleep(0.1)
    #print(arduino.inWaiting())
    IMUDat = get_IMUDat()
    #print(IMUDat)
    IMUDatSplit = IMUDat.split(",")
    #print(len(IMUDatSplit))
    
    if len(IMUDatSplit)>8:

        Azimuth = hex_to_float((IMUDatSplit[2][2:]))
        Altitude = hex_to_float((IMUDatSplit[3][2:]))
        Yaw = hex_to_float((IMUDatSplit[4][2:]))

        AccelX = hex_to_float((IMUDatSplit[5][2:]))
        AccelY = hex_to_float((IMUDatSplit[6][2:]))
        AccelZ = hex_to_float((IMUDatSplit[7][2:]))
        commErrors = IMUDatSplit[8][2:]
        Response = [Azimuth, Altitude, Yaw, AccelX, AccelY, AccelZ,commErrors]

    return Response