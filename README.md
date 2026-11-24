# NEPAlert

**ESP32 Power Monitor and SMS Outage Alert System (Wokwi simulation)**

NEPAlert watches a simulated mains supply, estimates load current, power and energy use, and sends an SMS alert whenever power goes out or comes back. It runs entirely in the [Wokwi](https://wokwi.com) ESP32 simulator, so no hardware is needed.

## Features
- Detects mains outages and restorations, with a 1-second debounce to ignore flicker
- Estimates load current (A), power (W) and cumulative energy (Wh)
- Prints live readings to the Serial Monitor every second
- Sends SMS alerts through the Africa's Talking REST API over HTTPS
- Retries failed requests up to 3 times, and prints the HTTP status for debugging
- LED indicator that shows whether mains is present

## Simulated hardware

| Part | ESP32 pin | Stands in for |
|------|-----------|---------------|
| Slide switch | GPIO 14 (D14) | Mains present / absent |
| Potentiometer | GPIO 34 (D34) | Load current sensor |
| LED + 220 Ω resistor | GPIO 26 (D26) | Mains-on indicator |

### Wiring
- **Slide switch:** pin 1 to GND, middle pin to D14
- **Potentiometer:** VCC to 3V3, GND to GND, signal (middle) pin to D34
- **LED:** D26 to the resistor, the resistor to the LED anode, the LED cathode to GND

The mains input and the load current are simulated with the switch and the potentiometer. The switch reads LOW (connected to GND) when mains is "present". Nothing in the simulation uses real mains voltage.

## How it works
1. The ESP32 connects to Wokwi's built-in `Wokwi-GUEST` Wi-Fi network.
2. Once per second, while mains is on, it reads the potentiometer, scales it to 0–20 A, calculates power assuming 230 V, and adds to the running energy total.
3. When the switch changes state and stays changed for 1 second, the LED updates and an SMS is sent:
   - Outage: "Power outage detected. Switched to backup."
   - Restored: "Power restored. Energy used so far: X Wh"

## How to run
1. Create a new **ESP32** project on [wokwi.com](https://wokwi.com).
2. Replace the contents of `sketch.ino` and `diagram.json` with the files from this repository.
3. Create a free account at [africastalking.com](https://africastalking.com) and open the **Sandbox** app.
4. In the sandbox, go to **Settings → API Key** and generate a key. The username is `sandbox`.
5. Launch the sandbox **Simulator** and register a phone number in international format.
6. In `sketch.ino`, replace the placeholders with your own values:
   ```cpp
   const char* AT_USER = "sandbox";
   const char* AT_KEY  = "PASTE_YOUR_SANDBOX_API_KEY";
   const char* PHONE   = "+234XXXXXXXXXX";   // number registered in the AT simulator
   ```
7. Press **Play**, wait for `Wi-Fi connected`, turn the potentiometer to change the load, and flip the switch to trigger alerts.
8. Open the Africa's Talking simulator to see the SMS arrive.

**Never commit or share your real API key.** If you do, generate a new one in the dashboard.

## Phone numbers
Numbers must be in international format with the country code, for example `+234...` (Nigeria), `+254...` (Kenya) or `+233...` (Ghana). The country isn't limited to Nigeria. Live SMS depends on the countries Africa's Talking supports.

## Notes
- The Africa's Talking sandbox does **not** send real SMS. Messages only appear in the sandbox simulator.
- Wokwi has no GSM module, so SMS is sent through an HTTP API over Wi-Fi.
- `client.setInsecure()` skips HTTPS certificate checks and is only acceptable for a simulation.

## Troubleshooting

| Symptom | Likely cause |
|---------|--------------|
| Only dots print after startup | Wi-Fi isn't connecting. Stop and restart the simulation, or reload the page. |
| No readings printing | The switch is in the "mains off" position. Flip it. |
| `status: 401` | Wrong or inactive API key, or the username isn't `sandbox`. Generate a new key in the Sandbox app. |
| `status: -1` | Connection failure. Restart and retry. The code retries automatically. |
| `status: 201` but no message | The phone number isn't registered in the simulator, or doesn't match exactly. |
| `DeliveryFailure` in the dashboard | The simulator wasn't ready for that number. Open it first, then trigger a new alert. |

## Planned upgrades
- Real mains sensing with an isolated sensor, and a CT clamp or PZEM-004T for current measurement
- GSM module (SIM800L or a 4G module) so alerts still send when Wi-Fi is down
- Backup power for the ESP32 so it survives an outage
- OLED display for live readings
- High-load alert threshold
- Daily energy cost estimate in naira

## Repository structure
```
NEPAlert/
├── sketch.ino
├── diagram.json
├── README.md
└── screenshots/
```

## Tech
C++ (Arduino framework), ESP32, Wokwi, Africa's Talking SMS API, HTTPS/REST
