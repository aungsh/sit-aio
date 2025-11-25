# SIT: All In One

### Team Members — Group P2N 2025

<p align="left">
  <a href="https://github.com/aungsh">
    <img src="https://github.com/aungsh.png" width="70" style="border-radius: 50%; margin-right: 20px" />
  </a>
  <a href="https://github.com/achrinza">
    <img src="https://github.com/achrinza.png" width="70" style="border-radius: 50%; margin-right: 20px" />
  </a>
  <a href="https://github.com/EiKyarPhyu262">
    <img src="https://github.com/EiKyarPhyu262.png" width="70" style="border-radius: 50%; margin-right: 20px" />
  </a>
    <a href="https://github.com/Hxruya">
    <img src="https://github.com/Hxruya.png" width="70" style="border-radius: 50%; margin-right: 20px" />
  </a>
  <a href="https://github.com/Lazy-Narwhal">
    <img src="https://github.com/Lazy-Narwhal.png" width="70" style="border-radius: 50%; margin-right: 20px" />
  </a>
  <a href="https://github.com/N1sh0">
    <img src="https://github.com/N1sh0.png" width="70" style="border-radius: 50%; margin-right: 20px" />
  </a>
</p>

## ICT1011: Computer Architecture and Organisation

- Singapore Institute of Technology
- Group P2N 2025
- Trimester 1, AY2025/26

<p> <img src="https://img.shields.io/badge/Next.js-000?style=for-the-badge&logo=nextdotjs&logoColor=white"/> <img src="https://img.shields.io/badge/Tailwind-38B2AC?style=for-the-badge&logo=tailwindcss&logoColor=white"/> <img src="https://img.shields.io/badge/Prisma-2D3748?style=for-the-badge&logo=prisma&logoColor=white"/> <img src="https://img.shields.io/badge/PostgreSQL-31648C?style=for-the-badge&logo=postgresql&logoColor=white"/> <img src="https://img.shields.io/badge/Pusher-300D4F?style=for-the-badge&logo=pusher&logoColor=white"/> </p>

<p> <img src="https://img.shields.io/badge/TinyScreen+-6A1B9A?style=for-the-badge"/> <img src="https://img.shields.io/badge/TinyShield%20WiFi-0288D1?style=for-the-badge"/> <img src="https://img.shields.io/badge/NodeMCU%20V3-3F51B5?style=for-the-badge&logo=arduino&logoColor=white"/> <img src="https://img.shields.io/badge/TSL2572%20Sensor-607D8B?style=for-the-badge"/> <img src="https://img.shields.io/badge/TCS34725%20RGB%20Sensor-8E24AA?style=for-the-badge"/> </p>

## Project Overview

SIT: All-In-One is a hardware–software integrated system designed to improve student life at the Singapore Institute of Technology.

This project brings together multiple microcontroller devices and a full web platform to support:

- Real-time room vacancy status
- Pomodoro (Strady)
- Attendance taking
- Interactive classroom quizzes (ClassHoot)

Our goal is to combine embedded systems and web technologies to make student life smoother, more interactive, and more engaging.

## Core Features

### Room Vacancy Detection

- The room device uses a TSL2572 ambient light sensor to detect room brightness.
- Data is transmitted via api to the web server.
- The website shows real-time room vacancy status across campus.

### Attendance Taking

- A student device (TinyScreen+ with WiFi TinyShield) connects to an attendance device (NodeMCU + TCS34725 RGB sensor) via wifi.
- The student device displays a unique RGB color pattern.
- The attendance device scans and recognizes the color using the RGB sensor.
- Attendance is synced to the server and viewable in the portal.

### ClassHoot Quiz System

- Lecturers create quizzes on the web portal.
- A game room code is displayed on the room device or website.
- Students join using their student devices.

Real-time:

- answer submissions
- player list updates

This creates a classroom experience similar to Kahoot, but integrated with hardware devices.

## System Architecture

### Hardware Components

| Device                | Components                                           | Purpose                                          |
| --------------------- | ---------------------------------------------------- | ------------------------------------------------ |
| **Student Device**    | TinyScreen+ • WiFi TinyShield                        | Joins quizzes, submits answers, takes attendance |
| **Room Device**       | TinyScreen+ • WiFi TinyShield • TSL2572 Light Sensor | Detects room vacancy, displays quiz codes        |
| **Attendance Device** | NodeMCU V3 • TCS34725 RGB Sensor                     | Scans RGB signals for attendance                 |

### Software Components

- Next.js – Web portal + API backend
- Prisma – ORM for database
- PostgreSQL – Main database
- Pusher/WebSockets – Real-time communication
- Arduino / PlatformIO – Microcontroller firmware

### (Architecture Diagram Placeholder)

## Folder structure

```
.
├── student-device/
│   └── main.ino
├── student-device-pomodoro/
│   └── main.ino
├── attendace-device/
│   └── main.ino
├── room-device/
│   └── main.ino
├── web-proxy/
│   ├── deploy.sh
│   ├── Dockerfile
│   └── nginx.conf
└── web-portal/
    ├── app/
    │   ├── api/
    │   │   ├── attendance/
    │   │   │   └── check
    │   │   ├── classhoot/
    │   │   │   ├── check
    │   │   │   ├── gameroom
    │   │   │   ├── join
    │   │   │   └── start
    │   │   ├── classrooms
    │   │   ├── room-events
    │   │   └── room-vacancy
    │   ├── about
    │   ├── attendance
    │   ├── classhoot
    │   └── room-vacancy
    ├── components/
    │   ├── attendance
    │   ├── classhoot
    │   └── ui
    ├── lib/
    │   ├── db.ts
    │   ├── pusher.ts
    │   └── utils.ts
    ├── prisma/
    │   ├── migrations
    │   └── schema.prisma
    ├── public
    ├── types
    ├── .env
    ├── components.json
    ├── eslint.config.ts
    ├── next.config.ts
    ├── package.lock.json
    ├── package.json
    ├── postcss.config.mjs
    └── tsconfig.ts
```

## TODO: Installation & Setup Instructions
