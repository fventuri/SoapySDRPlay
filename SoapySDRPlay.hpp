/*
 * TODO license, copyright, etc
 * 
 */

#pragma once

#include <SoapySDR/Logger.h>
#include <SoapySDR/Device.hpp>

#if __APPLE__
    #include <mir_sdr.h>
#else
    #include <mirsdrapi-rsp.h>
#endif

class SoapySDRPlay : public SoapySDR::Device
{
public:
    SoapySDRPlay(const SoapySDR::Kwargs &args);

    ~SoapySDRPlay(void);

    /*******************************************************************
     * Identification API
     ******************************************************************/

    std::string getDriverKey(void) const;

    std::string getHardwareKey(void) const;

    SoapySDR::Kwargs getHardwareInfo(void) const;

    /*******************************************************************
     * Channels API
     ******************************************************************/

    size_t getNumChannels(const int) const;

    /*******************************************************************
     * Stream API
     ******************************************************************/

    SoapySDR::Stream *setupStream(
        const int direction,
        const std::string &format,
        const std::vector<size_t> &channels = std::vector<size_t>(),
        const SoapySDR::Kwargs &args = SoapySDR::Kwargs());

    void closeStream(SoapySDR::Stream *stream);

    size_t getStreamMTU(SoapySDR::Stream *stream) const;

    int activateStream(
        SoapySDR::Stream *stream,
        const int flags = 0,
        const long long timeNs = 0,
        const size_t numElems = 0);

    int deactivateStream(
        SoapySDR::Stream *stream,
        const int flags = 0,
        const long long timeNs = 0);

    int readStream(
        SoapySDR::Stream *stream,
        void * const *buffs,
        const size_t numElems,
        int &flags,
        long long &timeNs,
        const long timeoutUs = 100000);

    /*******************************************************************
     * Antenna API
     ******************************************************************/

    std::vector<std::string> listAntennas(const int direction, const size_t channel) const;

//    void setAntenna(const int direction, const size_t channel, const std::string &name);

    std::string getAntenna(const int direction, const size_t channel) const;

    /*******************************************************************
     * Frontend corrections API
     ******************************************************************/

    bool hasDCOffsetMode(const int direction, const size_t channel) const;

    void setDCOffsetMode(const int direction, const size_t channel, const bool automatic);

    bool getDCOffsetMode(const int direction, const size_t channel) const;

    bool hasDCOffset(const int direction, const size_t channel) const;

    void setDCOffset(const int direction, const size_t channel, const std::complex<double> &offset);

//    std::complex<double> getDCOffset(const int direction, const size_t channel) const;

    /*******************************************************************
     * Gain API
     ******************************************************************/

//    std::vector<std::string> listGains(const int direction, const size_t channel) const;
//
//    void setGainMode(const int direction, const size_t channel, const bool automatic);
//
//    bool getGainMode(const int direction, const size_t channel) const;
//
//    void setGain(const int direction, const size_t channel, const double value);
//
//    void setGain(const int direction, const size_t channel, const std::string &name, const double value);
//
//    double getGain(const int direction, const size_t channel, const std::string &name) const;
//
//    SoapySDR::Range getGainRange(const int direction, const size_t channel, const std::string &name) const;

    /*******************************************************************
     * Frequency API
     ******************************************************************/

    void setFrequency(const int direction, const size_t channel, const std::string &name, const double frequency, const SoapySDR::Kwargs &args = SoapySDR::Kwargs());

    double getFrequency(const int direction, const size_t channel, const std::string &name) const;

    std::vector<std::string> listFrequencies(const int direction, const size_t channel) const;

    SoapySDR::RangeList getFrequencyRange(const int direction, const size_t channel, const std::string &name) const;

    /*******************************************************************
     * Sample Rate API
     ******************************************************************/

    void setSampleRate(const int direction, const size_t channel, const double rate);

    double getSampleRate(const int direction, const size_t channel) const;

    std::vector<double> listSampleRates(const int direction, const size_t channel) const;

    void setBandwidth(const int direction, const size_t channel, const double bw);

    double getBandwidth(const int direction, const size_t channel) const;

    std::vector<double> listBandwidths(const int direction, const size_t channel) const;

private:

    static mir_sdr_Bw_MHzT mirGetBwMhzEnum(double bw);

    //device handle

    //stream
//    short *xi;
//    short *xq;
    std::vector<short> xi;
    std::vector<short> xi_buffer;
    std::vector<short> xq;
    std::vector<short> xq_buffer;
    unsigned int fs;
    int syncUpdate;

    //cached settings
    float ver;
    bool dcOffsetMode;
    int sps;
    int grc, rfc, fsc;
    int grChangedAfter;
    int newGr;
    int oldGr;
    double centerFreq;
    double rate;
    double bw;

    bool rxFloat;
};
