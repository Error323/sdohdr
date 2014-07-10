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

