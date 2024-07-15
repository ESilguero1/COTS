import serial

arduino = serial.Serial(port='COM3',   baudrate=115200, timeout=.1)

def go_to(motor, degree):
    arduino.flushInput()
    arduino.write(str.encode("32," + motor + "," + str(int(degree) * 25600) + ";"))
    print(arduino.readline())

while True:
    motor = input("What motor are you moving? (or enter e to exit program) ")
    if motor == "e":
        print("exiting program")
        break
    degree = input("What angle would you like to go to? ")
    print("Moving motor " + motor + " to " + degree + " degrees...")
    go_to(motor, degree)