/*
 * ImGui plugin example
 * Copyright (C) 2021 Jean Pierre Cimalando <jp-dev@inbox.ru>
 * Copyright (C) 2021-2022 Filipe Coelho <falktx@falktx.com>
 * SPDX-License-Identifier: ISC
 */

#include "DistrhoUI.hpp"
#include "ResizeHandle.hpp"

#include "implot.h"

#include "ADAREnvelope.h"
#include "WowFilter.h"

#include "synth303common.hpp"

START_NAMESPACE_DISTRHO

// --------------------------------------------------------------------------------------------------------------------

class PluginUI : public UI
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

    float fGain = 0.0f;
    float fOutputParam = 0.0f;
    ResizeHandle fResizeHandle;
    float fVco = 12.0;
    float fRes = 1.0;
    float fVmod = 1.0;
    float fVacc_amt = 1.0;

    bool accent = true;

    sst::surgext_rack::dsp::envelopes::ADAREnvelope vcf_env;
    WowFilter wowFilter;

    float atkTime = -9.482;
    float decTime = -2.223;

    float A = 1.633001;
    float B = 0.626000;
    float C = 0.324000;
    float D = 0.191000;
    float E = 4.462000;
    float base = -119.205;
    float VaccMul = 2.0;
    
    bool do_update = true;

    // inline float vcf_env_freq(float vcf_env, float Vco, float Vmod_amt, float Vacc) {
    //     float Vmod_scale = 6.9*Vmod_amt+1.3;
    //     float Vmod_bias = -1.2*Vmod_amt+3;
    //     float Vmod = (Vmod_scale * vcf_env + Vmod_bias) - 3.2f; // 3.2 == Q9 bias
    //     // float Vacc = accent ? wowFilter : 0.0f;
    //     // guest formula
    //     // Ic,11 = (A*Vco + B)*e^(C*Vmod + D*Vacc +E)
    //     return (A * Vco + B) * std::exp(C * Vmod + D * Vacc * VaccMul + E) + base; // + D * Vacc
    // }

    // ----------------------------------------------------------------------------------------------------------------

public:
   /**
      UI class constructor.
      The UI should be initialized to a default state that matches the plugin side.
    */
    PluginUI()
        : UI(DISTRHO_UI_DEFAULT_WIDTH, DISTRHO_UI_DEFAULT_HEIGHT),
          fResizeHandle(this)
    {   
        ImPlot::CreateContext();
        setGeometryConstraints(DISTRHO_UI_DEFAULT_WIDTH, DISTRHO_UI_DEFAULT_HEIGHT, true);

        // hide handle if UI is resizable
        if (isResizable())
            fResizeHandle.hide();

        vcf_env.activate(getSampleRate());
        wowFilter.prepare(getSampleRate());
    }

    ~PluginUI() {
        ImPlot::DestroyContext();

        // d_stdout("float Vco = %f;", Vco);
        // d_stdout("float Vmod_amt = %f;", Vmod_amt);
    }

protected:
    // ----------------------------------------------------------------------------------------------------------------
    // DSP/Plugin Callbacks

   /**
      A parameter has changed on the plugin side.@n
      This is called by the host to inform the UI about parameter changes.
    */
    void parameterChanged(uint32_t index, float value) override
    {
        switch (index) {
        case kParamGain:
            fGain = value;
            repaint();
            return;
        case kParamD:
            fOutputParam = value;
            return;
        }
    }

    float plot_accent_y[48000];
    void plot_vcf_env(float* dest_x, float* dest_y, int size) {
        vcf_env.immediatelyEnd();
        vcf_env.attackFrom(0.0f, 3, false, false);

        for (int i = 0; i < size; ++i)
        {
            vcf_env.process(atkTime, decTime, 3, 1, false);
            float Vacc = wowFilter.processSample(accent ? vcf_env.output * fVacc_amt : 0.0f);
            plot_accent_y[i] = Vacc;
            float freq = vcf_env_freq(vcf_env.output, fVco, fVmod, Vacc, A, B, C, D, E, base, VaccMul);
            dest_x[i] = i;
            dest_y[i] = freq;
        }
    }

    // ----------------------------------------------------------------------------------------------------------------
    // Widget Callbacks

   /**
      ImGui specific onDisplay function.
    */
    float plot_vcf_x[48000];
    float plot_vcf_y[48000];
    void onImGuiDisplay() override
    {
        const float width = getWidth();
        const float height = getHeight();
        const float margin = 20.0f * getScaleFactor();

        ImGui::SetNextWindowPos(ImVec2(margin, margin));
        ImGui::SetNextWindowSize(ImVec2(width - 2 * margin, height - 2 * margin));

        if (ImGui::Begin("synth 303 maker", nullptr, ImGuiWindowFlags_NoResize))
        {

            if (ImGui::SliderFloat("Gain (dB)", &fGain, -90.0f, 30.0f))
            {
                if (ImGui::IsItemActivated())
                    editParameter(kParamGain, true);

                setParameterValue(kParamGain, fGain);
            }

            // cutoff
            // 12 = 100%
            // 4.9 = 50%
            // 2.36 = 0%

            // envmod
            // 1.0 = 100%
            // 0.283 = 50%
            // 0.099 = 0%
            if (ImGui::SliderFloat("Cutoff", &fVco, 2.3f, 12.0f)) {
                setParameterValue(kParamCutoff, fVco);
            }
            if (ImGui::SliderFloat("Resonance", &fRes, 0.099f, 1.0f)) {
                setParameterValue(kParamResonance, fRes);
            }
            if (ImGui::SliderFloat("Envmod", &fVmod, 0.0f, 1.0f)) {
                setParameterValue(kParamVmod, fVmod);
            }

            if (ImGui::SliderFloat("Accent amount", &fVacc_amt, 0.0f, 1.0f)) {
                setParameterValue(kParamAccent, fVacc_amt);
            }

            if (ImGui::SliderFloat("Decay", &decTime, -2.223, 1.223)) {
                setParameterValue(kParamDecay, decTime);
            }

            if (ImGui::SliderFloat("Vcf Attack", &atkTime, -9.482, -4.0)) {
                setParameterValue(kParamVcfAttack, atkTime);
            }

            // A B C D : step 0.01 step fast 0.1
            // E : 0.1 0.5
            // base 0.5 2.0
            // VaccMull 0.2 0.6

            static char formulaText[2048] = "float freq = (A * Vco + B) * std::exp(C * Vmod + D * (Vacc * VaccMul) + E) + base;";
            ImGui::Text(formulaText, sizeof(formulaText));

            if (ImGui::InputFloat("A - base scaling", &A, 0.01, 0.1)) {
                setParameterValue(kParamFormulaA, A);
                do_update |= true;
            }

            if (ImGui::InputFloat("B - base offset", &B, 0.01, 0.1)) {
                setParameterValue(kParamFormulaB, B);
                do_update |= true;
            }

            if (ImGui::InputFloat("C - exponent Envmod scaling", &C, 0.01, 0.1)) {
                setParameterValue(kParamFormulaC, C);
                do_update |= true;
            }

            if (ImGui::InputFloat("D - exponent Accent scaling", &D, 0.01, 0.1)) {
                setParameterValue(kParamFormulaD, D);
                do_update |= true;
            }

            if (ImGui::InputFloat("E - exponent offset", &E, 0.1, 0.5)) {
                setParameterValue(kParamFormulaE, E);
                do_update |= true;
            }

            if (ImGui::InputFloat("Base - constant base frequency offset", &base, 0.5, 2.0)) {
                setParameterValue(kParamFormulaBase, base);
                do_update |= true;
            }

            if (ImGui::SliderFloat("VaccMul", &VaccMul, 0.0, 20.0)) {
                setParameterValue(kParamFormulaVaccMul, VaccMul);
            }

            if (ImGui::Button("Print parameters and limits")) {
                setParameterValue(kParamPrintParameters, 1.0f);
            }

            if (do_update) {
                d_stdout("Update!");
                do_update = false;
            }

            // if (ImGui::SliderFloat("D", &D, 0.0f, 2.0f)) {
                // setParameterValue(kParamVmod, fVmod);
            // }

            // if (ImGui::IsItemDeactivated())
            // {
            //     editParameter(kParamGain, false);
            // }

            // ImGui::LabelText("<- OutputParam", "%f", fOutputParam);
            if (ImGui::Checkbox("Accent", &accent)) {
                d_stdout("Accent! %d", accent);
            }

            // static char aboutText[2048] = "float A = 2.243000;\nfloat B = 0.626000;\nfloat C = 0.364000;\nfloat D = xx;\nfloat E = 4.462000;\nfloat base = -119.205;\n// guest formula\nIc,11 = (A*Vco + B)*e^(C*Vmod + D*Vacc +E) + base\n";

            wowFilter.setResonancePot(fRes);

            plot_vcf_env(plot_vcf_x, plot_vcf_y, 48000);
            if (ImPlot::BeginPlot("Guest formula!", ImVec2(-1.0,-1.0))) {
                // ImPlot::SetupAxis(ImAxis_Y2, "", ImPlotAxisFlags_AuxDefault);
                // ImPlot::SetAxes(ImAxis_X1, ImAxis_Y1);
                ImPlot::PlotLine("Freq", plot_vcf_x, plot_vcf_y, 48000);
                // ImPlot::SetAxes(ImAxis_X1, ImAxis_Y2);
                // ImPlot::PlotLine("Accent", plot_vcf_x, plot_accent_y, 48000);
                ImPlot::EndPlot();
            }
        }
        ImGui::End();
    }

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginUI)
};

// --------------------------------------------------------------------------------------------------------------------

UI* createUI()
{
    return new PluginUI();
}

// --------------------------------------------------------------------------------------------------------------------

END_NAMESPACE_DISTRHO
