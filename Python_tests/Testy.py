import keyboard

while True:
    if keyboard.is_pressed("p"):
        print("You pressed p")
        break
    if keyboard.is_pressed("down"):
        print("Reach the bottom!")