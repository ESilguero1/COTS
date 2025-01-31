def BuildScanArray(MatrixSize, step_size):
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
            row_scan_value = (-(Rows//2) + row) * step_size
            col_scan_value = (-(Columns//2) + column) * step_size

            matrix[row].append((col_scan_value, row_scan_value))
            
        print (matrix[row])

    return matrix 

BuildScanArray(5, .1)           