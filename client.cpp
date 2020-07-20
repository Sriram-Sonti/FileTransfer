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
struct sockaddr_in sockaddr;
socklen_t len;
vector<char> nullvec;

// make initial contact with server
struct packet send_syn_get_ack()
{
  // send SYN to server
  struct packet syn;
  input_into_packet(syn, gen_SEQ_no(), 0, true, false, false, PACKET_SIZE);
  fill_data(syn);
  sendto(sockfd, &syn, sizeof(syn), 0, (const struct sockaddr *)&sockaddr, sizeof(sockaddr));

  // receive ack from server and return it
  struct packet ack;
  size_t recv_code = recvfrom(sockfd, &ack, sizeof(ack), 0, (struct sockaddr *)&sockaddr, &len);
  return ack;
}

// put data from file into a vector of characters
vector<char> get_file_data(char* fd)
{
  ifstream FILE(fd);
  vector<char> file_data((istreambuf_iterator<char>(FILE)), (istreambuf_iterator<char>()));
  FILE.close();
  return file_data;
}

// send first PACKET_SIZE bytes from file data
vector<char> packetify(vector<char>& file_data)
{
  vector<char> packet_data;
  if (file_data.size() > PACKET_SIZE)
  {
    packet_data.assign(file_data.begin(), file_data.begin()+PACKET_SIZE);
    file_data.assign(file_data.begin()+PACKET_SIZE, file_data.end());
  }
  else
  {
    packet_data = file_data;
    file_data = nullvec;
  }
  return packet_data;
}

void send_data_get_ack(int &SEQ_no, int &ACK_no, vector<char> packet_data)
{
  // create the packet we will send
  struct packet packet_to_send;
  
  // copy packet_data into the packet and send it
  copy(packet_data.begin(), packet_data.end(), packet_to_send.data_);
  input_into_packet(packet_to_send, ACK_no, SEQ_no, false, true, false, packet_data.size());
  sendto(sockfd, &packet_to_send, sizeof(packet_to_send), 0, (const struct sockaddr *)&sockaddr, sizeof(sockaddr));

  // receive ack for packet from server
  struct packet ack;
  size_t recv_code = recvfrom(sockfd, &ack, sizeof(ack), 0, (struct sockaddr *)&sockaddr, &len);

  // set sequence and ack numbers for the next packet to use
  SEQ_no = ack.header_.SEQ_no;
  ACK_no = ack.header_.ACK_no;
}

void fin_protocol(int SEQ_no, int ACK_no)
{
  struct packet fin, ack_from_server, fin_from_server, ack_to_send;

  // sending FIN
  input_into_packet(fin, SEQ_no, ACK_no, false, false, true, 1);
  sendto(sockfd, &fin, sizeof(fin), 0, (const struct sockaddr *)&sockaddr, sizeof(sockaddr));

  // receive ACK from server
  size_t recv_code = recvfrom(sockfd, &ack_from_server, sizeof(ack_from_server), 0, (struct sockaddr *)&sockaddr, &len);

  // receive FIN from server
  recv_code = recvfrom(sockfd, &fin_from_server, sizeof(fin_from_server), 0, (struct sockaddr *)&sockaddr, &len);
  
  // send ACK for FIN from server
  input_into_packet(ack_to_send, fin_from_server.header_.ACK_no, fin_from_server.header_.SEQ_no + 1, false, true, false, 0);
  sendto(sockfd, &ack_to_send, sizeof(ack_to_send), 0, (const struct sockaddr *)&sockaddr, sizeof(sockaddr));
}

int main(int argc, char **argv)
{
  // create socket and get hostname and port
  sockfd = socket(AF_INET, SOCK_DGRAM, 0);
  char* hostname = argv[1];
  PORT = atoi(argv[2]);

  // create address for local socket, and bind to it
  memset(&sockaddr, 0, sizeof(sockaddr_in));
  sockaddr.sin_family = AF_INET;
  sockaddr.sin_port = htons(PORT);
  sockaddr.sin_addr.s_addr = htonl(INADDR_ANY);
  len = sizeof(sockaddr);
  bind(sockfd, (const struct sockaddr *)&sockaddr, sizeof(struct sockaddr));

  // send SYN to server and get ACK packet
  struct packet ack_for_syn = send_syn_get_ack();

  // put file data into a vector
  vector<char> file_data = get_file_data(argv[3]);
  
  // get SEQ and ACK numbers for ACK for next packet
  int next_packet_SEQ_no = ack_for_syn.header_.SEQ_no + 1;
  int next_packet_ACK_no = ack_for_syn.header_.ACK_no;

  // start uploading data to server
  while (!file_data.empty())
  {
    // convert file data into packets of size PACKET_SIZE
    vector<char> packet_data = packetify(file_data);

    // send data over and get ACK as confirmation
    send_data_get_ack(next_packet_SEQ_no, next_packet_ACK_no, packet_data);
  }

  // send FIN, get ACK, get servers' FIN, and send ACK for that FIN
  fin_protocol(next_packet_SEQ_no, next_packet_ACK_no);

  // close socket
  close(sockfd);

  return 0;
}
