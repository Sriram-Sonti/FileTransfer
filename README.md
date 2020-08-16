For this C++ project, which allows for files to be uploaded from the client to the server, I created a packet-handling protocol (that works on top of the standard C socket programming) that works very similarly to TCP.

My TCP-esque packet design is to be found in packet.h. It consists of a 12-byte header a 512 byte body. The header consists of flags for syn, ack, and fin, sequence number and ack number, as well as the size of data in the body.

The client creates a socket at the passed-in port and binds to it. Then, it sequentially sends to the server a SYN packet. Then it receives the server's ACK for that packet. (It uses the sequence and ack number in that to set the sequence and ack numbers for its own packets). Next, it gets all the data from the requested file and puts it into a vector. It sends this data over packet by packet (512 bytes) and waits for ACK. Once all the data is sent, it sends FIN, waits for ACK, waits for server's FIN, and sends its own ACK for that FIN.

The server uses recvfrom to accept a packet. It then checks if that packets is a SYN, FIN, or ordinary packet. If SYN, it responds with an ACK. If ordinary packet with data, it writes the data to a vector. When it receives a FIN, it writes this vector to a file "received_file" and does the FIN exit procedure (send ACK for client's FIN, send own FIN, accept client's ACK).

To run the program, please run `make`.

Then start the server first with the following command:

`./server <portnumber>`

And, in another terminal window, start the client with:

`./client <hostname> <portnumber> <filename>`

The client will then send over the requested file to the server in TCP packets of size 512.

Todo:
1. Add option to encrypt the files. 
2. Allow bi-directional file transfer
