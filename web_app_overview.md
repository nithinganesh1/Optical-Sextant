# Web App Overview

This project has a small web dashboard that helps monitor and control the Sun Tracker.

## What the web app does

The website shows live information from the tracking system, such as:

- live camera feed from the tracker
- current sun-tracking status and readings
- motor/angle data and system health
- optical/sextant diagram for the sun tracking view
- event logs and status messages
- buttons to control or command the system

In simple words, the web page is the user interface for the tracker. It lets the operator watch the system work and understand what is happening in real time.

## Where the data comes from

The data is not typed directly into the page by hand. The browser gets information from the Python server through API endpoints.

The server sends data in real time using:

- /api/stream — live stream of updates from the tracker
- /api/state — latest snapshot of values
- /api/command — commands sent to the tracker such as home, tracking on/off, or move to a new angle

This means the webpage is constantly updating from the tracker hardware, cameras, and sensor readings instead of being a static page.

## Where forms and user input happen

The dashboard includes small forms and control buttons where the user enters or sends commands.

Examples include:

- a number field to move the X motor to a target angle
- buttons to home the motors
- buttons to nudge the motors by a few steps
- toggles for tracking on/off
- event and status controls managed by the browser

In the HTML, these are normally created with form elements, input fields, and buttons. When a user clicks a button or submits a form, the browser sends the request to the server, and the Python app processes it.

## What HTML does

HTML is the structure of the page.

It tells the browser:

- where the header is
- where the camera feed goes
- where the charts and sensor values are placed
- where the buttons and text are displayed
- how the dashboard is organized into sections
- where the input forms and commands are located

So HTML is the page layout, content, and form structure.

## What CSS does

CSS is the design and styling.

It controls:

- colors and background
- spacing and layout
- fonts and text styles
- card sizes and alignment
- dark dashboard theme
- responsive design for different screen sizes
- visual highlight for warnings, values, and status indicators

So CSS makes the page look clean, readable, and professional.

## Very short summary

- HTML = structure, content, and form layout
- CSS = style and appearance
- JavaScript + Python backend = live updates and command handling
- The web app = live dashboard for monitoring and controlling the Sun Tracker
