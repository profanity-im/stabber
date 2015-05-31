#include <glib.h>
#include <stdlib.h>
#include <stdio.h>
#include <stabber.h>
#include <pthread.h>

int
main(int argc , char *argv[])
{
    int port = 0;
    int httpport = 0;

    GOptionEntry entries[] =
    {
        { "port", 'p', 0, G_OPTION_ARG_INT, &port, "Listen port", NULL },
        { "http", 'h', 0, G_OPTION_ARG_INT, &httpport, "HTTP Listen port", NULL },
        { NULL }
    };

    GError *error = NULL;
    GOptionContext *context;

    context = g_option_context_new(NULL);
    g_option_context_add_main_entries(context, entries, NULL);
    if (!g_option_context_parse(context, &argc, &argv, &error)) {
        g_print("%s\n", error->message);
        g_option_context_free(context);
        g_error_free(error);
        return 1;
    }

    g_option_context_free(context);

    if (port == 0) {
        printf("Port must be specified with -p <port>\n");
        return 1;
    }

    stbbr_start(port, httpport);

    pthread_exit(0);
}
