import keyboard
import time;

def print_options():
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