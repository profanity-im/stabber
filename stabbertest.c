#include <stabber.h>
#include <pthread.h>
#include <stdlib.h>

int main(void)
{
    stbbr_start(5230);

    stbbr_auth_passwd("password");

    stbbr_for("roster",
        "<iq id=\"roster\" type=\"result\" to=\"stabber@localhost/profanity\">"
            "<query xmlns=\"jabber:iq:roster\" ver=\"362\">"
                "<item jid=\"buddy1@localhost\" subscription=\"both\" name=\"Buddy1\"/>"
                "<item jid=\"buddy2@localhost\" subscription=\"both\" name=\"Buddy2\"/>"
            "</query>"
        "</iq>");

    stbbr_for("prof_presence_1",
        "<presence to=\"stabber@localhost\" from=\"buddy1@localhost/mobile\">"
            "<show>dnd</show>"
            "<status>busy!</status>"
        "</presence>"
        "<presence to=\"stabber@localhost\" from=\"buddy1@localhost/laptop\">"
            "<show>chat</show>"
            "<status>Talk to me!</status>"
        "</presence>"
        "<presence to=\"stabber@localhost\" from=\"buddy2@localhost/work\">"
            "<show>away</show>"
            "<status>Out of office</status>"
        "</presence>");

    stbbr_for("prof_msg_2",
        "<message id=\"message1\" to=\"stabber@localhost\" from=\"buddy1@localhost/mobile\" type=\"chat\">"
        "<body>Welcome!!</body>"
        "</message>");

    stbbr_for("prof_msg_3",
        "<message id=\"message2\" to=\"stabber@localhost\" from=\"buddy1@localhost/laptop\" type=\"chat\">"
        "<body>From me laptop</body>"
        "</message>");

    pthread_exit(NULL);
}
