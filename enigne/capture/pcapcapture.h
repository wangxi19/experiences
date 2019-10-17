#ifndef PCAPCAPTURE_H
#define PCAPCAPTURE_H
#include <pcap.h>
#include <stdint.h>
#include <PacketDecode.h>
#include <stdint.h>

typedef bool (*PfnCommitPktToStore) (const uint8_t* packet, const pcap_pkthdr& packetHeader, void* context);
typedef bool (*PfnCommitPktToAnal) (const uint8_t* packet, const pcap_pkthdr& packetHeader, const PacketAttribute& PA, void* context);

class PcapCapture
{
public:
    PcapCapture();
    ~PcapCapture();

    bool init(PfnCommitPktToStore pfnCmtPktToStor,
              void* pfnCmtPktToStorCtxt,
              PfnCommitPktToAnal pfnCmtPktToAnal,
              void* mPfnCmtPktToAnalCtxt,
              const char* deviceName = "any",
              int iSnaplen = 1600,
              int iPromiscuous = 1,
              int iToMs = 1000);

    void start();

    void setPfnCmtPktToStor(PfnCommitPktToStore pfnCmtPktToStor, void *ctxt);

    void setPfnCmtPktToAnal(PfnCommitPktToAnal pfnCmtPktToAnal, void* ctxt);

    static void pfnPcapHandler(u_char* user, const struct pcap_pkthdr* h, const u_char* bytes);

public:
    PacketAttribute PA;
    char mDevice[1024]{0};
    char mErrorBuffer[PCAP_ERRBUF_SIZE]{0};
    int mSnaplen{1600};
    int mPromiscuous{1};
    int mToMs{1000};

    pcap_t* mPcapHandle{nullptr};

    PfnCommitPktToStore mPfnCmtPktToStor{nullptr};
    void* mPfnCmtPktToStorContext{nullptr};
    PfnCommitPktToAnal mPfnCmtPktToAnal{nullptr};
    void* mPfnCmtPktToAnalContext{nullptr};
};

#endif // PCAPCAPTURE_H
