# BAMTQ
## Burst-Aware Multi-Queue Traffic Management System

### Overview
BAMTQ is a real-time network traffic analysis and intelligent scheduling system designed to manage network congestion efficiently. The system captures packets, classifies traffic flows, detects burst traffic using Exponentially Weighted Moving Average (EWMA), and schedules packets using a hybrid EDF (Earliest Deadline First) and DWRR (Deficit Weighted Round Robin) approach.

### Features
- Real-time packet capture and monitoring
- Traffic flow classification
- Burst detection using EWMA
- Multi-queue traffic management
- EDF-based packet scheduling
- DWRR-based fair resource allocation
- Deadline-aware packet dropping
- Tenant-based traffic tracking
- Throughput and performance monitoring

### Technology Stack

#### Programming Language
- Python

#### Networking & Simulation
- Scapy
- NS-3

#### Platform
- Ubuntu Linux

### System Workflow
Packet Capture  
↓  
Traffic Classification  
↓  
Burst Detection (EWMA)  
↓  
Queue Assignment  
↓  
EDF + DWRR Scheduling  
↓  
Throughput Monitoring & Packet Delivery

### Future Enhancements
- Machine Learning based traffic prediction
- Dynamic queue optimization
- SDN integration
- Real-time dashboard visualization
- Cloud-native deployment

### Contributors
- Lubdha Chaudhari
- Riya Burle

Project developed as part of the B.Tech Artificial Intelligence & Data Science program.
