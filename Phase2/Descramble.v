`timescale 1ns / 1ps

module Descramble(
    input clk,
    input reset,
    input [15:0]scramble_data,
    output reg [15:0]descramble_data,
    output reg data_valid
    );
    
    reg [17:0]sr_x = 18'b100000000000000000;
    reg [17:0]sr_y = 18'b011111111111111111; 
    reg [17:0]initial_x = 18'b000000000000000001;
    reg [17:0]initial_y = 18'b111111111111111111;   
    reg [15:0]header_pattern = 16'b1111111111111111;     
    wire a,b,d;
    wire [1:0] c;
    wire [1:0] R;
    reg [1:0]state;
    reg [3:0] header_counter;
    parameter IDLE = 2'b0;
    parameter descramble = 2'b1;
    assign a = sr_x[5] ^ sr_x[7] ^ sr_x[16];
    assign b = sr_y[6] ^ sr_y[7] ^ sr_y[9] ^ sr_y[10] ^ sr_y[11] ^ sr_y[12] ^ sr_y[13] ^ sr_y[14] ^ sr_y[15] ^ sr_y[16];
    assign c = (a ^ b) * 2;
    assign d = sr_x[1] ^ sr_y[1];
    assign R = {1'b0,d} + c;
//    assign R = 3;
    always@(posedge clk)
    begin
        if(!reset)
        begin
            sr_x <= initial_x;
            sr_y <= initial_y;
            header_counter <= 0;
            data_valid <= 0;
            state <= IDLE;
        end
        else
        begin
            case(state)
                IDLE:
                begin
                    if(scramble_data == header_pattern)
                    begin
                        header_counter <= header_counter + 1;
                        if(header_counter == 3)
                        begin
                            header_counter <= 0;
                            state <= descramble;
                            data_valid <= 0;
                        end
                    end
                    else
                    begin
                        header_counter <= 0;
                        data_valid <= 0;
                    end
                end
                descramble:
                begin
                    if(scramble_data != header_pattern)
                    begin
                        data_valid <= 1;
                        sr_x[16] <= sr_x[17]; 
                        sr_x[15] <= sr_x[16]; 
                        sr_x[14] <= sr_x[15]; 
                        sr_x[13] <= sr_x[14]; 
                        sr_x[12] <= sr_x[13]; 
                        sr_x[11] <= sr_x[12]; 
                        sr_x[10] <= sr_x[11]; 
                        sr_x[9] <= sr_x[10]; 
                        sr_x[8] <= sr_x[9]; 
                        sr_x[7] <= sr_x[8]; 
                        sr_x[6] <= sr_x[7]; 
                        sr_x[5] <= sr_x[6]; 
                        sr_x[4] <= sr_x[5]; 
                        sr_x[3] <= sr_x[4]; 
                        sr_x[2] <= sr_x[3]; 
                        sr_x[1] <= sr_x[2]; 
                        sr_x[0] <= sr_x[1]; 
                        sr_x[17] <= sr_x[0] ^ sr_x[7];
                        
                        sr_y[16] <= sr_y[17]; 
                        sr_y[15] <= sr_y[16]; 
                        sr_y[14] <= sr_y[15]; 
                        sr_y[13] <= sr_y[14]; 
                        sr_y[12] <= sr_y[13]; 
                        sr_y[11] <= sr_y[12]; 
                        sr_y[10] <= sr_y[11]; 
                        sr_y[9] <= sr_y[10]; 
                        sr_y[8] <= sr_y[9]; 
                        sr_y[7] <= sr_y[8]; 
                        sr_y[6] <= sr_y[7]; 
                        sr_y[5] <= sr_y[6]; 
                        sr_y[4] <= sr_y[5]; 
                        sr_y[3] <= sr_y[4]; 
                        sr_y[2] <= sr_y[3]; 
                        sr_y[1] <= sr_y[2]; 
                        sr_y[0] <= sr_y[1]; 
                        sr_y[17] <= sr_y[0] ^ sr_y[5] ^ sr_y[7] ^ sr_y[10];
                        
                        case(R)
                            0: 
                                descramble_data <= scramble_data;
                            1:
                            begin
                                // imag
                                descramble_data[15:8] <=  ~scramble_data[7:0] + 8'b1;
                                //real
                                descramble_data[7:0] <=  scramble_data[15:8];
                            end
                            2:
                            begin
                                // imag
                                descramble_data[15:8] <= ~scramble_data[15:8] + 8'b1;
                                //real
                                descramble_data[7:0] <= ~scramble_data[7:0] + 8'b1;
                            end
                            3:
                            begin
                                // imag
                                descramble_data[15:8] <= scramble_data[7:0];
                                //real
                                descramble_data[7:0] <= ~scramble_data[15:8] + 8'b1;
                            end
                        endcase
                    end
                    else
                    begin
                        data_valid <= 0;
                        state <= IDLE;
                        header_counter <= 1;
                    end
                end
                default:
                begin
                    state <= IDLE;
                    data_valid <= 0;
                end
            endcase
        end
    end
endmodule
