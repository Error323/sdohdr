#include <iostream>
#include <fstream>
#include <limits>
#include <vector>
#include <string>

#include <getopt.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>

#include "timer.h"
#include "udp.h"

using namespace std;

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
  cout << "  Example: sdohdr -f file.bin" << endl;
  cout << "  Example: sdohdr -f file.bin -t 8 -u 53234" << endl << endl;
  cout << " -h\tdisplay this help message" << endl;
  cout << " -f\tfile to read from or write to" << endl;
  cout << " -u\tread from udp using PORT" << endl;
  cout << " -t\ttime to read from stdin in seconds (default 10s)" << endl;
  exit(ret);
}

int setup(const int port)
{
  int fd = -1;
  sockaddr_in myaddr;
  if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
  {
    perror("Cannot create socket\n");
    exit(1);
  }

  myaddr.sin_family = AF_INET;
  myaddr.sin_addr.s_addr = htonl(INADDR_ANY);
  myaddr.sin_port = htons(port);

  if (bind(fd, (sockaddr*)&myaddr, sizeof(myaddr)) < 0)
  {
    perror("Bind failed\n");
    exit(1);
  }

  return fd;
}

int main(int argc, char *argv[])
{
  double max_duration = 10.0;
  string file_name;
  int port = 0;

  int c;
  while ((c = getopt(argc, argv, "hu:f:t:")) != -1)
  {
    switch (c)
    {
    case 'u': port = atoi(optarg); break;
    case 'f': file_name = string(optarg); break;
    case 't': max_duration = atof(optarg); break;
    case 'h': print_and_exit(EXIT_SUCCESS);
    case '?':
    default: print_and_exit(EXIT_FAILURE);
    }
  }

  packet_t packet;
  char *ptr = reinterpret_cast<char*>(&packet);

  if (port != 0)
  {
    std::vector<packet_t> buffer;
    int invalid = 0;

    sockaddr_in remaddr;
    socklen_t addrlen = sizeof(remaddr);
    int fd = setup(port);
    size_t bytes_received = 0;
    size_t total_bytes = 0;

    double start_time = timer::GetRealTime();
    while (true)
    {
      bytes_received = recvfrom(fd, ptr, sizeof(packet), 0, (sockaddr *)&remaddr, &addrlen);
      if (bytes_received > 0)
      {
        if (is_valid(packet.header))
          buffer.push_back(packet);
        else
          invalid++;
        total_bytes += bytes_received;
      }
      if (timer::GetRealTime() - start_time > max_duration)
        break;
    }

    ofstream file(file_name.c_str(), ios::out|ios::binary);
    for (int i = 0, n = buffer.size(); i < n; i++)
      file.write(reinterpret_cast<char*>(&buffer[i]), sizeof(packet));
    file.close();

    cout << "Valid   " << buffer.size() << endl;
    cout << "Invalid " << invalid << endl;
    cout << "Total   " << total_bytes << "B received, " << buffer.size()*sizeof(packet) << "B written" << endl;
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
    while (!file.eof())
    {
      file.read(ptr, sizeof(packet));
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
