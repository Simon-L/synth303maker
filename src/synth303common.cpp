#include "synth303common.hpp"
#include <cmath>

float vcf_env_freq(float vcf_env, float Vco, float Vmod_amt, float Vacc, float A, float B, float C, float D, float E, float base, float VaccMul, float VaccOffset) {
    float Vmod_scale = 6.9*Vmod_amt+1.3;
    float Vmod_bias = -1.2*Vmod_amt+3;
    float Vmod = (Vmod_scale * vcf_env + Vmod_bias) - 3.2f; // 3.2 == Q9 bias
    // guest formula
    // Ic,11 = (A*Vco + B)*e^(C*Vmod + D*Vacc +E)
    return (A * Vco + B) * std::exp(C * Vmod + D * (Vacc * VaccMul) + E) + base; // + D * Vacc
}
