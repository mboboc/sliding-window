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

/* converts payload to pkt */
void back_to_pkt(msg *t, pkt_t *pkt) {
    *pkt = *((pkt_t *)t->payload);
}

/* finds index of akn msg in msg_buffer */
int find_index(msg *msg_buffer, int size, int index) {
    int i;
    pkt_t pkt;

    for (i = 0; i < size; i++) {
        back_to_pkt(&(msg_buffer[i]), &pkt);
        if (pkt.seq_number == index) {
            return 1;
        }
    }
    return -1;
}

/* returns size of file */
long int findSize(const char *file_name) {
    struct stat st;

    if (stat(file_name, &st) == 0)
        return (st.st_size);
    else
        return -1;
}

/* returns numbers of pkts needed to send a file */
int return_nb_of_pkt(int file_size) {
    if (file_size % (MSGSIZE - 2 * sizeof(int)) != 0) {
        return file_size / (MSGSIZE - 2 * sizeof(int)) + 1;
    } else {
        return file_size / (MSGSIZE - 2 * sizeof(int));
    }
}

/* debugging */
void print_msg_buffer(msg *msg_buffer, int size) {
    int i;
    pkt_t pkt;
    for (i = 0; i < size; i++) {
        back_to_pkt(&(msg_buffer[i]), &pkt);
        printf("%d ", pkt.seq_number);
    }
    printf("\n");
}

int main(int argc, char** argv) {
    msg     r, t;
    pkt_t   pkt;
    int     openFlags = O_RDWR;
    int     filePerms = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH;
    int     bdp, wdw_size, data_size, n_of_pkt;
    int     data_capacity = MSGSIZE - 2 * sizeof(int);
    int     count_send, i, fid, flag, lost = -1;
    long    file_size;

    init(HOST, PORT);
    bdp = atoi(argv[2]) * atoi(argv[3]) * 1000;
    wdw_size = bdp / MSGSIZE;

    msg     msg_buffer[wdw_size];

    /* create pkt with name & size and send*/
    file_size = findSize(argv[1]);
    n_of_pkt = return_nb_of_pkt(file_size);
    quick_pack(&t, 0, 0, argv[1], n_of_pkt);
    while (lost == -1) {
        send_message(&t);
        lost = recv_message_timeout(&r, atoi(argv[3]) * 2);
    }

    /* open file to read */
    fid = open(argv[1], openFlags, filePerms);
    if (fid < 0) {
        printf("[SEND]: file not open.\n");
        return -1;
    }

    /* send window */
    char buffer[data_capacity + 1];
    count_send = 2;
    flag = -1;
    while (count_send <= wdw_size) {
        data_size = read(fid, &buffer, data_capacity);
        printf("[SEND]: Current sequence = %d with size %d.\n", count_send, data_size);
        if (data_size <= 0) {
            flag = i;
            break;
        }
        quick_pack(&t, count_send, 0, buffer, data_size);
        send_message(&t);
        msg_buffer[i] = t;
        memset(buffer, 0, data_capacity);
        i++;
        count_send++;
    }
    count_send--;

    /* wait for akn and continue to send */
    int     k;
    pkt_t   pkt_aux;
    int     seq;
    int     index;
    if (flag < 0) {
        i = 0;
        while ((data_size = read(fid, buffer, data_capacity)) > 0) {
            lost = recv_message_timeout(&r, atol(argv[3]) * 2);
            if (lost == -1) {
                printf("[SEND] Resending...");
                for (k = 0; k < wdw_size; k++) {
                    send_message(&(msg_buffer[k]));
                }
            } else {
                if (data_size < 0) {
                    break;
                }
                back_to_pkt(&r, &pkt_aux);
                seq = pkt_aux.seq_number;
                index = find_index(msg_buffer, wdw_size, seq);
                quick_pack(&(msg_buffer[index]), count_send, 0, buffer, data_size);
                quick_pack(&t, count_send, 0, buffer, data_size); //dimesniunea e in octeti
                send_message(&t);
                printf("[SEND]: Current sequence = %d with size %d.\n", count_send, data_size);
                memset(buffer, 0, data_capacity);
                count_send++;
            }
        }
        flag = wdw_size;
    }

    /* receive last akn */
    i = 0;
    pkt_t aux;
    while (i < flag) {
        lost = recv_message_timeout(&r, 100 * atol(argv[3]));
        if (lost == -1) {
            printf("[SEND] Resending...");
            for (k = 0; k < flag; k++) {
                back_to_pkt(&(msg_buffer[k]), &pkt);
                if (pkt.seq_number != -100) {
                    send_message(&(msg_buffer[k]));
                }
            }
        } else {
            back_to_pkt(&r, &pkt);
            for (k = 0; k < flag; k++) {
                back_to_pkt(&(msg_buffer[k]), &aux);
                if (aux.seq_number == pkt.seq_number) {
                    quick_pack(&(msg_buffer[k]), -100, 0, pkt.data, msg_buffer[k].len);
                }
            }
            i++;
        }
    }

    return 0;
}
