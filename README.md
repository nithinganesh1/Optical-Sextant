# Optical Sextant --- Sun Tracker

A software and optical-geometry module for a double-reflection sextant
integrated into a camera-assisted Sun Tracker. It models the optical
path, converts the X-axis motor position into a sextant reading,
estimates the Sun altitude from camera alignment error, and exposes
geometry for the dashboard's optical diagram.

> **Scope:** This README documents the supplied `sextant.py` and
> `optics.js` implementations and their integration points. It does not
> claim that the schematic dimensions are measured physical dimensions
> or that the complete instrument has been experimentally calibrated.

## 1. Optical principle

The instrument uses two reflections:

``` text
Sun → index mirror B (movable) → horizon mirror A (fixed) → telescope C
```

-   **Mirror B --- index mirror:** rotates with the X-axis mechanism.
-   **Mirror A --- horizon mirror:** fixed relative to the telescope
    axis.
-   **Telescope C:** observes the reflected Sun image together with the
    horizon reference.

The index mirror changes the direction of the reflected Sun ray by twice
the mirror's angular rotation. When the reflected Sun image and horizon
image coincide, the sextant reading equals the Sun altitude under the
geometry and alignment assumptions used by this implementation.

The ideal coincidence condition is:

\[ `\theta `{=tex}= `\frac{\alpha}{2}`{=tex} \]

where: - (`\theta`{=tex}) is the index-mirror rotation away from its
parallel-to-horizon-mirror reference position. - (`\alpha`{=tex}) is the
Sun altitude.

The corresponding sextant reading is:

\[ R = 2`\theta`{=tex} \]

At coincidence, (R=`\alpha`{=tex}).

## 2. Motor angle and sextant reading

The Python module `sextant.py` maps the logical X-axis motor angle to
the index-mirror angle and sextant reading.

The configured reference is:

  ---------------------------------------------------------------------------
  Quantity                                        Value Meaning
  ------------------------ ---------------------------- ---------------------
  `SURFACE_LEVEL_X_DEG`                             60° X-axis position where
                                                        the index mirror is
                                                        parallel to the
                                                        horizon mirror;
                                                        reading is 0°

  `LIMB_MAX_READING_DEG`                           120° Maximum represented
                                                        sextant reading

  `MIRROR_A_DEG`                                    60° Fixed horizon-mirror
                                                        line angle used by
                                                        the optical model

  `ALTITUDE_MAX_DEG`                                90° Maximum physical Sun
                                                        altitude accepted by
                                                        the Python
                                                        calculation

  `deg_per_step` default                   0.1125°/step Default motor angle
                                                        per step passed to
                                                        the residual
                                                        calculation
  ---------------------------------------------------------------------------

The mapping is:

\[ `\theta `{=tex}= 60\^`\circ `{=tex}- X \]

\[ R = 2(60\^`\circ`{=tex}-X) \]

The inverse mapping is:

\[ X = 60\^`\circ `{=tex}- `\frac{R}{2}`{=tex} \]

Examples:

    X motor angle   Index-mirror turn (`\theta`{=tex})   Sextant reading (R)
  --------------- ------------------------------------ ---------------------
              60°                                   0°                    0°
              45°                                  15°                   30°
              30°                                  30°                   60°
              15°                                  45°                   90°
               0°                                  60°                  120°

The intended measurement scan is **X = 0° to 60°**, corresponding to a
**0° to 120°** limb reading. The Python module separately reports the
wider configured motor range of **X = -60° to 60°**. These ranges are
not interchangeable: `in_scan_range` describes the sextant measurement
scan, while `in_motor_range` describes the wider motor range encoded in
`sextant.py`.

## 3. Camera correction and Sun-altitude estimate

The Camera 1 tracker measures the horizontal pixel error between the
detected Sun centre and the configured target point:

-   `err_px = sun_x - target_x`
-   Positive error means the Sun is to the right of the target.
-   `px_per_step` is the measured image displacement per X motor step.
-   `deg_per_step` is the motor angle represented by one step.

The residual angular difference is calculated by `residual_deg()`:

\[ `\Delta `{=tex}= `\frac{2\,d_s\,e_{px}}{p_s}`{=tex} \]

where: - (`\Delta`{=tex}) is the residual altitude difference in
degrees. - (d_s) is degrees per motor step. - (e\_{px}) is Camera 1
pixel error. - (p_s) is pixels moved per motor step.

The factor of two is present because a mirror rotation changes the
reflected ray angle by twice the mirror rotation.

When the Sun is detected, `compute()` estimates:

\[ `\alpha`{=tex}\_{`\text{estimated}`{=tex}} = R + `\Delta`{=tex} \]

The estimate is returned as `sun_altitude_deg` only if it lies within
the accepted physical range of 0° to 90° (with the module's small
numerical tolerance). If the Sun is not detected, or the estimate is
outside that range, the altitude is `None`.

`residual_deg` returns `None` when the pixel error is unavailable or
`px_per_step` is zero/false. The caller should therefore treat a missing
residual or altitude as unavailable data, not as a measured zero.

### Coincidence / lock

The `coincident` output is true only when both conditions are true:

-   `sun_detected` is true.
-   The caller supplies `locked=True`.

The sextant module does not independently decide whether the camera is
locked. It receives that tracking state from the caller.

## 4. Optical ray model (`static/optics.js`)

`optics.js` is a pure JavaScript geometry module. It does not access the
DOM, camera, motors, Flask or hardware. It can be loaded by the browser
dashboard or imported in Node.js for tests.

### Coordinate convention

The drawing uses SVG screen coordinates:

-   +X points right.
-   +Y points down.
-   Angles are in degrees and measured clockwise on screen.
-   A ray direction at angle (a) is represented by:

\[ `\mathbf{d}`{=tex}(a)=(`\cos `{=tex}a,`\sin `{=tex}a) \]

This is a screen-coordinate convention; it is not the usual Cartesian
convention with +Y upward.

### Reflection equation

For a ray direction (`\mathbf `{=tex}d) and a unit vector
(`\mathbf `{=tex}u) along the mirror line, the reflected direction is:

\[ `\mathbf `{=tex}d' =
2(`\mathbf `{=tex}d`\cdot`{=tex}`\mathbf `{=tex}u)`\mathbf `{=tex}u-`\mathbf `{=tex}d
\]

The model applies this operation twice:

1.  Reflect the incoming Sun ray from index mirror B.
2.  Reflect the resulting ray from fixed horizon mirror A.

The incoming Sun direction is represented as:

\[
`\mathbf `{=tex}d\_{`\text{in}`{=tex}}=(`\cos`{=tex}`\alpha`{=tex},`\sin`{=tex}`\alpha`{=tex})
\]

The model defines the index-mirror line angle as:

\[ `\phi`{=tex}\_B = `\phi`{=tex}\_A+`\theta`{=tex} \]

where the fixed mirror line angle is `MIRROR_A_DEG = 60°`.

For the ideal geometry, the exit-ray tilt is:

\[ `\text{exit tilt}`{=tex}=2`\theta`{=tex}-`\alpha`{=tex} \]

The JavaScript result uses a sign convention where positive exit tilt
means the ray travels upward on the SVG screen. At
(`\theta`{=tex}=`\alpha`{=tex}/2), the exit ray is horizontal and
follows the telescope/horizon axis.

### Schematic geometry constants

The `GEOM` object contains drawing parameters, not a physical
calibration record:

  ------------------------------------------------------------------------
  Constant                                     Value Purpose
  --------------------- ---------------------------- ---------------------
  `A`                                     (250, 390) Horizon-mirror pivot

  `BA_DISTANCE`                                  215 Distance from B to A
                                                     along the coincident
                                                     ray

  `MIRROR_A_DEG`                                 60° Fixed mirror line
                                                     angle in SVG
                                                     coordinates

  `AXIS_END_X`                                   700 Telescope-axis
                                                     drawing endpoint

  `SUN_DISTANCE`                                 190 Sun-symbol drawing
                                                     distance from B

  `MIRROR_HALF`                                   34 Half-length of the
                                                     drawn index mirror

  `A_HALF`                                        30 Half-length of the
                                                     drawn horizon mirror

  `LIMB_RADIUS`                                  290 Radius of the drawn
                                                     sextant limb

  `THETA_DRAW_MIN` /                       -4° / 64° Drawing clamp for
  `THETA_DRAW_MAX`                                   index-mirror angle

  `ALT_DRAW_MAX`                                120° Drawing clamp for
                                                     altitude input
  ------------------------------------------------------------------------

The point B is derived from A and the configured distance:

\[ B=A-D\_{BA},`\mathbf `{=tex}u(2`\phi`{=tex}\_A) \]

This positions B along the model's chosen coincident-ray direction.

The drawing accepts altitude values up to 120° so it can display the
full schematic limb/ghost-ray range. This is a visualization limit, not
a statement that the Sun can physically reach 120° altitude. The Python
measurement calculation restricts a valid Sun altitude to 0°--90°.

### Ray intersection and mirror aperture

`trace(thetaDeg, altitudeDeg)` calculates:

-   Sun symbol position.
-   Mirror pivots A and B.
-   Incoming and reflected ray directions.
-   Intersection (`hit`) of the ray after mirror B with the line of
    mirror A.
-   Whether the intersection is outside the simplified silvered region
    (`missed`).
-   Exit ray endpoint (`exitEnd`) when it travels toward positive X.
-   Exit-ray tilt (`exitTiltDeg`).
-   Endpoints for the two drawn mirror lines.

The intersection is computed parametrically. If the ray is parallel to
the mirror-A line, or the intersection is not forward along the ray,
`hit` remains `null`. The simplified aperture test marks the ray as
missed when its hit point is farther than `2 * A_HALF` from pivot A.

`limbPoint(thetaDeg, extra)` returns a point on the drawn limb around B
for the supplied index-mirror angle.

## 5. Python API (`sextant.py`)

The Python module is intentionally independent of hardware, OpenCV and
Flask. It contains pure calculations and returns ordinary Python values.

  ---------------------------------------------------------------------------------------
  Function                                            Purpose
  --------------------------------------------------- -----------------------------------
  `index_mirror_deg(x_deg)`                           Converts X motor angle to
                                                      index-mirror turn (`\theta`{=tex})

  `reading_deg(x_deg)`                                Converts X motor angle to sextant
                                                      reading

  `x_deg_for_reading(reading)`                        Converts a requested reading to X
                                                      motor angle

  `residual_deg(err_px, px_per_step, deg_per_step)`   Converts Camera 1 pixel error into
                                                      an angular residual

  `compute(...)`                                      Returns the complete sextant state
                                                      dictionary
  ---------------------------------------------------------------------------------------

### `compute()` inputs

``` python
compute(
    x_deg,
    err_px=None,
    px_per_step=None,
    deg_per_step=0.1125,
    sun_detected=False,
    locked=False,
)
```

### `compute()` output fields

  -------------------------------------------------------------------------
  Field                               Description
  ----------------------------------- -------------------------------------
  `x_deg`                             Supplied logical X motor angle

  `theta_deg`                         Index-mirror turn from the reference
                                      position

  `reading_deg`                       Sextant reading, (2`\theta`{=tex})

  `arc_fraction`                      Reading divided by the 120° limb
                                      maximum

  `in_scan_range`                     Whether X is in the 0°--60°
                                      measurement scan

  `in_motor_range`                    Whether X is in the -60°--60°
                                      configured motor range

  `on_limb`                           Whether the reading is within the
                                      0°--120° limb

  `mirror_a_deg`                      Fixed mirror angle constant

  `residual_deg`                      Camera-derived residual, or `None`

  `sun_altitude_deg`                  Estimated Sun altitude in 0°--90°, or
                                      `None`

  `zenith_distance_deg`               (90\^`\circ`{=tex}-`\alpha`{=tex}),
                                      or `None`

  `coincident`                        `sun_detected and locked`

  `surface_level_x_deg`               X reference angle corresponding to
                                      reading zero
  -------------------------------------------------------------------------

### Example

``` python
import sextant

# Motor angle only: ideal instrument reading
reading = sextant.reading_deg(30.0)
print(f"Sextant reading: {reading:.2f} deg")  # 60.00 deg

# Full state with camera information
state = sextant.compute(
    x_deg=30.0,
    err_px=2.0,
    px_per_step=1.5,
    deg_per_step=0.1125,
    sun_detected=True,
    locked=False,
)

print(state["theta_deg"])
print(state["reading_deg"])
print(state["residual_deg"])
print(state["sun_altitude_deg"])
```

## 6. Integration with the Sun Tracker

The supplied `tracker_runtime.py` imports `sextant` and runs the
camera/motor tracking loop in a background thread. It maintains Camera 1
and Camera 2 measurements, tracking controllers, the stepper link,
IMU/gimbal status, events and a latest-state snapshot.

The runtime's documented architecture is:

``` text
Camera 1 ── Sun detection / target pixel error ── X controller ── X stepper
     │
     └──────────────────────────────────────────────┐
                                                    │
X motor position + Camera 1 calibration ── sextant.py
                                                    │
                                                    ▼
                                             Dashboard snapshot
                                                    │
Camera 2 ── Sun-to-line distance ── Y controller ── Y stepper
                                                    │
IMU / gimbal serial telemetry ──────────────────────┤
                                                    ▼
                                            Flask dashboard
                                                    │
                                     optics.js draws ray geometry
```

The roles are distinct:

-   `sextant.py` computes the instrument reading and altitude-related
    values.
-   `optics.js` computes the schematic double-reflection ray geometry
    for visualization.
-   `tracker_runtime.py` coordinates cameras, controllers, serial
    communication and the live snapshot.
-   The Flask server exposes the snapshot and camera streams to the
    browser.
-   The dashboard JavaScript consumes the state and renders the optical
    diagram.

The runtime uses the X-axis camera calibration to obtain `px_per_step`.
In the supplied runtime excerpt, `x_px_per_step()` uses `X_PX_PER_STEP`
when explicitly configured; otherwise it derives the value from the X
controller gain and sign. Keep this calibration consistent with the
actual camera orientation and motor direction.

The gimbal/IMU telemetry is a separate stabilization/status path. It
should not be confused with the double-reflection sextant angle
calculation.

## 7. Running and testing

The sextant maths can be tested without connecting the tracker hardware.

From the project directory:

``` bash
python3 - <<'PY'
import sextant

for x in (60, 45, 30, 15, 0):
    print(
        f"X={x:5.1f} deg | "
        f"reading={sextant.reading_deg(x):6.1f} deg"
    )
PY
```

For the JavaScript geometry module in Node.js:

``` bash
node - <<'JS'
const Optics = require("./static/optics.js");

const result = Optics.trace(30, 60);
console.log({
  theta: result.theta,
  altitude: result.altitude,
  hit: result.hit,
  missed: result.missed,
  exitTiltDeg: result.exitTiltDeg
});

console.log(Optics.limbPoint(30));
JS
```

The browser dashboard can load `static/optics.js` as a normal script; in
that environment the module attaches `Optics` to the global root
(`window`).

These examples test the mathematical modules only. They do not start
cameras, connect to the motor controller or verify physical alignment.

## 8. Calibration and alignment considerations

Before relying on altitude values:

1.  Home the X mechanism and verify that the reported logical X angle
    corresponds to the actual mechanical position.
2.  Verify the reference pose: X = 60° must represent the configured
    zero-reading/surface-level position.
3.  Calibrate Camera 1's pixel movement per X motor step using the
    tracker calibration routine.
4.  Confirm the sign of `px_per_step` against the actual camera
    orientation and motor direction.
5.  Verify the Camera 1 target point/ROI and ensure the detected Sun
    centre is stable.
6.  Confirm the fixed horizon mirror and telescope reference are
    mechanically aligned.
7.  Compare the computed reading with a known reference before treating
    it as a measured altitude.

The software equations assume the stated mirror geometry and a correctly
established mechanical zero. Errors in mirror mounting, camera
orientation, backlash, motor-step calibration or reference alignment
will affect the result.

The source code does not provide a complete optical calibration
procedure for mirror flatness, telescope boresight, index error,
refraction correction, or atmospheric/astronomical corrections. Those
should not be assumed to be implemented.

## 9. Important implementation notes

-   **Angle conventions differ by module.** `sextant.py` uses logical
    motor and instrument angles. `optics.js` uses clockwise SVG screen
    angles with +Y downward. Convert deliberately when adding new
    geometry or dashboard features.
-   **Drawing limits are not physical limits.** The JavaScript model
    clamps its drawing inputs for visualization. The Python module
    applies its own measurement and motor-range checks.
-   **A missing value is not zero.** `None` means the calculation is
    unavailable or outside the accepted range.
-   **Lock state is supplied externally.** `compute()` does not
    determine image coincidence from pixels; it reports the `locked`
    flag supplied by the tracker.
-   **The aperture check is schematic.** It uses a distance threshold
    around A rather than a detailed model of mirror shape, thickness,
    mounting or clear aperture.
-   **The model is idealized.** No wavelength-dependent effects, mirror
    imperfections, telescope distortion, atmospheric refraction or
    instrument index-error correction are present in the supplied
    functions.

## 10. Troubleshooting

  -----------------------------------------------------------------------
  Symptom                             What to check
  ----------------------------------- -----------------------------------
  Reading changes in the wrong        Verify X motor direction and the
  direction                           `SURFACE_LEVEL_X_DEG - X`
                                      convention

  Reading is offset at the horizon    Re-check the X = 60° mechanical
  reference                           reference and mirror alignment

  Sun altitude is `None`              Check `sun_detected`,
                                      `px_per_step`, pixel error, and
                                      whether the calculated altitude
                                      falls within 0°--90°

  Residual is `None`                  Check that `err_px` is available
                                      and `px_per_step` is non-zero

  Dashboard ray and reported reading  Check that both use the same X
  disagree                            angle, index-mirror angle and angle
                                      units

  JavaScript ray reports `hit: null`  Check for a parallel ray/mirror
                                      configuration or an intersection
                                      behind the ray origin

  JavaScript ray reports              The modeled hit is outside the
  `missed: true`                      simplified silvered region of
                                      mirror A

  Image does not become coincident    Check camera target/ROI,
                                      calibration sign and gain, motor
                                      direction, mechanical backlash and
                                      optical alignment
  -----------------------------------------------------------------------

## 11. File responsibilities

  -----------------------------------------------------------------------
  File                                Responsibility
  ----------------------------------- -----------------------------------
  `sextant.py`                        Pure Python sextant angle, reading,
                                      residual and altitude calculations

  `static/optics.js`                  Pure JavaScript double-reflection
                                      ray geometry and limb drawing
                                      points

  `tracker_runtime.py`                Live camera, controller, motor, IMU
                                      and snapshot runtime

  `dashboard_server.py`               Flask routes for dashboard, state,
                                      commands, SSE and MJPEG

  `templates/dashboard.html`          Dashboard page structure

  `static/optics.js`                  Optical ray model used by the
                                      dashboard

  `static/dashboard.js`               Dashboard-side state and
                                      interaction logic (if present in
                                      the project)

  `static/dashboard.css`              Dashboard styling
  -----------------------------------------------------------------------

------------------------------------------------------------------------

**Project:** Sun Tracker --- Optical Sextant\
**Primary calculation module:** `sextant.py`\
**Optical visualization module:** `static/optics.js`
