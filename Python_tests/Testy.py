import numpy as np


def rotate_vector_cartesian(azimuth, altitude, angle_deg=-135):
    """
    Rotate a 2D vector (azimuth, altitude) by angle_deg degrees.
    Returns the rotated vector in Cartesian coordinates.
    """
    # Convert rotation angle to radians
    phi = np.deg2rad(angle_deg)

    # Apply 2×2 rotation matrix directly in Cartesian space
    x_new = azimuth * np.cos(phi) - altitude * np.sin(phi)
    y_new = azimuth * np.sin(phi) + altitude * np.cos(phi)

    return x_new, y_new

# Example
print(rotate_vector_cartesian(-1, 0))  
# Expected output: (0.0, -1.41421356) rotated by -135°
