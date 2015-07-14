curl --data '<iq type="result" to="stabber@localhost/profanity"><query xmlns="jabber:iq:roster" ver="362"><item jid="buddy1@localhost" subscription="both" name="Buddy1"/><item jid="buddy2@localhost" subscription="both" name="Buddy2"/></query></iq>' http://localhost:5231/for?query=jabber:iq:roster

echo "Press enter to send message..."
read

curl --data '<message id="mesg10" to="stabber@localhost/profanity" from="buddy1@localhost/laptop" type="chat"><body>Here is a message sent from stabber, using the Python bindings</body></message>' http://localhost:5231/send
