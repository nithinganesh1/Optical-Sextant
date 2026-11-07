# Suntracker Dashboard

A Python-based dashboard and hardware-control system for a
camera-assisted Sun Tracker. The project combines live camera feeds, Sun
detection and image processing, optical-sextant geometry, motor/stepper
control, serial communication, and embedded-controller firmware.

The Flask dashboard provides live tracker state, camera streams, and
command endpoints. The optical-sextant module models the
double-reflection path and converts the tracker axis position and
image-alignment error into a sextant reading and estimated Sun altitude.

> **Hardware warning:** This project can command moving motors and
> mechanisms. Check wiring, limit switches, motor directions, travel
> limits, and emergency-stop behaviour before enabling motion.

------------------------------------------------------------------------

## Features

-   Flask-based dashboard for monitoring and issuing tracker commands.
-   Server-sent events (SSE) for periodic dashboard state updates.
-   MJPEG camera-stream endpoints.
-   Sun detection and image-processing logic.
-   Optical-sextant ray-tracing model implemented in JavaScript.
-   Python calculations for sextant reading, residual correction, and
    Sun-altitude estimation.
-   Serial communication with embedded motor controllers.
-   Separate firmware directories for the ESP32 bridge and gimbal
    controllers.
-   Runtime integration for tracker, camera, and hardware-control
    components.

------------------------------------------------------------------------

## Project layout

``` text
Suntracker_Dash_final_final/
├── app.py
├── dashboard_server.py
├── serial_check.py
├── sextant.py
├── stepper_link.py
├── suntracker.py
├── sun_controller.py
├── tracker_runtime.py
├── requirements.txt
├── README.md
├── web_app_overview.md
├── Assets/
├── static/
│   ├── optics.js
│   ├── ...
│   └── ...
├── templates/
│   └── ...
├── esp32_bridge/
│   └── ...
├── gimbol/
│   └── ...
└── gimbol_x_z/
    └── ...
```

The entries shown with `...` represent files inside those directories;
the exact contents can vary as the project develops.

### Main Python files

  -----------------------------------------------------------------------
  File                                Responsibility
  ----------------------------------- -----------------------------------
  `dashboard_server.py`               Flask dashboard server and
                                      dashboard API routes.

  `tracker_runtime.py`                Connects the dashboard to the
                                      tracker runtime, controllers, and
                                      hardware links.

  `app.py`                            Standalone camera tracking and
                                      motion application.

  `suntracker.py`                     Sun detection and image-processing
                                      logic.

  `sun_controller.py`                 Tracking and axis-controller logic.

  `stepper_link.py`                   Serial communication wrapper for
                                      stepper hardware.

  `sextant.py`                        Pure Python optical-sextant
                                      calculations.

  `serial_check.py`                   Serial-port validation and
                                      diagnostic helper.
  -----------------------------------------------------------------------

### Other directories and files

  -----------------------------------------------------------------------
  Path                                Responsibility
  ----------------------------------- -----------------------------------
  `static/`                           Dashboard CSS and JavaScript.
                                      Includes `optics.js`, which models
                                      the optical ray path.

  `templates/`                        HTML templates used by the Flask
                                      dashboard.

  `Assets/`                           Project assets used by the
                                      dashboard or its supporting
                                      interface.

  `esp32_bridge/`                     ESP32 firmware for the motor/serial
                                      bridge.

  `gimbol/`                           Gimbal and IMU-related firmware.

  `gimbol_x_z/`                       Alternate gimbal firmware variant.

  `requirements.txt`                  Python package dependencies.

  `web_app_overview.md`               Additional notes about the web
                                      application.
  -----------------------------------------------------------------------

------------------------------------------------------------------------

## Requirements

-   Python 3
-   The Python packages listed in `requirements.txt`
-   A compatible camera, if camera tracking or video streaming is
    required
-   The relevant embedded controller and motor hardware, if motion
    control is required
-   Serial access permissions for the connected controller

The exact camera indices, serial device names, and hardware
configuration depend on the machine and connected devices.

------------------------------------------------------------------------

## Installation

### 1. Clone or open the project

``` bash
cd ~/Evolve/Projects/Suntracker_Dash_final_final
```

If you have cloned the repository into a different location, use that
directory instead.

### 2. Create a virtual environment

``` bash
python3 -m venv .venv
source .venv/bin/activate
```

### 3. Install dependencies

``` bash
python -m pip install --upgrade pip
pip install -r requirements.txt
```

If installation fails, check that the virtual environment is active and
review the package error before installing additional system
dependencies.

------------------------------------------------------------------------

## Running the application

### Run the dashboard server

``` bash
python dashboard_server.py
```

The dashboard server starts the Flask application and tracker runtime.
The server supports configuration options for host, port, serial device,
and disabling selected hardware features. Check the command-line help
for the options available in the current checkout:

``` bash
python dashboard_server.py --help
```

### Run the standalone tracker application

``` bash
python app.py
```

`app.py` is a separate application path. Do not run it at the same time
as `dashboard_server.py` when both processes would attempt to access the
same camera or serial device.

------------------------------------------------------------------------

## Dashboard API

The Flask dashboard exposes the following routes:

  -----------------------------------------------------------------------
  Route                   Method / type           Purpose
  ----------------------- ----------------------- -----------------------
  `/`                     HTTP GET                Serves the dashboard
                                                  page.

  `/api/stream`           Server-Sent Events      Streams tracker-state
                                                  snapshots periodically.

  `/api/state`            HTTP GET                Returns the current
                                                  tracker state.

  `/api/command`          HTTP command endpoint   Receives dashboard
                                                  commands.

  `/video/cam1`           MJPEG stream            Camera 1 video feed.

  `/video/cam2`           MJPEG stream            Camera 2 video feed.
  -----------------------------------------------------------------------

The SSE endpoint sends snapshots at approximately 100 ms intervals.
Actual update and video rates depend on the runtime, camera, and system
load.

For details about the dashboard interface and its controls, also refer
to `web_app_overview.md`.

------------------------------------------------------------------------

## Optical sextant

The optical-sextant model represents a double-reflection path:

1.  Sunlight reaches the index mirror **B**.
2.  The beam reflects from the index mirror, whose angle changes with
    the tracker axis.
3.  The reflected beam reaches the fixed horizon mirror **A**.
4.  The beam reflects again and travels toward the telescope axis **C**.

The JavaScript model in `static/optics.js` calculates and displays this
ray path. The Python module `sextant.py` converts the tracker-axis
position and image-alignment error into sextant values.

### Coordinate and angle conventions

The SVG model uses screen coordinates:

-   Positive X points right.
-   Positive Y points down.
-   Angles are measured clockwise in degrees.
-   The telescope axis is horizontal at the height of mirror A.
-   The horizon beam travels from the left.
-   The fixed mirror A line angle is 60°.
-   The index mirror B line angle is `60° + theta`.

The reflection calculation uses:

``` text
d' = 2(d · u)u - d
```

where `d` is the incoming direction and `u` is the unit vector along the
mirror line.

### Tracker angle and sextant reading

The Python model defines the surface-level X-axis position as 60°.

``` text
theta = 60° - X
sextant_reading = 2 × theta
```

Therefore:

    Logical X position   Index-mirror angle `theta`   Sextant reading
  -------------------- ---------------------------- -----------------
                   60°                           0°                0°
                    0°                          60°              120°

The inverse conversion is:

``` text
X = 60° - (sextant_reading / 2)
```

These are logical model relationships. Confirm the physical motor
direction and zero position during calibration.

### Image-alignment residual

The Python calculation can correct the sextant reading using the pixel
difference between the detected Sun position and the target position.

``` text
err_px = sun_position - target_position

residual_deg =
    2 × deg_per_step × err_px / px_per_step
```

The sign convention is:

-   Positive `err_px`: the detected Sun is to the right of the target.
-   Negative `err_px`: the detected Sun is to the left of the target.

When the Sun is detected and the optical alignment is within the model's
valid altitude range:

``` text
sun_altitude = sextant_reading + residual_deg
zenith_distance = 90° - sun_altitude
```

The model returns no Sun-altitude estimate when the required
detection/calibration inputs are unavailable or the corrected result is
outside the configured altitude range.

### Python API: `sextant.py`

The module is designed as pure calculation code. It does not directly
access cameras, motors, serial ports, OpenCV, or Flask.

Important constants:

  ---------------------------------------------------------------------------
  Constant                                        Value Meaning
  ------------------------ ---------------------------- ---------------------
  `SURFACE_LEVEL_X_DEG`                             60° Logical X position
                                                        corresponding to
                                                        surface level.

  `LIMB_MAX_READING_DEG`                           120° Maximum sextant
                                                        reading in the model.

  `MIRROR_A_DEG`                                    60° Fixed mirror line
                                                        angle.

  `ALTITUDE_MAX_DEG`                                90° Maximum accepted Sun
                                                        altitude.

  `EPS`                                          `1e-6` Small numerical
                                                        tolerance.
  ---------------------------------------------------------------------------

Main functions:

-   `compute(...)` --- calculates the sextant reading, scan/motor range
    flags, residual, and altitude-related values.
-   `residual_deg(...)` --- calculates the pixel-error correction when
    the required calibration values are available.
-   `x_deg_for_reading(...)` --- converts a sextant reading back to the
    logical X-axis position.

Example:

``` python
from sextant import compute

result = compute(
    x_deg=30.0,
    err_px=2.5,
    px_per_step=4.0,
    deg_per_step=0.1125,
    sun_detected=True,
    locked=False,
)

print(result)
```

The `compute()` result includes values such as:

-   `theta`
-   `reading`
-   `arc_fraction`
-   `in_scan_range`
-   `in_motor_range`
-   `on_limb`
-   `residual_deg`
-   `sun_altitude_deg`
-   `zenith_distance_deg`
-   `coincident`
-   `surface_level_x_deg`

### JavaScript ray model: `static/optics.js`

`optics.js` is a browser-compatible geometry module and can also be
loaded in Node.js for calculation tests. It does not require the
dashboard DOM to perform its geometry calculations.

Important model parameters include:

  ------------------------------------------------------------------------
  Parameter                                    Value Meaning
  --------------------- ---------------------------- ---------------------
  `A`                             `{x: 250, y: 390}` Fixed mirror A pivot
                                                     in the SVG model.

  `BA_DISTANCE`                                  215 Distance between the
                                                     mirror pivots in the
                                                     model.

  `MIRROR_A_DEG`                                 60° Fixed mirror line
                                                     angle.

  `AXIS_END_X`                                   700 Telescope-axis
                                                     drawing endpoint.

  `SUN_DISTANCE`                                 190 Sun-ray drawing
                                                     distance.

  `MIRROR_HALF`                                   34 Half-length used to
                                                     draw the index
                                                     mirror.

  `A_HALF`                                        30 Half-length used to
                                                     draw mirror A.

  `LIMB_RADIUS`                                  290 Radius used for the
                                                     limb drawing.
  ------------------------------------------------------------------------

The drawing clamps theta to -4° through 64° and altitude to 0° through
120°. These are visualization bounds, not proof of physical hardware
travel limits.

Main functions:

-   `trace(thetaDeg, altitudeDeg)` --- returns the calculated ray path,
    mirror positions, hit/miss information, and exit-ray tilt.
-   `limbPoint(thetaDeg, extra)` --- returns a point on the displayed
    limb.

At the model's coincidence condition:

``` text
theta = altitude / 2
exit_tilt = 2 × theta - altitude = 0°
```

The outgoing ray is horizontal, and the reflected images coincide in the
idealized geometry.

The ray model uses schematic SVG dimensions. They should not be treated
as measured physical dimensions of the built instrument.

------------------------------------------------------------------------

## Hardware and serial setup

### Serial devices

Depending on the connected board and USB-to-serial adapter, Linux may
expose the controller as:

``` text
/dev/ttyUSB0
/dev/ttyUSB1
/dev/ttyACM0
```

List available serial devices:

``` bash
ls /dev/ttyUSB* /dev/ttyACM* 2>/dev/null
```

Check serial-port permissions:

``` bash
groups
```

If the user is not in the `dialout` group, add the user and log out/in
for the change to take effect:

``` bash
sudo usermod -aG dialout "$USER"
```

Use `serial_check.py` to help validate serial connectivity. Confirm the
correct device and baud rate against the firmware before starting the
tracker.

### Cameras

Check the camera index configuration used by the tracking application. A
camera may appear at index `0`, `1`, or another index depending on the
connected devices and operating system.

Before enabling tracking:

-   Confirm each camera opens successfully.
-   Verify the correct camera is assigned to each stream.
-   Check the frame dimensions and focus.
-   Ensure no other process is holding the camera.

### Motors and embedded controllers

The firmware directories contain controller-specific code. Before
connecting the software to live hardware:

-   Confirm the firmware matches the connected controller.
-   Verify STEP, DIR, ENABLE, limit-switch, and serial wiring.
-   Confirm motor direction and axis assignment.
-   Test limit switches and homing at low speed.
-   Verify the configured travel range.
-   Keep a way to stop motor power immediately during testing.

Do not assume pin assignments or motion limits are identical across
`gimbol/`, `gimbol_x_z/`, and `esp32_bridge/`. Check the firmware source
for the exact board and wiring configuration being used.

------------------------------------------------------------------------

## Typical operating workflow

1.  Connect the cameras and embedded controllers.
2.  Verify the serial device and baud rate.
3.  Check the camera feeds independently.
4.  Confirm IMU and motor-controller communication.
5.  Start the dashboard server.
6.  Confirm the dashboard state and camera streams are updating.
7.  Verify homing and limit-switch status.
8.  Enable tracking or motion only after the mechanical setup is
    confirmed.
9.  Monitor the camera feed, tracker state, and motor behaviour during
    operation.

------------------------------------------------------------------------

## Calibration notes

The optical model and physical instrument must be aligned before using
calculated readings as measurements.

Recommended checks:

1.  **Mirror alignment:** Set the instrument to its surface-level
    reference and verify the fixed mirror and index mirror geometry.
2.  **Axis zero:** Confirm the physical X-axis position that corresponds
    to the logical 60° surface-level reference.
3.  **Motor direction:** Verify that positive and negative commands move
    the axis in the expected direction.
4.  **Pixel scale:** Measure `px_per_step` for the actual camera,
    resolution, lens, and mechanical arrangement.
5.  **Step calibration:** Verify `deg_per_step` against measured
    physical movement.
6.  **Coincidence:** Confirm that the detected Sun image and reference
    image coincide at the expected optical condition.
7.  **Altitude validation:** Compare calculated values against a trusted
    reference over several positions.

Calibration values can change when the camera, lens, resolution, mirror
mounting, or mechanical arrangement changes.

------------------------------------------------------------------------

## Troubleshooting

  -----------------------------------------------------------------------
  Problem                             Checks
  ----------------------------------- -----------------------------------
  Dashboard does not start            Activate `.venv`, install
                                      `requirements.txt`, and inspect the
                                      Python traceback.

  Serial device not found             Check USB connection, device path,
                                      baud rate, and `dialout`
                                      permissions.

  Camera cannot open                  Check camera index, device
                                      permissions, and whether another
                                      process is using it.

  Video stream is blank               Confirm the camera opens in the
                                      runtime and inspect the
                                      corresponding camera endpoint.

  Dashboard state does not update     Check the tracker runtime and
                                      `/api/stream` or `/api/state`
                                      responses.

  Motors do not move                  Verify controller power, serial
                                      communication, enable wiring,
                                      STEP/DIR signals, and limit-switch
                                      state.

  Homing does not complete            Check switch wiring and polarity,
                                      configured travel limits, and
                                      firmware homing settings.

  Sextant reading looks incorrect     Verify the X-axis reference, mirror
                                      alignment, angle convention, and
                                      `deg_per_step`.

  Altitude correction is incorrect    Verify the sign of `err_px`,
                                      `px_per_step`, camera alignment,
                                      and pixel-to-step calibration.

  Optical rays do not meet            Check the selected theta/altitude
                                      values and whether the ray
                                      intersection falls within the
                                      mirror model.
  -----------------------------------------------------------------------

------------------------------------------------------------------------

## Development notes

-   Keep `sextant.py` independent of hardware and UI code so its
    calculations can be tested separately.
-   Keep `static/optics.js` focused on geometry and ray tracing rather
    than dashboard event handling.
-   Avoid running `app.py` and `dashboard_server.py` concurrently if
    they share camera or serial resources.
-   Treat the JavaScript optical drawing as a schematic model, not a
    physical CAD model.
-   Re-test camera, serial, homing, and limit behaviour after changing
    firmware or hardware wiring.

------------------------------------------------------------------------

## License

This project is provided as-is for personal, experimental, and
educational use. No separate license terms are specified here; add the
appropriate license file if the project is intended for redistribution
or public use.
