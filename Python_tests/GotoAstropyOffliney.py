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
iers.conf.auto_download = False
iers.conf.auto_max_age = None


loc = EarthLocation(lat=33.1424005*u.deg, lon=-96.8599673*u.deg, height=0*u.m)


arduino = serial.Serial(port='COM3', baudrate=115200, timeout=.2)


#PosAdjustment = 0.01

PosAdjustment = 0.05
OffsetAltitude = 0
OffsettAzimuth = 0

LastAltitude = 0
LastAzimuth = 0
TargetObject = ''

MatrixSize = 5
step_size = .021



Objects = ['moon', 'mars', 'jupiter', 'saturn', 'mercury', 'venus', 'sun']


def DelayAndCheckForBackspace(delay_in_seconds):
    BackspaceDetected = False
    
    for secs in range(0,int(delay_in_seconds * 100)):
        time.sleep(1/100)
        if secs % 100 == 0:
            print('.')
        if keyboard.is_pressed('backspace'):
            print ('backspace detected. Exiting tracking...')
            BackspaceDetected = True
            break

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

    global MatrixSize
    global step_size
    StartingAlt = CurrentAltitude
    StartingAz = CurrentAzimuth
    
    matrix = BuildScanArray(MatrixSize, step_size)
    print ('Press Backspace to exit',range(MatrixSize))
    for row in range(0,MatrixSize):
        BackspaceDetected = False
        if row%2 == 0: # even row. First row is even
            for col in range(0,MatrixSize):
                NextAzimuth = CurrentAzimuth + matrix[row][col][0]
                NextAltitude = CurrentAltitude + matrix[row][col][1]
                #print(row,col,"1(" + str(NextAzimuth) + ", " + str(NextAltitude) + ")")
                go_to(NextAltitude, NextAzimuth)
                #BackspaceDetected = DelayAndCheckForBackspace(step_size * 15)
                Delay = 0.75+abs(CurrentAzimuth-NextAzimuth)/2.0
                BackspaceDetected = DelayAndCheckForBackspace(Delay)
                print(row,col,"1(" + str(CurrentAzimuth) + ", " + str(NextAzimuth) + ", " + str(Delay)+ ")")
                if BackspaceDetected:
                    break
        else:        # odd row scan
            for col in range(0,MatrixSize):
                NextAzimuth = CurrentAzimuth + matrix[row][MatrixSize-1-col][0]
                NextAltitude = CurrentAltitude + matrix[row][MatrixSize-1-col][1]
                #print(row,col,"2(" + str(NextAzimuth) + ", " + str(NextAltitude) + ")")
                go_to(NextAltitude, NextAzimuth)
                Delay = 0.75+abs(CurrentAzimuth-NextAzimuth)/2.0
                BackspaceDetected = DelayAndCheckForBackspace(Delay)
                print(row,col,"1(" + str(CurrentAzimuth) + ", " + str(NextAzimuth) + ", " + str(Delay)+ ")")
                if BackspaceDetected:
                    break
        if BackspaceDetected:
            break
    if BackspaceDetected == False:
        go_to(StartingAlt, StartingAz) # If we are here, means the scan did not yield any desired results Return to starting point

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


print ("moon coordinates",get_coords('moon'))

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
    
def print_options():
    print ("moon coordinates",get_coords('moon'))
    print("What object would you like to track?")
    print("or press 'h' to Home motors,")
    print("or press 'g' to Go to object,")
    print("or press 't' to Track object,")
    print("or press 'j' to set Joystick control,")
    print("press 'f' to switch to Fast mode")
    print("press 's' to switch to Slow mode")
    print("press 'a' to perform a scAn")
    print("or press 'm' to enable Mirror mode")
    print("or press 'n' to disable mirror mode")
    print("'e' to exit")

def is_float(string):
    try:
        float(string)
        return True
    except ValueError:
        return False
    
def is_int(string):
    try:
        float(string)
        return True
    except ValueError:
        return False


if 1:
    if 1:
        CurrentAltitude =  0.0
        CurrentAzimuth = 0.0
        JoystickIsToggled = False
        OverRideControllerCoordinates(0,0)
        print_options()
        while True:
            arduino.flushInput()

            #print()
            first = True
            arduino.flushInput()
            if keyboard.is_pressed("h"):
                keyboard.press('backspace')
                print("Re-centering motors. Please wait...")
                go_home()
                KeyInput = input("Press Enter To Return To Menu")
                print_options()
                
            elif keyboard.is_pressed("f"):
                keyboard.press('backspace')
                fast = True
                print("Switching to fast mode...")
                arduino.write(str.encode("20,1;"))
                KeyInput = input("Press Enter To Return To Menu")
                print_options()
             
            elif keyboard.is_pressed("s"):
                keyboard.press('backspace')
                fast = False
                print("Switching to slow mode...")
                arduino.write(str.encode("20,0;"))
                KeyInput = input("Press Enter To Return To Menu")
                print_options()
               
            elif keyboard.is_pressed("m"):
                keyboard.press('backspace')
                mirror = True
                print("Enabling mirror mode...")
                arduino.write(str.encode("21,1;"))
                KeyInput = input("Press Enter To Return To Menu")
                print_options()

            elif keyboard.is_pressed("n"):
                keyboard.press('backspace')
                mirror = False
                print("Disabling mirror mode...")
                arduino.write(str.encode("21,0;"))
                KeyInput = input("Press Enter To Return To Menu")
                print_options()
                
            elif keyboard.is_pressed("e"):
                print("Exiting program.") 
                
                break
            
            elif keyboard.is_pressed("a"):
                keyboard.press('backspace')
  
                input_is_valid = False
                while not input_is_valid:
                    MatrixSize_input = input("Enter scan matrix size (ODD number): ")
                    if is_int(MatrixSize_input):
                        MatrixSize_input = int(MatrixSize_input)
                    else:
                        continue
                    if MatrixSize_input > 1 and MatrixSize_input < 100 and MatrixSize_input % 2 == 1:
                        MatrixSize = MatrixSize_input
                        input_is_valid = True
                    print()
                print("You chose scan matrix size " + str(MatrixSize))
                input_is_valid = False
                while not input_is_valid:
                    step_size_input = input("Enter scan step size: ")
                    if is_float(step_size_input):
                        step_size_input = float(step_size_input)
                    else:
                        continue
                    if step_size_input > 0.0 and step_size_input < 5.0:
                        step_size = step_size_input
                        input_is_valid = True
                        PosAdjustment = step_size # Hack to configure this value. TODO. Add separate control input for this in the next rev
                    print()
                print("You chose step size " + str(step_size))

                print("Scanning for " + TargetObject   + " (press backspace to stop Scan)...")
                Scan_For_Object()

                KeyInput = input("Press Enter To Return To Menu")
                print_options()
                    
            elif keyboard.is_pressed("t"):
                keyboard.press('backspace')
                if TargetObject != "":
                    print("Tracking " + TargetObject   + " (press backspace to stop tracking)...")
                    track_object(TargetObject)
                    #tracking_thread = threading.Thread(target=track_object, args=(TargetObject,))
                    #tracking_thread.start()
                    #tracking_thread.join()
                    #break
                else:
                    print("PLease go to a planet first")
                
                KeyInput = input("\0Press Enter To Return To Menu")
                print_options()
                
            
            elif keyboard.is_pressed("g"):
                keyboard.press('backspace')
                print ('Select from this list', Objects)
                print('Zero indexed')

                ObjectQuery = input("")
                
                if ObjectQuery != "" and len(ObjectQuery) == 1:
                    TargetObject = (Objects[int(ObjectQuery)])
                    coords = get_coords(TargetObject)
                    if coords[0] < 0:
                        print("Sorry, the requested object is not currently visible")
                        continue
                    LastAltitude = coords[0]+OffsetAltitude
                    CurrentAltitude = LastAltitude
                    
                    LastAzimuth = coords[1]+OffsettAzimuth
                    CurrentAzimuth = LastAzimuth
                    go_to(LastAltitude, LastAzimuth)
                    
                print ("ObjectQuery", ObjectQuery)
                KeyInput = input("Press Enter To Return To Menu")
                print_options()
                    
            elif keyboard.is_pressed('j'):
                keyboard.press('backspace')
                
                JSselection  = input("Enter 1 for joystick control or 2 for enabling joystick")
                if is_int(JSselection):
                    if JSselection == "1":
                        if JoystickIsToggled:
                            print("Setting joystick to control focus")
                        else:
                            print("Setting joystick to control Alt/Az")
                        JoystickIsToggled = not JoystickIsToggled
                        arduino.write(str.encode("40,1;"))
                        
                    elif JSselection == "2":
                        arduino.write(str.encode("41,1;"))
                        print("joystick Enabled")
                        
                
                KeyInput = input("Press Enter To Return To Menu")
                print_options()
                
            elif keyboard.is_pressed("c"):
                keyboard.press('backspace')
                print(OffsetAltitude, OffsettAzimuth,CurrentAltitude, CurrentAzimuth, LastAltitude, CurrentAzimuth )
                if CurrentAltitude != 0:
                    OffsetAltitude = CurrentAltitude - LastAltitude
                        
                if CurrentAzimuth != 0:   
                    OffsettAzimuth = CurrentAzimuth - LastAzimuth
                        
                print("Calibration OffsetAltitude,OffsettAzimuth ",OffsetAltitude, OffsettAzimuth)
                KeyInput = input("\0Press Enter To Return To Menu")
                print_options()

            elif keyboard.is_pressed("right"):
                CurrentAzimuth = CurrentAzimuth+PosAdjustment
                send_coords(CurrentAltitude, CurrentAzimuth)
                print(CurrentAltitude - LastAltitude, CurrentAzimuth - LastAzimuth)
                Delay = 0.3+abs(CurrentAzimuth-LastAzimuth)/2.0
                time.sleep(Delay)
                
            elif keyboard.is_pressed("left"):
                CurrentAzimuth = CurrentAzimuth-PosAdjustment
                send_coords(CurrentAltitude, CurrentAzimuth)
                print(CurrentAltitude - LastAltitude, CurrentAzimuth - LastAzimuth)
                Delay = 0.3+abs(CurrentAzimuth-LastAzimuth)/2.0
                time.sleep(Delay)
                
            elif keyboard.is_pressed("up"):
                if mirror == False:
                    CurrentAltitude = CurrentAltitude+PosAdjustment
                else:
                    CurrentAltitude = CurrentAltitude-PosAdjustment
                send_coords(CurrentAltitude, CurrentAzimuth)
                print(CurrentAltitude - LastAltitude, CurrentAzimuth - LastAzimuth)
                Delay = 0.3+abs(CurrentAzimuth-LastAzimuth)/2.0
                time.sleep(Delay)
                
            elif keyboard.is_pressed("down"):

                if mirror == False:
                    CurrentAltitude = CurrentAltitude-PosAdjustment
                else:
                    CurrentAltitude = CurrentAltitude+PosAdjustment
                    
                send_coords(CurrentAltitude, CurrentAzimuth)
                print(CurrentAltitude - LastAltitude, CurrentAzimuth - LastAzimuth)
                Delay = 0.3+abs(CurrentAzimuth-LastAzimuth)/2.0
                time.sleep(Delay)
                
            
            
   


