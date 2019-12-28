/*
 * The MIT License (MIT)
 * 
 * Copyright (c) 2015 Charles J. Cliffe
 * Copyright (c) 2019 Franco Venturi - changes for SDRplay API version 3
 *                                     and Dual Tuner for RSPduo

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

static bool isAtExitRegistered = false;
static sdrplay_api_DeviceT rspDevs[SDRPLAY_MAX_DEVICES];
bool isSdrplayApiOpen = false;
sdrplay_api_DeviceT *deviceSelected = nullptr;

static void close_sdrplay_api(void)
{
   if (deviceSelected)
   {
      sdrplay_api_ReleaseDevice(deviceSelected);
      deviceSelected = nullptr;
   }
   if (isSdrplayApiOpen == true)
   {
      sdrplay_api_Close();
      isSdrplayApiOpen = false;
   }
}

static std::vector<SoapySDR::Kwargs> findSDRPlay(const SoapySDR::Kwargs &args)
{
   std::unordered_map<int,std::string> deviceNames = {
       {SDRPLAY_RSP1_ID,   "RSP1"},
       {SDRPLAY_RSP1A_ID,  "RSP1A"},
       {SDRPLAY_RSP2_ID,   "RSP2"},
       {SDRPLAY_RSPduo_ID, "RSPduo"},
       {SDRPLAY_RSPdx_ID,  "RSPdx"}
   };
   std::unordered_map<sdrplay_api_RspDuoModeT,std::string> rspDuoModeNames = {
       {sdrplay_api_RspDuoMode_Unknown,      "Unknown"},
       {sdrplay_api_RspDuoMode_Single_Tuner, "Single"},
       {sdrplay_api_RspDuoMode_Dual_Tuner,   "Dual Tuner"},
       {sdrplay_api_RspDuoMode_Master,       "Master"},
       {sdrplay_api_RspDuoMode_Slave,        "Slave"}
   };
   std::unordered_map<sdrplay_api_TunerSelectT,std::string> tunerNames = {
       {sdrplay_api_Tuner_Neither, "Neither"},
       {sdrplay_api_Tuner_A,       "A"},
       {sdrplay_api_Tuner_B,       "B"},
       {sdrplay_api_Tuner_Both,    "Both"}
   };

   std::vector<SoapySDR::Kwargs> results;
   std::string labelHint;
   if (args.count("label") != 0) labelHint = args.at("label");

   sdrplay_api_RspDuoModeT rspDuoModeHint = sdrplay_api_RspDuoMode_Unknown;
   if (args.count("rspduo_mode") != 0)
   {
      try
      {
         rspDuoModeHint = (sdrplay_api_RspDuoModeT) stoi(args.at("rspduo_mode"));
      }
      catch (std::invalid_argument&)
      {
         bool found = false;
         for (auto rspDuoModeName : rspDuoModeNames)
         {
            if (strcasecmp(args.at("rspduo_mode").c_str(),
                           rspDuoModeName.second.c_str()) == 0)
            {
               found = true;
               rspDuoModeHint = rspDuoModeName.first;
               break;
            }
         }
         if (!found)
         {
            throw;
         }
      }
   }

   sdrplay_api_TunerSelectT tunerHint = sdrplay_api_Tuner_Neither;
   if (args.count("tuner") != 0)
   {
      try
      {
         tunerHint = (sdrplay_api_TunerSelectT) stoi(args.at("tuner"));
      }
      catch (std::invalid_argument&)
      {
         bool found = false;
         for (auto tunerName : tunerNames)
         {
            if (strcasecmp(args.at("tuner").c_str(),
                           tunerName.second.c_str()) == 0)
            {
               found = true;
               tunerHint = tunerName.first;
               break;
            }
         }
         if (!found)
         {
            throw;
         }
      }
   }

   unsigned int nDevs = 0;
   char lblstr[128];

   if (isAtExitRegistered == false)
   {
       atexit(close_sdrplay_api);
       isAtExitRegistered = true;
   }

   if (isSdrplayApiOpen == false)
   {
      sdrplay_api_ErrT err;
      if ((err = sdrplay_api_Open()) != sdrplay_api_Success)
      {
          return results;
      }
      isSdrplayApiOpen = true;
   }

   if (deviceSelected)
   {
      sdrplay_api_ReleaseDevice(deviceSelected);
      deviceSelected = nullptr;
   }

   std::string baseLabel = "SDRplay Dev";

   // list devices by API
   sdrplay_api_LockDeviceApi();
   sdrplay_api_GetDevices(&rspDevs[0], &nDevs, SDRPLAY_MAX_DEVICES);

   size_t posidx = labelHint.find(baseLabel);

   int labelDevIdx = -1;
   if (posidx != std::string::npos)
      labelDevIdx = labelHint.at(posidx + baseLabel.length()) - 0x30;

   int devIdx = 0;
   for (unsigned int i = 0; i < nDevs; ++i)
   {
      switch (rspDevs[i].hwVer)
      {
      case SDRPLAY_RSP1_ID:
      case SDRPLAY_RSP1A_ID:
      case SDRPLAY_RSP2_ID:
      case SDRPLAY_RSPdx_ID:
         if (labelDevIdx < 0 || devIdx == labelDevIdx)
         {
            SoapySDR::Kwargs dev;
            dev["driver"] = "sdrplay";
            sprintf_s(lblstr, 128, "%s%d %s %.*s",
                      baseLabel.c_str(), devIdx,
                      deviceNames[rspDevs[i].hwVer].c_str(),
                      SDRPLAY_MAX_SER_NO_LEN, rspDevs[i].SerNo);
            dev["label"] = lblstr;
            results.push_back(dev);
         }
         ++devIdx;
         break;
      case SDRPLAY_RSPduo_ID:
         for (sdrplay_api_RspDuoModeT rspDuoMode : {
                  sdrplay_api_RspDuoMode_Single_Tuner,
                  sdrplay_api_RspDuoMode_Dual_Tuner,
                  sdrplay_api_RspDuoMode_Master,
                  sdrplay_api_RspDuoMode_Slave})
         {
            if (rspDevs[i].rspDuoMode & rspDuoMode)
            {
               if (rspDuoMode == sdrplay_api_RspDuoMode_Dual_Tuner)
               {
                  if (rspDevs[i].tuner == sdrplay_api_Tuner_Both)
                  {
                     if ((labelDevIdx < 0 || devIdx == labelDevIdx) &&
                         (rspDuoModeHint == sdrplay_api_RspDuoMode_Unknown ||
                          rspDuoMode == rspDuoModeHint) &&
                         (tunerHint == sdrplay_api_Tuner_Neither ||
                          sdrplay_api_Tuner_Both == tunerHint))
                     {
                        SoapySDR::Kwargs dev;
                        dev["driver"] = "sdrplay";
                        sprintf_s(lblstr, 128, "%s%d %s %.*s - %s",
                                  baseLabel.c_str(), devIdx,
                                  deviceNames[rspDevs[i].hwVer].c_str(),
                                  SDRPLAY_MAX_SER_NO_LEN, rspDevs[i].SerNo,
                                  rspDuoModeNames[rspDuoMode].c_str());
                        dev["label"] = lblstr;
                        dev["rspduo_mode"] = std::to_string(rspDuoMode);
                        dev["tuner"] = std::to_string(rspDevs[i].tuner);
                        dev["rspduo_sample_freq"] = std::to_string(SoapySDRPlay::defaultRspDuoSampleFreq);
                        results.push_back(dev);
                     }
                     ++devIdx;
                  }
               } else {
                  for (sdrplay_api_TunerSelectT tuner : {
                           sdrplay_api_Tuner_A,
                           sdrplay_api_Tuner_B})
                  {
                     if (rspDevs[i].tuner & tuner)
                     {
                        if ((labelDevIdx < 0 || devIdx == labelDevIdx) &&
                            (rspDuoModeHint == sdrplay_api_RspDuoMode_Unknown ||
                             rspDuoMode == rspDuoModeHint) &&
                            (tunerHint == sdrplay_api_Tuner_Neither ||
                             tuner == tunerHint))
                        {
                           SoapySDR::Kwargs dev;
                           dev["driver"] = "sdrplay";
                           sprintf_s(lblstr, 128, "%s%d %s %.*s - Tuner %s %s",
                                     baseLabel.c_str(), devIdx,
                                     deviceNames[rspDevs[i].hwVer].c_str(),
                                     SDRPLAY_MAX_SER_NO_LEN, rspDevs[i].SerNo,
                                     tunerNames[tuner].c_str(),
                                     rspDuoModeNames[rspDuoMode].c_str());
                           dev["label"] = lblstr;
                           dev["rspduo_mode"] = std::to_string(rspDuoMode);
                           dev["tuner"] = std::to_string(tuner);
                           dev["rspduo_sample_freq"] = std::to_string(
                                  rspDuoMode == sdrplay_api_RspDuoMode_Slave ?
                                  rspDevs[i].rspDuoSampleFreq : SoapySDRPlay::defaultRspDuoSampleFreq);
                           results.push_back(dev);
                        }
                        ++devIdx;
                     }
                  }
               }
            }
         }
         break;
      }
   }

   // unlock sdrplay API and close it in case some other driver needs it
   sdrplay_api_UnlockDeviceApi();
   if (isSdrplayApiOpen == true)
   {
      sdrplay_api_Close();
      isSdrplayApiOpen = false;
   }
   return results;
}

static SoapySDR::Device *makeSDRPlay(const SoapySDR::Kwargs &args)
{
    return new SoapySDRPlay(args);
}

static SoapySDR::Registry registerSDRPlay("sdrPlay", &findSDRPlay, &makeSDRPlay, SOAPY_SDR_ABI_VERSION);
