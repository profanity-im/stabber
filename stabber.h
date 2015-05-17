#ifndef __H_STABBER
#define __H_STABBER

int stbbr_start(int port);

int stbbr_auth_passwd(char *password);
void stbbr_for(char *id, char *stream);

#endif
