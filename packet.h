#include <cstring>
#define SEQ_MAX 25600
#define PACKET_SIZE 512

struct header
{
  bool SYN_flag;
  bool ACK_flag;
  bool FIN_flag;
  short SEQ_no;
  short ACK_no;
  int payload_size;
};

struct packet
{
  header header_;
  char data_[512];
};

void input_into_packet (struct packet& p, short seq,  short ack, bool SYN_flag, bool ACK_flag, bool FIN_flag, int payload_size)
{
  p.header_.SEQ_no = seq; 
  p.header_.ACK_no = ack;
  p.header_.SYN_flag = SYN_flag;
  p.header_.ACK_flag = ACK_flag;
  p.header_.FIN_flag = FIN_flag; 
  p.header_.payload_size = payload_size; 
}

int gen_SEQ_no()
{
  srand (time(NULL));
  return rand() % SEQ_MAX;
}

void fill_data(struct packet &p)
{
  std::string data_to_input(512, '*');
  strcpy(p.data_, data_to_input.c_str());
}