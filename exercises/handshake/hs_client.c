// Handshake client
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <czmq.h>

int main(void) {
    void *ctx = zmq_ctx_new();
    void *s_req = zmq_socket(ctx, ZMQ_REQ);
    if (zmq_connect(s_req, "tcp://localhost:5555") == -1) {
        // Failed to connect to socket, print error and exit.
        fprintf(stderr, "Error connecting to socket. errno: %s\n", strerror(errno));
        exit(1);
    }

    { // Send and receive a message.
        char* msg = "handshake";

        zmq_msg_t req;
        if (zmq_msg_init_size(&req, strlen(msg)) == -1) {
            // Failed to write message, print error and exit.
            fprintf(stderr, "Error initializing message. errno: %s\n", strerror(errno));
            exit(1);
        }
        memcpy(zmq_msg_data(&req), msg, strlen(msg));

        // Send a handshake to the server.
        printf("Sending a handshake to the friendly server...\n");
        if (zmq_msg_send(&req, s_req, 0) == -1) {
            // Failed to send message, print error and exit.
            fprintf(stderr, "Error sending message. errno: %s\n", strerror(errno));
            exit(1);
        }

        // Release message.
        zmq_msg_close(&req);

        // Create a response message.
        zmq_msg_t res;
        if (zmq_msg_init(&res) == -1) {
            fprintf(stderr, "Error initiating response message. errno: %s\n", strerror(errno));
            exit(1);
        }

        // Block and wait for a response.
        if (zmq_msg_recv(&res, s_req, 0) == -1) {
            fprintf(stderr, "Error receiving message. errno: %s\n", strerror(errno));
            exit(1);
        }

        // Print the message.
        char* res_buffer = (char*) malloc(zmq_msg_size(&res) + 1);
        memcpy(res_buffer, zmq_msg_data(&res), zmq_msg_size(&res));
        res_buffer[zmq_msg_size(&res)] = '\0';
        printf("received message: %s\n", res_buffer);

        // Release message.
        zmq_msg_close(&res);
        free(res_buffer);
    }

    // Cleaning up...
    zmq_close(s_req);
    zmq_ctx_destroy(ctx);
    return 0;
}
