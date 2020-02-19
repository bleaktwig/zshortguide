// Handshake server
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <zmq.h>

int main(void) {
    void *ctx = zmq_ctx_new();
    void *rep = zmq_socket(ctx, ZMQ_REP);
    int rc = zmq_bind(rep, "tcp://*:5555");
    assert(rc == 0);

    zmq_msg_t msg;
    rc = zmq_msg_init(&msg);
    assert(rc == 0);

    // Block until a handshake is received.
    printf("Waiting for a client to give me a handshake...\n");
    rc = zmq_msg_recv(&msg, rep, 0);
    if (rc == -1) { // Failed to receive a message, print error and exit.
        fprintf(stderr, "Error receiving message. errno: %s\n", strerror(errno));
        exit(1);
    }

    // Process the message and assert that it's a handshake.
    size_t msg_size = zmq_msg_size(&msg);
    if (msg_size == 0) printf("Handshake received. Exiting...\n");
    else printf("The message received isn't a handshake. I'm sad now.\n");

    // Release message.
    zmq_msg_close(&msg);

    // Closing up...
    zmq_close(rep);
    zmq_ctx_destroy(ctx);
    return 0;
}
