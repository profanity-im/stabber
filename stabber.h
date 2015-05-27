#ifndef __H_STABBER
#define __H_STABBER

int stbbr_start(int port);
void stbbr_stop(void);

void stbbr_set_timeout(int seconds);

int stbbr_auth_passwd(char *password);
void stbbr_for(char *id, char *stream);

int stbbr_verify(char *stanza);
int stbbr_verify_last(char *stanza);

void stbbr_send(char *stream);

#endif
