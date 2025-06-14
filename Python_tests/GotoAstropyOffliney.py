import requests
import json
import serial
import keyboard
import time
import threading
from astropy.time import Time
from astropy.utils import iers
from astropy.coordinates import solar_system_ephemeris, EarthLocation
from astropy.coordinates import get_body, get_moon
from astropy.coordinates import SkyCoord, EarthLocation, AltAz
from astropy import units as u
import time;
import sys
import struct
import string


REQUEST_POS_NO_MOVE     = 12

# force astropy to work OFFLINE
#iers.conf.auto_download = False
#iers.conf.auto_max_age = None


loc = EarthLocation(lat=33.1424005*u.deg, lon=-96.8599673*u.deg, height=0*u.m)


arduino = serial.Serial(port='COM3', baudrate=115200, timeout=.2)


#PosAdjustment = 0.01

PosAdjustment = 0.02
OffsetAltitude = 0
OffsettAzimuth = 0

LastAltitude = 0
LastAzimuth = 0
TargetObject = ''

Objects = ['moon', 'mars', 'jupiter', 'saturn', 'mercury', 'venus', 'sun']


def DelayAndCheckForBackspace(delay_in_seconds):
    BackspaceDetected = False
    
    for secs in range(0,delay_in_seconds):
        time.sleep(1)
        print('.')
        if keyboard.is_pressed('backspace'):
            print ('backspace detected. Exiting tracking...')
            break
            BackspaceDetected = True

    return BackspaceDetected
            
def send_coords(alt, az):

        
    #print("send_coords",str(alt) + ", " + str(az))
    if az > 180.0: # If on left hand side of polar coordiantes, simply mirro azimuth to prevent cords to wrap all the way around
        az = -180.0 + (az-180.0)

    if alt > 90.0 :
        print("Altitude range error")
        
    else:
        az = -az
        y_ticks = int(alt * 25600)
        x_ticks = int(az * 25600)
        print('alt sent', alt, y_ticks)
        print('az sent', az, x_ticks)
        arduino.write(str.encode("32,1," + str(y_ticks) + ";"))
        arduino.write(str.encode("32,0," + str(x_ticks) + ";"))
  
def OverRideControllerCoordinates(alt, az):
    print('OverRideControllerCoordinates:',str(alt) + ", " + str(az))
    #Uwrap coordinates. Remap from +=180 to 360. e.g. -20 = 340
    if az < 0.0:
        az = 360.0 + az
    if alt < 0.0 :
        alt = 360.0 + alt
 
    y_ticks = int(alt * 25600)
    x_ticks = int(az * 25600)
    print('alt OR', alt, y_ticks)
    print('az OR', az, x_ticks)
    arduino.write(str.encode("12,1," + str(y_ticks) + ";")) 
    arduino.write(str.encode("12,0," + str(x_ticks) + ";"))
  
        
        
def get_coords(objectString):
    t = Time.now()
    with solar_system_ephemeris.set('jpl'):
      Object = get_body(objectString, t, loc)

    altazframe = AltAz(obstime=t, location=loc, pressure=0)
    Objectaz=Object.transform_to(altazframe)

    #print(Objectaz.alt.degree,Objectaz.az.degree)
    return [Objectaz.alt.degree,Objectaz.az.degree]

def go_home():
    arduino.write(str.encode("32,1,0;"))
    arduino.write(str.encode("32,0,0;"))

def go_to(Altitude, Azimuth):
    send_coords(Altitude, Azimuth)

def track_object(target_object):
    global LastAltitude
    global CurrentAltitude
    global LastAzimuth
    global CurrentAzimuth

    print ('Press Backspace to exit')
    while not keyboard.is_pressed('backspace'):
        
        coords = get_coords(target_object)
        LastAltitude = coords[0]+OffsetAltitude
        CurrentAltitude = LastAltitude
        
        LastAzimuth = coords[1]+OffsettAzimuth
        CurrentAzimuth = LastAzimuth
        go_to(LastAltitude, LastAzimuth)
        
         #for seconds in range(60):
        for secs in range(0,10):
            time.sleep(1)
            print('.')
            if keyboard.is_pressed('backspace'):
                print ('backspace detected. Exiting tracking...')
            #arduino.write(str.encode("42;"))
                break
                return

def BuildScanArray(MatrixSize, step_size):
    #Elevation 
    Rows = MatrixSize
    row_scan_value = 0

    #Azimuth
    Columns = MatrixSize
    col_scan_value = 0

    matrix = []

    for row in range(Rows):
        matrix.append([])
        for column in range(Columns):
            row_scan_value = (-(Rows//2) + row) * step_size
            col_scan_value = (-(Columns//2) + column) * step_size

            matrix[row].append((col_scan_value, row_scan_value))
            
        print (matrix[row])

    return matrix            


def Scan_For_Object():
    global CurrentAltitude
    global CurrentAzimuth

    MatrixSize = 5
    step_size = 0.021

    matrix = BuildScanArray(MatrixSize, step_size)
    print ('Press Backspace to exit')
        
    while not keyboard.is_pressed('backspace'):

        for element in range(MatrixSize):
            if element%2 == 0: # even row 
                NextAzimuth = CurrentAzimuth + matrix[element][0][0]
                NextAltitude = CurrentAltitude + matrix[element][0][1]
                print("(" + str(NextAzimuth) + ", " + str(NextAltitude) + ")")
                go_to(NextAltitude, NextAzimuth)
                BackspaceDetected = DelayAndCheckForBackspace(MatrixSize*2)
                if BackspaceDetected == False:
                    NextAzimuth = CurrentAzimuth + matrix[element][-1][0]
                    NextAltitude = CurrentAltitude + matrix[element][-1][1]
                    print("(" + str(NextAzimuth) + ", " + str(NextAltitude) + ")")
                    go_to(NextAltitude, NextAzimuth)
                    BackspaceDetected = DelayAndCheckForBackspace(MatrixSize*2)

            else:        # odd row scan
                NextAzimuth = CurrentAzimuth + matrix[element][-1][0]
                NextAltitude = CurrentAltitude + matrix[element][-1][1]
                print("(" + str(NextAzimuth) + ", " + str(NextAltitude) + ")")
                go_to(NextAltitude, NextAzimuth)
                BackspaceDetected = DelayAndCheckForBackspace(MatrixSize*2)
                if BackspaceDetected == False:
                    NextAzimuth = CurrentAzimuth + matrix[element][0][0]
                    NextAltitude = CurrentAltitude + matrix[element][0][1]
                    print("(" + str(NextAzimuth) + ", " + str(NextAltitude) + ")")
                    go_to(NextAltitude, NextAzimuth)
                    BackspaceDetected = DelayAndCheckForBackspace(MatrixSize*2)


def hex_to_float(hex_string):
    try:
        hex_string = hex_string.zfill(8)
        
        # Remove '0x' prefix and any leading/trailing spaces, then convert to bytes
        hex_string = hex_string.replace("0x", "").strip()
        byte_data = bytes.fromhex(hex_string)

        # Unpack the byte data as a float (assuming big-endian format)
        float_value = struct.unpack('>f', byte_data)[0]
        return float_value
    
    except ValueError as e:
        return f"Error: Invalid hex string - {e}"
    except struct.error as e:
        return f"Error: Could not unpack hex data - {e}"

def ReadIMUdataSet():
    Response = []
    arduino.write(str.encode("27;")) # GET_IMU_AVE_DATA
    time.sleep(0.1)
    #print(arduino.inWaiting())
    IMUDat= str(arduino.read(arduino.inWaiting()))
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



fast = False
mirror = False
arduino.flushInput()

##for sec in range(0,10):
##    time.sleep(0.1)
##    print(".", end="")
##    
arduino.flushInput()
##
arduino.write(str.encode("20,0;")) # SET_JS_SLOW_FAST
arduino.write(str.encode("21,0;")) # SET_JS_MIRROR_MODE
##
##
##
##for sec in range(0,10):
##    time.sleep(0.1)
##    print(".", end="")
##
arduino.flushInput()
##
##
##
##for sec in range(0,20):
##    IMUData = ReadIMUdataSet()
##    for e in range(0,len(IMUData)):
##        try:
##            print (round(float(IMUData[e]),3),"    ", end="")
##        except:
##            pass
##
##        
##    print("")
##    time.sleep(0.1)
##    print(",", end="")
##    
    


if 1:
    if 1:
        CurrentAltitude =  0.0
        CurrentAzimuth = 0.0
##        print ("IMUData",CurrentAltitude, CurrentAzimuth)
        #OverRideControllerCoordinates(CurrentAltitude,CurrentAzimuth)

##        CurrentAltitude =  CurrentAltitude-0.02
##        
##        send_coords(CurrentAltitude,CurrentAzimuth)
##        time.sleep(0.3)
##        CurrentAltitude =  CurrentAltitude-0.02
##        
##        send_coords(CurrentAltitude,CurrentAzimuth)
        

        print ("moon coordinates",get_coords('moon'))

            #arduino.write(str.encode("40,1;"))
        print("What object would you like to track?")
        print("or press 'h' to re-center motors,")
        print("or press 'g' to go to object,")
        print("or press 't' to track object,")
        print("or press 'j' to enable joystick,")
        print("press 'f' to switch to fast mode")
        print("press 's' to switch to slow mode")
        print("press 'a' to perform a scAn")
        print("or press 'm' to enable mirror mode")
        print("or press 'n' to disable mirror mode")
        print("'e' to exit")

                
        while True:
            arduino.flushInput()

            #print()
            first = True
            arduino.flushInput()
            if keyboard.is_pressed("h"):
                print("Re-centering motors. Please wait...")
                go_home()
                KeyInput = input("Press Enter")
                
            elif keyboard.is_pressed("f"):
                fast = True
                print("Switching to fast mode...")
                arduino.write(str.encode("20,1;"))
                KeyInput = input("Press Enter")
             
            elif keyboard.is_pressed("s"):
                fast = False
                print("Switching to slow mode...")
                arduino.write(str.encode("20,0;"))
                KeyInput = input("Press Enter")
               
            elif keyboard.is_pressed("m"):
                mirror = True
                print("Enabling mirror mode...")
                arduino.write(str.encode("21,1;"))
                KeyInput = input("Press Enter")

            elif keyboard.is_pressed("n"):
                mirror = False
                print("Disabling mirror mode...")
                arduino.write(str.encode("21,0;"))
                KeyInput = input("Press Enter")
                
            elif keyboard.is_pressed("e"):
                print("Exiting program.") 
                
                break
            
            elif keyboard.is_pressed("a"):
                arduino.write(str.encode("41;")) # disable joystick during Scan
                if TargetObject != "":
                    print("Scanning for " + TargetObject   + " (press backspace to stop Scan)...")
                    Scan_For_Object()

                else:
                    print("PLease go to a planet first")
                KeyInput = input("Press Enter")
                    
            elif keyboard.is_pressed("t"):
                arduino.write(str.encode("41;")) # disable joystick during tracking
                if TargetObject != "":
                    print("Tracking " + TargetObject   + " (press backspace to stop tracking)...")
                    track_object(TargetObject)
                    #tracking_thread = threading.Thread(target=track_object, args=(TargetObject,))
                    #tracking_thread.start()
                    #tracking_thread.join()
                    #break
                else:
                    print("PLease go to a planet first")
                
                KeyInput = input("Press Enter")
                
            
            elif keyboard.is_pressed("g"):
                keyboard.press('backspace')
                print ('Select from this list', Objects)
                print('Zero indexed')

                ObjectQuery = input("")
                
                if ObjectQuery != "" and len(ObjectQuery) == 1:
                    TargetObject = (Objects[int(ObjectQuery)])
                    coords = get_coords(TargetObject)
                    LastAltitude = coords[0]+OffsetAltitude
                    CurrentAltitude = LastAltitude
                    
                    LastAzimuth = coords[1]+OffsettAzimuth
                    CurrentAzimuth = LastAzimuth
                    go_to(LastAltitude, LastAzimuth)
                    
                print ("ObjectQuery", ObjectQuery)
                KeyInput = input("Press Enter")
                    
            elif keyboard.is_pressed("j"):
                print("Enabled Joystick...")
                arduino.write(str.encode("40,1;"))
                KeyInput = input("Press Enter")
                
            elif keyboard.is_pressed("c"):
                print(OffsetAltitude, OffsettAzimuth,CurrentAltitude, CurrentAzimuth, LastAltitude, CurrentAzimuth )
                if CurrentAltitude != 0:
                    OffsetAltitude = CurrentAltitude - LastAltitude
                        
                if CurrentAzimuth != 0:   
                    OffsettAzimuth = CurrentAzimuth - LastAzimuth
                        
                print("Calibration OffsetAltitude,OffsettAzimuth ",OffsetAltitude, OffsettAzimuth)
                KeyInput = input("Press Enter")

            elif keyboard.is_pressed("right"):
                CurrentAzimuth = CurrentAzimuth+PosAdjustment
                send_coords(CurrentAltitude, CurrentAzimuth)
                print(CurrentAltitude - LastAltitude, CurrentAzimuth - LastAzimuth)
            elif keyboard.is_pressed("left"):
                CurrentAzimuth = CurrentAzimuth-PosAdjustment
                send_coords(CurrentAltitude, CurrentAzimuth)
                print(CurrentAltitude - LastAltitude, CurrentAzimuth - LastAzimuth)
                
            elif keyboard.is_pressed("up"):
                if mirror == False:
                    CurrentAltitude = CurrentAltitude+PosAdjustment
                else:
                    CurrentAltitude = CurrentAltitude-PosAdjustment
                send_coords(CurrentAltitude, CurrentAzimuth)
                print(CurrentAltitude - LastAltitude, CurrentAzimuth - LastAzimuth)
                
            elif keyboard.is_pressed("down"):

                if mirror == False:
                    CurrentAltitude = CurrentAltitude-PosAdjustment
                else:
                    CurrentAltitude = CurrentAltitude+PosAdjustment
                    
                send_coords(CurrentAltitude, CurrentAzimuth)
                print(CurrentAltitude - LastAltitude, CurrentAzimuth - LastAzimuth)
                
            time.sleep(PosAdjustment*15.0)
            
   


