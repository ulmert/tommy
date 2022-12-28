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

#include "userosc.h"

uint32_t dutySampleCount;
uint8_t noteOn;

void OSC_INIT(uint32_t platform, uint32_t api)
{
  (void)platform;
  (void)api;

  dutySampleCount = 0;
  noteOn = 0;
}

void OSC_CYCLE(const user_osc_param_t * const params, int32_t *yn, const uint32_t frames)
{
  if (noteOn) {
    dutySampleCount = (uint32_t)((48000.f) / osc_notehzf((params->pitch)>>8));
    noteOn = 0;
  }

  q31_t * __restrict y = (q31_t *)yn;
  const q31_t * y_e = y + frames;

  for (; y != y_e; ) {
    if (dutySampleCount) {
       *(y++) = f32_to_q31(1);
      dutySampleCount--;
    } else {
       *(y++) = f32_to_q31(0);
    }
  }
  
}

void OSC_NOTEON(const user_osc_param_t * const params) 
{
  noteOn = 1;
}

void OSC_NOTEOFF(const user_osc_param_t * const params)
{
  
}

void OSC_PARAM(uint16_t index, uint16_t value)
{ 
}