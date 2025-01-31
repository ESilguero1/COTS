import serial
import keyboard
import time

# The following function creates a matrix of size Rows by Columns
# with coefficients that enable scanning a region


size = 5


#Elevation 
Rows = size
row_scan_value = 0

#Azimuth
Columns = size
col_scan_value = 0

matrix = []

search_coeff = 25600

def BuildScanArray(MatrixSize, search_tunning_coeff):
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
            row_scan_value = -(Rows//2) + row
            col_scan_value = -(Columns//2) + column
            if row_scan_value >= 0:
                row_scan_value =  (row_scan_value*search_tunning_coeff)
            else:
                row_scan_value = (row_scan_value*search_tunning_coeff)
                
            if col_scan_value >= 0:
                col_scan_value = (col_scan_value*search_tunning_coeff)
            else:
                col_scan_value = (col_scan_value*search_tunning_coeff)

            matrix[row].append((col_scan_value, row_scan_value))
            
        print (matrix[row])

    return matrix  

matrix = BuildScanArray(5, 0.1)
print(matrix)
