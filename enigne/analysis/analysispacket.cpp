#include "analysispacket.h"
#include "../logtool/logtool.h"

#include <iostream>
#include <netinet/tcp.h>
#include <netinet/ip.h>

//extern void LOGERROR(const char* format, ...);
//extern void LOGWARN(const char* format, ...);

AnalysisPkt::AnalysisPkt()
{

}

AnalysisPkt::~AnalysisPkt()
{

}

bool AnalysisPkt::init(int iPktRolbuferLen)
{
    mPacketRollBuffer.Init(iPktRolbuferLen);

    size_t stackSize{2*1024*1024};
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setstacksize(&attr, stackSize);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

    auto ret = pthread_create(&mPthreadt, &attr, pfnAnalysis, this);
    if (ret != 0) {
        LOGERROR("Fail to create pthread for pfnAnalysis\n");
        return false;
    }

    pthread_setname_np(mPthreadt, "pfnAnalysis");

    return true;
}

bool AnalysisPkt::pfnProcess(const uint8_t *packet, const pcap_pkthdr &packetHeader, const PacketAttribute &PA, void *context)
{
    return ((AnalysisPkt*)context)->Process(packet, packetHeader, PA);
}

bool AnalysisPkt::Process(const uint8_t *packet, const pcap_pkthdr &packetHeader, const PacketAttribute &PA)
{
    (void)(PA);

    auto tpPtr = mPacketRollBuffer.getWriteBucket(NO_WAIT);
    if (tpPtr == nullptr) {
        LOGWARN("Fail to getWriteBucket \n");
        return false;
    }

    if (packetHeader.caplen > PACKETBUFFERLEN) {
        LOGWARN("Packet's length is greater than %d\n", PACKETBUFFERLEN);
        return false;
    }

    auto bufferPtr = std::get<0>(*tpPtr);
    auto& headerRe = std::get<1>(*tpPtr);
    auto& PARe = std::get<2>(*tpPtr);

    PARe = PA;
    PARe.packet_data = ((uint8_t*)PA.packet_data - packet) + bufferPtr;

    memcpy(bufferPtr, packet, packetHeader.caplen);
    headerRe = packetHeader;

//    AnalysisPacket((unsigned char *)bufferPtr, headerRe.caplen, & PARe);
    if (!(PARe.g_flags & GLOBAL_FLAG_IP_EXIST)) {
        return true;
    }

    mPacketRollBuffer.Write();
    return true;
}

void *AnalysisPkt::pfnAnalysis(void *context)
{
    auto ptr = ((AnalysisPkt*)context);
    while (true) {
        auto tpPtr = ptr->mPacketRollBuffer.getReadBucket(WAIT_FOREVER);
        if (tpPtr == nullptr) {
            continue;
        }

        ptr->Analysis(std::get<0>(*tpPtr), std::get<1>(*tpPtr), std::get<2>(*tpPtr));
        ptr->mPacketRollBuffer.Read();
    }
}

bool AnalysisPkt::Analysis(const uint8_t* packet, const pcap_pkthdr& packetHeader, const PacketAttribute& PA)
{
    //test only
    if (!(PA.g_flags & GLOBAL_FLAG_TCP_EXIST)) {
        return true;
    }

    auto ptrtcphdr = (tcphdr*)(PA.Header[PA.InnerTCPIndex].header_offset + packet);

    auto ptriphdr = (ip*)(PA.Header[PA.InnerIPIndex].header_offset + packet);

    std::cout << inet_ntoa(ptriphdr->ip_src);
    std::cout.flush();
    std::cout << ":" << ntohs(ptrtcphdr->source) << " -> "
              << inet_ntoa(ptriphdr->ip_dst) << ":" << ntohs(ptrtcphdr->dest) << std::endl;

    return true;
}
