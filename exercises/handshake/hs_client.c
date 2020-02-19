// Handshake client
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <zmq.h>

int main(void) {
    void *ctx = zmq_ctx_new();
    void *req = zmq_socket(ctx, ZMQ_REQ);
    int rc = zmq_connect(req, "tcp://localhost:5555");
    if (rc == -1) { // Failed to receive a message, print error and exit.
        fprintf(stderr, "Error connecting to socket. errno: %s\n", strerror(errno));
        exit(1);
    }

    zmq_msg_t msg;
    rc = zmq_msg_init_size(&msg, 0);
    assert(rc == 0);

    // Send a handshake to the server.
    printf("Sending a handshake to the friendly server...\n");
    rc = zmq_msg_send(&msg, req, 0);
    assert(rc == 0);

    // Release message.
    zmq_msg_close(&msg);

    // Cleaning up...
    zmq_close(req);
    zmq_ctx_destroy(ctx);
    return 0;
}
