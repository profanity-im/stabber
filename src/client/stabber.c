#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#include "server/server.h"
#include "server/prime.h"

typedef struct server_args_t {
    int port;
} StabberArgs;

static void*
_start_server_cb(void *data)
{
    StabberArgs *args = (StabberArgs*)data;
    server_run(args->port);

    return NULL;
}

int
stbbr_main(int port)
{
    return server_run(port);
}

int
stbbr_start(int port)
{
    StabberArgs *args = malloc(sizeof(StabberArgs));
    args->port = port;

    pthread_t server_thread;
    int res = pthread_create(&server_thread, NULL, _start_server_cb, (void*)args);
    if (res != 0) {
        return 0;
    }
    pthread_detach(server_thread);

    return 1;
}

int
stbbr_auth_passwd(char *password)
{
    prime_required_passwd(password);
    return 1;
}

int
stbbr_for(char *id, char *stanza)
{
    prime_for(id, stanza);
    return 1;
}
