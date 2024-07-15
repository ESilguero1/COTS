import requests
import json
import serial
import keyboard
import time
import threading

arduino = serial.Serial(port='COM3', baudrate=115200, timeout=.1)

def send_coords(alt, az):
    print(str(alt) + ", " + str(az))
    if az >= 181:
        az = -180 + (az-180)
    if alt >= 91:
        print("error")
    else:
        az = -az
        y_ticks = int(alt * 25600)
        x_ticks = int(az * 25600)
        arduino.write(str.encode("32,1," + str(y_ticks) + ";"))
        arduino.write(str.encode("32,0," + str(x_ticks) + ";"))

def go_home():
    arduino.write(str.encode("32,1,0;"))
    arduino.write(str.encode("32,0,0;"))

def track_object(query):
    while not keyboard.is_pressed('backspace'):
        response = requests.get("http://localhost:8090/api/objects/info?name=" + query + "&format=json").json()
        send_coords(response['altitude'], response['azimuth'])
        for _ in range(60):
            if keyboard.is_pressed('backspace'):
                arduino.write(str.encode("42;"))
                return
            time.sleep(1)

fast = False
mirror = False
arduino.flushInput()
arduino.write(str.encode("20,0;"))
arduino.write(str.encode("21,0;"))

while True:
    arduino.flushInput()
    arduino.write(str.encode("40;"))
    print("What object would you like to track?")
    print("or enter 'h' to re-center motors,")
    print("'e' to exit")
    if not fast:
        print("'f' to switch to fast mode")
    else:
        print("'s' to switch to slow mode")
    if not mirror:
        print("or 'm' to enable mirror mode")
    else:
        print("or 'n' to disable mirror mode")
    query = input("")
    print()
    first = True
    arduino.flushInput()
    if query == "h":
        print("Re-centering motors. Please wait...")
        go_home()
    elif query == "f":
        fast = True
        print("Switching to fast mode...")
        arduino.write(str.encode("20,1;"))
        time.sleep(2)
    elif query == "s":
        fast = False
        print("Switching to slow mode...")
        arduino.write(str.encode("20,0;"))
        time.sleep(2)
    elif query == "m":
        mirror = True
        print("Enabling mirror mode...")
        arduino.write(str.encode("21,1;"))
        time.sleep(2)
    elif query == "n":
        mirror = False
        print("Disabling mirror mode...")
        arduino.write(str.encode("21,0;"))
        time.sleep(2)
    elif query == "e":
        print("Exiting program.")
        break
    else:
        arduino.write(str.encode("41;"))
        print("Tracking " + query + " (press backspace to stop tracking)...")
        tracking_thread = threading.Thread(target=track_object, args=(query,))
        tracking_thread.start()
        tracking_thread.join()
    print()
