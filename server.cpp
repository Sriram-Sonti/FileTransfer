#include <iostream>
#include <errno.h>
#include <cstring>
#include <fstream>
#include <istream>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <vector>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "packet.h"

using namespace std;

// globals
static int PORT;
int sockfd;
vector<char> nullvec;
struct sockaddr_in client_addr;
struct sockaddr_in my_addr;
socklen_t len;

void fin_response(struct packet fin_from_client)
{
    // send ACK for FIN we received
    struct packet ack_to_send;
    input_into_packet(ack_to_send, fin_from_client.header_.ACK_no, fin_from_client.header_.SEQ_no + 1, false, true, false, 0);
    sendto(sockfd, &ack_to_send, sizeof(ack_to_send), 0, (const struct sockaddr *)&client_addr, len);

    // send our own FIN
    struct packet fin;
    input_into_packet(fin, fin_from_client.header_.ACK_no, ack_to_send.header_.ACK_no, false, false, true, 1);
    sendto(sockfd, &fin, sizeof(fin), 0, (const struct sockaddr *)&client_addr, len);

    // recieve ACK from client
    struct packet fin_ack_received;
    size_t recv_code = recvfrom(sockfd, &fin_ack_received, sizeof(fin_ack_received), 0, (struct sockaddr *)&client_addr, &len);
}

int main(int argc, char **argv)
{
    // create socket
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    PORT = atoi(argv[1]);
    
    // fill client address
    memset(&client_addr, 0, sizeof(struct sockaddr_in));
    len = sizeof(client_addr);
    
    // create address for server
    memset(&my_addr, 0, sizeof(sockaddr_in));
    my_addr.sin_family = AF_INET;
    my_addr.sin_port = htons(PORT);
    my_addr.sin_addr.s_addr = htonl(INADDR_ANY); 
    
    // bind address to socket
    bind(sockfd, (const struct sockaddr *)&my_addr, sizeof(struct sockaddr));

    // entire data we receive
    vector<char> data_received_tot;

    // we will initally get a syn
    bool SYN_flag = true;

    // packet we received
    struct packet incoming_packet;

    while (true)
    {
        if(SYN_flag)
        {
            // get the SYN
            size_t recv_code = recvfrom(sockfd, &incoming_packet, sizeof(incoming_packet), 0, (struct sockaddr *)&client_addr, &len);

            // send ack for received SYN
            struct packet ack;
            input_into_packet(ack, gen_SEQ_no(), incoming_packet.header_.SEQ_no, true, true, false, PACKET_SIZE);
            fill_data(ack);
            sendto(sockfd, &ack, sizeof(ack), 0, (const struct sockaddr *)&client_addr, len);
            
            // no more SYNs to receive
            SYN_flag = false;
        }

        else
        {
            size_t recv_code = recvfrom(sockfd, &incoming_packet, sizeof(incoming_packet), 0, (struct sockaddr *)&client_addr, &len);
            if (incoming_packet.header_.FIN_flag==true)
            {
                // write all the received data to file
                std::ofstream out("received_file");
                out.write((const char*)&data_received_tot[0], data_received_tot.size());
                out.close();

                // handle FIN response
                fin_response(incoming_packet);

                // wait for next request
                SYN_flag = true;
                data_received_tot = nullvec;
            }

            else
            {
                // send ACK for data we received
                struct packet ack;
                input_into_packet(ack, incoming_packet.header_.ACK_no, incoming_packet.header_.SEQ_no + incoming_packet.header_.payload_size, false, true, false, 0);
                sendto(sockfd, &ack, sizeof(ack), 0, (const struct sockaddr *)&client_addr, len);

                // add data we received to our vector
                data_received_tot.insert(data_received_tot.end(), incoming_packet.data_, incoming_packet.data_ + incoming_packet.header_.payload_size);
            }
        }
    }

    close(sockfd);
    return 0;
}
