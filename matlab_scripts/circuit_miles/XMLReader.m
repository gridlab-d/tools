% Basic XML parser to pull the information into a Matlab structure.
% Reccomend reducing the size of the XML as much as possible -- anything
% more than about 40 MB seems to choke.  I have been cutting the XML down
% to just the powerflow section.
%
% Created by Jason Fuller, PNNL, 04/24/2015
clear;
clc;

dir = 'C:\Users\d3x289\Documents\CSI\Build_04232015\CircuitMiles\';
xml_file = 'test_XML2.xml';

filename = [dir xml_file];

% PARSEXML Convert XML file to a MATLAB structure.
disp('Reading XML file...');
tree = xmlread(filename);
disp('Finished reading XML file...');

% Recurse over child nodes. This could run into problems 
% with very deeply nested trees.
disp('Parsing XML/DOM into Matlab structure...');
theStruct = parseChildNodes(tree);
disp('Finished parsing XML/DOM into Matlab structure.');

save('theStruct.mat','theStruct');
