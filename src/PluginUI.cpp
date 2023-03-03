/*
 * ImGui plugin example
 * Copyright (C) 2021 Jean Pierre Cimalando <jp-dev@inbox.ru>
 * Copyright (C) 2021-2022 Filipe Coelho <falktx@falktx.com>
 * SPDX-License-Identifier: ISC
 */
#include "DistrhoUI.hpp"
#include "ResizeHandle.hpp"

#include <locale.h>
#include "imgui_internal.h"

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
        kParamFormulaVaccOffset,
        kParamFormulaLimiter,
        kParamPrintParameters,
        kParamOutputLPFFreq,
        kParamCount
    };

    float fGain = 0.0f;
    float fOutputParam = 0.0f;
    ResizeHandle fResizeHandle;
    float fVco = 7.604;
    float fRes = 1.0;
    float fVmod = 1.0;
    float fVacc_amt = 1.0;

    // bool accent = true;

    sst::surgext_rack::dsp::envelopes::ADAREnvelope vcf_env;
    WowFilter wowFilter;

    float atkTime = -9.482;
    float decTime = -2.223;

    // float A = 1.633001;
    // float B = 0.626000;
    // float C = 0.324000;
    // float D = 0.191000;
    // float E = 4.462000;
    // float base = -119.205;
    // float VaccMul = 2.0;

    float A = 0.0;
    float B = 0.0;
    float C = 0.0;
    float D = 0.0;
    float E = 0.0;
    float base = -0.0;
    float VaccMul = 0.0;
    float VaccOffset = 0.0;
    
    bool do_update = true;

    float OutputLPFFreq = 25000.0;

    bool showPlot = false;
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

        ImGui::GetCurrentContext()->PlatformLocaleDecimalPoint = *localeconv()->decimal_point;
        d_stdout("Decimal point set to %c", ImGui::GetCurrentContext()->PlatformLocaleDecimalPoint);
    }

    ~PluginUI() {
        // ImPlot::DestroyContext();

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
        if (index == kParamD) return;

        switch (index) {
        case kParamGain:
            fGain = value;
            d_stdout("GUI fGain %f", fGain);
            repaint();
            return;
        // case kParamD:
        //     fOutputParam = value;
        //     d_stdout("GUI fOutputParam %f", fOutputParam);
        //     repaint();
        //     return;
        case kParamCutoff:
            fVco = value;
            d_stdout("GUI fVco %f", fVco);
            repaint();
            return;
        case kParamResonance:
            fRes = value;
            d_stdout("GUI fRes %f", fRes);
            repaint();
            return;
        case kParamVmod:
            fVmod = value;
            d_stdout("GUI fVmod %f", fVmod);
            repaint();
            return;
        case kParamAccent:
            fVacc_amt = value;
            d_stdout("GUI fVacc_amt %f", fVacc_amt);
            repaint();
            return;
        case kParamDecay:
            decTime = value;
            d_stdout("GUI decTime %f", decTime);
            repaint();
            return;
        case kParamVcfAttack:
            atkTime = value;
            d_stdout("GUI atkTime %f", atkTime);
            repaint();
            return;
        case kParamFormulaA:
            A = value;
            d_stdout("GUI A %f", A);
            repaint();
            return;
        case kParamFormulaB:
            B = value;
            d_stdout("GUI B %f", B);
            repaint();
            return;
        case kParamFormulaC:
            C = value;
            d_stdout("GUI C %f", C);
            repaint();
            return;
        case kParamFormulaD:
            D = value;
            d_stdout("GUI D %f", D);
            repaint();
            return;
        case kParamFormulaE:
            E = value;
            d_stdout("GUI E %f", E);
            repaint();
            return;
        case kParamFormulaBase:
            base = value;
            d_stdout("GUI base %f", base);
            repaint();
            return;
        case kParamFormulaVaccMul:
            VaccMul = value;
            d_stdout("GUI VaccMul %f", VaccMul);
            repaint();
            return;
        case kParamOutputLPFFreq:
            OutputLPFFreq = value;
            d_stdout("GUI OutputLPFFreq %f", OutputLPFFreq);
            repaint();
            return;
        }
    }


    // ----------------------------------------------------------------------------------------------------------------
    // Widget Callbacks

   /**
      ImGui specific onDisplay function.
    */
    float plot_vcf_x[48000];
    float plot_vcf_y[48000];
    float plot_vcf_y_acc[48000];
    float plot_accent_y[48000];
    float plot_accent_vmod[48000];
    int reduced_size;

    void onImGuiDisplay() override
    {

        const float width = getWidth();
        const float height = getHeight();
        const float margin = 4.0f * getScaleFactor();

        ImGui::SetNextWindowPos(ImVec2(margin, margin));
        ImGui::SetNextWindowSize(ImVec2(width - 2 * margin, height - 2 * margin));

        if (ImGui::Begin("synth 303 maker", nullptr, ImGuiWindowFlags_NoResize))
        {
            ImGui::Text("");
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

            ImGui::Text("");
            ImGui::Text("Synth");
            if (ImGui::SliderFloat("Cutoff - 0.51 -> 7.604", &fVco, 0.0f, 12.0f)) {
                setParameterValue(kParamCutoff, fVco);
                do_update |= true;
            }
            if (ImGui::SliderFloat("Resonance", &fRes, 0.0f, 1.0f)) {
                setParameterValue(kParamResonance, fRes);
                do_update |= true;
            }
            if (ImGui::SliderFloat("Envmod - 0.057 -> 1.0", &fVmod, 0.0f, 1.0f)) {
                setParameterValue(kParamVmod, fVmod);
                do_update |= true;
            }

            if (ImGui::SliderFloat("Accent amount", &fVacc_amt, 0.0f, 1.0f)) {
                setParameterValue(kParamAccent, fVacc_amt);
                do_update |= true;
            }

            if (ImGui::SliderFloat("Decay", &decTime, -2.223, 1.223)) {
                setParameterValue(kParamDecay, decTime);
                do_update |= true;
            }

            ImGui::Text("");

            if (ImGui::SliderFloat("Vcf Attack", &atkTime, -9.482, -4.0)) {
                setParameterValue(kParamVcfAttack, atkTime);
                do_update |= true;
            }

            // A B C D : step 0.01 step fast 0.1
            // E : 0.1 0.5
            // base 0.5 2.0
            // VaccMull 0.2 0.6
            ImGui::Text("");
            ImGui::Text("Filter frequency curve formula");

            static char formulaText[2048] = "float freq = (A * Vco + B) * std::exp(C * Vmod + D * (Vacc * VaccMul) + E) + base;";
            ImGui::Text(formulaText, sizeof(formulaText));

            ImGui::Text("");
            // if (ImGui::InputFloat("A - base scaling - default = 2.14", &A, 0.01, 0.1)) {
            if (ImGui::SliderFloat("A - base scaling - default = 2.14", &A, 0.0, 10.0)) {
                setParameterValue(kParamFormulaA, A);
                do_update |= true;
            }

            // if (ImGui::InputFloat("B - base offset - default = 0.3", &B, 0.01, 0.1)) {
            if (ImGui::SliderFloat("B - base offset - default = 0.3", &B, 0.0, 5.0)) {
                setParameterValue(kParamFormulaB, B);
                do_update |= true;
            }

            // if (ImGui::InputFloat("C - exponent Envmod scaling - default = 0.45", &C, 0.01, 0.1)) {
            if (ImGui::SliderFloat("C - exponent Envmod scaling - default = 0.45", &C, 0.0, 5.0)) {
                setParameterValue(kParamFormulaC, C);
                do_update |= true;
            }

            // if (ImGui::InputFloat("D - exponent Accent scaling", &D, 0.01, 0.1)) {
            if (ImGui::SliderFloat("D - exponent Accent scaling", &D, 0.0, 5.0)) {
                setParameterValue(kParamFormulaD, D);
                do_update |= true;
            }

            // if (ImGui::InputFloat("E - exponent offset - default = 4.62", &E, 0.01, 0.1)) {
            if (ImGui::SliderFloat("E - exponent offset - default = 4.62", &E, 0.0, 10.0)) {
                setParameterValue(kParamFormulaE, E);
                do_update |= true;
            }

            // if (ImGui::InputFloat("Base - constant base frequency offset", &base, 0.5, 2.0)) {
            //     setParameterValue(kParamFormulaBase, base);
            //     do_update |= true;
            // }

            if (ImGui::SliderFloat("VaccMul", &VaccMul, 0.0, 20.0)) {
                setParameterValue(kParamFormulaVaccMul, VaccMul);
                do_update |= true;
            }

            // if (ImGui::SliderFloat("VaccOffset", &VaccOffset, -2.0, 2.0)) {
            //     setParameterValue(kParamFormulaVaccOffset, VaccOffset);
            // }

            if (ImGui::SliderFloat("OutputLPFFreq", &OutputLPFFreq, 100, 48000.0)) {
                setParameterValue(kParamOutputLPFFreq, OutputLPFFreq);
            }

            if (ImGui::Button("Print parameters and limits")) {
                setParameterValue(kParamPrintParameters, 1.0f);
            }

            if (ImGui::Checkbox("Show plot", &showPlot)) {
                if (showPlot) do_update = true;
            }

            if (do_update) {
                d_stdout("Update!");

                wowFilter.setResonancePot(fRes);

                bool accent = false;

                for (int plot = 0; plot < 2; ++plot)
                {
                    vcf_env.immediatelyEnd();
                    vcf_env.attackFrom(0.0f, 3, false, false);

                    int reduced_x = 0;

                    for (int samp = 0; samp < 48000; ++samp)
                    {   
                        vcf_env.process(atkTime, accent ? -2.223 : decTime, 3, 1, false);
                        float Vacc = wowFilter.processSample(accent ? vcf_env.output * fVacc_amt : 0.0f);

                        float Vmod_scale = 6.9*fVmod+1.3;
                        float Vmod_bias = -1.2*fVmod+3;
                        float Vmod = (Vmod_scale * vcf_env.output + Vmod_bias) - 3.2f; // 3.2 == Q9 bias
                        float freq = vcf_env_freq(vcf_env.output, fVco, fVmod, Vacc, A, B, C, D, E, base, VaccMul, VaccOffset);

                        if (samp % 35 == 0) {
                            plot_accent_vmod[reduced_x] = C * Vmod + D * (Vacc * VaccMul) + E;
                            plot_accent_y[reduced_x] = Vacc;
                            plot_vcf_x[reduced_x] = samp;
                            if (plot == 0) {
                                plot_vcf_y[reduced_x] = freq;
                            }
                            if (plot == 1) {
                                plot_vcf_y_acc[reduced_x] = freq;
                            }
                            reduced_x += 1;
                        }
                    }

                    accent = true;
                    reduced_size = reduced_x;
                }

                do_update = false;
                d_stdout("reduced_size: %d", reduced_size);
            }

            // if (ImGui::SliderFloat("D", &D, 0.0f, 2.0f)) {
                // setParameterValue(kParamVmod, fVmod);
            // }

            // if (ImGui::IsItemDeactivated())
            // {
            //     editParameter(kParamGain, false);
            // }

            // ImGui::LabelText("<- OutputParam", "%f", fOutputParam);
            // if (ImGui::Checkbox("Accent", &accent)) {
            //     d_stdout("Accent! %d", accent);
            // }

            // static char aboutText[2048] = "float A = 2.243000;\nfloat B = 0.626000;\nfloat C = 0.364000;\nfloat D = xx;\nfloat E = 4.462000;\nfloat base = -119.205;\n// guest formula\nIc,11 = (A*Vco + B)*e^(C*Vmod + D*Vacc +E) + base\n";

            if (showPlot) {
                if (ImPlot::BeginPlot("Guest formula!", ImVec2(-1.0,-1.0))) {
                    ImPlot::SetupLegend(ImPlotLocation_North|ImPlotLocation_East, 0);
                    ImPlot::SetupAxis(ImAxis_Y1, "Frequency (Hz)", ImPlotAxisFlags_AutoFit);
                    // ImPlot::SetupAxis(ImAxis_Y2, "", ImPlotAxisFlags_AutoFit);
                    ImPlot::SetAxes(ImAxis_X1, ImAxis_Y1);
                    ImPlot::PlotLine("Freq (non accented)", plot_vcf_x, plot_vcf_y, reduced_size);
                    ImPlot::PlotLine("Freq (accented)", plot_vcf_x, plot_vcf_y_acc, reduced_size);
                    // ImPlot::SetAxes(ImAxis_X1, ImAxis_Y2);
                    // ImPlot::PlotLine("Accent", plot_vcf_x, plot_accent_y, 48000/2);
                    // ImPlot::PlotLine("Vmod", plot_vcf_x, plot_accent_vmod, 48000/2);
                    ImPlot::EndPlot();
                }
            }
            // bool open = true;
            // ImPlot::ShowDemoWindow(&open);
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
