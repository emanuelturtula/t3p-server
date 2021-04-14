# TicTacToe Server

In this repository, you will find the T3P Server. This was a project requested in the course Laboratorio de Redes de Computadoras, from FIUBA. This server is based in RFC-TicTacToe (see useful links section for more details).

## How to compile and run

This server needs a linux kernel based OS in order to be compiled and executed. Before compiling, take a look at **config.h**, located in folder **headers**. Here you will find some
configurations to be made, such as the server IP and the TCP and UDP ports that runs on.

In the root folder, you will find a bash script: **make_server.sh**. This script will compile all the files and output an executable file called **server.out** in the same folder.

After running the script, type ./server.out to start the server.

## Useful links

<a href="https://docs.google.com/document/d/1eurJnPB9OFcRuYwLIdP7G9ug2nZrvtSjIa1J8TGMKkw/edit" target="_top">RFC-TicTacToe</a>

<a href="https://github.com/emanuelturtula/t3p-server" target="_top">TicTacToe Client</a>

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

## Authors

Emanuel Turtula

Gonzalo Rizzo

Gerónimo Ferrari
