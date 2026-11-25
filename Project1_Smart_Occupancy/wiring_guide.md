# Project 1: Tinkercad Circuit & Wiring Guide

## Hardware Components
- 1x Arduino Uno R3
- 1x Breadboard (Full or Half-size)
- 1x 16x2 Character LCD (HD44780 parallel)
- 1x 10 kOhm Potentiometer (for LCD contrast `V0`)
- 2x Tactile Push Buttons (Entry & Exit)
- 1x Green LED + 1x Red LED
- 3x 220 Ohm Resistors (Green LED, Red LED, LCD Backlight `A`)
- 2x 10 kOhm Resistors (Pull-down resistors for Entry and Exit buttons)
- Male-to-Male Jumper Wires

---

## Conflict-Free Arduino Uno Pinout Table

| Function / Component | Arduino Pin | Breadboard & Resistor Wiring | Logic Level |
| :--- | :--- | :--- | :--- |
| **16x2 LCD RS** | `D12` | Direct jumper to LCD Pin 4 (`RS`) | Output |
| **16x2 LCD Enable (E)** | `D11` | Direct jumper to LCD Pin 6 (`E`) | Output |
| **16x2 LCD D4** | `D5` | Direct jumper to LCD Pin 11 (`D4`) | Output |
| **16x2 LCD D5** | `D4` | Direct jumper to LCD Pin 12 (`D5`) | Output |
| **16x2 LCD D6** | `D3` | Direct jumper to LCD Pin 13 (`D6`) | Output |
| **16x2 LCD D7** | `D2` | Direct jumper to LCD Pin 14 (`D7`) | Output |
| **Entry Push Button** | `D8` | Terminal 1 -> `+5V`; Terminal 2 -> `D8` AND `10 kOhm` resistor to `GND` | Active-HIGH Input |
| **Exit Push Button** | `D9` | Terminal 1 -> `+5V`; Terminal 2 -> `D9` AND `10 kOhm` resistor to `GND` | Active-HIGH Input |
| **Green LED (Available)** | `D10` | `D10` -> `220 Ohm` resistor -> Anode (longer/bent leg), Cathode -> `GND` | Active-HIGH Output |
| **Red LED (Full)** | `D13` | `D13` -> `220 Ohm` resistor -> Anode (longer/bent leg), Cathode -> `GND` | Active-HIGH Output |

---

## Complete 16x2 LCD Power & Contrast Pin Connections

| LCD Pin | Name | Connection |
| :--- | :--- | :--- |
| 1 | `GND` (`VSS`) | Breadboard `-` (`GND`) rail |
| 2 | `VCC` (`VDD`) | Breadboard `+` (`5V`) rail |
| 3 | `V0` (Contrast) | Middle wiper terminal of `10 kOhm` potentiometer (outer legs to `5V` and `GND`) |
| 4 | `RS` | Arduino `D12` |
| 5 | `RW` (Read/Write) | Breadboard `-` (`GND`) rail (sets LCD permanently to Write mode) |
| 6 | `E` | Arduino `D11` |
| 7 - 10 | `D0 - D3` | Not connected (4-bit parallel mode) |
| 11 - 14 | `D4 - D7` | Arduino `D5`, `D4`, `D3`, `D2` |
| 15 | `LED+` (`A`) | `220 Ohm` resistor -> Breadboard `+` (`5V`) rail |
| 16 | `LED-` (`K`) | Breadboard `-` (`GND`) rail |

> **Important Rail Check in Tinkercad:** Connect Arduino `5V` to the bottom red `+` rail, Arduino `GND` to the bottom blue `-` rail, and bridge `+` to `+` and `-` to `-` across top/bottom rails if components sit on both halves of the breadboard.

---

## Step-by-Step Simulation Verification Checklist

Run these 6 sequential checks in Tinkercad and verify both LEDs, the 16x2 LCD, and the Serial Monitor (`9600 baud`):

1. **Startup (Count = 0 / Max = 10):**
   - Green LED: **ON** | Red LED: **OFF**
   - LCD Line 1: `In:  0 / Max: 10` | LCD Line 2: `Avail: 10  OPEN `
   - Serial Monitor logs dynamic heap allocation confirmation and initial state.
2. **Invalid Exit at Zero:**
   - Press **Exit (`D9`)** while occupancy is `0`.
   - Count stays `0` (never goes `-1`), Green remains **ON**, Red remains **OFF**, and Serial prints `[ERROR] Exit denied: facility is already empty (0 people).`
3. **Valid Entries (1 through 9):**
   - Press **Entry (`D8`)** repeatedly up to `9`.
   - Hold the button briefly (1 second) to confirm `millis()` edge debouncing only counts **1 entry per click**.
   - Green remains **ON**, Red remains **OFF**, `Avail` drops cleanly (` 9` down to ` 1`).
4. **Reaching Maximum Capacity (`10 / 10`):**
   - Press **Entry (`D8`)** once more to reach `10`.
   - Green LED switches **OFF**, Red LED switches **ON**.
   - LCD Line 1: `In: 10 / Max: 10` | LCD Line 2: `Avail:  0  FULL!`
5. **Invalid Entry When Full:**
   - Press **Entry (`D8`)** again at `10`.
   - Count stays capped at `10`, Red LED stays strictly **ON** (`HIGH`), Green stays **OFF**, and Serial prints `[ERROR] Entry denied: maximum capacity (10) reached.`
6. **Valid Exit From Full (`10 -> 9`):**
   - Press **Exit (`D9`)** once.
   - Count becomes `9`, `Avail:  1  OPEN `, Red LED switches **OFF**, Green LED switches **ON**.
7. **Serial Control Commands (Memory & Dynamic Config Test):**
   - Type `STATUS`, `FLUSH`, `CAP 5`, and `RESET` into Tinkercad's Serial Monitor input box to observe `free()`/`calloc()` cleanup and live capacity updates.
