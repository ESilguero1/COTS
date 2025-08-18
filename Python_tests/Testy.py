import numpy as np

# Original vector in Cartesian coordinates
x = -1
y = 1

# Convert to polar coordinates
theta = np.arctan2(y, x)
rho = np.hypot(x, y)

# Desired rotation angle (in radians)
rotation_angle = np.radians(-135) # Rotate by 45 degrees

# Apply the rotation
new_theta = theta + rotation_angle

# Convert back to Cartesian coordinates
new_x = rho * np.cos(new_theta)
new_y = rho * np.sin(new_theta)

print(f"Original vector: ({x}, {y})")
print(f"Rotated vector: ({new_x}, {new_y})")