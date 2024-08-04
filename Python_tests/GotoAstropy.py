import requests
import json
import serial
import keyboard
import time
import threading
from astropy.time import Time
from astropy.coordinates import solar_system_ephemeris, EarthLocation
from astropy.coordinates import get_body, get_moon
from astropy.coordinates import SkyCoord, EarthLocation, AltAz
from astropy import units as u
import time;


loc = EarthLocation(lat=33.1424005*u.deg, lon=-96.8599673*u.deg, height=0*u.m)

arduino = serial.Serial(port='COM3', baudrate=115200, timeout=.1)

CurrentAltitude = 0
CurrentAzimuth = 0

Objects = ['moon', 'mars', 'jupiter', 'saturn', 'mercury', 'venus', 'sun']

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

def go_to(objectString):
    t = Time.now()
    with solar_system_ephemeris.set('jpl'):
      Object = get_body(objectString, t, loc)

    altazframe = AltAz(obstime=t, location=loc, pressure=0)
    Objectaz=Object.transform_to(altazframe)

    print(Objectaz.alt.degree,Objectaz.az.degree)
    send_coords(Objectaz.alt.degree,Objectaz.az.degree)

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
    #arduino.write(str.encode("40,1;"))
    print("What object would you like to track?")
    print("or enter 'h' to re-center motors,")
    print("or enter 'g' to go to object,")
    print("or enter 't' to go to object,")
    print("or enter 'j' to enable joystick,")
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
    #print()
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
    elif query == "t":
        arduino.write(str.encode("41;"))
        print("Tracking " + query + " (press backspace to stop tracking)...")
        tracking_thread = threading.Thread(target=track_object, args=(query,))
        tracking_thread.start()
        tracking_thread.join()
        break
    elif query == "g":
        print ('Select from this list', Objects)
        print('Zero indexed')
        ObjectQuery = input("")
        if ObjectQuery != "":
            go_to(Objects[int(ObjectQuery)])
            
    elif query == "j":
        print("Enabled JOystick...")
        print(arduino.write(str.encode("40,1;")))
        time.sleep(2)
    print()
