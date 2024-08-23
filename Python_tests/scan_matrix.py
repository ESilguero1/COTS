import serial
import keyboard
import time

# The following function creates a matrix of size Rows by Columns
# with coefficients that enable scanning a region

arduino = serial.Serial(port='COM3', baudrate=115200, timeout=.1)

size = 0
while size <= 1:
	size = int(input("How large is the matrix? ")) * 2 + 1

#Elevation 
Rows = size
row_scan_value = 0

#Azimuth
Columns = size
col_scan_value = 0

matrix = []

search_coeff = 25600

for row in range(Rows):
	matrix.append([])
	for column in range(Columns):
		row_scan_value = -(Rows//2) + row
		col_scan_value = -(Columns//2) + column
		if row_scan_value >= 0:
			row_scan_value = " " + str(row_scan_value)
		else:
			row_scan_value = str(row_scan_value)
		if col_scan_value >= 0:
			col_scan_value = " " + str(col_scan_value)
		else:
			col_scan_value = str(col_scan_value)

		matrix[row].append((col_scan_value, row_scan_value))

for i in range(size):
	if i%2 == 0:
		send_coords(CurrentAzimuth + matrix[i][0][0], CurrentAltitude + matrix[i][0][1])
		for _ in range(60):
			if keyboard.is_pressed('backspace'):
				arduino.write(str.encode("42;"))
				break
			time.sleep(1)
		send_coords(CurrentAzimuth + matrix[i][-1][0], CurrentAltitude + matrix[i][-1][1])
		for _ in range(60):
			if keyboard.is_pressed('backspace'):
				arduino.write(str.encode("42;"))
				break
			time.sleep(1)
	else:
		send_coords(CurrentAzimuth + matrix[i][-1][0], CurrentAltitude + matrix[i][-1][1])
		for _ in range(60):
			if keyboard.is_pressed('backspace'):
				arduino.write(str.encode("42;"))
				break
			time.sleep(1)
		send_coords(CurrentAzimuth + matrix[i][0][0], CurrentAltitude + matrix[i][0][1])
		for _ in range(60):
			if keyboard.is_pressed('backspace'):
				arduino.write(str.encode("42;"))
				break
			time.sleep(1)