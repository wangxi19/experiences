
#include "pcapcapture.h"
#include "../logtool/logtool.h"
#include <string.h>

//extern void LOGERROR(const char* format, ...);
//extern void LOGWARN(const char* format, ...);

PcapCapture::PcapCapture()
{

}

PcapCapture::~PcapCapture()
{
    if (nullptr != mPcapHandle) {
        pcap_close(mPcapHandle);
        mPcapHandle = nullptr;
    }
}

bool PcapCapture::init(PfnCommitPktToStore pfnCmtPktToStor, void *pfnCmtPktToStorCtxt, PfnCommitPktToAnal pfnCmtPktToAnal, void* mPfnCmtPktToAnalCtxt, const char *deviceName, int iSnaplen, int iPromiscuous, int iToMs)
{
    if (nullptr != mPcapHandle) {
        pcap_close(mPcapHandle);
        mPcapHandle = nullptr;
    }

    if (pfnCmtPktToStor == nullptr || pfnCmtPktToAnal == nullptr) {
        LOGERROR("pfnCmtPktToStor and pfnCmtPktToAnal both don't be nullptr\n");
        return false;
    }

    setPfnCmtPktToStor(pfnCmtPktToStor, pfnCmtPktToStorCtxt);
    setPfnCmtPktToAnal(pfnCmtPktToAnal, mPfnCmtPktToAnalCtxt);
    mSnaplen = iSnaplen;
    mPromiscuous = iPromiscuous;
    mToMs = iToMs;

    if (deviceName == nullptr) {
        LOGERROR("Invalid device name\n");
        return false;
    }

    strncpy(mDevice, deviceName, sizeof(mDevice) - 1);

    mPcapHandle = pcap_create(mDevice, mErrorBuffer);
    if (nullptr == mPcapHandle) {
        LOGERROR("Fail to execute pcap_create; device %s; error: %s\n", mDevice, mErrorBuffer);
        return false;
    }

    if (PCAP_ERROR_ACTIVATED == pcap_set_snaplen(mPcapHandle, mSnaplen)) {
        LOGERROR("Fail to set snaplen %d, PCAP_ERROR_ACTIVATED\n", mSnaplen);
        return false;
    }

    if (PCAP_ERROR_ACTIVATED == pcap_set_snaplen(mPcapHandle, mSnaplen)) {
        LOGERROR("Fail to set snaplen %d, PCAP_ERROR_ACTIVATED\n", mSnaplen);
        return false;
    }

    if (PCAP_ERROR_ACTIVATED == pcap_set_promisc(mPcapHandle, mPromiscuous)) {
        LOGERROR("Fail to set promiscuous %d, PCAP_ERROR_ACTIVATED\n", mPromiscuous);
        return false;
    }

    if (PCAP_ERROR_ACTIVATED == pcap_set_timeout(mPcapHandle, mToMs)) {
        LOGERROR("Fail to set timeout %d ms, PCAP_ERROR_ACTIVATED\n", mToMs);
        return false;
    }

    switch (pcap_activate(mPcapHandle)) {
    case PCAP_WARNING_PROMISC_NOTSUP:
        LOGWARN("device %s does not support promiscuous model\n", mDevice);
        break;
//    case PCAP_WARNING_TSTAMP_TYPE_NOTSUP:
//        LOGWARN("device %s does not support the requested time stamp type\n", mDevice);
//        break;
    case PCAP_WARNING:
        LOGWARN("device %s get a warning when activated; info: %s\n", mDevice, pcap_geterr(mPcapHandle));
        break;
    case PCAP_ERROR_ACTIVATED:
        LOGERROR("device %s has been activated\n", mDevice);
        return false;
        break;

    case PCAP_ERROR_NO_SUCH_DEVICE:
        LOGERROR("device %s does not exist\n", mDevice);
        return false;
        break;

    case PCAP_ERROR_PERM_DENIED:
        LOGERROR("does not suitable permission to open the capture source; device %s\n", mDevice);
        return false;
        break;

    case PCAP_ERROR_RFMON_NOTSUP:
        LOGERROR("moniter mode is not supported on device %s\n", mDevice);
        return false;
        break;

    case PCAP_ERROR_IFACE_NOT_UP:
        LOGERROR("device %s is not up\n", mDevice);
        return false;
        break;

    case PCAP_ERROR:
        LOGERROR("Unknown error. info: %s\n", pcap_geterr(mPcapHandle));
        return false;
        break;

    case 0:

        break;
    default:
        LOGERROR("UNknown error when pcap_activate; device %s\n", mDevice);
        return false;
        break;
    }

    return true;
}

void PcapCapture::start()
{
    if (mPcapHandle == nullptr) {
        return;
    }

    pcap_pkthdr pcapPktHdr;
    memset(&pcapPktHdr, 0, sizeof(pcapPktHdr));

    auto ret = pcap_loop(mPcapHandle, -1, pfnPcapHandler, (u_char*)this);
    if (ret == -1) {
        LOGERROR("some error occured, info: %s\n", pcap_geterr(mPcapHandle));
        return;
    } else if (ret == -2) {
        LOGWARN("pcap_breakloop has been called before any packets was processed\n");
        return;
    }

    return;
}

void PcapCapture::setPfnCmtPktToStor(PfnCommitPktToStore pfnCmtPktToStor, void* ctxt)
{
    mPfnCmtPktToStor = pfnCmtPktToStor;
    mPfnCmtPktToStorContext = ctxt;
}

void PcapCapture::setPfnCmtPktToAnal(PfnCommitPktToAnal pfnCmtPktToAnal, void *ctxt)
{
    mPfnCmtPktToAnal = pfnCmtPktToAnal;
    mPfnCmtPktToAnalContext = ctxt;
}

void PcapCapture::pfnPcapHandler(u_char *user, const pcap_pkthdr *h, const u_char *bytes)
{
    if (bytes == nullptr || h == nullptr) {
        return;
    }

    auto thisPtr = (PcapCapture*) user;
    memset(&thisPtr->PA, 0, sizeof(thisPtr->PA));
    AnalysisPacket((unsigned char *)bytes, h->caplen, & thisPtr->PA);
    if (!(thisPtr->PA.g_flags & GLOBAL_FLAG_IP_EXIST)) {
        return;
    }

    thisPtr->mPfnCmtPktToStor(bytes, *h, thisPtr->mPfnCmtPktToStorContext);
    thisPtr->mPfnCmtPktToAnal(bytes, *h, thisPtr->PA, thisPtr->mPfnCmtPktToAnalContext);
}
