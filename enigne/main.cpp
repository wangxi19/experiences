#include "logtool/logtool.h"
#include <stdio.h>
#include <analysis/analysispacket.h>
#include <capture/pcapcapture.h>

bool ___ (const uint8_t* packet, const pcap_pkthdr& packetHeader, void* context) {
    return true;
}

int main(int argc, char* argv[]) {
    LogTool::SetLogFd(fileno(stdout));
    PcapCapture capture;
    AnalysisPkt Anal;
    Anal.init();
    capture.init(___, 0, AnalysisPkt::pfnProcess, &Anal, argv[1]);
    capture.start();
}
