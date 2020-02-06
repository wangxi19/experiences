#include <iostream>
#include <pcap.h>
#include <pcap/bpf.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

using namespace std;

#if OK
typedef struct pcaprec_hdr_s {
        uint32_t ts_sec;         /* timestamp seconds */
        uint32_t ts_usec;        /* timestamp microseconds */
        uint32_t incl_len;       /* number of octets of packet saved in file */
        uint32_t orig_len;       /* actual length of packet */
} pcaprec_hdr_t;

int main()
{
    bpf_program _bpf;
    if (0 != pcap_compile_nopcap(10240, DLT_EN10MB, &_bpf, "(port 22)", 0, 0)) {
        cerr << "Fail to pcap_compile_nopcap " << endl;
        exit(1);
    }

    char* buf = (char*)malloc(1024*1024);
    int fd= open("/tmp/pkt.pcap", O_RDONLY);
    int fd2 = open("/tmp/pktf.pcap", O_CREAT|O_WRONLY|O_TRUNC);
    auto sz = read(fd, buf, 1024*1024);
    write(fd2,buf,24);
    char* ptr = buf;
    pcaprec_hdr_t* header;
    int offset = 0;
    int pktCount=0;
    if (sz > 0) {
        ptr += 24;
        offset += 24;
        while ((size_t)(sz - offset) > sizeof(pcaprec_hdr_t)) {
            header = (pcaprec_hdr_t*) ptr;

            uint8_t* _ptr = (uint8_t*)(ptr + sizeof(pcaprec_hdr_t));
            if (0 < bpf_filter(_bpf.bf_insns, _ptr, header->orig_len, header->orig_len)) {
                write(fd2, ptr, sizeof(pcaprec_hdr_t) + header->orig_len);
            }

            ptr += header->orig_len + sizeof(pcaprec_hdr_t);
            offset += header->orig_len + sizeof(pcaprec_hdr_t);
            pktCount++;
        }
    }
    close(fd);
    close(fd2);
    free(buf);
    cout << pktCount << endl;
    return 0;
}
#endif
typedef struct pcaprec_hdr_s {
        uint32_t ts_sec;         /* timestamp seconds */
        uint32_t ts_usec;        /* timestamp microseconds */
        uint32_t incl_len;       /* number of octets of packet saved in file */
        uint32_t orig_len;       /* actual length of packet */
} pcaprec_hdr_t;

extern "C" int _pcap_compile(int snaplen_arg, int linktype_arg, pcap_t *p, struct bpf_program *program,
         const char *buf, int optimize, bpf_u_int32 mask);

int main() {
    bpf_program _bpf;
    if (0 != _pcap_compile(10240, DLT_EN10MB, 0, &_bpf, "(port 22)", 0, 0)) {
        cerr << "Fail to pcap_compile_nopcap " << endl;
        exit(1);
    }

    char* buf = (char*)malloc(1024*1024);
    int fd= open("/tmp/pkt.pcap", O_RDONLY);
    int fd2 = open("/tmp/pktf.pcap", O_CREAT|O_WRONLY|O_TRUNC);
    auto sz = read(fd, buf, 1024*1024);
    write(fd2,buf,24);
    char* ptr = buf;
    pcaprec_hdr_t* header;
    int offset = 0;
    int pktCount=0;
    if (sz > 0) {
        ptr += 24;
        offset += 24;
        while ((size_t)(sz - offset) > sizeof(pcaprec_hdr_t)) {
            header = (pcaprec_hdr_t*) ptr;

            uint8_t* _ptr = (uint8_t*)(ptr + sizeof(pcaprec_hdr_t));
            if (0 < bpf_filter(_bpf.bf_insns, _ptr, header->orig_len, header->orig_len)) {
                write(fd2, ptr, sizeof(pcaprec_hdr_t) + header->orig_len);
            }

            ptr += header->orig_len + sizeof(pcaprec_hdr_t);
            offset += header->orig_len + sizeof(pcaprec_hdr_t);
            pktCount++;
        }
    }
    close(fd);
    close(fd2);
    free(buf);
    cout << pktCount << endl;
    return 0;
    return 0;
}
