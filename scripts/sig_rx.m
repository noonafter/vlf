clear all, close all
% fid = fopen("D:/project/vlf/build/src/app/rawdata/ch3/20250124/RAW_CH3_192000_RI32_BE_20250124_162930","r","b");
fid = fopen("D:\project\sdp_measure\out\debug\output\user\file\2025\02\17\R0_C1_225124667_10000_MSK.bin","r","l");

data_real = fread(fid,"int32");
data_real_norm = data_real/2^29;

fid0 = fopen("data_ch0_subch133","r","l");
sub_data0 = fread(fid0,"float32");
data_i0 = sub_data0(1:2:end);
data_q0 = sub_data0(2:2:end);
data_cplx0 = data_i0 + 1i*data_q0;

fid1 = fopen("data_ch1_subch133","r","l");
sub_data1 = fread(fid1,"float32");
data_i1 = sub_data1(1:2:end);
data_q1 = sub_data1(2:2:end);
data_cplx1 = data_i1 + 1i*data_q1;

fid2 = fopen("data_subch133","r","l");
sub_data2 = fread(fid2,"float32");
data_i2 = sub_data2(1:2:end);
data_q2 = sub_data2(2:2:end);
data_cplx2 = data_i2 + 1i*data_q2;


fid3 = fopen("energy","r","l");

data_real2 = fread(fid3,"float32");
min(data_real2)
max(data_real2(data_real2<1))
data_delete = data_real2(data_real2<0.004);
% data_real_norm = data_real/2^29;
fclose("all");
% fsa = 192e3;
% fsy = 75;
