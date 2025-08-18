import keyboard
import time
import motor_control
from serial_interface import flush
from user_interface import print_options
from logic import is_int, is_float
from astronomy_data import get_coords, Objects
from astropy.utils import iers
from system_operations import Scan_For_Object, track_object
from system_operations import OffsetAltitude, OffsetAzimuth, go_to

REQUEST_POS_NO_MOVE     = 12

# force astropy to work OFFLINE
iers.conf.auto_download = False
iers.conf.auto_max_age = None

PosAdjustment = 0.05

LastAltitude = 0
LastAzimuth = 0
TargetObject = ''

MatrixSize = 5
step_size = .021

fast = False
mirror = False   

def main():
    global OffsetAltitude
    global OffsetAzimuth
    
    CurrentAltitude =  0.0
    CurrentAzimuth = 0.0
    JoystickIsToggled = False
    motor_control.OverRideControllerCoordinates(0,0)
    print_options()
    while True:
        flush()
        first = True
        if keyboard.is_pressed("h"):
            keyboard.press('backspace')
            print("Re-centering motors. Please wait...")
            motor_control.go_home()
            KeyInput = input("Press Enter To Return To Menu")
            print_options()
            
        elif keyboard.is_pressed("f"):
            keyboard.press('backspace')
            fast = True
            print("Switching to fast mode...")
            motor_control.set_js_speed(1)
            KeyInput = input("Press Enter To Return To Menu")
            print_options()
            
        elif keyboard.is_pressed("s"):
            keyboard.press('backspace')
            fast = False
            print("Switching to slow mode...")
            motor_control.set_js_speed(0)
            KeyInput = input("Press Enter To Return To Menu")
            print_options()
            
        elif keyboard.is_pressed("m"):
            keyboard.press('backspace')
            mirror = True
            print("Enabling mirror mode...")
            motor_control.set_js_mirror(1)
            KeyInput = input("Press Enter To Return To Menu")
            print_options()

        elif keyboard.is_pressed("n"):
            keyboard.press('backspace')
            mirror = False
            print("Disabling mirror mode...")
            motor_control.set_js_mirror(0)
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
                
                LastAzimuth = coords[1]+OffsetAzimuth
                CurrentAzimuth = LastAzimuth
                go_to(LastAltitude, LastAzimuth)
                
            print ("ObjectQuery", ObjectQuery)
            KeyInput = input("Press Enter To Return To Menu")
            print_options()
                
        elif keyboard.is_pressed('j'):
            keyboard.press('backspace')
            
            JSselection  = input("Enter 1 for joystick to control focus or 2 for enabling joystick to control Alt/Az")
            if is_int(JSselection):
                if JSselection == "1":
                    if JoystickIsToggled:
                        print("Setting joystick to control focus")
                    else:
                        print("Setting joystick to control Alt/Az")
                    JoystickIsToggled = not JoystickIsToggled
                    motor_control.set_js_focus()
                    
                elif JSselection == "2":
                    motor_control.set_js_alt_az()
                    print("joystick Enabled")
                    
            
            KeyInput = input("Press Enter To Return To Menu")
            print_options()
            
        elif keyboard.is_pressed("c"):
            keyboard.press('backspace')
            print(OffsetAltitude, OffsetAzimuth,CurrentAltitude, CurrentAzimuth, LastAltitude, CurrentAzimuth )
            if CurrentAltitude != 0:
                OffsetAltitude = CurrentAltitude - LastAltitude
                    
            if CurrentAzimuth != 0:   
                OffsetAzimuth = CurrentAzimuth - LastAzimuth
                    
            print("Calibration OffsetAltitude,OffsetAzimuth ",OffsetAltitude, OffsetAzimuth)
            KeyInput = input("\0Press Enter To Return To Menu")
            print_options()

        elif keyboard.is_pressed("right"):
            CurrentAzimuth = CurrentAzimuth+PosAdjustment
            go_to(CurrentAltitude, CurrentAzimuth)
            print(CurrentAltitude - LastAltitude, CurrentAzimuth - LastAzimuth)
            Delay = 0.3+abs(CurrentAzimuth-LastAzimuth)/2.0
            time.sleep(Delay)
            
        elif keyboard.is_pressed("left"):
            CurrentAzimuth = CurrentAzimuth-PosAdjustment
            go_to(CurrentAltitude, CurrentAzimuth)
            print(CurrentAltitude - LastAltitude, CurrentAzimuth - LastAzimuth)
            Delay = 0.3+abs(CurrentAzimuth-LastAzimuth)/2.0
            time.sleep(Delay)
            
        elif keyboard.is_pressed("up"):
            if mirror == False:
                CurrentAltitude = CurrentAltitude+PosAdjustment
            else:
                CurrentAltitude = CurrentAltitude-PosAdjustment
            go_to(CurrentAltitude, CurrentAzimuth)
            print(CurrentAltitude - LastAltitude, CurrentAzimuth - LastAzimuth)
            Delay = 0.3+abs(CurrentAzimuth-LastAzimuth)/2.0
            time.sleep(Delay)
            
        elif keyboard.is_pressed("down"):

            if mirror == False:
                CurrentAltitude = CurrentAltitude-PosAdjustment
            else:
                CurrentAltitude = CurrentAltitude+PosAdjustment
                
            go_to(CurrentAltitude, CurrentAzimuth)
            print(CurrentAltitude - LastAltitude, CurrentAzimuth - LastAzimuth)
            Delay = 0.3+abs(CurrentAzimuth-LastAzimuth)/2.0
            time.sleep(Delay)

if __name__ == "__main__":
    main()