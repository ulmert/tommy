/*
    BSD 3-Clause License

    Copyright (c) 2022, Jacob Ulmert
    All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright notice, this
      list of conditions and the following disclaimer.

    * Redistributions in binary form must reproduce the above copyright notice,
      this list of conditions and the following disclaimer in the documentation
      and/or other materials provided with the distribution.

    * Neither the name of the copyright holder nor the names of its
      contributors may be used to endorse or promote products derived from
      this software without specific prior written permission.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
    AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
    DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
    FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
    DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
    SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
    CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
    OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
    OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "usermodfx.h"
#include "fx_api.h"
#include "float_math.h"

#define NVOICES 3

#define NOISETHRESHOLD 0.01 

#define BUFMINLENGTH 4096
#define BUFMAXLENGTH 32767

#define SAMPLEMODE_NOTRIG 0
#define SAMPLEMODE_SINGLETRIG 1
#define SAMPLEMODE_RETRIG 2

#define k_samplerate_recipf (2.08333333333333e-005f)

#define SDIV 32767

int16_t bufA[BUFMAXLENGTH + 1] __sdram;
int16_t bufB[BUFMAXLENGTH + 1] __sdram;

int16_t *pBufSampling;
uint16_t samplingIdx;
uint16_t samplingBufLen;
uint16_t lastSampledBufLength;
float samplingRootFreq;
float samplingTrigFreq;

int16_t *pBufPlayback;
uint16_t playbackBufLength;
float playbackRootFreq;
float playbackStep[NVOICES];
float playbackIdx[NVOICES];
uint16_t playbackBufLen[NVOICES];

float xfadePlaybackStep[NVOICES];
float xfadePlaybackIdx[NVOICES];
float xfadeLastGain[NVOICES];
uint16_t xfadePlaybackBufLen[NVOICES];

uint32_t dutySampleCount;

uint8_t isSampling, swapBuffers, sampleMode;

uint8_t playbackVceIdx;

void MODFX_INIT(uint32_t platform, uint32_t api)
{
    pBufSampling = &bufA[0]; 
    pBufPlayback = &bufB[0];

    isSampling = 0;

    swapBuffers = 0;

    playbackBufLength = 0;

    dutySampleCount = 0;

    uint8_t j = NVOICES;
    while (j > 0) {
        j--;
        playbackIdx[j] = BUFMAXLENGTH;
        xfadePlaybackIdx[j] = BUFMAXLENGTH;
        xfadePlaybackBufLen[j] = BUFMAXLENGTH;
    }

    sampleMode = SAMPLEMODE_NOTRIG;

    samplingBufLen = BUFMAXLENGTH;
    lastSampledBufLength = BUFMAXLENGTH;

    playbackVceIdx = 0;
}

void MODFX_PROCESS(const float *main_xn, float *main_yn,
                   const float *sub_xn,  float *sub_yn,
                   uint32_t frames)
{
 
  for (uint32_t i = 0; i < frames; i++) {
    const float oscillatorSample = main_xn[i + i + 1];

    const float audioCleanedSample = (main_yn[i + i] - oscillatorSample) * 2;
 
    main_yn[i + i + 1] = 0;
   
    if (oscillatorSample > NOISETHRESHOLD || oscillatorSample < -NOISETHRESHOLD) {
    
        if (oscillatorSample > NOISETHRESHOLD) {
            dutySampleCount++;
        }

    } else {
        if (dutySampleCount > (48000 / 1760)) {

            if (swapBuffers) {
                int16_t *pTemp = pBufSampling;
                pBufSampling = pBufPlayback;
                pBufPlayback = pTemp;
                swapBuffers = 0;
                playbackRootFreq = samplingRootFreq;
                lastSampledBufLength = samplingBufLen;
            }

            const float freq = 1.f / ((float)dutySampleCount * (1.f / 48000.f));

            if (!isSampling) {
                if (sampleMode == SAMPLEMODE_SINGLETRIG) {
                    samplingIdx = 0;
                    samplingTrigFreq = freq;  
                    samplingRootFreq = freq;
                    samplingBufLen = BUFMAXLENGTH;
                    isSampling = 1;

                } else if (sampleMode == SAMPLEMODE_RETRIG && 
                        (!samplingTrigFreq ||
                        (samplingTrigFreq > (freq - 0.5) && samplingTrigFreq < (freq + 0.5)))) {

                    samplingIdx = 0;
                    samplingTrigFreq = freq;    
                    samplingRootFreq = freq;
                    samplingBufLen = playbackBufLength;
                    isSampling = 1;
                }
            }
            
            if (sampleMode != SAMPLEMODE_SINGLETRIG) {

                xfadePlaybackStep[playbackVceIdx] = playbackStep[playbackVceIdx];
                xfadePlaybackIdx[playbackVceIdx] = playbackIdx[playbackVceIdx];
                xfadeLastGain[playbackVceIdx] = 1.f - playbackIdx[playbackVceIdx] / playbackBufLen[playbackVceIdx];
                xfadePlaybackBufLen[playbackVceIdx] = playbackBufLen[playbackVceIdx];

                playbackStep[playbackVceIdx] = freq / playbackRootFreq;
                playbackIdx[playbackVceIdx] = 0; 

                if (sampleMode == SAMPLEMODE_RETRIG) {
                    playbackBufLen[playbackVceIdx] = lastSampledBufLength;
                } else {
                    playbackBufLen[playbackVceIdx] = playbackBufLength;
                }

                playbackVceIdx++;
                if (playbackVceIdx >= NVOICES) {
                    playbackVceIdx = 0;
                }
            }

        }
        dutySampleCount = 0;
    }

    uint8_t j = NVOICES;

    while (j > 0) {
        j--;
        if (playbackIdx[j] < playbackBufLen[j]) {
            if (playbackIdx[j] > 128.f) {
                const float d = (1 - ((playbackIdx[j] - 128.f) / (float)(playbackBufLen[j] - 128)));
                const uint16_t playbackIdxInt = playbackIdx[j];
                const float fr = playbackIdx[j] - playbackIdxInt;

                const float sample = ((float)pBufPlayback[playbackIdxInt] * (1.0 - fr)) + ((float)pBufPlayback[playbackIdxInt + 1] * fr);
            
                main_yn[i + i + 1] += (sample / (float)SDIV) * d;

            } else {
                const uint16_t playbackIdxInt = playbackIdx[j];
                const float fr = playbackIdx[j] - playbackIdxInt;

                float sample = ((float)pBufPlayback[playbackIdxInt] * (1.0 - fr)) + ((float)pBufPlayback[playbackIdxInt + 1] * fr);

                if (xfadePlaybackStep[j] < xfadePlaybackBufLen[j]) {
                    const float d = xfadeLastGain[j] * (1.0 - (playbackIdx[j] / 128.f));
                    
                    const uint16_t playbackIdxInt = xfadePlaybackIdx[j];
                    const float fr = xfadePlaybackIdx[j] - playbackIdxInt;

                    sample = sample + (((float)pBufPlayback[playbackIdxInt] * (1.0 - fr)) + ((float)pBufPlayback[playbackIdxInt + 1] * fr)) * d;

                    xfadePlaybackIdx[j] += xfadePlaybackStep[j];

                }
                main_yn[i + i + 1] += (sample / (float)SDIV);
            }
            playbackIdx[j] += playbackStep[j];
        }
    }

    if (isSampling && (samplingIdx > 0 || sampleMode != SAMPLEMODE_SINGLETRIG || audioCleanedSample > 0.01 || audioCleanedSample < -0.01)) {
        float d = 1;
        if (samplingIdx < 128) {
            d = ((float)samplingIdx / 128.f);
        }
        pBufSampling[samplingIdx] = (int16_t) ((audioCleanedSample * d) * (float)SDIV); 

        if (samplingIdx >= samplingBufLen) {
            isSampling = 0;
            swapBuffers = 1;

            if (sampleMode == SAMPLEMODE_SINGLETRIG) {
                sampleMode = SAMPLEMODE_NOTRIG;    
            }
        } else {
            samplingIdx++;
        }
    }

    if (sampleMode == SAMPLEMODE_SINGLETRIG) {
        main_yn[i + i] = audioCleanedSample;
    } else {
        main_yn[i + i] =  main_yn[i + i + 1];
    }
  }

}

void MODFX_PARAM(uint8_t index, int32_t value)
{
  const float valf = q31_to_f32(value);
  switch (index) {
    case k_user_modfx_param_time:
        playbackBufLength = BUFMINLENGTH + ((BUFMAXLENGTH - BUFMINLENGTH) * valf);
        break;

    case k_user_modfx_param_depth:
        if (valf < 0.1) {
            sampleMode = SAMPLEMODE_SINGLETRIG;
        } else if (valf > 0.9) {
            sampleMode = SAMPLEMODE_RETRIG;
            samplingTrigFreq = 0;
        } else {
            sampleMode = SAMPLEMODE_NOTRIG;
        }
        break;
    
    default:
        break;
  }
}