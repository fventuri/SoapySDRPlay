/*
 * The MIT License (MIT)
 * 
 * Copyright (c) 2015 Charles J. Cliffe

 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:

 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.

 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "SoapySDRPlay.hpp"
#include <SoapySDR/Registry.hpp>

#if !defined(_M_X64) && !defined(_M_IX86)
#define sprintf_s(buffer, buffer_size, stringbuffer, ...) (sprintf(buffer, stringbuffer, __VA_ARGS__))
#endif

static std::map<std::string, SoapySDR::Kwargs> _cachedResults;

static std::vector<SoapySDR::Kwargs> findSDRPlay(const SoapySDR::Kwargs &args)
{
   std::vector<SoapySDR::Kwargs> results;
   unsigned int nDevs = 0;
   char lblstr[128];

   //Enable (= 1) API calls tracing,
   //but only for debug purposes due to its performance impact. 
   mir_sdr_DebugEnable(0);

   std::string baseLabel = "SDRplay Dev";

   // list devices by API
   mir_sdr_DeviceT rspDevs[MAX_RSP_DEVICES];
   mir_sdr_GetDevices(&rspDevs[0], &nDevs, MAX_RSP_DEVICES);

  for (unsigned int i = 0; i < nDevs; i++)
  {
     if (rspDevs[i].devAvail)
     {
        SoapySDR::Kwargs dev;
        dev["serial"] = rspDevs[i].SerNo;
        const bool serialMatch = args.count("serial") == 0 or args.at("serial") == dev["serial"];
        if (not serialMatch) continue;
        if (rspDevs[i].hwVer > 253)
        {
           sprintf_s(lblstr, sizeof(lblstr), "SDRplay Dev%d RSP1A %s", i, rspDevs[i].SerNo);
        }
        else if (rspDevs[i].hwVer == 3)
        {
           sprintf_s(lblstr, sizeof(lblstr), "SDRplay Dev%d RSPduo %s", i, rspDevs[i].SerNo);
        }
        else
        {
           sprintf_s(lblstr, sizeof(lblstr), "SDRplay Dev%d RSP%d %s", i, rspDevs[i].hwVer, rspDevs[i].SerNo);
        }
        dev["label"] = lblstr;
        results.push_back(dev);
        _cachedResults[rspDevs[i].SerNo] = dev;
     }
  }

    //fill in the cached results for claimed handles
    for (const auto &serial : SoapySDRPlay_getClaimedSerials())
    {
        if (_cachedResults.count(serial) == 0) continue;
        if (args.count("serial") != 0 and args.at("serial") != serial) continue;
        results.push_back(_cachedResults.at(serial));
    }

   return results;
}

static SoapySDR::Device *makeSDRPlay(const SoapySDR::Kwargs &args)
{
    return new SoapySDRPlay(args);
}

static SoapySDR::Registry registerSDRPlay("sdrplay", &findSDRPlay, &makeSDRPlay, SOAPY_SDR_ABI_VERSION);
