#ifndef BAC_H
#define BAC_H

float bac_from_weight_and_alcohol_grams(float weight, float alcohol_grams, int start_time, int end_time, float widmark_constant);
float bac_from_weight_and_alcohol_grams_and_metabolism_rate(float weight, float alcohol_grams, int start_time, int end_time, float widmark_constant, float metabolism_rate);
float bac_for_women_from_weight_and_alcohol_grams(float weight, float alcohol_grams, int start_time, int end_time);
float bac_for_men_from_weight_and_alcohol_grams(float weight, float alcohol_grams, int start_time, int end_time);

#endif
