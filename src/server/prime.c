#include <string.h>

static char *required_passwd = NULL;

void
prime_required_passwd(char *password)
{
    required_passwd = strdup(password);
}

char *
prime_get_passwd(void)
{
    return required_passwd;
}
