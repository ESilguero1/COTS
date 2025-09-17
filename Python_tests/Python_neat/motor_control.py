from serial_interface import write_to_serial_3_parms
from serial_interface import write_to_serial_2_parms

REQUEST_POS_NO_MOVE = 12
SET_JS_SLOW_FAST = 20
SET_JS_MIRROR_MODE= 21
SET_MOVE_POS = 32
JS_ENABLE = 41
JS_DISABLE = 40

def set_pos(azimuth, altitude):
    write_to_serial_3_parms(SET_MOVE_POS, 1, altitude)
    write_to_serial_3_parms(SET_MOVE_POS, 0, azimuth)

def OverRideControllerCoordinates(alt, az):
    print('OverRideControllerCoordinates:',str(alt) + ", " + str(az))
    if az < 0.0:
        az = 360.0 + az
    if alt < 0.0 :
        alt = 360.0 + alt
 
    alt_ticks = int(alt * 25600)
    az_ticks = int(az * 25600)
    print('alt OR', alt, alt_ticks)
    print('az OR', az, az_ticks)
    write_to_serial_3_parms(REQUEST_POS_NO_MOVE, 1, alt_ticks)
    #write_to_serial_3_parms(SET_JS_MIRROR_MODE, 0, az_ticks)

def go_home():
    write_to_serial_3_parms(SET_MOVE_POS, 1, 0)
    write_to_serial_3_parms(SET_MOVE_POS, 0, 0)

# 1 for fast, 0 for slow
def set_js_speed(speed):
    write_to_serial_2_parms(SET_JS_SLOW_FAST, speed)

# 1 for mirror mode, 0 for normal
def set_js_mirror(enabled):
    write_to_serial_2_parms(SET_JS_MIRROR_MODE, enabled)

def set_js_focus():
    write_to_serial_2_parms(JS_ENABLE, 1)

def set_js_alt_az():
    write_to_serial_2_parms(JS_DISABLE, 1)