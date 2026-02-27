#include "config.h"


/* ============================================================
 * ======================= VARIABLES ==========================
 * ============================================================ */

TargetIdentifier TargetID;


/* ============================================================
 * ======================= METHODS ============================
 * ============================================================ */

// Get the complete targer MCU identifier as a struct
TargetIdentifier getTargetIdentifier() {
    TargetIdentifier TargetID;
    
    TargetID.part       = NRF_FICR->INFO.PART;
    TargetID.variant    = NRF_FICR->INFO.VARIANT;
    TargetID.UID        = ((uint64_t)NRF_FICR->DEVICEID[1] << 32) | NRF_FICR->DEVICEID[0];

    return TargetID;
}