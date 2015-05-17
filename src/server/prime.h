#ifndef __H_PRIME
#define __H_PRIME

void prime_init(void);
void prime_free_all(void);

void prime_required_passwd(char *password);
char* prime_get_passwd(void);

int prime_for(char *id, char *stream);
char* prime_get_for(const char *id);

#endif
