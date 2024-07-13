import requests
import json
import serial
import keyboard
import time

arduino = serial.Serial(port='/dev/cu.usbmodem14101',   baudrate=115200, timeout=.1)

def send_coords(alt, az):
    print(str(alt) + ", " + str(az))
    if az >= 181:
        az = -180 + (az-180)
    if alt >= 91:
        print("error")
    else:
        az = -az
        print(str(alt) + ", " + str(az))
        y_ticks = int(alt * 25600)
        x_ticks = int(az * 25600)
        arduino.write(str.encode("32,1," + str(y_ticks) + ";"))
        arduino.write(str.encode("32,0," +  str(x_ticks) + ";"))

def go_home():
    arduino.write(str.encode("32,1,0;"))
    arduino.write(str.encode("32,0,0;"))

while True:
    query = input("What object would you like to track?\n(or enter 'h' to recenter motors, \n'e' to exit, \n'f' to switch to fast mode, \n's' to switch to slow mode, \n'm' to enable mirror mode,  or  \n'n' to disable mirror mode)\n")
    first = True
    arduino.flushInput()
    if query == "h":
        print("Re-centering motors. Please wait...")
        go_home()
    elif query == "f":
        print("Switching to fast mode...")
        arduino.write(str.encode("20,1;"))
        time.sleep(2)
    elif query ==  "s":
        print("Switching to slow mode...")
        arduino.write(str.encode("20,0;"))
        time.sleep(2)
    elif query == "m":
        print("Enabling mirror mode...")
        arduino.write(str.encode("21,1;"))
        time.sleep(2)
    elif query == "n":
        print("Disabling mirror mode...")
        arduino.write(str.encode("21,0;"))
        time.sleep(2)
    elif query == "e":
        print("Exiting program.")
        break
    else:
        print("Tracking " + query + " (press backspace to stop tracking)...")
        while True:
            response = requests.get("http://localhost:8090/api/objects/info?name=" + query + "&format=json").json()
            send_coords(response['altitude'], response['azimuth'])

            if keyboard.is_pressed('backspace'):
                arduino.write(str.encode("42;"))
                break
            if first:
                time.sleep(120)
                first = False
            time.sleep(30)