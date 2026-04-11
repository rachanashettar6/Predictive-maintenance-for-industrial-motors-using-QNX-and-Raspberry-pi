# 🔧 QNX Real-Time Industrial Monitoring System

This project demonstrates a real-time industrial monitoring system built using **QNX RTOS**, integrating sensor data from ESP32 and pushing it to the cloud (Firebase).

---

## 📌 Overview

The system collects real-time sensor data (temperature, RPM, vibration, risk level) from an ESP32 device and processes it using a QNX-based server.

It uses **QNX native message passing (MsgSend / MsgReceive)** to ensure deterministic and efficient communication between system components.

---

## 🏗️ Architecture

ESP32 → TCP Socket Server → QNX Channel → Processing Thread → Firebase

- ESP32 sends sensor data via HTTP
- QNX server receives data using sockets
- Data is passed internally using QNX IPC
- Processed data is pushed to Firebase

---

## ⚙️ Key Features

- ✅ Real-time data processing using **QNX RTOS**
- ✅ **Priority-based scheduling (SCHED_FIFO)**
- ✅ **Multithreading (pthread)** for concurrent clients
- ✅ **QNX IPC (ChannelCreate, MsgSend, MsgReceive)**
- ✅ Cloud integration using **Firebase Realtime Database**
- ✅ Scalable architecture for multiple sensor nodes

---

## 🧠 QNX Concepts Used

- ChannelCreate()
- MsgSend() / MsgReceive()
- MsgReply()
- Real-time scheduling (SCHED_FIFO)
- POSIX Threads (pthread)

---

## 📡 Technologies Used

- C / C++
- QNX Neutrino RTOS
- POSIX Sockets
- libcurl (HTTP client)
- Firebase Realtime Database
- ESP32 (sensor node)

---

## ▶️ How It Works

1. ESP32 sends sensor data in JSON format
2. QNX server receives the request via TCP socket
3. Data is extracted and sent to a QNX channel
4. Processing thread reads data using MsgReceive()
5. Data is pushed to Firebase using HTTP PUT

---

## 📊 Sample Data Format

```json
{
  "temp": 30.5,
  "rpm": 1200,
  "vibration": "LOW",
  "risk": "SAFE"
}
