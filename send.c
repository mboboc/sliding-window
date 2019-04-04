#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "link_emulator/lib.h"

#define HOST "127.0.0.1"
#define PORT 10000

/* data_size reprezinta cat am citit din fisier, cata informatie utila se afla in payload*/

typedef struct {
    int seq_number;
    int checksum;
    char data[MSGSIZE - 2 * sizeof(int)];
} pkt_t;

/* prepares pkt and packs pkt_t structure into payload of msg */
void quick_pack(msg *t, int seq_number, int checksum, char *buffer, int data_size) {
    pkt_t pkt;
    pkt.seq_number = seq_number;
    pkt.checksum = checksum;
    memcpy(pkt.data, buffer, data_size);
    memset(t, 0, sizeof(msg));
    memcpy(t->payload, &pkt, sizeof(pkt_t));
    t->len = data_size;
}

void pack_akn(msg *t, int seq_number) {
    pkt_t pkt;
    pkt.seq_number = seq_number;
    pkt.checksum = 0;
    memset(t, 0, sizeof(msg));
    memcpy(t->payload, &pkt, sizeof(pkt_t));
    t->len = -100; //AKN
}

void back_to_pkt(msg *t, pkt_t *pkt) {
    *pkt = *((pkt_t *)t->payload);
}

int main(int argc, char** argv) {
    msg     r, t;
    pkt_t   pkt_r;
    int     bdp;
    int     data_size, wdw_size;
    int     buffer_size = MSGSIZE - 2 * sizeof(int);
    int     count_send, i, fid, flag = -1;
    int     openFlags = O_CREAT | O_RDWR;
    int     filePerms = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH;

    init(HOST, PORT);

    /* bandwidth delay product */
    bdp = atol(argv[2]) * atol(argv[3]) * 1000;
    wdw_size = bdp / MSGSIZE;
    printf("[SEND]: Window size = %d.\n", wdw_size);
    //msg     t_buffer[wdw_size];
    /* create pkt with name and send*/
    quick_pack(&t, 0, 0, argv[1], strlen(argv[1])); // trimit numele cu terminatorul de sir
    printf("[SEND]:Sending file name.\n");
    send_message(&t);
    recv_message(&r);
    back_to_pkt(&r, &pkt_r);
    printf("[SEND]: Got akn for file name with seq = %d. \n", pkt_r.seq_number);

    fid = open(argv[1], openFlags, filePerms);
    if (fid < 0) {
        printf("[SEND]: Send error: file not open.\n");
    }

    printf("[SEND]: Starting to read from file.\n");
    char buffer[buffer_size + 1];
    count_send = 1;
    while (count_send <= wdw_size) {
        data_size = read(fid, &buffer, buffer_size);
        printf("[SEND]: Current sequence = %d with size %d.\n", count_send, data_size);
        // if there are less packets than wdw_size
        if (data_size <= 0) {
            printf("Am citit 0 bytes.\n");
            quick_pack(&t, -1, 0, buffer, data_size); //seq_number = -1 means end of transmission
            send_message(&t);
            flag = i;
            break;
        }
        quick_pack(&t, count_send, 0, buffer, data_size); //dimesniunea e in octeti
        send_message(&t);
        memset(buffer, 0, buffer_size);
        i++;
        count_send++;
    }
    if (flag < 0) {
        while ((data_size = read(fid, buffer, buffer_size)) > 0) {
            recv_message(&r);
            // if I finished
            if (data_size < 0) {
                quick_pack(&t, -1, 0, buffer, data_size); //seq_number = -1 means end of transmission
                send_message(&t);
                break;
            }
            quick_pack(&t, count_send, 0, buffer, data_size); //dimesniunea e in octeti
            send_message(&t);
            memset(buffer, 0, buffer_size);
            count_send++;
        }
    }
    i = 0;
    if (flag == -1)
        flag = wdw_size;
    while (i < flag) {
        recv_message(&r);
        back_to_pkt(&r, &pkt_r);
        printf("[SEND]: Am primit ACK pentru %d\n", pkt_r.seq_number);
        i++;
    }
    return 0;
}
