# TicTacToe Server

In this repository, you will find the T3P Server. This was a project requested in the course Laboratorio de Redes de Computadoras, from FIUBA. This server is based in RFC-TicTacToe (see useful links section for more details).

## How to compile and run

This server needs a linux kernel based OS in order to be compiled and executed. Before compiling, take a look at **config.h**, located in folder **headers**. Here you will find some
configurations to be made, such as the server IP and the TCP and UDP ports that runs on.

In the root folder, you will find a bash script: **make_server.sh**. This script will compile all the files and output an executable file called **server.out** in the same folder.

After running the script, type ./server.out to start the server.

## Testing the server responses

For testing TCP commands, we developed a little client that allows as to write a command in the standard input and print the server response in the standard output. In the other hand, UDP messages were tested using the tool netcat.

### Login and Logout

In this case, we tested some cases that may happen when a user tries to log in. It is important to highlight that when the client starts, it first connects to the server and then sends the LOGIN command. The server reads this command and decides what to do with it. If the response code is not 200, then the server will automatically close its socket, so after a wrong login, we need to restart the test client.

The steps taken in this test were:

1. Login normally

2. Send a Login command whilst in lobby.

3. Logout normally

4. Login with an incorrect name (not compliant with specified in RFC).

5. Login another client using a name already taken by another client in the server.

<div align="center">
    <img src="/docs/LOGIN.png" width="3000px" height="500px" </img> 
</div>

### HEARTBEAT

The heartbeat timeout time can be configured in config.txt file. By default, it is set to 30 seconds, but just for the sake of the test, we changed it to 10 seconds.

Steps taken in this test:

1. Login

2. Send HEARTBEAT command and see that the server received it correctly.

3. Wait 10 seconds for the server to log out the client due to heartbeat expiration.

<div align="center">
    <img src="/docs/HEARTBEAT.png" width="3000px" height="500px" </img> 
</div>

### Lobby context messages

This test is intented to show that in the Lobby context we can only send certain messages, whilst others will be responded with an Out of context message. To avoid being logged out because of heartbeat timeout, we changed the HEARTBEAT_TIMEOUT variable to 10000 seconds.

<div align="center">
    <img src="/docs/LOBBY_CONTEXT.png" width="3000px" height="500px" </img> 
</div>

### Random invite

To test RANDOMINVITE message, we changed the HEARTBEAT_TIMEOUT variable to 10000 seconds here too. To show different behaviours with this command, we took the following steps:

1. Login only with one client (ema).

2. Send a RANDOMINVITE command and see that there were no players available, as the only client online was 'ema'.

3. Login a second client (gonza).

4. Send another RANDOMINVITE from client 'ema'. Receive a 200|OK message.

5. 'gonza' receives an INVITEFROM|ema, which we respond with a DECLINE answer. Then 'ema' receives a DECLINE (INVITATIONRESPONSE).

6. 'ema' sends RANDOMINVITE again.

7. 'gonza' receives another INVITEFROM|ema, but doesn't respond for a time corresponding to INVITATION_TIMEOUT (30 seconds in this case). Then 'gonza' receives an INVITATIONTIMEOUT and 'ema' gets a DECLINE response.

8. Lastly, 'ema' sends a RANDOMINVITE, which 'gonza' will answer with an ACCEPT. 'ema' gets an ACCEPT and the server informs that both players are in a match, receiving randomly each of them a TURNWAIT and a TURNPLAY message.

<div align="center">
    <img src="/docs/RANDOMINVITE_1.png" width="3000px" height="500px" </img> 
</div>

Another behaviour to check with this command is its randomness. Then we:

1. Logged a third client (gero).

2. 'ema' send 4 RANDOMINVITE.

3. 'gonza' received 1 of them and 'gero' 3.

<div align="center">
    <img src="/docs/RANDOMINVITE_2.png" width="3000px" height="500px" </img> 
</div>

### Invite

In this test, we check how INVITE command responds. We needed 3 clients in this case:

1. Login the 3 clients (ema, gonza, gero).

2. 'ema' sends an INVITE|g, which will be rejected by the server because 'g' is not a valid name.

3. 'ema' invites 'gonza' to a match. Gonza receives an INVITEFROM|ema, which he declines. Then 'ema' receives a DECLINE.

4. 'gonza' invites 'gero', then 'ema' invites 'gero', but the server says 'gero' is occupied.

<div align="center">
    <img src="/docs/INVITE.png" width="3000px" height="500px" </img> 
</div>

### Match

Preserving the 10000 seconds of heartbeat timeout for testing, we proceed to start a match between 2 clients:

1. Login 2 clients (ema, gonza).

2. 'ema' sends RANDOMINVITE.

3. 'gonza' accepts the INVITEFROM|ema.

4. 'ema' receives ACCEPT. Then 'gonza' receives randomly TURNPLAY and 'ema' a TURNWAIT.

5. 'gonza' tries different combinations to mark a slot, receiving bad request in all of them. In first and second instances, sends a bad written command. In the third case, he writes a 0 in the slot place, which is a bad slot. However, bad slot message is used for slots that are occupied, so this is taken as a bad request. Then he sends a MARKSLOT|1, which is correct, so receives an OK answer, followed by a TURNWAIT (note here that the TURNWAIT command contains the recently written slot 1 by a circle player).

6. 'ema' receives a TURNPLAY, so marks slot number 2. Next TURNPLAY/TURNWAIT commands contain both slots marked previously (1 and 2).

7. The game continues till the end. It's useful to highlit that 'ema' wrote slot 4, then 'gonza' tries to write in the same slot, but receives a bad slot message.

8. Lastly, 'gonza' wins, so receives a MATCHEND|YOUWIN, while 'ema' receives a MATCHEND|YOULOSE.

9. Then another game starts after 'ema' sends a RANDOMINVITE.

10. 'gonza' starts again. This time, he doesn't answer during the MARKSLOT_TIMEOUT (30 seconds in this case), so the match ends, with a 'ema' receiving MATCHEND|TIMEOUTWIN and 'gonza' receiving MATCHEND|TIMEOUTLOSE.

11. We repeat the same steps. In this case, 'ema' starts, but doesn't answer, so 'gonza' receives a MATCHEND|TIMEOUTWIN and 'ema' a MATCHEND|TIMEOUTLOSE.

<div align="center">
    <img src="/docs/MATCH_1.png" width="3000px" height="500px" </img> 
</div>

Then we test what would happen if a match ends in draw.

1. Repeat the steps taken before till the match starts.

2. Play until the match ends in draw.

3. Both 'ema' and 'gonza' receives MATCHEND|DRAW.

<div align="center">
    <img src="/docs/MATCH_2.png" width="3000px" height="500px" </img> 
</div>

The last type of message that can be sent by the server is the CONNECTIONLOST one. This happens if one of the players gets disconnected during a match, for example if a heartbeat expires. So to test this, we first change the heartbeat timeout to a reasonable time (20 seconds for instance). 

1. Login with both clients

2. 'ema' sends a heartbeat, so 'gonza' will be the first one to be logged out.

3. Start a match.

4. After some seconds, 'gonza' gets disconnected (see server output), so 'ema' receives a MATCHEND|CONNECTIONLOST.

<div align="center">
    <img src="/docs/MATCH_3.png" width="3000px" height="500px" </img> 
</div>

Lastly, we show the behaviour with a GIVEUP command together with what we would get in a DISCOVER UDP message in three different cases. We extended the HEARTBEAT_TIMEOUT time once more for the sake of the test.

1. Login 3 clients.

2. 'ema' RANDOMINVITEs gero

3. We send a DISCOVER message using netcat utility, and receive a 200|OK|gonza \r\n ema|gero \r\n \r\n, which is correct as 'gonza' is available and 'gero' and 'ema' are occupied playing a match.

4. 'gero' gives up, receiving a MATCHEND|YOULOSE, while 'ema' receives MATCHEND|YOUWIN.

5. We send again a DISCOVER. This time we receive a 200|OK|ema|gonza|gero \r\n \r\n \r\n, corresponding the three players availble to 'ema', 'gonza' and 'gero'. No players are occupied.

6. 'gonza' invites 'ema' and a match is started.

7. We send a last DISCOVER, receiving a 200|OK|gero \r\n ema|gonza \r\n \r\n, which is correct once more.

8. 'gonza' gives up, receiving a MATCHEND|YOULOSE, while 'ema' receives MATCHEND|YOUWIN.

<div align="center">
    <img src="/docs/MATCH_4.png" width="3000px" height="500px" </img> 
</div>

## Useful links

<a href="https://docs.google.com/document/d/1eurJnPB9OFcRuYwLIdP7G9ug2nZrvtSjIa1J8TGMKkw/edit" target="_top">RFC-TicTacToe</a>

<a href="https://github.com/emanuelturtula/t3p-server" target="_top">TicTacToe Client</a>

## Authors

Emanuel Turtula

Gonzalo Rizzo

Gerónimo Ferrari
