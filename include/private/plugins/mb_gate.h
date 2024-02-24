/*
 * Copyright (C) 2024 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2024 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugins-mb-gate
 * Created on: 3 авг. 2021 г.
 *
 * lsp-plugins-mb-gate is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * lsp-plugins-mb-gate is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with lsp-plugins-mb-gate. If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef PRIVATE_PLUGINS_MB_GATE_H_
#define PRIVATE_PLUGINS_MB_GATE_H_

#include <lsp-plug.in/plug-fw/plug.h>
#include <lsp-plug.in/plug-fw/core/IDBuffer.h>
#include <lsp-plug.in/dsp-units/ctl/Bypass.h>
#include <lsp-plug.in/dsp-units/ctl/Counter.h>
#include <lsp-plug.in/dsp-units/dynamics/Gate.h>
#include <lsp-plug.in/dsp-units/filters/DynamicFilters.h>
#include <lsp-plug.in/dsp-units/filters/Equalizer.h>
#include <lsp-plug.in/dsp-units/filters/Filter.h>
#include <lsp-plug.in/dsp-units/util/Analyzer.h>
#include <lsp-plug.in/dsp-units/util/Delay.h>
#include <lsp-plug.in/dsp-units/util/FFTCrossover.h>
#include <lsp-plug.in/dsp-units/util/MeterGraph.h>
#include <lsp-plug.in/dsp-units/util/Sidechain.h>

#include <private/meta/mb_gate.h>

namespace lsp
{
    namespace plugins
    {
        /**
         * Multiband Gate plugin Series
         */
        class mb_gate: public plug::Module
        {
            public:
                enum g_mode_t
                {
                    MBGM_MONO,
                    MBGM_STEREO,
                    MBGM_LR,
                    MBGM_MS
                };

            protected:
                enum sync_t
                {
                    S_GATE_CURVE    = 1 << 0,
                    S_HYST_CURVE    = 1 << 1,
                    S_EQ_CURVE      = 1 << 2,
                    S_BAND_CURVE    = 1 << 3,

                    S_DYN_CURVE     = S_GATE_CURVE | S_HYST_CURVE,
                    S_ALL           = S_DYN_CURVE | S_EQ_CURVE | S_BAND_CURVE
                };

                enum xover_mode_t
                {
                    XOVER_CLASSIC,                              // Classic mode
                    XOVER_MODERN,                               // Modern mode
                    XOVER_LINEAR_PHASE                          // Linear phase mode
                };

                typedef struct gate_band_t
                {
                    dspu::Sidechain     sSC;                // Sidechain module
                    dspu::Equalizer     sEQ[2];             // Sidechain equalizers
                    dspu::Gate          sGate;              // Gate
                    dspu::Filter        sPassFilter;        // Passing filter for 'classic' mode
                    dspu::Filter        sRejFilter;         // Rejection filter for 'classic' mode
                    dspu::Filter        sAllFilter;         // All-pass filter for phase compensation
                    dspu::Delay         sScDelay;           // Delay for lookahead purpose

                    float              *vBuffer;            // Crossover band data
                    float              *vSc;                // Transfer function for sidechain
                    float              *vTr;                // Transfer function for band
                    float              *vVCA;               // Voltage-controlled amplification value for each band
                    float               fScPreamp;          // Sidechain preamp

                    float               fFreqStart;
                    float               fFreqEnd;

                    float               fFreqHCF;           // Cutoff frequency for low-pass filter
                    float               fFreqLCF;           // Cutoff frequency for high-pass filter
                    float               fMakeup;            // Makeup gain
                    float               fEnvLevel;          // Envelope level
                    float               fGainLevel;         // Gain adjustment level
                    size_t              nLookahead;         // Lookahead amount

                    bool                bEnabled;           // Enabled flag
                    bool                bCustHCF;           // Custom frequency for high-cut filter
                    bool                bCustLCF;           // Custom frequency for low-cut filter
                    bool                bMute;              // Mute channel
                    bool                bSolo;              // Solo channel
                    bool                bExtSc;             // External sidechain
                    size_t              nSync;              // Synchronize output data flags
                    size_t              nFilterID;          // Identifier of the filter

                    plug::IPort        *pExtSc;             // External sidechain
                    plug::IPort        *pScSource;          // Sidechain source
                    plug::IPort        *pScSpSource;        // Sidechain split source
                    plug::IPort        *pScMode;            // Sidechain mode
                    plug::IPort        *pScLook;            // Sidechain lookahead
                    plug::IPort        *pScReact;           // Sidechain reactivity
                    plug::IPort        *pScPreamp;          // Sidechain preamp
                    plug::IPort        *pScLpfOn;           // Sidechain low-pass on
                    plug::IPort        *pScHpfOn;           // Sidechain hi-pass on
                    plug::IPort        *pScLcfFreq;         // Sidechain low-cut frequency
                    plug::IPort        *pScHcfFreq;         // Sidechain hi-cut frequency
                    plug::IPort        *pScFreqChart;       // Sidechain band frequency chart

                    plug::IPort        *pEnable;            // Enable gate
                    plug::IPort        *pSolo;              // Soloing
                    plug::IPort        *pMute;              // Muting
                    plug::IPort        *pHyst;              // Hysteresis flag
                    plug::IPort        *pThresh[2];         // Threshold
                    plug::IPort        *pZone[2];           // Reduction zone
                    plug::IPort        *pAttack;            // Attack time
                    plug::IPort        *pRelease;           // Release time
                    plug::IPort        *pHold;              // Hold time
                    plug::IPort        *pReduction;         // Reduction
                    plug::IPort        *pMakeup;            // Makeup gain

                    plug::IPort        *pFreqEnd;           // Frequency range end
                    plug::IPort        *pCurveGraph[2];     // Gate curve graph
                    plug::IPort        *pEnvLvl;            // Envelope level meter
                    plug::IPort        *pCurveLvl;          // Reduction curve level meter
                    plug::IPort        *pMeterGain;         // Reduction gain meter
                } gate_band_t;

                typedef struct split_t
                {
                    bool                bEnabled;           // Split band is enabled
                    float               fFreq;              // Split band frequency

                    plug::IPort        *pEnabled;           // Enable port
                    plug::IPort        *pFreq;              // Split frequency
                } split_t;

                typedef struct channel_t
                {
                    dspu::Bypass        sBypass;            // Bypass
                    dspu::Filter        sEnvBoost[2];       // Envelope boost filter
                    dspu::Delay         sDelay;             // Delay for lookahead compensation purpose
                    dspu::Delay         sDryDelay;          // Delay for dry signal
                    dspu::Delay         sXOverDelay;        // Delay for crossover
                    dspu::Equalizer     sDryEq;             // Dry equalizer
                    dspu::FFTCrossover  sFFTXOver;          // FFT crossover for linear phase

                    gate_band_t         vBands[meta::mb_gate_metadata::BANDS_MAX];       // Gateander bands
                    split_t             vSplit[meta::mb_gate_metadata::BANDS_MAX-1];     // Split bands
                    gate_band_t        *vPlan[meta::mb_gate_metadata::BANDS_MAX];        // Execution plan (band indexes)
                    size_t              nPlanSize;              // Plan size

                    float              *vIn;                // Input data buffer
                    float              *vOut;               // Output data buffer
                    float              *vScIn;              // Sidechain data buffer (if present)

                    float              *vInAnalyze;         // Input signal analysis
                    float              *vInBuffer;          // Input buffer
                    float              *vBuffer;            // Common data processing buffer
                    float              *vScBuffer;          // Sidechain buffer
                    float              *vExtScBuffer;       // External sidechain buffer
                    float              *vTr;                // Transfer function
                    float              *vTrMem;             // Transfer buffer (memory)

                    size_t              nAnInChannel;       // Analyzer channel used for input signal analysis
                    size_t              nAnOutChannel;      // Analyzer channel used for output signal analysis
                    bool                bInFft;             // Input signal FFT enabled
                    bool                bOutFft;            // Output signal FFT enabled

                    plug::IPort        *pIn;                // Input
                    plug::IPort        *pOut;               // Output
                    plug::IPort        *pScIn;              // Sidechain
                    plug::IPort        *pFftIn;             // Pre-processing FFT analysis data
                    plug::IPort        *pFftInSw;           // Pre-processing FFT analysis control port
                    plug::IPort        *pFftOut;            // Post-processing FFT analysis data
                    plug::IPort        *pFftOutSw;          // Post-processing FFT analysis controlport
                    plug::IPort        *pAmpGraph;          // Gate's amplitude graph
                    plug::IPort        *pInLvl;             // Input level meter
                    plug::IPort        *pOutLvl;            // Output level meter
                } channel_t;

            protected:
                dspu::Analyzer          sAnalyzer;              // Analyzer
                dspu::DynamicFilters    sFilters;               // Dynamic filters for each band in 'modern' mode
                dspu::Counter           sCounter;               // Sync counter
                size_t                  nMode;                  // Gate channel mode
                bool                    bSidechain;             // External side chain
                bool                    bEnvUpdate;             // Envelope filter update
                xover_mode_t            enXOver;                // Crossover mode
                bool                    bStereoSplit;           // Stereo split mode
                size_t                  nEnvBoost;              // Envelope boost
                channel_t              *vChannels;              // Gate channels
                float                   fInGain;                // Input gain
                float                   fDryGain;               // Dry gain
                float                   fWetGain;               // Wet gain
                float                   fZoom;                  // Zoom
                uint8_t                *pData;                  // Aligned data pointer
                float                  *vSc[2];                 // Sidechain signal data
                float                  *vAnalyze[4];            // Analysis buffer
                float                  *vBuffer;                // Temporary buffer
                float                  *vEnv;                   // Gate envelope buffer
                float                  *vTr;                    // Transfer buffer
                float                  *vPFc;                   // Pass filter characteristics buffer
                float                  *vRFc;                   // Reject filter characteristics buffer
                float                  *vFreqs;                 // Analyzer FFT frequencies
                float                  *vCurve;                 // Curve
                uint32_t               *vIndexes;               // Analyzer FFT indexes
                core::IDBuffer         *pIDisplay;              // Inline display buffer

                plug::IPort            *pBypass;                // Bypass port
                plug::IPort            *pMode;                  // Global operating mode
                plug::IPort            *pInGain;                // Input gain port
                plug::IPort            *pOutGain;               // Output gain port
                plug::IPort            *pDryGain;               // Dry gain port
                plug::IPort            *pWetGain;               // Wet gain port
                plug::IPort            *pDryWet;                // Dry/Wet gain balance port
                plug::IPort            *pReactivity;            // Reactivity
                plug::IPort            *pShiftGain;             // Shift gain port
                plug::IPort            *pZoom;                  // Zoom port
                plug::IPort            *pEnvBoost;              // Envelope adjust
                plug::IPort            *pStereoSplit;           // Split left/right independently

            protected:
                static bool compare_bands_for_sort(const gate_band_t *b1, const gate_band_t *b2);
                static dspu::sidechain_source_t     decode_sidechain_source(int source, bool split, size_t channel);
                static size_t                       select_fft_rank(size_t sample_rate);
                static void                         process_band(void *object, void *subject, size_t band, const float *data, size_t sample, size_t count);

            protected:
                void                do_destroy();

            public:
                explicit mb_gate(const meta::plugin_t *metadata, bool sc, size_t mode);
                mb_gate(const mb_gate &) = delete;
                mb_gate(mb_gate &&) = delete;
                virtual ~mb_gate() override;

                mb_gate & operator = (const mb_gate &) = delete;
                mb_gate & operator = (mb_gate &&) = delete;

                virtual void        init(plug::IWrapper *wrapper, plug::IPort **ports) override;
                virtual void        destroy() override;

            public:
                virtual void        update_settings() override;
                virtual void        update_sample_rate(long sr) override;
                virtual void        ui_activated() override;

                virtual void        process(size_t samples) override;
                virtual bool        inline_display(plug::ICanvas *cv, size_t width, size_t height) override;

                virtual void        dump(dspu::IStateDumper *v) const override;
        };

    } /* namespace plugins */
} /* namespace lsp */

#endif /* PRIVATE_PLUGINS_MB_GATE_H_ */
