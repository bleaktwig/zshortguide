// Handshake server
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <zmq.h>

int main(void) {
    void *ctx = zmq_ctx_new();
    void *s_rep = zmq_socket(ctx, ZMQ_REP);
    if (zmq_bind(s_rep, "tcp://*:5555") == -1) {
        fprintf(stderr, "Error binding socket. errno: %s\n", strerror(errno));
        exit(1);
    }

    printf("Server initiated. Waiting for messages...\n");
    while(1) {
        int exit_signal = 0;

        zmq_msg_t req;
        if (zmq_msg_init(&req) == -1) {
            fprintf(stderr, "Error initiating request message. errno: %s\n", strerror(errno));
            exit(1);
        }

        // Block until a handshake is received.
        if (zmq_msg_recv(&req, s_rep, 0) == -1) {
            fprintf(stderr, "Error receiving message. errno: %s\n", strerror(errno));
            exit(1);
        }

        // Save and release message.
        char* req_buffer = (char*) malloc(zmq_msg_size(&req) + 1);
        memcpy(req_buffer, zmq_msg_data(&req), zmq_msg_size(&req));
        req_buffer[zmq_msg_size(&req)] = '\0';
        zmq_msg_close(&req);

        // Print the message.
        printf("received message: %s\n", req_buffer);
        if (strncmp(req_buffer, "exit", sizeof(*req_buffer)) == 0) {
            printf("Someone is telling me to exit!\n");
            exit_signal = 1;
        }

        free(req_buffer);

        // Answer the handshake appropiately.
        char* msg = "back handshake";
        zmq_msg_t res;
        if (zmq_msg_init_size(&res, strlen(msg)) == -1) {
            fprintf(stderr, "Error initiating response message. errno: %s\n", strerror(errno));
            exit(1);
        }
        memcpy(zmq_msg_data(&res), msg, strlen(msg));

        if (zmq_msg_send(&res, s_rep, 0) == -1) {
            // Failed to send message, print error and exit.
            fprintf(stderr, "Error sending message. errno: %s\n", strerror(errno));
            exit(1);
        }

        // Release message.
        zmq_msg_close(&res);

        if (exit_signal != 0) break;
    }

    // Cleaning up...
    zmq_close(s_rep);
    zmq_ctx_destroy(ctx);
    return 0;
}
