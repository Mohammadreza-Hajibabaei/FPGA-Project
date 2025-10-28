#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <math.h>

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

void GetSinWave(int point, int max_amp, int amp_val, u8 *sin_tab);
void GetSquareWave(int point, int max_amp, int amp_val, u8 *Square_tab);
void GetTriangleWave(int point, int max_amp, int amp_val, u8 *Triangle_tab);
void GetSawtoothWave(int point, int max_amp, int amp_val, u8 *Sawtooth_tab);
void GetSubSawtoothWave(int point, int max_amp, int amp_val, u8 *SubSawtooth_tab);

void init_scrambler(Scrambler *scrambler) {
    scrambler->shift_register = 0x0401; // Initial value
}

uint8_t scramble_bit(Scrambler *scrambler, uint8_t input_bit) {
    uint8_t feedback = ((scrambler->shift_register >> 10) & 1) ^ ((scrambler->shift_register >> 8) & 1);
    uint8_t scrambled_bit = input_bit ^ feedback;
    scrambler->shift_register = (scrambled_bit << 0) | (scrambler->shift_register << 1);
    return scrambled_bit;
}

void scramble_data(Scrambler *scrambler, uint8_t *data, size_t length) {
    for (size_t i = 0; i < length; i++) {
        uint8_t feedback = ((scrambler->shift_register >> 0) & 1) ^ ((scrambler->shift_register >> 1) & 1);
        data[i] = feedback ^ data[i];
        scrambler->shift_register = (scrambler->shift_register >> 1) | (feedback << 14);;
    }
//    for (size_t i = 0; i < length; i++) {
//        for (int bit = 7; bit >= 0; bit--) {
//            uint8_t input_bit = (data[i] >> bit) & 1;
//            uint8_t scrambled_bit = scramble_bit(scrambler, input_bit);
//            data[i] = (data[i] & ~(1 << bit)) | (scrambled_bit << bit);
//        }
//    }
}

uint16_t calculate_checksum(CCS_DS_Header *header) {
    uint32_t sum = header->protocol_id + header->source_addr + header->dest_addr + header->msg_type + header->length;
    return ~(sum & 0xFFFF); // Simple one's complement
}

CCS_DS_Header generate_header(uint8_t protocol_id, uint16_t source_addr, uint16_t dest_addr, uint8_t msg_type, uint16_t length) {
    CCS_DS_Header header;
    header.protocol_id = protocol_id;
    header.source_addr = source_addr;
    header.dest_addr = dest_addr;
    header.msg_type = msg_type;
    header.length = length;
    header.checksum = calculate_checksum(&header);
    return header;
}

void insert_header_and_scramble(Scrambler *scrambler, uint8_t *data, CCS_DS_Header header) {
    // Convert header to byte array
    uint8_t header_bytes[HEADER_LENGTH];
    memcpy(header_bytes, &header, sizeof(header));

    // Insert header at the beginning of the frame
    for (int i = 0; i < HEADER_LENGTH; i++) {
        data[i] = header_bytes[i];
    }

    // Scramble the rest of the frame data
    scramble_data(scrambler, data + HEADER_LENGTH, DATA_LENGTH);
}

void GetSinWave(int point, int max_amp, int amp_val, u8 *sin_tab) {
    int i;
    double radian;
    double x;
    double PI = 3.1416;
    /* radian value */
    radian = 2 * PI / point;

    for (i = 0; i < point; i++) {
        x = radian * i;
        sin_tab[i] = (u8)((amp_val / 2) * sin(x) + max_amp / 2);
    }
}

void GetSquareWave(int point, int max_amp, int amp_val, u8 *Square_tab) {
    int i;

    for (i = 0; i < point; i++) {
        if (i < point / 2)
            Square_tab[i] = (u8)((max_amp - amp_val) / 2 + amp_val - 1);
        else
            Square_tab[i] = (u8)((max_amp - amp_val) / 2);
    }
}

void GetTriangleWave(int point, int max_amp, int amp_val, u8 *Triangle_tab) {
    int i;
    double tap_val;
    tap_val = 2 * (double)amp_val / (double)point;

    for (i = 0; i < point; i++) {
        if (i < point / 2)
            Triangle_tab[i] = (u8)(i * tap_val + (max_amp - amp_val) / 2);
        else
            Triangle_tab[i] = (u8)(amp_val - 1 - (i - point / 2) * tap_val + (max_amp - amp_val) / 2);
    }
}

void GetSawtoothWave(int point, int max_amp, int amp_val, u8 *Sawtooth_tab) {
    int i;
    double tap_val;
    tap_val = (double)amp_val / (double)point;

    for (i = 0; i < point; i++)
        Sawtooth_tab[i] = (u8)(i * tap_val + (max_amp - amp_val) / 2);
}

void GetSubSawtoothWave(int point, int max_amp, int amp_val, u8 *SubSawtooth_tab) {
    int i;
    double tap_val;
    tap_val = (double)amp_val / (double)point;

    for (i = 0; i < point; i++)
        SubSawtooth_tab[i] = (u8)(amp_val - 1 - i * tap_val + (max_amp - amp_val) / 2);
}

void process_wave(const char* filename, void (*wave_func)(int, int, int, u8*), CCS_DS_Header header, const char* wave_name) {
    u8 data[TOTAL_SAMPLES];
    wave_func(TOTAL_SAMPLES, MAX_AMP, AMP_VAL, data);

    Scrambler scrambler;

    FILE *file = fopen(filename, "w");
    if (file == NULL) {
        printf("Error opening file: %s\n", filename);
        return;
    }

    int frame_count = (TOTAL_SAMPLES + DATA_LENGTH - 1) / DATA_LENGTH;
    for (int i = 0; i < frame_count; i++) {
        init_scrambler(&scrambler);
        uint8_t frame_data[BYTES_PER_FRAME] = {0};

        // Fill frame data with wave samples
        int sample_start = i * DATA_LENGTH;
        int sample_end = sample_start + DATA_LENGTH;
        for (int j = sample_start, k = HEADER_LENGTH; j < sample_end && j < TOTAL_SAMPLES; j++, k++) {
            frame_data[k] = data[j];
        }

        insert_header_and_scramble(&scrambler, frame_data, header);

        // Write scrambled data to file
        // fprintf(file, "%s Wave Frame %d:\n", wave_name, i + 1);
        for (int j = 0; j < BYTES_PER_FRAME; j++) {
            fprintf(file, "%02X\n", frame_data[j]);
        }
        // fprintf(file, "\n");
    }

    fclose(file);
}

int main() {
    CCS_DS_Header header = generate_header(0x01, 0x1001, 0x2001, 0x01, FRAME_LENGTH);

    process_wave("sin_wave.txt", GetSinWave, header, "Sin");
    process_wave("square_wave.txt", GetSquareWave, header, "Square");
    process_wave("triangle_wave.txt", GetTriangleWave, header, "Triangle");
    process_wave("sawtooth_wave.txt", GetSawtoothWave, header, "Sawtooth");
    process_wave("sub_sawtooth_wave.txt", GetSubSawtoothWave, header, "SubSawtooth");

    return 0;
}
