
import struct

def hex_to_float(hex_string):
    try:
        hex_string = hex_string.zfill(8)
        
        # Remove '0x' prefix and any leading/trailing spaces, then convert to bytes
        hex_string = hex_string.replace("0x", "").strip()
        byte_data = bytes.fromhex(hex_string)

        # Unpack the byte data as a float (assuming big-endian format)
        float_value = struct.unpack('>f', byte_data)[0]
        return float_value
    
    except ValueError as e:
        return f"Error: Invalid hex string - {e}"
    except struct.error as e:
        return f"Error: Could not unpack hex data - {e}"
    

def ConverToPolarCoordinates(neg_pos_degrees):
    if neg_pos_degrees < 0.0:
        neg_pos_degrees = 360 + neg_pos_degrees
        
    return neg_pos_degrees

def WrapPolarCoordinatesAround(degrees):
    if degrees >= 360.0:
        degrees -= 360.0
        
    return degrees
    
IntValue = "0"

print (hex_to_float(IntValue))

PolarOffsetDegrees = 0 # IMU is mounted 180 degrees off
PolarDegreesMeasurement = ConverToPolarCoordinates(-30)

if (PolarDegreesMeasurement > PolarOffsetDegrees):
    PolarDegreesMeasurement = WrapPolarCoordinatesAround(PolarDegreesMeasurement-PolarOffsetDegrees)
else:
    PolarDegreesMeasurement = WrapPolarCoordinatesAround(PolarOffsetDegrees-PolarDegreesMeasurement)
    
print (WrapPolarCoordinatesAround(PolarDegreesMeasurement))
        
        
