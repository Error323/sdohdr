#include <iostream>
#include <fstream>
#include <limits>
#include <vector>
#include <string>
#include <getopt.h>
#include "timer.h"

using namespace std;

#define NUM_UNI_SUBBANDS 8
#define NUM_UNI_DIPOLES 96
#define NUM_BYTES_PER_WORD 4

// Data format of the AARTFAAC Uniboard output packets.
typedef struct
{
  unsigned long long rsp_reserved_1: 59;
  unsigned int rsp_rsp_clock: 1;
  unsigned int rsp_sdo_mode: 2;
  unsigned int rsp_lane_id: 2;
  unsigned int rsp_station_id: 16;
  unsigned int nof_words_per_block: 16; // 8sbbands X 96 dipoles=768 (4B = 1 word)
  unsigned int nof_blocks_per_packet: 16; // 2
  unsigned int rsp_sync: 1;
  unsigned int rsp_reserved_0: 13;
  unsigned long long int rsp_bsn: 50;       // Two timeslices share the same BSN
} __attribute ((__packed__)) header_t;

/* Uniboard data format:
 UDPhdr->userhdr(22B)->
 |d0_sb0_t0_p0_[r,i]...d95_sb0_t0_p1_[r,i]|... //subband 0
 |d0_sb7_t0_p1_[r,i]...d95_sb7_t0_p1_[r,i]|... // ts0

 |d0_sb0_t1_p0_[r,i]...d95_sb0_t1_p1_[r,i]|...
 |d0_sb7_t1_p0_[r,i]...d95_sb7_t1_p1_[r,i]|    // ts1
 d = dipole 0-95, p = pol 0 or 1 of an antenna, sb = subband, t = time sample,
 [r,i] = real/imag.
 1 UDP packet = 2 timeslices, 8 subbands, 96 dipoles.
*/
typedef struct
{
  short data [NUM_UNI_SUBBANDS * NUM_UNI_DIPOLES * 4];
} __attribute ((__packed__)) body_t;

typedef struct
{
  header_t header;
  body_t body[2]; // Two timeslices
} __attribute ((__packed__)) packet_t;

void print_header(const header_t &hdr, const int id)
{
  cout << "--------- " << id << " ---------" << endl;
  cout << "clock      " << hdr.rsp_rsp_clock << endl;
  cout << "sdo_mode   " << hdr.rsp_sdo_mode << endl;
  cout << "lane_id    " << hdr.rsp_lane_id << endl;
  cout << "station_id " << hdr.rsp_station_id << endl;
  cout << "words      " << hdr.nof_words_per_block << endl;
  cout << "blocks     " << hdr.nof_blocks_per_packet << endl;
  cout << "sync       " << hdr.rsp_sync << endl;
  cout << "bsn        " << hdr.rsp_bsn << endl;
}

bool is_valid(const header_t &hdr)
{
  size_t size_payload = hdr.nof_words_per_block * hdr.nof_blocks_per_packet * NUM_BYTES_PER_WORD;
  return size_payload == sizeof(body_t);
}

void print_and_exit(const int ret)
{
  cout << "Usage: sdohdr [OPTION]..." << endl;
  cout << "  Parse udp data from file or stdin" << endl;
  cout << "  Example: sdohdr -f file.bin -n 5" << endl;
  cout << "  Example: sdohdr -f file.bin -t 8 -i" << endl << endl;
  cout << " -h\tdisplay this help message" << endl;
  cout << " -f\tfile to read from or write to" << endl;
  cout << " -i\tread from stdin" << endl;
  cout << " -n\tnumber of packets to parse" << endl;
  cout << " -t\ttime to read from stdin in seconds (default 10s)" << endl;
  exit(ret);
}

int main(int argc, char *argv[])
{
  int n = numeric_limits<int>::max();
  double max_duration = 10.0;
  bool use_stdin = false;
  string file_name;

  int c;
  while ((c = getopt(argc, argv, "ihf:n:t:")) != -1)
  {
    switch (c)
    {
    case 'i': use_stdin = true; break;
    case 'f': file_name = string(optarg); break;
    case 'n': n = atoi(optarg); break;
    case 't': max_duration = atof(optarg); break;
    case 'h': print_and_exit(EXIT_SUCCESS);
    case '?':
    default: print_and_exit(EXIT_FAILURE);
    }
  }

  packet_t packet;

  if (use_stdin)
  {
    packet_t packet;
    char *ptr = reinterpret_cast<char*>(&packet);
    std::vector<packet_t> buffer;
    double start_time = timer::GetRealTime(), duration = 0.0;
    int invalid = 0;
    size_t i;

    while (true)
    {
      i = 0;
      while (i < sizeof(packet))
      {
        if (read(STDIN_FILENO, ptr+i, 1) > 0)
          i++;
        duration = timer::GetRealTime() - start_time;
        if (duration > max_duration)
          break;
      }

      if (duration > max_duration)
        break;

      if (is_valid(packet.header)) 
        buffer.push_back(packet);
      else
        invalid++;
    }

    ofstream file(file_name.c_str(), ios::out|ios::binary);
    for (int i = 0, n = buffer.size(); i < n; i++)
      file.write(reinterpret_cast<char*>(&buffer[i]), sizeof(packet));
    file.close();

    cout << "Valid   " << buffer.size() << endl;
    cout << "Invalid " << invalid << endl;
    cout << "Total   " << (buffer.size()+invalid)*sizeof(packet) << "B" << endl;
  }
  else
  {
    ifstream file(file_name.c_str(), ios::in|ios::binary);
    if (!file.good() || !file.is_open())
    {
      cerr << "Invalid filename" << endl;
      exit(EXIT_FAILURE);
    }
    int i = 0;
    while (!file.eof() && i < n)
    {
      file.read(reinterpret_cast<char*>(&packet), sizeof(packet));
      if (!is_valid(packet.header))
        cerr << "Invalid header for id " << i << endl;
      else
        print_header(packet.header, i);
      i++;
    }
    file.close();
  }
    
  return EXIT_SUCCESS;
}
