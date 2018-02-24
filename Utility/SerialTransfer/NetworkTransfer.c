#define _XOPEN_SOURCE	700
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <netdb.h>

#define MIN(x, y)	(((x) < (y)) ? (x) : (y))

#define SERIAL_FIFOMAXSIZE		16

int main(int argc, char *argv[])
{
	char filename[256];
	char buf[SERIAL_FIFOMAXSIZE];
	struct addrinfo ai, *ai_ret;
	int sock, rc_gai;
	size_t tmp, len, sent;
	unsigned char ack;
	FILE* fp;

	if (argc < 2) {
		fprintf(stderr, "Input File Name: ");
		gets(filename);
	} else {
		strcpy(filename, argv[1]);
	}

	if ((fp = fopen(filename, "rb")) == NULL) {
		fprintf(stderr, "%s File Open Error \n", filename);
		return 0;
	}

	fseek(fp, 0, SEEK_END);
	len = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	printf("File Name %s, Data Length %lu Byte\n",
			filename, len);

	memset(&ai, 0, sizeof(ai));
	ai.ai_family = AF_INET;
	ai.ai_socktype = SOCK_STREAM;
	ai.ai_flags = AI_ADDRCONFIG;

	if ((rc_gai = getaddrinfo("127.0.0.1", "4444", &ai, &ai_ret)) != 0) {
		fprintf(stderr, "Fail: getaddrinfo(): %s\n", gai_strerror(rc_gai));
		exit(EXIT_FAILURE);
	}

	if ((sock = socket(ai_ret->ai_family, ai_ret->ai_socktype,
					ai_ret->ai_protocol)) == -1) {
		fprintf(stderr, "Fail: socket()\n");
		exit(EXIT_FAILURE);
	}

	if (connect(sock, ai_ret->ai_addr, ai_ret->ai_addrlen) == -1) {
		fprintf(stderr, "Fail: connect()\n");
		exit(EXIT_FAILURE);
	} else {
		printf("Socket Connect Succes, (IP/Port) (127.0.0.1/4444)\n");
	}

	if (send(sock, &len, 4, 0) != 4) {
		fprintf(stderr, "Data Length Send Fail, [%lu] Byte\n", len);
		return 0;
	} else {
		printf("Data Length Send Success, [%lu] Byte\n", len);
	}

	if (recv(sock, &ack, 1, 0) != 1) {
		fprintf(stderr, "Ack Receive Error\n");
		return 0;
	}

	printf("Now Data Transfer...");
	sent = 0;
	while (sent < len) {
		tmp = MIN(len - sent, SERIAL_FIFOMAXSIZE);
		sent += tmp;

		if (fread(buf, 1, tmp, fp) != tmp) {
			fprintf(stderr, "File Read Error\n");
			return 0;
		}

		if (send(sock, buf, tmp, 0) != tmp) {
			fprintf(stderr, "Socket Send Error\n");
			return 0;
		}

		if (recv(sock, &ack, 1, 0) != 1) {
			fprintf(stderr, "Ack Receive Error\n");
			return 0;
		}

		printf("#");
	}

	fclose(fp);
	close(sock);

	printf("\nSend Complete. [%lu] Byte \n", sent);
	printf("Press Enter Key To Exit\n");
	getchar();

	return 0;
}
