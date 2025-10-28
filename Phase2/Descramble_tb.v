`timescale 1ns / 1ps
module Descramble_tb();
    reg clk = 0;
    reg reset;
    reg [15:0]scramble_data[0:1087];
    reg [15:0]descramble_data[0:1023];
    wire data_valid;
    wire [15:0]descramble_out;
    reg [7:0]out;
    reg [15:0]scramble_in;
    reg [3:0]counter = 0;
    reg [10:0] i = 0;
    reg [10:0] j = 0;
    always #10 clk = ~clk; 
    initial
    begin
        $readmemb("C:/Users/babbage/Desktop/university/FPGA/project/phase2_myself/tri_scramble.txt", scramble_data);
        reset = 0;
        #20
        reset = 1;
    end
    always@(posedge clk)
    begin
        if(counter > 10)
        begin
            scramble_in <= scramble_data[i];
            i <= i + 1;
            if(data_valid == 1)
            begin
                descramble_data[j] <= descramble_out;
                out <= descramble_out[7:0];
                j <= j + 1;
            end
        end
        else
        begin
            counter <= counter + 1;
        end
    end
    initial
    begin
        wait(j == 1024);
        $writememb("C:/Users/babbage/Desktop/university/FPGA/project/phase2_myself/tri_descramble.txt", descramble_data);
    end
    Descramble uut(
        .clk(clk),
        .reset(reset),
        .scramble_data(scramble_in),
        .descramble_data(descramble_out),
        .data_valid(data_valid)
    );
endmodule
