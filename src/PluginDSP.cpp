/*
 * ImGui plugin example
 * Copyright (C) 2021 Jean Pierre Cimalando <jp-dev@inbox.ru>
 * Copyright (C) 2021-2022 Filipe Coelho <falktx@falktx.com>
 * SPDX-License-Identifier: ISC
 */

#include "DistrhoPlugin.hpp"
#include "CParamSmooth.hpp"

#include "chowdsp_dsp_utils/chowdsp_dsp_utils.h"

#include "ADAREnvelope.h"
#include "WowFilter.h"
#include "Osc303.hpp"
#include "AcidFilter.hpp"

#include <memory>

#include "synth303common.hpp"

// --------------------------------------------------------------------------------------------------------------------

#ifndef MIN
#define MIN(a,b) ( (a) < (b) ? (a) : (b) )
#endif

#ifndef MAX
#define MAX(a,b) ( (a) > (b) ? (a) : (b) )
#endif

#ifndef CLAMP
#define CLAMP(v, min, max) (MIN((max), MAX((min), (v))))
#endif

#ifndef DB_CO
#define DB_CO(g) ((g) > -90.0f ? powf(10.0f, (g) * 0.05f) : 0.0f)
#endif

START_NAMESPACE_DISTRHO

// --------------------------------------------------------------------------------------------------------------------

class PluginDSP : public Plugin
{
    enum Parameters {
        kParamGain = 0,
        kParamA,
        kParamB,
        kParamC,
        kParamD,
        kParamCutoff,
        kParamResonance,
        kParamVmod,
        kParamAccent,
        kParamDecay,
        kParamVcfAttack,
        kParamFormulaA,
        kParamFormulaB,
        kParamFormulaC,
        kParamFormulaD,
        kParamFormulaE,
        kParamFormulaBase,
        kParamFormulaVaccMul,
        kParamFormulaLimiter,
        kParamPrintParameters,
        kParamCount
    };

    double fSampleRate = getSampleRate();
    float fGainDB = 0.0f;
    float fGainLinear = 1.0f;
    std::unique_ptr<CParamSmooth> fSmoothGain = std::make_unique<CParamSmooth>(20.0f, fSampleRate);

    static constexpr int BLOCK_SIZE{32};
    float squareBuffer[4];
    float sawBuffer[4];
    Osc303 osc = Osc303();
    AcidFilter filter;
    WowFilter wowFilter;

    float atkTime = -9.482;
    float decTime = -2.223;

    float fVco = 12.0;
    float fRes = 1.0;
    float fVmod = 1.0;
    float fVacc_amt = 1.0;

    float A = 1.633001;
    float B = 0.626000;
    float C = 0.324000;
    float D = 0.191000;
    float E = 4.462000;
    float base = -119.205;
    float VaccMul = 2.0;

    sst::surgext_rack::dsp::envelopes::ADAREnvelope vca_env;
    sst::surgext_rack::dsp::envelopes::ADAREnvelope vcf_env;

public:
   /**
      Plugin class constructor.@n
      You must set all parameter values to their defaults, matching ParameterRanges::def.
    */
    PluginDSP()
        : Plugin(kParamCount, 0, 0) // parameters, programs, states
    {
    }

    ~PluginDSP() {
        printParameters();
    }

protected:

    // ----------------------------------------------------------------------------------------------------------------
    // Init

    void printParameters() {
        d_stdout("---------");

        d_stdout("float atkTime = %f;", atkTime);
        d_stdout("float decTime = %f;", decTime);

        d_stdout("float fVco = %f;", fVco);
        d_stdout("float fRes = %f;", fRes);
        d_stdout("float fVmod = %f;", fVmod);
        d_stdout("float fVacc_amt = %f;", fVacc_amt);

        d_stdout("float A = %f;", A);
        d_stdout("float B = %f;", B);
        d_stdout("float C = %f;", C);
        d_stdout("float D = %f;", D);
        d_stdout("float E = %f;", E);
        d_stdout("float base = %f;", base);
        d_stdout("float VaccMul = %f;", VaccMul);

        print_limits();

        d_stdout("---------");
    }

   /**
      Initialize the parameter @a index.@n
      This function will be called once, shortly after the plugin is created.
    */
    void initParameter(uint32_t index, Parameter& parameter) override
    {
        switch (index) {
        case kParamGain:
            parameter.ranges.min = -90.0f;
            parameter.ranges.max = 30.0f;
            parameter.ranges.def = -0.0f;
            parameter.hints = kParameterIsAutomatable;
            parameter.name = "Gain";
            parameter.shortName = "Gain";
            parameter.symbol = "gain";
            parameter.unit = "dB";
            return;
        case kParamD:
            parameter.hints = kParameterIsOutput;
            return;
        case kParamCutoff:
            parameter.ranges.min = 1.321f;
            parameter.ranges.max = 12.0f;
            parameter.ranges.def = 12.0f;
            parameter.name = "Cutoff";
            return;
        case kParamResonance:
            parameter.ranges.min = 0.0f;
            parameter.ranges.max = 1.0f;
            parameter.ranges.def = 1.0f;
            parameter.name = "Resonance";
            return;
        case kParamVmod:
            parameter.ranges.min = 0.0f;
            parameter.ranges.max = 1.0f;
            parameter.ranges.def = 1.0f;
            parameter.name = "Envmod";
            return;
        case kParamDecay:
            parameter.ranges.min = -2.223f;
            parameter.ranges.max = 1.32f;
            parameter.ranges.def = -2.223f;
            parameter.name = "Decay";
            return;
        case kParamVcfAttack:
            parameter.ranges.min = -9.482;
            parameter.ranges.max = -4.0f;
            parameter.ranges.def = -9.482f;
            parameter.name = "Vcf Attack";
            return;
        case kParamAccent:
            parameter.ranges.min = 0.0f;
            parameter.ranges.max = 1.0f;
            parameter.ranges.def = 0.0f;
            parameter.name = "Accent";
            return;
        // float A = 2.243000;
        case kParamFormulaA:
            parameter.ranges.min = 0.0;
            parameter.ranges.max = 10.0;
            parameter.ranges.def = 2.243;
            parameter.name = "A";
            return;
        // float B = 0.626000;
        case kParamFormulaB:
            parameter.ranges.min = 0.0;
            parameter.ranges.max = 10.0;
            parameter.ranges.def = 0.626;
            parameter.name = "B";
            return;
        // float C = 0.364000;
        case kParamFormulaC:
            parameter.ranges.min = 0.0;
            parameter.ranges.max = 10.0;
            parameter.ranges.def = 0.364;
            parameter.name = "C";
            return;
        // float D = 0.25;
        case kParamFormulaD:
            parameter.ranges.min = 0.0;
            parameter.ranges.max = 10.0;
            parameter.ranges.def = 1.121;
            parameter.name = "D";
            return;
        // float E = 4.462000;
        case kParamFormulaE:
            parameter.ranges.min = 0.0;
            parameter.ranges.max = 10.0;
            parameter.ranges.def = 4.462;
            parameter.name = "E";
            return;
        // float base = -119.205;
        case kParamFormulaBase:
            parameter.ranges.min = -200.0;
            parameter.ranges.max = 200.0;
            parameter.ranges.def = -119.205;
            parameter.name = "base";
            return;
        case kParamFormulaVaccMul:
            parameter.ranges.min = 0.0;
            parameter.ranges.max = 20.0;
            parameter.ranges.def = 2.0;
            parameter.name = "Vacc multiplier";
            return;
        case kParamFormulaLimiter:
            parameter.hints = kParameterIsOutput;
            return;
        case kParamPrintParameters:
            parameter.name = "foo";
            return;
        }
    }

    // ----------------------------------------------------------------------------------------------------------------
    // Internal data

   /**
      Get the current value of a parameter.@n
      The host may call this function from any context, including realtime processing.
    */
    float getParameterValue(uint32_t index) const override
    {
        switch (index) {
        case kParamD:
            return 0.314f;
        case kParamGain:
            return fGainDB;
        }
    }

    void print_limits() {
        float freq_min = vcf_env_freq(0.0, fVco, fVmod, 0.0, A, B, C, D, E, base, VaccMul);
        float freq_max = vcf_env_freq(1.01, fVco, fVmod, 0.0, A, B, C, D, E, base, VaccMul);
        d_stdout("Freq min %f Freq max %f", freq_min, freq_max);
    }

   /**
      Change a parameter value.@n
      The host may call this function from any context, including realtime processing.@n
      When a parameter is marked as automatable, you must ensure no non-realtime operations are performed.
      @note This function will only be called for parameter inputs.
    */
    void setParameterValue(uint32_t index, float value) override
    {
        switch (index) {
        case kParamGain:
            fGainDB = value;
            fGainLinear = DB_CO(CLAMP(value, -90.0, 30.0));
            break;
        case kParamCutoff:
            fVco = value;
            d_stdout("fVco %f", fVco);
            // d_stdout("Min %0.3fHz Max %0.3f", vcf_env_freq(0.0, fVco, fVmod), vcf_env_freq(1.01, fVco, fVmod));
            break;
        case kParamResonance:
            fRes = value;
            d_stdout("fRes %f", fRes);
            break;
        case kParamVmod:
            fVmod = value;
            d_stdout("fVmod %f", fVmod);
            // d_stdout("Min %0.3fHz Max %0.3f", vcf_env_freq(0.0, fVco, fVmod), vcf_env_freq(1.01, fVco, fVmod));
            break;
        case kParamAccent:
            fVacc_amt = value;
            d_stdout("fVacc_amt %f", fVacc_amt);
            break;
        case kParamDecay:
            decTime = value;
            d_stdout("decTime %f", decTime);
            break;
        case kParamVcfAttack:
            atkTime = value;
            d_stdout("atkTime %f", atkTime);
            break;
        case kParamFormulaA:
            A = value;
            d_stdout("DSP A %f", A);
            break;
        case kParamFormulaB:
            B = value;
            d_stdout("DSP B %f", B);
            break;
        case kParamFormulaC:
            C = value;
            d_stdout("DSP C %f", C);
            break;
        case kParamFormulaD:
            D = value;
            d_stdout("DSP D %f", D);
            break;
        case kParamFormulaE:
            E = value;
            d_stdout("DSP E %f", E);
            break;
        case kParamFormulaBase:
            base = value;
            d_stdout("DSP base %f", base);
            break;
        case kParamFormulaVaccMul:
            VaccMul = value;
            d_stdout("DSP VaccMul %f", VaccMul);
            break;
        case kParamPrintParameters:
            printParameters();
            break;
        }

        print_limits();
    }

    // ----------------------------------------------------------------------------------------------------------------
    // Audio/MIDI Processing

   /**
      Activate this plugin.
    */
    void activate() override
    {
        fSmoothGain->flush();

        osc.prepare(getSampleRate());
        osc.setPitchCV(1.0f);

        vca_env.activate(getSampleRate());
        vcf_env.activate(getSampleRate());
        wowFilter.prepare(getSampleRate());

        filter.prepare(getSampleRate(), 300.0, 0.66);

        d_stdout("DSP Activate @ %.0fHz (%d samples)", getSampleRate(), getBufferSize());
    }

    void deactivate() override
    {
        d_stdout("DSP Deactivate");
    }

    // long int total_frames;
    // bool hiOrLo = false;
    bool gate;
    bool accent;
   /**
      Run/process function for plugins without MIDI input.
      @note Some parameters might be null if there are no audio inputs or outputs.
    */
    void run(const float** inputs, float** outputs, uint32_t frames, const MidiEvent* midiEvents, uint32_t midiEventCount) override
    {
        // get the left and right audio inputs
        const float* const inpL = inputs[0];
        const float* const inpR = inputs[1];

        // get the left and right audio outputs
        float* const outL = outputs[0];
        float* const outR = outputs[1];

        for (int m = 0; m < midiEventCount; ++m)
        {   
            uint8_t b0 = midiEvents[m].data[0]; // status + channel
            uint8_t b1 = midiEvents[m].data[1]; // note
            uint8_t b2 = midiEvents[m].data[2]; // velocity
            // d_stdout("0x%x %d %d", b0, b1, b2);
            // if ((b0 != 0x90) || (b0 != 0x80)) {
            //     d_stdout("Blaaa");
            //     continue;
            // }
            if (b0 == 0x90) { 
                gate = true;
                vcf_env.attackFrom(0.0f, 3, false, false); // from, shape, isDigital, isGated
                vca_env.attackFrom(0.0f, 1, false, false); // from, shape, isDigital, isGated
                accent = b2 > 100;
                if (accent) d_stdout("Accent!");
                float note_cv = (std::clamp((int)b1, 12, 72) - 12) / 12.0;
                osc.setPitchCV(note_cv);
                d_stdout("Note: %d CV %f", b1, note_cv);
            }
            if (b0 == 0x80) gate = false;

            // gate = _gate;
            // bool _gate =  (0x90 & midiEvents[m].data[0]) ? true : false;
            // uint8_t note = midiEvents[m].data[1];
            // bool _accent = midiEvents[m].data[2] > 110 ? true : false;
        }  

        wowFilter.setResonancePot(fRes);

        // apply gain against all samples
        for (uint32_t i=0; i < frames; ++i)
        {
            vcf_env.process(atkTime, accent ? -2.223 : decTime, 3, 1, false); // atk, dec, atk shape, dec shape, gate

            // if ((i % BLOCK_SIZE) == 0) {
                // update coefs
            float Vacc = wowFilter.processSample(accent ? vcf_env.output * fVacc_amt : 0.0f);
            float freq = vcf_env_freq(vcf_env.output, fVco, fVmod, Vacc, A, B, C, D, E, base, VaccMul);
            if (freq >= (getSampleRate() / 2.0)) {
                d_stdout("!!!!! limit freq %f", freq);
            }
            freq = std::clamp((double)freq, 1.0, getSampleRate() / 2.0);
            filter.calcCoeffs(freq, fRes);
            // }

            osc.process(squareBuffer, sawBuffer, 4);
            float filt;
            // square
            // filt = filter.processSample(squareBuffer);
            // saw
            filt = filter.processSample(sawBuffer);

            vca_env.process(-10.2877, gate ? std::log2(10.0f) : -7.38f, 1, 1, false); // atk, dec, atk shape, dec shape, gate

            float lastVCAEnv = vca_env.output;
            const float gain = fSmoothGain->process(fGainLinear);

            outputs[0][i] = filt * lastVCAEnv * gain;
            outputs[1][i] = Vacc;
            outputs[2][i] = vcf_env.output;
            outputs[3][i] = freq/(getSampleRate() / 2.0);
        }
    }

    // ----------------------------------------------------------------------------------------------------------------
    // Callbacks (optional)

   /**
      Optional callback to inform the plugin about a sample rate change.@n
      This function will only be called when the plugin is deactivated.
      @see getSampleRate()
    */
    void sampleRateChanged(double newSampleRate) override
    {
        fSampleRate = newSampleRate;
        fSmoothGain->setSampleRate(newSampleRate);
    }

    // ----------------------------------------------------------------------------------------------------------------
    
    // Information
    /**
       Get the plugin label.@n
       This label is a short restricted name consisting of only _, a-z, A-Z and 0-9 characters.
     */
     const char* getLabel() const noexcept override {return "synth303maker";}
    /**
       Get an extensive comment/description about the plugin.@n
       Optional, returns nothing by default.
     */
     const char* getDescription() const override {return "Make it threeeeeeee";}
    /**
       Get the plugin author/maker.
     */
     const char* getMaker() const noexcept override {return "Jean Pierre Cimalando, falkTX, Simon L";}
    /**
       Get the plugin license (a single line of text or a URL).@n
       For commercial plugins this should return some short copyright information.
     */
     const char* getLicense() const noexcept override {return "ISC";}
    /**
       Get the plugin version, in hexadecimal.
       @see d_version()
     */
     uint32_t getVersion() const noexcept override {return d_version(1, 0, 0);}
    /**
       Get the plugin unique Id.@n
       This value is used by LADSPA, DSSI and VST plugin formats.
       @see d_cconst()
     */
     int64_t getUniqueId() const noexcept override {return d_cconst('a', 'b', 'c', 'd');}

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginDSP)
};

// --------------------------------------------------------------------------------------------------------------------

Plugin* createPlugin()
{
    return new PluginDSP();
}

// --------------------------------------------------------------------------------------------------------------------

END_NAMESPACE_DISTRHO
