%This script generates a three-phase voltage output with the specific magnitudes/ramps
%and frequency/ramps over specified time intervals.
%
%Created March 14, 2018 by Frank Tuffner
%Modified April 28, 2020 by Frank Tuffner

%Prepare workspace
close all;
clear all;
clc;

%Define the base output file name - paths support
BaseFolder = '.';

%Specify the output files - put in cell format
%The script will append "_X.csv" to the end of this, where X is A, B, and C
OutputFile= {
            [BaseFolder '\data_volt_freq_player'];
            };

%Nominal voltage magnitude - make an array - must align with OutputFile
NominalVoltageMag=[
                    7200.0;
                  ];

%Starting frequency angles for A, B, and C (degrees)
StartingVoltageAngles=[0 -120 120];

%Nominal frequency of the system (base phasor frequency)
NominalFrequency=60.0;

%Timestep for the simulation (seconds)
SimulationTimeStep=0.001;

%Starting time - YYYY-MM-DD HH:MM:SS (only to whole seconds supported)
StartTime='2000-01-01 00:00:00';

%TimeZone, if desired (blank for not)
TimeZone='EST';

%Duration of the player file (seconds)
SimDuration=10;

%Output format - 0 for rectangular, 1 for polar
OutputFormat=1;

%Voltages and frequencies over intervals - must be ascending - [startT endT vstart(pu) vend(pu) fstart(pu) fend(pu)]
%Voltages are specified in per-unit
%Frequencies are specified in per-unit
%Times are relative to the start time in seconds
%System starts at nominal voltage and frequency
%Note that gaps in the time will just hold the previous value
%Note that start is inclusive, end is not ( [start end) )

%7-cycle times
VoltageVals=[
                %Voltage magnitude, but constant frequency
                0.0         4.5         1.0         1.0         1.0         1.0;
                4.501       4.521       1.0         0.5         1.0         1.0;
                4.521       4.621       0.5         0.5         1.0         1.0;
                4.621       4.641       0.5         1.0         1.0         1.0;
                
                %Constant magnitude, but varying frequency
                5.0         5.5         1.0         1.0         1.0         1.0;
                5.501       5.521       1.0         1.0         1.0         0.92;
                5.521       5.621       1.0         1.0         0.92        0.92;
                5.621       5.641       1.0         1.0         0.92        1.0;
                5.641       5.800       1.0         1.0         1.0         1.0;
                
                %Frequency and magnitude vary
                8.0         8.5         1.0         1.0         1.0         1.0;
                8.501       8.521       1.0         0.5         1.0         0.92;
                8.521       8.621       0.5         0.5         0.92        0.92;
                8.621       8.641       0.5         1.0         0.92        1.0;
                ];
            
%Condensed output? - If a value is replicated, don't put a unique timestamp in the player file
%If 0, then all timesteps will be output
CondenseOutput=1;

%Digits on the decimal (e.g., 0.4321 has 4) for the outputs
%Can sometimes make a very small difference, if the steps are too big
DigitsOnDecimal=4;

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% Code below here - avoid modifications
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

%% Initial setup items

%Number of files to generate
NumFilesGen=size(OutputFile,1);

%Make sure it matches the voltage specification
NumVoltagesSpec=length(NominalVoltageMag);

if (NumVoltagesSpec ~= NumFilesGen)
    error('OutputFile count and NominalVoltageMag need to have the same number of elements!');
end

%Number of transitions
NumVoltageVals=size(VoltageVals,1);

%Create the offset time vector
OffsetTimeVect=(0:SimulationTimeStep:SimDuration).';

%Length
SimLength=length(OffsetTimeVect);

%Create the angle base values
AngleBaseVals=(cosd(StartingVoltageAngles)+1j*sind(StartingVoltageAngles));

%% Create the full output time vector

%Get the base value
BaseTime=datenum(StartTime,'yyyy-mm-dd HH:MM:SS');

%Now make MATLAB time vector
MATLABTimeVect=BaseTime+OffsetTimeVect/(24*3600);

%Determine if we're a millisecond or greater setup
if (mod(SimulationTimeStep,0.001) ~= 0)
    %Smaller than ms - base is just seconds (to prevent rounding)
    BaseTimeOutputLeft=datestr(MATLABTimeVect,'yyyy-mm-dd HH:MM:SS');
    
    %Get floor value
    TempTimeValueSeconds=floor(OffsetTimeVect);
    
    %Get the partial seconds
    TempTimeValuePartial = OffsetTimeVect - TempTimeValueSeconds;
    
    %Now print them
    BaseTimeOutputRight=num2str(TempTimeValuePartial,'%.6f');
    
    %Combine them
    BaseTimeOutput = [BaseTimeOutputLeft BaseTimeOutputRight(:,2:8)];
else
    %MS or bigger - use standard outputs for printing
    BaseTimeOutput=datestr(MATLABTimeVect,'yyyy-mm-dd HH:MM:SS.FFF');
end

%Append on the timezone, if desired
if (~isempty(TimeZone))
    OutputFinalTime=[BaseTimeOutput repmat([' ' TimeZone],SimLength,1)];
else
    OutputFinalTime=BaseTimeOutput;
end

%% Loop for the file output sets

for kFileVal=1:NumFilesGen

    %Create the starting voltage vectors
    PrevVoltageValue=NominalVoltageMag(kFileVal)*AngleBaseVals;

    %Create an empty voltage output array, just because
    VoltageOutputArray=zeros(SimLength,3);

    %% Create the voltage output values

    %Current writing index
    CurrWriteIndex=1;
    
    %Create the frequency increment -- assume nominal at first
    FreqIncrement=ones(1,3);

    %Loop through and output
    for fVals=1:NumVoltageVals
        %Initial check - make sure it is incrementing
        if (VoltageVals(fVals,2)<=VoltageVals(fVals,1))
            error('Times for VoltageVals must be increasing!');
        end

        %See if we're below the expected range - if so, replicate until we reach it
        while (OffsetTimeVect(CurrWriteIndex) < VoltageVals(fVals,1))
            %Determine what it should be
            NewVoltageValue = PrevVoltageValue.*FreqIncrement;

            %Store it
            VoltageOutputArray(CurrWriteIndex,:) = NewVoltageValue;

            %Book-keep
            CurrWriteIndex = CurrWriteIndex + 1;
            PrevVoltageValue = NewVoltageValue;
        end
        
        %See if we're above the lower range
        if (OffsetTimeVect(CurrWriteIndex) >= VoltageVals(fVals,1))   %In range
            
            %See how far this goes

            %Figure out the stopping index
            TimeIndexVal = find(OffsetTimeVect>=VoltageVals(fVals,2),1);

            %Make sure it worked
            if (isempty(TimeIndexVal)==true)
                %see if the start time is beyond the simulation
                if (VoltageVals(fVals,1) > SimDuration)
                    %Warn that the values may be odd, if it is a ramp
                    if (VoltageVals(fVals,3)~=VoltageVals(fVals,4))
                        %Warn about ramps
                        disp(['Ramp implied by entry row ' num2str(fVals,'%d') ' goes beyond simulation time - output will be skewed']);
                    end

                    %Populate the fill variables
                    WriteToIndex=SimDuration;
                else
                    %Genuine failure, somehow
                    error('Time:%f could not be found in the array - make sure it isn''t a typo!',VoltageVals(fVals,1));
                end
            else %Found a valid time
                %Just pass the index in
                WriteToIndex=TimeIndexVal;
            end

            %Calculate the fill length
            FillLength=WriteToIndex-CurrWriteIndex+1;

            %Populate the voltage vector - use Linspace, just because
            FillMagnitudes=(NominalVoltageMag(kFileVal)*linspace(VoltageVals(fVals,3),VoltageVals(fVals,4),FillLength)).';

            %Populate the frequency vector - again use Linspace, just because
            FillFrequenciesPre=(NominalFrequency*(linspace(VoltageVals(fVals,5),VoltageVals(fVals,6),FillLength)-1.0)).';
            
            %Change this value into an increment
            FreqIncrementArray=exp(1j*(FillFrequenciesPre*2*pi*SimulationTimeStep));
            
            %Loop through this and store (have to do it this way for frequency)
            for wVals=1:FillLength
                
                %Pull the angles of the previous time
                PrevAngles = angle(PrevVoltageValue);
                
                %Make it a polar number
                PrevAnglesBase=cos(PrevAngles)+1j*sin(PrevAngles);
                
                %Pull the frequency increment for "tracking"
                FreqIncrement = repmat(FreqIncrementArray(wVals),1,3);
                
                %Create new value
                NewVoltageValue = repmat(FillMagnitudes(wVals),1,3).*PrevAnglesBase.*FreqIncrement;

                %Store it
                VoltageOutputArray(CurrWriteIndex,:) = NewVoltageValue;

                %Book-keep
                CurrWriteIndex = CurrWriteIndex + 1;
                PrevVoltageValue = NewVoltageValue;
            end
        end
    end
    
    %Check to make sure we finished - if not, just replicate
    while (CurrWriteIndex <= SimLength)
        %Determine what it should be
        NewVoltageValue = PrevVoltageValue.*FreqIncrement;

        %Store it
        VoltageOutputArray(CurrWriteIndex,:) = NewVoltageValue;

        %Book-keep
        CurrWriteIndex = CurrWriteIndex + 1;
        PrevVoltageValue = NewVoltageValue;
    end
    
    %% Write out the files

    %create the filenames
    OutputFileNames=[repmat(OutputFile{kFileVal},3,1) ['_A.csv'; '_B.csv'; '_C.csv']];

    %Open all the file handles
    fHandleA=fopen(OutputFileNames(1,:),'wt');
    fHandleB=fopen(OutputFileNames(2,:),'wt');
    fHandleC=fopen(OutputFileNames(3,:),'wt');

    %Initialize the "previous tracker", in case we use it
    %Will only check phase A, since if it duplicates, they all will (frequency common)
    PrevTrackerValue=-99999;

    %Create the output spec
    if (OutputFormat==0)    %Rectangular
        OutputSpec=['%s,%+0.' num2str(DigitsOnDecimal,'%d') 'f%+0.' num2str(DigitsOnDecimal,'%d') 'fj\n'];
    else %Must be polar, by lack of options
        OutputSpec=['%s,%+0.' num2str(DigitsOnDecimal,'%d') 'f%+0.' num2str(DigitsOnDecimal,'%d') 'fd\n'];
    end

    %Write the output data
    for tVals=1:SimLength

        %See which mode we are
        if (CondenseOutput==1)
            if (PrevTrackerValue == VoltageOutputArray(tVals,1))
                %Just go to next one
                continue;
            else
                %Update the tracker
                PrevTrackerValue = VoltageOutputArray(tVals,1);
            end
        end

        %Determine type to print (rectangular or polar)
        if (OutputFormat==0)
            %Print A
            fprintf(fHandleA,OutputSpec,OutputFinalTime(tVals,:),real(VoltageOutputArray(tVals,1)),imag(VoltageOutputArray(tVals,1)));

            %Print B
            fprintf(fHandleB,OutputSpec,OutputFinalTime(tVals,:),real(VoltageOutputArray(tVals,2)),imag(VoltageOutputArray(tVals,2)));

            %Print C
            fprintf(fHandleC,OutputSpec,OutputFinalTime(tVals,:),real(VoltageOutputArray(tVals,3)),imag(VoltageOutputArray(tVals,3)));
        else
            %Print A
            fprintf(fHandleA,OutputSpec,OutputFinalTime(tVals,:),abs(VoltageOutputArray(tVals,1)),angle(VoltageOutputArray(tVals,1))/pi*180);

            %Print B
            fprintf(fHandleB,OutputSpec,OutputFinalTime(tVals,:),abs(VoltageOutputArray(tVals,2)),angle(VoltageOutputArray(tVals,2))/pi*180);

            %Print C
            fprintf(fHandleC,OutputSpec,OutputFinalTime(tVals,:),abs(VoltageOutputArray(tVals,3)),angle(VoltageOutputArray(tVals,3))/pi*180);
        end
    end

    %Close the file handles
    fclose(fHandleA);
    fclose(fHandleB);
    fclose(fHandleC);
end