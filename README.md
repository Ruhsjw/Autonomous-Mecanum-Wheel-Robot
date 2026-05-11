# Autonomous Mobile Robot Platform

A fully autonomous mecanum-wheel robotic platform designed for real-time navigation, localization, and task-level autonomy in dynamic competition environments.

The system integrates embedded control, VIVE Lighthouse–based localization, autonomous navigation, wall following, and wireless robot management on an ESP32-S3 platform.

---

## Overview

This project was developed for a robotics competition focused on autonomous navigation, localization, and objective-based task execution under strict real-time and hardware constraints.

The robot was capable of:

- Autonomous navigation
- Wall following
- Real-time localization
- Path tracking
- Task-level behavior coordination
- Dynamic target interaction
- Wireless teleoperation and monitoring

The platform combined perception, embedded systems, control, and autonomy into a fully integrated robotic system.

---

## System Architecture

```text
VIVE Lighthouse Tracking
            ↓
     Localization Pipeline
            ↓
    State Estimation & Navigation
            ↓
      Behavior State Machine
            ↓
     Motion Controller (PID)
            ↓
      ESP32-S3 Motor Control
            ↓
      Mecanum-Wheel Platform

