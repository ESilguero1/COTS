import keyboard
import time;
from motor_control import set_pos
from logic import BuildScanArray
from user_interface import DelayAndCheckForBackspace
from astronomy_data import get_coords

OffsetAltitude = 0
OffsetAzimuth = 0

def go_to(alt, az):
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
        set_pos(x_ticks, y_ticks)

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
        
        LastAzimuth = coords[1]+OffsetAzimuth
        CurrentAzimuth = LastAzimuth
        go_to(LastAltitude, LastAzimuth)
        
         #for seconds in range(60):
        for secs in range(0,10):
            time.sleep(1)
            print('.')
            if keyboard.is_pressed('backspace'):
                print ('backspace detected. Exiting tracking...')
                break
                return  