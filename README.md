# SAM – Send A Message

![SAM Banner](https://github.com/MNTadros/SAM-CYD/blob/main/imgs/SAM_CYD.png)

SAM (Send A Message) is a friendly desktop IoT device powered by the ESP32 CYD microcontroller. It receives messages from a web dashboard and displays them as, sender name, timestamp, and content, on a 2.8″ TFT, adding a personal touch to your workspace.

---

## Features

- **Real-time message display** (sender name, message text, date/time)  
- **ESP32 CYD + 2.8″ TFT** for a compact yellow desktop device  
- **Custom Flask-based dashboard** (HTML/JS) for sending messages  
- **Wi-Fi connectivity via HTTP/REST JSON API**  
- Lightweight, cheerful design that fits any desk

---

## Tech Stack

- **Firmware:** ESP32 CYD (Arduino/C++) with [TFT_eSPI](https://github.com/Bodmer/TFT_eSPI)  
- **Dashboard:** Flask (Python 3) + HTML/CSS/JavaScript  
- **Communication:** HTTP/REST (JSON)  
- **Docs:** Markdown served via GitHub Pages

---

## Getting Started

### 1. Flash the ESP32 CYD

1. Open your Arduino IDE (or PlatformIO) and load the code in `src/main/`.  
2. In `config.txt` (inside `src/main/`), update:

   ```text
   device_id=samID
   ssid=YourNetworkSSID
   password=YourWiFiPassword
   api=http://<SERVER_IP>:<PORT>/api/message/
   ```
3. Upload to your ESP32 CYD. The device will reboot and attempt to connect to Wi-Fi.

### 2. Run the Flask Dashboard

1. Change into the dashboard folder:

   ```bash
   cd src/web_dashboard
   ```
2. (Optional but recommended) create a virtual environment and install dependencies:

   ```bash
   python3 -m venv venv
   source venv/bin/activate
   pip install -r requirements.txt
   ```
3. Start Flask:

   ```bash
   flask run --host=0.0.0.0 --port=5000
   ```
4. In your browser, navigate to `http://<SERVER_IP>:5000/`. You’ll see a simple form to enter **Sender**, **Message**, and **Timestamp**. Submit a test message.

### 3. Verify on the Device

If everything’s configured correctly, your ESP32 CYD’s TFT screen will show:

- **Sender** (in bold)
- **Message** body
- **Date & Time** (e.g. `2025-06-05  14:30`)

---

## Configuration

The device reads Wi-Fi & API settings from a plain `config.txt`. Example:

```
device_id=samID
ssid=MyHomeNetwork
password=SuperSecurePass
api=http://192.168.1.100:5000/api/message/
```

Place `config.txt` alongside the firmware source (under `src/main/`) before uploading.

---

## Documentation

Full documentation is in the `docs/` folder. You can also view it on GitHub Pages:

```
https://mntadros.github.io/SAM-CYD/
```

- `docs/index.html` – Home page  
- `docs/setup.html` – Getting Started & Setup guide  
- `docs/what.html` – “What is CYD” + Bill of Materials
