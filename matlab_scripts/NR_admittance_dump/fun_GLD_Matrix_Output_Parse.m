function [YMatrixGrid,LocationRows,LocationNames]=fun_GLD_Matrix_Output_Parse(filenamein)
% Parses GLD-produced admittance and voltage information for comparison
% [YMatrixGrid,LocationRows,LocationNames]=fun_GLD_Matrix_Output_Parse(filenamein)
% filenamein = full path to the output text file
%
% YMatrixGrid - full representation (not sparse) admittance matrix - in partitioned rectangular format
% LocationRows - start and stop row associated with each bus in YMatrixGrid
% LocationNames - bus names associated with the LocationRows
%
% Functionalized April 25, 2014 by Frank Tuffner
% Updated January 7, 2016 by Frank Tuffner (new output format)

% Copyright (c) 2016 Battelle Memorial Institute.  The Government
% retains a paid-up nonexclusive, irrevocable worldwide license to
% reproduce, prepare derivative works, perform publicly and display
% publicly by or for the Government, including the right to distribute
% to other Government contractors.

%Make sure the input file exists
if (~exist(filenamein,'file'))
    error('GLD Output file does not exist!');
end

%Get file handle
fp = fopen(filenamein);

%Skip the first three lines, they are header-type information
fgetl(fp);
fgetl(fp);
fgetl(fp);

%Read in matrix breakdown stuff
LocationData = textscan(fp,'%d %d %s','Delimiter',',');

LocationRows=[(LocationData{1}+1) (LocationData{2}+1)];
LocationNames=char(LocationData{3});

%Timestamp information is next - skip it, for now

%Get size
DataSizeValue=textscan(fp,'Matrix Information - non-zero element count = %d',1,'HeaderLines',1);

%Round out the line, so the header count makes sense
fgetl(fp);

%Read matrix values
MatrixData = textscan(fp,'%d %d %f',DataSizeValue{1},'Delimiter',',','HeaderLines',1,'TreatAsEmpty','-1.#IND00');

%Get some information
rowind=MatrixData{1};
colind=MatrixData{2};
ydata=MatrixData{3};

%Size is one bigger (zero-ref)
xsze=max(rowind)+1;
ysze=max(colind)+1;

%Ignore the rest for now - may be other timesteps
fclose(fp);

%Make an un-sparse matrix
YMatrixGrid=zeros(xsze,ysze);

%Populate it and make sure nothing duplicates
for kvals=1:length(rowind)
    if (YMatrixGrid((rowind(kvals)+1),(colind(kvals)+1))==0)
        if (ydata(kvals)==0.0)
            YMatrixGrid((rowind(kvals)+1),(colind(kvals)+1))=-99999;
            disp(['Element (' num2str((rowind(kvals)+1)) ',' num2str((colind(kvals)+1)) ') was zero']);
        else
            YMatrixGrid((rowind(kvals)+1),(colind(kvals)+1))=ydata(kvals);
        end
    else
        disp([num2str(rowind(kvals)+1) '-' num2str(colind(kvals)+1) ' - duplicate entry']);
    end
end
