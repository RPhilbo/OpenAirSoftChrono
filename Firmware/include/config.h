#ifndef CONFIG_H
#define CONFIG_H

#include <stdint.h>
#include <Arduino.h>

// Uncomment this to use BB direction coming from USB-C interface.
//#define bbDirectionFromUSB 
//ToDo: Changing into a variable, so that it can be changed via bluetooth later.


/* ============================================================
 * ======================= ANNOUNCEMENT =======================
 * ============================================================ */

struct TargetIdentifier {
    uint32_t part;
    uint32_t variant;
    uint64_t UID;
};


/* ============================================================
 * ======================= PROTOTYPES =========================
 * ============================================================ */

TargetIdentifier getTargetIdentifier();

#endif