#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include "link_emulator/lib.h"

#define HOST "127.0.0.1"
#define PORT 10001

typedef struct {
    int seq_number;
    int checksum;
    char data[MSGSIZE - 2 * sizeof(int)];
} pkt_t;

void back_to_pkt(msg *t, pkt_t *pkt) {
    *pkt = *((pkt_t *)t->payload);
}

void pack_akn(msg *t, pkt_t *pkt, int seq_number) {
    memset(pkt, 0, sizeof(pkt_t));
    pkt->seq_number = seq_number;
    pkt->checksum = 0;
    memset(t, 0, sizeof(msg));
    memcpy(t->payload, pkt, sizeof(pkt_t));
    t->len = -100;
}

int main(int argc, char** argv) {
    msg r, t;
    pkt_t pkt_file_name, pkt, pkt_r;
    int fid;
    init(HOST, PORT);
    char ofile_name_aux[] = "recv_";
    char ofile_name[10];
    int n_of_pkt;

    memset(&r, 0, sizeof(msg));
    recv_message(&r);
    n_of_pkt = r.len;
    back_to_pkt(&r, &pkt_file_name);
    pack_akn(&t, &pkt, pkt_file_name.seq_number);
    send_message(&t);
    strcat(ofile_name, ofile_name_aux);
    strcat(ofile_name, pkt_file_name.data);
    
    fid = open(ofile_name, O_WRONLY | O_CREAT, S_IRUSR, S_IWUSR, S_IXUSR);
    if (fid < 0) {
        printf("[RECV]: Error: file not open.\n");
        return -1;
    }

    while (n_of_pkt--) {
        memset(&r, 0, sizeof(msg));
        recv_message(&r);
        back_to_pkt(&r, &pkt_r);
        printf("[RECV]: Recieved:%d.\n", pkt_r.seq_number);
        write(fid, pkt_r.data, r.len);
        pack_akn(&t, &pkt, pkt_r.seq_number);
        send_message(&t);
    }
    printf("[RECV]: Transmission complete.\n");

    return 0;
}