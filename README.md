# Stabber
Stubbed XMPP (Jabber) Server.

# Overview
Stabber acts as a stubbed XMPP service for testing purposes, the API allows:
* Sending of XMPP stanzas.
* Responding to XMPP stanzas with stubbed responses, by id, and by query namespace
* Verifying that XMPP stanzas were received.

An HTTP API is also included, currently only supporting the `stbbr_send` and `stbbr_for_id` operations.

The project is work in progress with only the basics implemented, and is being developed alongside https://github.com/boothj5/profanity
Currently the only API is written in C.

# Installing
```
./bootstrap.sh
./configure
make
make install (as root)
```
# Using the C API
Include the following header in your tests:
```
#include <stabber.h>
````
Include the following in the linker path when compiling tests:
```
-lstabber
```

# C API

### Starting
To start Stabber on port 5230 for example:
```c
stbbr_start(5230, 0);
```
The second argument is the port to use for the HTTP interface, a value of 0 will not start the HTTP daemon.

### Stopping
To stop Stabber:
```c
stbbr_stop();
```

### Authentication
Currently only legacy authentication is supported, to set the password that stabber expects when an account connects:
```c
stbbr_auth_passwd("mypassword");
```
The default if not set is "password".

### Sending stanzas
To make Stabber send an XMPP stanza:
```c
stbbr_send(
    "<iq id=\"ping1\" type=\"get\" to=\"stabber@localhost/profanity\" from=\"localhost\">"
        "<ping xmlns=\"urn:xmpp:ping\"/>"
    "</iq>"
);
```

### Responding to stanzas
As well as being able to send an XMPP stanza at any time, you can also respond to a stanza by its id attribute:
```c
stbbr_for_id("msg_21",
    "<message id=\"message17\" to=\"stabber@localhost\" from=\"buddy1@localhost/mobile\" type=\"chat\">"
        "<body>I'm not real!</body>"
    "</message>"
);
```
To respond to an IQ get query (for example a roster request), use the following:
```c
stbbr_for_query("jabber:iq:roster",
    "<iq type=\"result\" to=\"stabber@localhost/profanity\">"
        "<query xmlns=\"jabber:iq:roster\" ver=\"362\">"
            "<item jid=\"buddy1@localhost\" subscription=\"both\" name=\"Buddy1\"/>"
            "<item jid=\"buddy2@localhost\" subscription=\"both\" name=\"Buddy2\"/>"
        "</query>"
    "</iq>"
);
```
Note that no ID is included in the stubbed response, Stabber will use the ID sent in the query. If an ID is supplied, it wil be overwitten by Stabber, again  using the ID sent in the query.

### Verify sent stanzas
To verify that you sent a particular stanza to Stabber:
```c
stbbr_received(
    "<message id=\"msg24415\" to=\"buddy1@localhost/mobile\" type=\"chat\">"
        "<body>I know, its a test.</body>"
    "</message>"
);
```
The above function returns 1 if the stanza has been received, and 0 if it hasn't.
The following function check that it was specifically the last stanza received:
```c
stbbr_last_received(
    "<message id=\"msg24415\" to=\"buddy1@localhost/mobile\" type=\"chat\">"
        "<body>I know, its a test.</body>"
    "</message>"
);
```
Both verifications allow for wildcards (*) as attribute values, for example, if you don't know the id's that are generated by your client:
```c
stbbr_received(
    "<message id=\"*\" to=\"buddy1@localhost/mobile\" type=\"chat\">"
        "<body>I know, its a test.</body>"
    "</message>"
);
````
By default the verification calls block for up to 10 seconds, the timeout in seconds can be set with:
```c
stbbr_set_timeout(3);
```
A value of 0 or less is non-blocking and will return immediately.

### Waiting
Sometimes a test needs to wait until the client being tested has had time to send some specific stanzas. The following will block until a stanza with a particular ID has been received by Stabber:

```c
stbbr_wait_for("someid");
```

# Logs
Stabber logs to:
```
~/.local/share/stabber/logs/stabber.log
```

# HTTP API
To start stabber in standalone mode:
```
stabber -p 5230 -h 5231
```
The second argument is the HTTP port on which Stabber will listen.

To send a message to a client currently connected to Stabber on port 5230, send a POST request to `http://localhost:5231/send` with the body containing the stanza to send, e.g.:
```
curl --data '<message id="mesg10" to="stabber@localhost/profanity" from="buddy1@localhost/laptop" type="chat"><body>Here is a message sent from stabber, using the HTTP api</body></message>' http://localhost:5231/send
```

To respond to a stanza with a specfic id sent from the client, send a POST request to `http://localhost:5231/for?id=<id>` where `<id>` is the the id you wish to respond to, e.g.:
```
curl --data '<message id="messageid1" to="stabber@localhost/profanity" from="buddy1@localhost/work" type="chat"><body>heres my answer!</body></message>' http://localhost:5231/for?id=prof_msg_1
```

# Examples
Example tests for Profanity can be found at: https://github.com/boothj5/profanity/tree/stabber-tests/functionaltests
