#include <unity.h>
#include <stdio.h>
#include "calculations.h"


/* ============================================================
 * ======================= NUMBERS ============================
 * ============================================================ */

float distance_m  = 0.02f;              // meter
float distance_mm = distance_m*1000;    // millimeter

float time_s      = 0.0001f;        // seconds
float time_ms     = time_s*1000;    // milliseconds
float time_us     = time_ms*1000;   // microseconds
uint32_t ticks    = 672;            // timer ticks

float bbWeight   = 0.00036f;        // BB weight in kg
float bbWeight_g = bbWeight*1000;   // grams


/* ============================================================
 * ======================= TEST ENV ===========================
 * ============================================================ */

// Functions to set up and tear down the test environment
void setUp(void) {
  // set stuff up here
}

// Clean up after each test case
void tearDown(void) {
  // clean stuff up here
}

// A simple test case to check if the testing framework is working
void test_example(void) {
    TEST_ASSERT_EQUAL(1, 1);
}


/* ============================================================
 * ======================= TEST CASES =========================
 * ============================================================ */



/* ============================================================
 * ======================= TIME ===============================
 * ============================================================ */

// Ticks to microseconds conversion test case
void test_calculateTicksToMicroseconds(void) {
  float time = calculateTicksToMicroseconds(ticks);
  TEST_ASSERT_EQUAL_FLOAT (42.f, time);
}

// Ticks to microseconds conversion test case
void test_calculateTicksToMilliseconds(void) {
  float time = calculateTicksToMilliseconds(ticks);
  TEST_ASSERT_EQUAL_FLOAT (42.f, time);
}


/* ============================================================
 * ======================= VELOCITY ===========================
 * ============================================================ */

 // Test case for calculateVelocitySI function
void test_calculateVelocitySI(void) {
  float velocity = calculateVelocitySI(distance_m, time_s);
  TEST_ASSERT_EQUAL_FLOAT (200.f, velocity);
}

// Test case for calculateVelocityMilli function
void test_calculateVelocityMilli(void) {
  float velocity = calculateVelocityMilli(distance_mm, time_ms);
  TEST_ASSERT_EQUAL_FLOAT (200.f, velocity);
}

// Test case for calculateVelocityMicro function
void test_calculateVelocityMicro(void) {
  float velocity = calculateVelocityMicro(distance_mm, time_us);
  TEST_ASSERT_EQUAL_FLOAT (200.f, velocity);
}


/* ============================================================
 * ======================= ENERGY =============================
 * ============================================================ */

// Test case for calculateEnergy function
void test_calculateEnergySI(void) {
  float energy = calculateEnergySI(150.0f, 0.0004);
  TEST_ASSERT_EQUAL_FLOAT (4.5f, energy);
}

// Test case for calculateEnergy function
void test_calculateEnergyGramm(void) {
  float energy = calculateEnergyGramm(150.0f, 0.4f);
  TEST_ASSERT_EQUAL_FLOAT (4.5f, energy);
}


/* ============================================================
 * ======================= UNIT CONVERSION ====================
 * ============================================================ */

// Test case for unit conversion m/s to feet/s
void test_calculateEnergyGramm(void) {
  float VelocityRetard = convertVelocityMSToFTS(150.0f);
  TEST_ASSERT_EQUAL_FLOAT (492.126, VelocityRetard);
}


/* ============================================================
 * ======================= Test Runs ==========================
 * ============================================================ */

 // Main function to run the tests
int main(int argc, char **argv) {
    UNITY_BEGIN();
    RUN_TEST(test_example);

    // Time
    RUN_TEST(test_calculateTicksToMicroseconds);
    RUN_TEST(test_calculateTicksToMilliseconds);

    // Velocity
    RUN_TEST(test_calculateVelocitySI);
    RUN_TEST(test_calculateVelocityMilli);
    RUN_TEST(test_calculateVelocityMicro);

    // Energy
    RUN_TEST(test_calculateEnergySI);
    RUN_TEST(test_calculateEnergyGramm);

    // Units
    RUN_TEST(test_calculateEnergyGramm);

    return UNITY_END();
}