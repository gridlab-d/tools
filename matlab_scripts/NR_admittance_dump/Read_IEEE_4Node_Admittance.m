% Code to read in the admittance file output by the NR solver
% for the sample 4-node test system
%
% Created January 8, 2016 by Frank Tuffner

% Copyright (c) 2016 Battelle Memorial Institute.  The Government
% retains a paid-up nonexclusive, irrevocable worldwide license to
% reproduce, prepare derivative works, perform publicly and display
% publicly by or for the Government, including the right to distribute
% to other Government contractors.

%Clear the workspace
clear all;
close all;
clc;

%Specify the file name
FileName='admittance_dump.txt';

%Call the function
[YMatrix,RowLocations,LocationNames]=fun_GLD_Matrix_Output_Parse(FileName);

%Just do a simple sparse matrix plot
figure();
spy(YMatrix);