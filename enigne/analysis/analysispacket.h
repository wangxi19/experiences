#ifndef ANALYSISPACKET_H
#define ANALYSISPACKET_H

#include <stdint.h>
#include <PacketDecode.h>
#include <pcap.h>
#include <rollBuffer.h>
#include <tuple>
#include <pthread.h>

#define PACKETBUFFERLEN 65535
class AnalysisPkt
{
public:
    explicit AnalysisPkt();
    ~AnalysisPkt();
    bool init(int iPktRolbuferLen = 10000);
    static bool pfnProcess(const uint8_t* packet, const pcap_pkthdr& packetHeader, const PacketAttribute& PA, void* context);
    bool Process(const uint8_t* packet, const pcap_pkthdr& packetHeader, const PacketAttribute& PA);

    static void* pfnAnalysis(void* context);

    //not thread safe
    bool Analysis(const uint8_t* packet, const pcap_pkthdr& packetHeader, const PacketAttribute& PA);
private:
    CRollBuffer<std::tuple<uint8_t[PACKETBUFFERLEN], pcap_pkthdr, PacketAttribute>> mPacketRollBuffer;
    pthread_t mPthreadt{0};
};

#endif // ANALYSISPACKET_H
