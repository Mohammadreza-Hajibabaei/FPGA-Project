%% setting the properties
clear
clc
n = 1024;
Fs = n;
T = 1/Fs;
L = n;
t = (0:L-1)*T;
freq = 2;
%% making wave
% square wave
t_s = linspace(0,10,n);
y_square = square(t_s) * 0.9844;

% traingular wave
% Generate a linear ramp from 0 to 1 in N/2 samples
ramp_up = linspace(0, 1, n/2);
% Generate a linear ramp from 1 to 0 in N/2 samples
ramp_down = linspace(1, 0, n/2);
% Combine the ramps to create a single period of the triangular wave
y_tri = [ramp_up ramp_down] * 0.9844;

%sawtooth wave
f = 2;
% Generate a time vector
t_saw = linspace(0, f, n);
% Create the sawtooth wave
y_saw = sawtooth(2 * pi * t_saw) * 0.9844;

% sin wave
y_sin = sin(2*pi*freq*t) * 0.9844;
%% converting wave to fixed point array format
fixpoint_array_sin = zeros([1,n]);
fixpoint_array_square = zeros([1,n]);
fixpoint_array_tri = zeros([1,n]);
fixpoint_array_sawtooth = zeros([1,n]);

fixpoint_array_sin_bin = zeros([n,16]);
fixpoint_array_square_bin = zeros([n,16]);
fixpoint_array_tri_bin = zeros([n,16]);
fixpoint_array_sawtooth_bin = zeros([n,16]);

for i = 1:n
    fixpoint_array_sin_bin(i,9:16)= fixpoint(str2double(fi(y_sin(i),1,8,7).Value));
    fixpoint_array_square_bin(i,9:16)= fixpoint(str2double(fi(y_square(i),1,8,7).Value));
    fixpoint_array_tri_bin(i,9:16)= fixpoint(str2double(fi(y_tri(i),1,8,7).Value));
    fixpoint_array_sawtooth_bin(i,9:16)= fixpoint(str2double(fi(y_saw(i),1,8,7).Value));
    
    fixpoint_array_sin(i) = str2double(fi(y_sin(i),1,8,7).Value);
    fixpoint_array_square(i) = str2double(fi(y_square(i),1,8,7).Value);
    fixpoint_array_tri(i) = str2double(fi(y_tri(i),1,8,7).Value);
    fixpoint_array_sawtooth(i) = str2double(fi(y_saw(i),1,8,7).Value);
end
%% scrambleing data
clc
initial_x = zeros([1,18]);
initial_x(18) = 1;
initial_y = ones([1,18]);
sr_x = initial_x;
sr_y = initial_y;
[sr_x,sr_y] = shift(sr_x,sr_y);

sin_scrambled = zeros([n,16]);
square_scrambled = zeros([n,16]);
sawtooth_scrambled = zeros([n,16]);
tri_scrambled = zeros([n,16]);
R_in = 10;
R = zeros([1,n]);
R_i = 3;
for i = 1:n
    [sr_x,sr_y,R_in] = lfsr(sr_x,sr_y);
    sin_scrambled(i,:) = scramble(fixpoint_array_sin_bin(i,:),R_in);
    square_scrambled(i,:) = scramble(fixpoint_array_square_bin(i,:),R_in);
    sawtooth_scrambled(i,:) = scramble(fixpoint_array_sawtooth_bin(i,:),R_in);
    tri_scrambled(i,:) = scramble(fixpoint_array_tri_bin(i,:),R_in);
    R(i) = R_in;
end
%% adding header
frame_length = 64;
number_of_frames = n / frame_length;
header_length = 4; 
sin_header = zeros([n + header_length * number_of_frames,16]);
square_header = zeros([n + header_length * number_of_frames,16]);
sawtooth_header = zeros([n + header_length * number_of_frames,16]);
tri_header = zeros([n + header_length * number_of_frames,16]);
frame_pattern = ones([1,16]);
k = 1;
for i = 1:68:n + header_length * number_of_frames
    for j = i:i+67
        if(j-i<4)
            sin_header(j,:) = frame_pattern;
            square_header(j,:) = frame_pattern;
            sawtooth_header(j,:) = frame_pattern;
            tri_header(j,:) = frame_pattern;
        else
            sin_header(j,:) = sin_scrambled(k,:);
            square_header(j,:) = square_scrambled(k,:);
            sawtooth_header(j,:) = sawtooth_scrambled(k,:);
            tri_header(j,:) = tri_scrambled(k,:);
            k = k + 1;
        end
    end
end
%% writing scrambled data to files
writeFile('sin_scramble.txt',sin_header);
writeFile('square_scramble.txt',square_header);
writeFile('sawtooth_scramble.txt',sawtooth_header);
writeFile('tri_scramble.txt',tri_header);

%% drawing the signals
sin_descramble = readFile('sin_descramble.txt',L,16);
square_descramble = readFile('square_descramble.txt',L,16);
sawtooth_descramble = readFile('sawtooth_descramble.txt',L,16);
tri_descramble = readFile('tri_descramble.txt',L,16);
sin_descramble_data = zeros([n,1]);
square_descramble_data = zeros([n,1]);
sawtooth_descramble_data = zeros([n,1]);
tri_descramble_data = zeros([n,1]);
for i=1:n
    sin_descramble_data(i) = fp2dec(sin_descramble(i,9:16));
    square_descramble_data(i) = fp2dec(square_descramble(i,9:16));
    sawtooth_descramble_data(i) = fp2dec(sawtooth_descramble(i,9:16));
    tri_descramble_data(i) = fp2dec(tri_descramble(i,9:16));
end
figure
subplot(2,2,1)
plot(sin_descramble_data)
title('descrambled sin wave')
subplot(2,2,2)
plot(square_descramble_data)
title('descrambled square wave')
subplot(2,2,3)
plot(sawtooth_descramble_data)
title('descrambled sawtooth wave')
subplot(2,2,4)
plot(tri_descramble_data)
title('descrambled tri wave')

figure
subplot(2,2,1)
plot(fixpoint_array_sin)
title('main sin wave')
subplot(2,2,2)
plot(fixpoint_array_square)
title('main square wave')
subplot(2,2,3)
plot(fixpoint_array_sawtooth)
title('main sawtooth wave')
subplot(2,2,4)
plot(fixpoint_array_tri)
title('main tri wave')
%%
% using this function for converting decimal number to fixpoint
function out = fixpoint(n)
% Split the number into integer and fractional parts
num = abs(n);
sign = n<0;
integer_part = fix(num);
fractional_part = num - integer_part;

% Convert integer part to binary representation
integer_binary_str = dec2bin(integer_part);

% Convert fractional part to binary representation
fractional_binary_str = ''; % Initialize empty string
precision = 7; % Number of bits for fractional part

for i = 1:precision
    fractional_part = fractional_part * 2;
    bit = fix(fractional_part);
    fractional_binary_str = [fractional_binary_str, num2str(bit)];
    fractional_part = fractional_part - bit;
end

% Convert binary strings to arrays of integers
integer_binary_array = double(integer_binary_str) - 48;
fractional_binary_array = double(fractional_binary_str) - 48;
% out = [sign,integer_binary_array, fractional_binary_array];
out = [];
if (sign > 0)
    out = [sign, reverseBit(fractional_binary_array)];
else
    out = [sign, fractional_binary_array];
end
% out = [sign, fractional_binary_array];
end
% using this function for writing data to file
function writeFile(fileName,data)
    fileID = fopen(fileName,'w');
    for i = 1:size(data,1)
        for j = 1:size(data,2)
            fprintf(fileID,'%d',data(i,j));
        end
        fprintf(fileID,'\n');
    end
    fclose(fileID);
end
% using this function fow reading data from file
function data = readFile(fileName,data_num,num_bit)
    % Open the file for reading
    fid = fopen(fileName, 'r');

    % Check if file opened successfully
    if fid == -1
        error('Could not open the file.');
    end
%     num_bit = 16;
    data = zeros([data_num,num_bit]);
    % Read each line of the file
    line = fgets(fid);
    j = 1;
    while ischar(line)
        for i = 1:num_bit
            data(j,i) = str2double(line(i));
        end
        % Read the next line
        line = fgets(fid);
        j = j + 1;
    end
    fclose(fid);
end
% using this function to converting fixpoint number to decimal
function out = fp2dec(input)
    sign = 0;
    if input(1) == 1
        sign = 1;
    end
    power = 0.5;%%1 befor
    out = 0;
    if(sign == 0)
        for i=2:size(input,2)
            out = out + input(i) * power;
            power = power / 2;
        end
    else
        string = reverseBit(input);
        for i=2:size(string,2)
            out = out + string(i) * power;
            power = power / 2;
        end
        out = -1 * out;
    end
    
end

function out = reverseBit(string)
    out = zeros(size(string));
    first = 0;
    index = 0;
    for i = max(size(string)):-1:1
        if(first>0)
            if(string(i) == 1)
                out(i) = 0;
            else
                out(i) = 1;
            end
        end
        
        if(string(i) == 1)
            first = first + 1;
            if(index == 0)
                index = i;
            end
        end
    end
    if(index > 0)
        out(index) = 1;
    end
end

function [out_x,out_y] = shift(sr_x,sr_y)
    out_x = zeros(size(sr_x));
    out_y = zeros(size(sr_y));
    for i = 1:size(sr_x,2) - 1
        out_x(i + 1) = sr_x(i);
        out_y(i + 1) = sr_y(i);
    end
    out_x(1) = xor(sr_x(11),sr_x(18));
    out_y(1) = xor(xor(sr_y(11),sr_y(8)) ,xor(sr_y(13),sr_y(18)));
end

function [out_x,out_y,R] = lfsr(sr_x,sr_y)
    a = xor(sr_x(3),xor(sr_x(12),sr_x(14)));
    b1 = xor(sr_y(3),sr_y(4));
    b2 = xor(sr_y(5),sr_y(6));
    b3 = xor(sr_y(7),sr_y(8));
    b4 =xor(sr_y(9),sr_y(10));
    b5 = xor(sr_y(12),sr_y(13));
    b = xor(xor(xor(b1,b2),xor(b3,b4)),b5); 
    c = xor(a,b) * 2;
    d = xor(sr_x(18),sr_y(18));
    R = d + c;
    [out_x,out_y] = shift(sr_x,sr_y);
end

function scramble_data = scramble(data,R)
    I = data(9:16);
    Q = data(1:8);
    scramble_data = zeros([1,16]);
    switch R
        case 0
            scramble_data(1:8) = Q;
            scramble_data(9:16) = I;
        case 1
            scramble_data(1:8) = I;
            scramble_data(9:16) = reverseBit(Q);
        case 2
            scramble_data(1:8) = reverseBit(Q);
            scramble_data(9:16) = reverseBit(I);
        case 3
            scramble_data(1:8) = reverseBit(I);
            scramble_data(9:16) = Q;
    end
end


