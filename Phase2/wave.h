// wave_processing.h

#ifndef WAVE_PROCESSING_H
#define WAVE_PROCESSING_H

#include <stdint.h>
#include <stddef.h>

#define FRAME_LENGTH 512
#define HEADER_LENGTH 10
#define DATA_LENGTH 54
#define BYTES_PER_FRAME (HEADER_LENGTH + DATA_LENGTH)
#define TOTAL_SAMPLES 1024
#define MAX_AMP 255
#define AMP_VAL 255

typedef uint8_t u8;

typedef struct {
    uint8_t protocol_id;
    uint16_t source_addr;
    uint16_t dest_addr;
    uint8_t msg_type;
    uint16_t length;
    uint16_t checksum;
} CCS_DS_Header;

typedef struct {
    uint16_t shift_register;
} Scrambler;







void init_scrambler(Scrambler *scrambler) ;

uint8_t scramble_bit(Scrambler *scrambler, uint8_t input_bit) ;

void scramble_data(Scrambler *scrambler, uint8_t *data, size_t length) ;

uint16_t calculate_checksum(CCS_DS_Header *header);

CCS_DS_Header generate_header(uint8_t protocol_id, uint16_t source_addr, uint16_t dest_addr, uint8_t msg_type, uint16_t length) ;

void insert_header_and_scramble(Scrambler *scrambler, uint8_t *data, CCS_DS_Header header);





// Function prototypes
void GetSinWave(int point, int max_amp, int amp_val, u8 *sin_tab);
void GetSquareWave(int point, int max_amp, int amp_val, u8 *Square_tab);
void GetTriangleWave(int point, int max_amp, int amp_val, u8 *Triangle_tab);
void GetSawtoothWave(int point, int max_amp, int amp_val, u8 *Sawtooth_tab);
void GetSubSawtoothWave(int point, int max_amp, int amp_val, u8 *SubSawtooth_tab);

void init_scrambler(Scrambler *scrambler);
uint8_t lfsr_step(uint16_t *lfsr);
void scramble_data(Scrambler *scrambler, uint8_t *data, size_t length);

uint16_t calculate_checksum(CCS_DS_Header *header);
CCS_DS_Header generate_header(uint8_t protocol_id, uint16_t source_addr, uint16_t dest_addr, uint8_t msg_type, uint16_t length);

void insert_header_and_scramble(Scrambler *scrambler, uint8_t *data, CCS_DS_Header header);

void process_wave(const char* filename, void (*wave_func)(int, int, int, u8*), CCS_DS_Header header, const char* wave_name);

#endif // WAVE_PROCESSING_H
