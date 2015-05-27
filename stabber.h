#ifndef __H_STABBER
#define __H_STABBER

int stbbr_start(int port);
void stbbr_stop(void);

void stbbr_set_timeout(int seconds);

int stbbr_auth_passwd(char *password);
void stbbr_for(char *id, char *stream);

int stbbr_received(char *stanza);
int stbbr_last_received(char *stanza);

void stbbr_send(char *stream);

#endif
