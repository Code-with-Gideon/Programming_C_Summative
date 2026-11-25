#include <LiquidCrystal.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Hardware pin assignments
const int LCD_RS = 12;
const int LCD_E  = 11;
const int LCD_D4 = 5;
const int LCD_D5 = 4;
const int LCD_D6 = 3;
const int LCD_D7 = 2;

const int PIN_BTN_ENTRY = 8;
const int PIN_BTN_EXIT  = 9;
const int PIN_LED_GREEN = 10;
const int PIN_LED_RED   = 13;

const int DEFAULT_MAX_CAPACITY = 10;
const unsigned long DEBOUNCE_MS = 50UL;
const unsigned long LCD_INTERVAL_MS = 250UL;
const int EVENT_BATCH_CAP = 8;

enum EventAction {
    ACT_NONE = 0,
    ACT_ENTRY_OK = 1,
    ACT_EXIT_OK = 2,
    ACT_ENTRY_REJECTED = -1,
    ACT_EXIT_REJECTED = -2
};

struct OccupancyRecord {
    int currentCount;
    int maxCapacity;
    unsigned long totalEntries;
    unsigned long totalExits;
    unsigned long rejectedEntries;
    unsigned long rejectedExits;
    bool lcdDirty;
};

struct EventEntry {
    unsigned long timestampMs;
    EventAction action;
    int countAfter;
};

struct DebouncedButton {
    int pin;
    int lastRaw;
    int stableState;
    unsigned long lastChangeMs;
};

LiquidCrystal lcd(LCD_RS, LCD_E, LCD_D4, LCD_D5, LCD_D6, LCD_D7);

static OccupancyRecord *gRecord = NULL;
static EventEntry *gBatchLog = NULL;
static int gBatchCount = 0;
static bool gAllocFailed = false;

static unsigned long gLastLcdUpdateMs = 0UL;

static DebouncedButton gEntryBtn = { PIN_BTN_ENTRY, LOW, LOW, 0UL };
static DebouncedButton gExitBtn  = { PIN_BTN_EXIT,  LOW, LOW, 0UL };

static char gSerialBuf[32];
static size_t gSerialLen = 0;

static void releaseOccupancySystem() {
    free(gBatchLog);
    gBatchLog = NULL;
    gBatchCount = 0;

    free(gRecord);
    gRecord = NULL;
}

static bool allocateOccupancySystem(int capacity) {
    gRecord = (OccupancyRecord *)calloc(1, sizeof(OccupancyRecord));
    if (gRecord == NULL) {
        return false;
    }

    gBatchLog = (EventEntry *)malloc((size_t)EVENT_BATCH_CAP * sizeof(EventEntry));
    if (gBatchLog == NULL) {
        free(gRecord);
        gRecord = NULL;
        return false;
    }

    gRecord->currentCount = 0;
    gRecord->maxCapacity = capacity;
    gRecord->lcdDirty = true;
    gBatchCount = 0;
    return true;
}

static const char *actionLabel(EventAction act) {
    switch (act) {
        case ACT_ENTRY_OK:       return "ENTRY_OK";
        case ACT_EXIT_OK:        return "EXIT_OK";
        case ACT_ENTRY_REJECTED: return "ENTRY_DENIED";
        case ACT_EXIT_REJECTED:  return "EXIT_DENIED";
        default:                 return "NONE";
    }
}

static void flushBatchLog() {
    if (gBatchLog == NULL) {
        return;
    }

    Serial.print(F("--- Flushing Event Batch ("));
    Serial.print(gBatchCount);
    Serial.println(F(" entries) ---"));

    for (int i = 0; i < gBatchCount; i++) {
        const EventEntry *e = gBatchLog + i;
        Serial.print(F("  t="));
        Serial.print(e->timestampMs);
        Serial.print(F("ms | "));
        Serial.print(actionLabel(e->action));
        Serial.print(F(" | Occupancy="));
        Serial.println(e->countAfter);
    }

    // Release completed batch block and allocate fresh buffer
    free(gBatchLog);
    gBatchLog = NULL;
    gBatchCount = 0;

    gBatchLog = (EventEntry *)malloc((size_t)EVENT_BATCH_CAP * sizeof(EventEntry));
    if (gBatchLog == NULL) {
        gAllocFailed = true;
        Serial.println(F("[FATAL] Reallocation failed after batch flush."));
        return;
    }
    Serial.println(F("Event batch buffer freed and reallocated cleanly."));
}

static void recordBatchEvent(const OccupancyRecord *rec, unsigned long nowMs, EventAction act) {
    if (rec == NULL || gBatchLog == NULL) {
        return;
    }
    if (gBatchCount >= EVENT_BATCH_CAP) {
        flushBatchLog();
        if (gBatchLog == NULL) {
            return;
        }
    }
    EventEntry *slot = gBatchLog + gBatchCount;
    slot->timestampMs = nowMs;
    slot->action = act;
    slot->countAfter = rec->currentCount;
    gBatchCount++;
}

static bool updateButtonDebounced(DebouncedButton *btn, unsigned long nowMs) {
    int raw = digitalRead(btn->pin);

    if (raw != btn->lastRaw) {
        btn->lastRaw = raw;
        btn->lastChangeMs = nowMs;
    }

    if ((nowMs - btn->lastChangeMs) >= DEBOUNCE_MS) {
        if (raw != btn->stableState) {
            btn->stableState = raw;
            if (btn->stableState == HIGH) {
                return true;
            }
        }
    }
    return false;
}

static void processEntry(OccupancyRecord *rec, unsigned long nowMs) {
    if (rec == NULL) {
        return;
    }

    if (rec->currentCount >= rec->maxCapacity) {
        rec->rejectedEntries++;
        rec->lcdDirty = true;
        recordBatchEvent(rec, nowMs, ACT_ENTRY_REJECTED);
        Serial.print(F("[ERROR] Entry denied: maximum capacity ("));
        Serial.print(rec->maxCapacity);
        Serial.println(F(") reached."));
        return;
    }

    rec->currentCount++;
    rec->totalEntries++;
    rec->lcdDirty = true;
    recordBatchEvent(rec, nowMs, ACT_ENTRY_OK);

    Serial.print(F("[ENTRY] Inside: "));
    Serial.print(rec->currentCount);
    Serial.print(F(" | Available: "));
    Serial.println(rec->maxCapacity - rec->currentCount);
}

static void processExit(OccupancyRecord *rec, unsigned long nowMs) {
    if (rec == NULL) {
        return;
    }

    if (rec->currentCount <= 0) {
        rec->rejectedExits++;
        rec->lcdDirty = true;
        recordBatchEvent(rec, nowMs, ACT_EXIT_REJECTED);
        Serial.println(F("[ERROR] Exit denied: facility is already empty (0 people)."));
        return;
    }

    rec->currentCount--;
    rec->totalExits++;
    rec->lcdDirty = true;
    recordBatchEvent(rec, nowMs, ACT_EXIT_OK);

    Serial.print(F("[EXIT]  Inside: "));
    Serial.print(rec->currentCount);
    Serial.print(F(" | Available: "));
    Serial.println(rec->maxCapacity - rec->currentCount);
}

static void updateStatusLEDs(const OccupancyRecord *rec, unsigned long nowMs) {
    (void)nowMs;
    if (rec == NULL) {
        return;
    }

    bool full = (rec->currentCount >= rec->maxCapacity);
    digitalWrite(PIN_LED_GREEN, full ? LOW : HIGH);
    digitalWrite(PIN_LED_RED, full ? HIGH : LOW);
}

static void updateLcdDisplay(OccupancyRecord *rec, unsigned long nowMs) {
    char line1[17];
    char line2[17];
    int remaining;

    if (rec == NULL || !rec->lcdDirty) {
        return;
    }
    if ((nowMs - gLastLcdUpdateMs) < LCD_INTERVAL_MS && gLastLcdUpdateMs != 0UL) {
        return;
    }

    remaining = rec->maxCapacity - rec->currentCount;
    if (remaining < 0) remaining = 0;

    snprintf(line1, sizeof(line1), "In:%3d / Max:%3d", rec->currentCount, rec->maxCapacity);
    snprintf(line2, sizeof(line2), "Avail:%3d  %s", remaining,
             (rec->currentCount >= rec->maxCapacity) ? "FULL!" : "OPEN ");

    lcd.setCursor(0, 0);
    lcd.print(line1);
    lcd.setCursor(0, 1);
    lcd.print(line2);

    rec->lcdDirty = false;
    gLastLcdUpdateMs = nowMs;
}

static void printStatusReport(const OccupancyRecord *rec) {
    if (rec == NULL) {
        return;
    }
    Serial.println(F("\n=== Occupancy Statistics ==="));
    Serial.print(F("Current Inside   : ")); Serial.println(rec->currentCount);
    Serial.print(F("Max Capacity     : ")); Serial.println(rec->maxCapacity);
    Serial.print(F("Available Spots  : ")); Serial.println(rec->maxCapacity - rec->currentCount);
    Serial.print(F("Total Entries    : ")); Serial.println(rec->totalEntries);
    Serial.print(F("Total Exits      : ")); Serial.println(rec->totalExits);
    Serial.print(F("Rejected Entries : ")); Serial.println(rec->rejectedEntries);
    Serial.print(F("Rejected Exits   : ")); Serial.println(rec->rejectedExits);
    Serial.print(F("Pending Batch    : ")); Serial.println(gBatchCount);
    Serial.println(F("Commands: STATUS | FLUSH | RESET | CAP <1-999>\n"));
}

static void handleSerialCommand(const char *cmd) {
    if (strcmp(cmd, "STATUS") == 0) {
        printStatusReport(gRecord);
    } else if (strcmp(cmd, "FLUSH") == 0) {
        flushBatchLog();
    } else if (strcmp(cmd, "RESET") == 0) {
        int cap = (gRecord != NULL) ? gRecord->maxCapacity : DEFAULT_MAX_CAPACITY;
        releaseOccupancySystem();
        if (!allocateOccupancySystem(cap)) {
            gAllocFailed = true;
            Serial.println(F("[FATAL] Reset allocation failed."));
            return;
        }
        Serial.println(F("[RESET] Released heap records and reinitialized system."));
    } else if (strncmp(cmd, "CAP ", 4) == 0) {
        int nextCap = atoi(cmd + 4);
        if (nextCap <= 0 || nextCap > 999) {
            Serial.println(F("[ERROR] Capacity must be between 1 and 999."));
        } else if (gRecord != NULL && nextCap < gRecord->currentCount) {
            Serial.println(F("[ERROR] New capacity cannot be lower than current occupancy."));
        } else if (gRecord != NULL) {
            gRecord->maxCapacity = nextCap;
            gRecord->lcdDirty = true;
            Serial.print(F("[CONFIG] Max capacity updated to "));
            Serial.println(nextCap);
        }
    } else if (cmd[0] != '\0') {
        Serial.print(F("[WARN] Unknown Serial command: "));
        Serial.println(cmd);
    }
}

static void pollSerialCommands() {
    while (Serial.available() > 0) {
        char ch = (char)Serial.read();
        if (ch == '\r') {
            continue;
        }
        if (ch == '\n') {
            gSerialBuf[gSerialLen] = '\0';
            handleSerialCommand(gSerialBuf);
            gSerialLen = 0;
        } else if (gSerialLen + 1 < sizeof(gSerialBuf)) {
            gSerialBuf[gSerialLen++] = (char)toupper((unsigned char)ch);
        }
    }
}

void setup() {
    Serial.begin(9600);
    lcd.begin(16, 2);

    pinMode(PIN_BTN_ENTRY, INPUT);
    pinMode(PIN_BTN_EXIT, INPUT);
    pinMode(PIN_LED_GREEN, OUTPUT);
    pinMode(PIN_LED_RED, OUTPUT);

    if (!allocateOccupancySystem(DEFAULT_MAX_CAPACITY)) {
        gAllocFailed = true;
        Serial.println(F("[FATAL] Dynamic memory allocation failed during setup."));
        lcd.print(F("MEM ALLOC ERROR"));
        return;
    }

    updateStatusLEDs(gRecord, 0UL);
    updateLcdDisplay(gRecord, 0UL);
    Serial.println(F("Smart Occupancy Control System Initialized."));
    printStatusReport(gRecord);
}

void loop() {
    unsigned long nowMs = millis();

    if (gAllocFailed) {
        digitalWrite(PIN_LED_RED, ((nowMs / 250UL) % 2UL == 0UL) ? HIGH : LOW);
        return;
    }

    pollSerialCommands();

    if (updateButtonDebounced(&gEntryBtn, nowMs)) {
        processEntry(gRecord, nowMs);
    }
    if (updateButtonDebounced(&gExitBtn, nowMs)) {
        processExit(gRecord, nowMs);
    }

    updateStatusLEDs(gRecord, nowMs);
    updateLcdDisplay(gRecord, nowMs);
}
