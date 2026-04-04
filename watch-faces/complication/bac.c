#include "bac.h"

float bac_for_men_from_weight_and_alcohol_grams(float weight, float alcohol_grams, int start_time, int end_time) {
  return bac_from_weight_and_alcohol_grams(weight, alcohol_grams, start_time, end_time, 0.68);
}

float bac_for_women_from_weight_and_alcohol_grams(float weight, float alcohol_grams, int start_time, int end_time) {
  return bac_from_weight_and_alcohol_grams(weight, alcohol_grams, start_time, end_time, 0.55);
}

float bac_from_weight_and_alcohol_grams(float weight, float alcohol_grams, int start_time, int end_time, float widmark_constant) {
  return bac_from_weight_and_alcohol_grams_and_metabolism_rate(weight, alcohol_grams, start_time, end_time, widmark_constant, 0.15);
}

float bac_from_weight_and_alcohol_grams_and_metabolism_rate(float weight, float alcohol_grams, int start_time, int end_time, float widmark_constant, float metabolism_rate) {
  // The Widmark model.
  // BAC = (Alcohol consumed in grams / (Body weight in grams * Widmark factor)) - (Metabolism rate * (Current time - Start time))
  float max_bac = (alcohol_grams / ((weight * 100) * widmark_constant)) * 100;
  float number_of_hours = (float) (end_time - start_time) / 3600;
  float result = ((alcohol_grams / ((weight * 100) * widmark_constant)) * 100) - (metabolism_rate * (number_of_hours));
  // We also need to return a negative BAC as 0.
  if (result < 0.0) {
    return 0;
  }
  return result;
}
