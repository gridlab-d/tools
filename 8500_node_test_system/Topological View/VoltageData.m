% Extracts voltage data from voltdump outputs and puts them into
% nice Matlab structures 
% - also extracts the coordinates to create
%   a map in VoltageView.m
% - can extract more than one voltdump to create a movie

clear;
clc;

dir = '';
load('C:\Users\d3x289\Documents\LAAR\8500 visualizations\8500 visualizations\coordinates.mat');

my_case = 2; %1=24 hour, 2=30 min

if (my_case == 2)
    my_limit = 435;
else
    my_limit = 24;
end

for jind=1:my_limit
    
    if (my_case == 2)
        my_file = [dir '8500_volt_' num2str(jind) '.csv'];
    else
        my_file = [dir '8500_schedule_volt_' num2str(jind) '.csv'];
    end
    
    fid = fopen(my_file,'r');
    Header1 = textscan(fid,'%s',8,'Delimiter',',');
    Header2 = textscan(fid,'%s %s %s %s %s %s %s',8,'Delimiter',',');
   
    if (jind == 1)
        temp = textscan(fid,'%s %f %f %f %f %f %f','Delimiter',',');
        data.voltage_names = temp{1};
        data.voltage{jind} = [temp{1,2:7}];
    else
        temp = textscan(fid,'%*s %f %f %f %f %f %f','Delimiter',',');
        data.voltage{jind} = [temp{1,1:6}];
    end
     
    fclose('all'); 
end

clear Header1 Header2 temp
%% Find the nodes in the coordinate system (if they exist)
maxk = length(data.voltage_names);
maxc = length(coordinates.X);  

for kind = 1:maxk
    %TODO: May want to make this a "strfind" - assign coordinates of
    %secondary system to the nearest neighbor
    find_val = strcmpi(coordinates.name,data.voltage_names(kind));
    find_ind = find(find_val == 1);

    if ( length(find_ind) == 1)
        data.voltX(kind) = coordinates.X(find_ind);
        data.voltY(kind) = coordinates.Y(find_ind);
    else
        data.voltX(kind,1) = -999999;
        data.voltY(kind,1) = -999999;
    end
    
    % turn to mag and angle
    vm_a = sqrt(data.voltage{1}(kind,1)^2 + data.voltage{1}(kind,2)^2);
    vm_b = sqrt(data.voltage{1}(kind,3)^2 + data.voltage{1}(kind,4)^2);
    vm_c = sqrt(data.voltage{1}(kind,5)^2 + data.voltage{1}(kind,6)^2);

        ta = complex(data.voltage{1}(kind,1),data.voltage{1}(kind,2));
    va_a = 180 / pi * angle(ta);
        tb = complex(data.voltage{1}(kind,3),data.voltage{1}(kind,4));
    va_b = 180 / pi * angle(tb);
        tc = complex(data.voltage{1}(kind,5),data.voltage{1}(kind,6));
    va_c = 180 / pi * angle(tc);

    % figure out the per unit value
    if ( vm_a ~= 0 )
        if ( vm_a < 600 )
            data.volt_pu(kind,1) = 120;
        elseif ( vm_a < 10000 )
            data.volt_pu(kind,1) = 7200;
        elseif ( vm_a < 90000 )
            data.volt_pu(kind,1) = 115000/sqrt(3);
        else
            data.volt_pu(kind,1) = 115000;
        end
    elseif ( vm_b ~= 0 )
        if ( vm_b < 600 )
            data.volt_pu(kind,1) = 120;
        elseif ( vm_b < 10000 )
            data.volt_pu(kind,1) = 7200;
        elseif ( vm_b < 90000 )
            data.volt_pu(kind,1) = 115000/sqrt(3);
        else
            data.volt_pu(kind,1) = 115000;
        end
    elseif ( vm_c ~= 0 )
        if ( vm_c < 600 )
            data.volt_pu(kind,1) = 120;
        elseif ( vm_c < 10000 )
            data.volt_pu(kind,1) = 7200;
        elseif ( vm_c < 90000 )
            data.volt_pu(kind,1) = 115000/sqrt(3);
        else
            data.volt_pu(kind,1) = 115000;
        end
    else
        disp(['error: all voltages ~= 0 for node ' data.voltage_names(kind)]);
    end

    clear find_ind find_val
end

%% Save stuff
disp('done saving');
save('voltage_data.mat','data');