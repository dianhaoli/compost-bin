README.md# Compost Bin Alert System

The Compost Bin Alert System is a simple web-based application that monitors compost bin levels in real time using Firebase. It displays live data such as distance and status for each bin to help track when bins are full or need attention.
Originally part of environmental startup CompostNow’s larger system managing 35 million lbs of compost.

## Overview

This system connects sensors (such as ultrasonic modules) to Firebase Realtime Database. The web dashboard automatically retrieves and displays each bin’s ID, name, distance, and status. It is designed to help users manage compost bins efficiently and promote sustainable waste management.

## Features

- Real-time updates from Firebase
- Displays multiple bins with readable names
- Shows distance readings from sensors
- Indicates bin status (e.g., full or empty)
- Simple, responsive web interface

## Technologies Used

Frontend: HTML, CSS, JavaScript  
Backend: Firebase Realtime Database  
Hardware (optional): Ultrasonic Sensor, ESP32 or Arduino  
Hosting: Firebase Hosting or local web server

## How It Works

1. Sensors send distance data to Firebase under the path:
   Sensor/data/binID
2. The web app listens to database changes using:
   onValue(ref(database, "Sensor/data"), callback)
3. The dashboard automatically updates each bin’s name, distance (in cm), and status in real time.

## Setup Instructions

1. Clone or download the project:
   git clone https://github.com/your-username/compost-bin-alert-system.git
2. Open index.html in your browser.
3. Ensure your Firebase Realtime Database contains valid data under Sensor/data/.
4. The dashboard will update automatically as data changes.

## Example Firebase Structure

{
  "Sensor": {
    "data": {
      "bin_206D37004F8C": {
        "distance": 12.5,
        "status": "Full"
      },
      "binac67b2ff0a4c": {
        "distance": 35.2,
        "status": "Active"
      }
    }
  }
}

## Author

Dan Li
Cary, NC  
GitHub: https://github.com/dianhaoli  
Email: dianhao.li@duke.edu


