//
// SoapySDR driver for the LiteX M2SDR.
//
// Copyright (c) 2021-2024 Enjoy Digital.
// SPDX-License-Identifier: Apache-2.0
// http://www.apache.org/licenses/LICENSE-2.0
//

#include <mutex>
#include <cstring>
#include <cstdlib>
#include <stdexcept>
#include <iostream>
#include <memory>

#include "liblitepcie.h"

#include <SoapySDR/Device.hpp>
#include <SoapySDR/Logger.hpp>
#include <SoapySDR/Time.hpp>
#include <SoapySDR/Formats.hpp>

#define DLL_EXPORT __attribute__ ((visibility ("default")))
#define BYTES_PER_SAMPLE 2 // TODO validate this

enum class TargetDevice { CPU, GPU };

class DLL_EXPORT SoapyLiteXM2SDR : public SoapySDR::Device {
  public:
    SoapyLiteXM2SDR(const SoapySDR::Kwargs &args);
    ~SoapyLiteXM2SDR(void);

    // Identification API
    std::string getDriverKey(void) const { return "XTRX over LitePCIe"; }
    std::string getHardwareKey(void) const { return "Fairwaves XTRX"; }
    SoapySDR::Kwargs getHardwareInfo(void) const;


    // Channels API
    size_t getNumChannels(const int) const { return 2; }
    bool getFullDuplex(const int, const size_t) const { return true; }

    std::string getNativeStreamFormat(const int /*direction*/,
                                      const size_t /*channel*/,
                                      double &fullScale) const {
        fullScale = 4096;
        return SOAPY_SDR_CS16;
    }

    // Stream API
    SoapySDR::Stream *setupStream(const int direction,
                                  const std::string &format,
                                  const std::vector<size_t> &channels,
                                  const SoapySDR::Kwargs &args);
    void closeStream(SoapySDR::Stream *stream) override;
    int activateStream(SoapySDR::Stream *stream, const int flags,
                       const long long timeNs, const size_t numElems) override;
    int deactivateStream(SoapySDR::Stream *stream, const int flags,
                         const long long timeNs) override;
    size_t getStreamMTU(SoapySDR::Stream *stream) const override;
    size_t getNumDirectAccessBuffers(SoapySDR::Stream *stream) override;
    int getDirectAccessBufferAddrs(SoapySDR::Stream *stream,
                                   const size_t handle, void **buffs) override;
    int acquireReadBuffer(SoapySDR::Stream *stream, size_t &handl,
                          const void **buffs, int &flags, long long &timeNs,
                          const long timeoutUs) override;
    void releaseReadBuffer(SoapySDR::Stream *stream, size_t handle) override;
    int acquireWriteBuffer(SoapySDR::Stream *stream, size_t &handle,
                           void **buffs, const long timeoutUs) override;
    void releaseWriteBuffer(SoapySDR::Stream *stream, size_t handle,
                            const size_t numElems, int &flags,
                            const long long timeNs = 0) override;

    std::vector<std::string> getStreamFormats(const int direction, const size_t channel) const;


    // Antenna API
    std::vector<std::string> listAntennas(const int direction,
                                          const size_t channel) const override;
    void setAntenna(const int direction, const size_t channel,
                    const std::string &name) override;
    std::string getAntenna(const int direction,
                           const size_t channel) const override;

    std::map<int, std::map<size_t, std::string>> _cachedAntValues;

    // Frontend corrections API
    bool hasDCOffsetMode(const int direction,
                         const size_t channel) const override;
    void setDCOffsetMode(const int direction, const size_t channel,
                         const bool automatic) override;
    bool SetNCOFrequency(const int direction, const size_t channel,
                         uint8_t index, double frequency, double phaseOffset=-1.0);
    bool getDCOffsetMode(const int direction,
                         const size_t channel) const override;
    bool hasDCOffset(const int direction,
                     const size_t channel) const override;
    void setDCOffset(const int direction, const size_t channel,
                     const std::complex<double> &offset) override;
    std::complex<double> getDCOffset(const int direction,
                                     const size_t channel) const override;
    bool hasIQBalance(const int /*direction*/, const size_t /*channel*/) const {
        return true;
    }
    void setIQBalance(const int direction, const size_t channel,
                      const std::complex<double> &balance) override;
    std::complex<double> getIQBalance(const int direction,
                                      const size_t channel) const override;

    bool _rxDCOffsetMode;
    std::complex<double> _txDCOffset;
    std::map<int, std::map<size_t, std::complex<double>>> _cachedIqBalValues;

    // Gain API
    std::vector<std::string> listGains(const int direction,
                                       const size_t channel) const override;
    void setGain(int direction, size_t channel, const double value) override;
    void setGain(const int direction, const size_t channel,
                 const std::string &name, const double value) override;
    double getGain(const int direction, const size_t channel,
                   const std::string &name) const override;
    SoapySDR::Range getGainRange(const int direction, const size_t channel,
                                 const std::string &name) const override;

    std::map<int, std::map<size_t, std::map<std::string, double>>>
        _cachedGainValues;

    // Frequency API
    void
    setFrequency(int direction, size_t channel, double frequency,
                 const SoapySDR::Kwargs &args) override;
    void
    setFrequency(const int direction, const size_t channel, const std::string &,
                 const double frequency,
                 const SoapySDR::Kwargs &args = SoapySDR::Kwargs()) override;
    double getFrequency(const int direction, const size_t channel,
                        const std::string &name) const override;
    std::vector<std::string> listFrequencies(const int,
                                             const size_t) const override;
    SoapySDR::RangeList getFrequencyRange(const int, const size_t,
                                          const std::string &) const override;

    std::map<int, std::map<size_t, std::map<std::string, double>>>
        _cachedFreqValues;

    // Sample Rate API
    void setSampleRate(const int direction, const size_t,
                       const double rate) override;
    double getSampleRate(const int direction, const size_t) const override;
    SoapySDR::RangeList getSampleRateRange(const int direction,
                                        const size_t) const override;

    std::map<int, double> _cachedSampleRates;

    // BW filter API
    void setBandwidth(const int direction, const size_t channel,
                      const double bw) override;
    double getBandwidth(const int direction,
                        const size_t channel) const override;
    std::vector<double> listBandwidths(const int direction,
                                       const size_t channel) const override;

    std::map<int, std::map<size_t, double>> _cachedFilterBws;

    // Clocking API
    double getTSPRate(const int direction) const;
    void setMasterClockRate(const double rate) override;
    double getMasterClockRate(void) const override;
    void setReferenceClockRate(const double rate) override;
    double getReferenceClockRate(void) const override;
    SoapySDR::RangeList getReferenceClockRates(void) const override;
    std::vector<std::string> listClockSources(void) const override;
    void setClockSource(const std::string &source) override;
    std::string getClockSource(void) const override;

    // Sensor API
    std::vector<std::string> listSensors(void) const override;
    SoapySDR::ArgInfo getSensorInfo(const std::string &key) const override;
    std::string readSensor(const std::string &key) const override;

    int readStream(
        SoapySDR::Stream *stream,
        void * const *buffs,
        const size_t numElems,
        int &flags,
        long long &timeNs,
        const long timeoutUs = 100000 );


    int writeStream(
            SoapySDR::Stream *stream,
            const void * const *buffs,
            const size_t numElems,
            int &flags,
            const long long timeNs = 0,
            const long timeoutUs = 100000);

  private:
    SoapySDR::Stream *const TX_STREAM = (SoapySDR::Stream *)0x1;
    SoapySDR::Stream *const RX_STREAM = (SoapySDR::Stream *)0x2;

    void set_iq_correction(const int direction, const size_t channel,
            const double phase, const double gain);

    struct litepcie_ioctl_mmap_dma_info _dma_mmap_info;
    void *_dma_buf;

    struct Stream {
        Stream() : opened(false), remainderHandle(-1), remainderSamps(0),
                   remainderOffset(0), remainderBuff(nullptr) {}

        bool opened;
        void *buf;
        struct pollfd fds;
        int64_t hw_count, sw_count, user_count;

        int32_t remainderHandle;
        size_t remainderSamps;
        size_t remainderOffset;
        int8_t* remainderBuff;
        std::string format;
        std::vector<size_t> channels;
    };

    struct RXStream: Stream {
        uint32_t vga_gain;
        uint32_t lna_gain;
        uint8_t amp_gain;
        double samplerate;
        uint32_t bandwidth;
        uint64_t frequency;

        bool overflow;
    };

    struct TXStream: Stream {
        uint32_t vga_gain;
        uint8_t amp_gain;
        double samplerate;
        uint32_t bandwidth;
        uint64_t frequency;
        bool bias;

        bool underflow;

        bool burst_end;
        int32_t burst_samps;
    } ;

    RXStream _rx_stream;
    TXStream _tx_stream;

    const char *dir2Str(const int direction) const {
        return (direction == SOAPY_SDR_RX) ? "RX" : "TX";
    }

    int _fd;
    double _masterClockRate;
    double _refClockRate;

    // calibration data
    std::vector<std::map<std::string, std::string>> _calData;

    // register protection
    std::mutex _mutex;
};