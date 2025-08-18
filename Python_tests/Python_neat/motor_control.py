from serial_interface import write_to_serial_3_parms
from serial_interface import write_to_serial_2_parms

ON_REQUEST_SET_POS_NO_MOVE = 12
ON_GET_ADC_BITS = 20
ON_GET_ADC_REF_VOLT= 21
ON_MOVE_POSITION = 32
ON_JS_ENABLE = 40
ON_JS_DISABLE = 41

def set_pos(x, y):
    write_to_serial_3_parms(ON_MOVE_POSITION, 1, y)
    write_to_serial_3_parms(ON_MOVE_POSITION, 0, x)

def OverRideControllerCoordinates(alt, az):
    print('OverRideControllerCoordinates:',str(alt) + ", " + str(az))
    if az < 0.0:
        az = 360.0 + az
    if alt < 0.0 :
        alt = 360.0 + alt
 
    y_ticks = int(alt * 25600)
    x_ticks = int(az * 25600)
    print('alt OR', alt, y_ticks)
    print('az OR', az, x_ticks)
    write_to_serial_3_parms(ON_REQUEST_SET_POS_NO_MOVE, 1, y_ticks)
    write_to_serial_3_parms(ON_GET_ADC_REF_VOLT, 0, x_ticks)

def go_home():
    write_to_serial_3_parms(ON_MOVE_POSITION, 1, 0)
    write_to_serial_3_parms(ON_MOVE_POSITION, 0, 0)

# 1 for fast, 0 for slow
def set_js_speed(speed):
    write_to_serial_2_parms(ON_GET_ADC_BITS, speed)

# 1 for mirror mode, 0 for normal
def set_js_mirror(enabled):
    write_to_serial_2_parms(ON_GET_ADC_REF_VOLT, enabled)

def set_js_focus():
    write_to_serial_2_parms(ON_JS_ENABLE, 1)

def set_js_alt_az():
    write_to_serial_2_parms(ON_JS_DISABLE, 1)