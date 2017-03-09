% Population script to add houses to a psuedo IEEE 4-node
%
% Requires fun_regionalization.m to be in the same folder
%
% Created May 10, 2014 by Frank Tuffner

% Prepare workspace
close all;
clear all;
clc;

%File name to output (GLM)
FileOutput='C:\Code\testing\Test_GLM_4Node_Test.glm';

%base recorder name
BaseRecorderName='MarketTest';

%Number of houses to populate
NumHouses=500;

%Minimum timestep (seconds)
MinTimeStep=60;

%Recorder interval
RecorderInterval=60;

%Market parameters
MarketPeriod=300;
MarketVerbose=0;
MarketPriceCap=100;
MarketInitialPrice=50;  %Statistics initialization
MarketInitialStdDev=10; %Statistics initialization
MarketCapacityReferenceBid=100;
MarketMaxCapacityReferenceQuantity=0;

%Time Parameters
StartTime='2009-01-01 00:00:00';
EndTime='2009-01-03 00:00:00';
TimeZone='PST+8PDT';

%Weather
WeatherFile='WA-Seattle.tmy2';

%Weather region
% 1 - West Coast - temperate
% 2 - North Central/Northeast - cold/cold
% 3 - Southwest - hot/arid
% 4 - Southeast/Central - hot/cold
% 5 - Southeast coastal - hot/humid
% 6 - Hawaii - sub-tropical (not part of original taxonomy)
WeatherRegion=1;

%Want market controllers in the system?
%0 = none (no market), 1 = on
Want_Controllers=1;

%Powerflow solver
%0 = FBS, 1 = NR
PowerflowSolver=1;

%Pause at exit?
PauseAtExit=1;

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%% Code below here -- Avoid modifications
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

%% Initial parameterization


%% Create parameter space

%Initialize random stream
StreamVal = RandStream.create('mrg32k3a','NumStreams',1);

%Insure it is default
if ( verLessThan('matlab','8.1') )
    RandStream.setDefaultStream(StreamVal); %#ok<SETRS>
else
    RandStream.setGlobalStream(StreamVal);
end

%House phases
TempVar=rand(NumHouses,1);
HousePhaseAssignment=3*ones(NumHouses,1);
HousePhaseAssignment(TempVar<=0.33)=1;
HousePhaseAssignment((TempVar>0.33) & (TempVar<=0.67))=2;

%Get number of houses per phase
NumHousesPerPhase=[sum(HousePhaseAssignment==1) sum(HousePhaseAssignment==2) sum(HousePhaseAssignment==3)];

%Pull region information
regional_data = fun_regionalization(WeatherRegion);

% Create a histogram of what the thermal integrity of the houses should be
% Ceiling function will create a few extra houses in histogram, but its needed
thermal_integrity = ceil(regional_data.thermal_percentages * NumHouses);

total_houses_by_type = sum(thermal_integrity.');

%only allow pool pumps on single family homes
no_pool_pumps = total_houses_by_type(1);

%Extract set point information
cool_sp = zeros(size(regional_data.cooling_setpoint{1}(:,1),1),3);
heat_sp = zeros(size(regional_data.heating_setpoint{1}(:,1),1),3);

for typeind=1:3
    cool_sp(:,typeind) = ceil(regional_data.cooling_setpoint{typeind}(:,1) * total_houses_by_type(typeind));
    heat_sp(:,typeind) = ceil(regional_data.heating_setpoint{typeind}(:,1) * total_houses_by_type(typeind));
end

%Large vs small percentage
TempValues=rand(NumHouses,1);
LargeVersusSmallValues=50*ones(NumHouses,1);
LargeVersusSmallValues(TempValues<=regional_data.percentageSmall)=-50;

%% Parameter - Schedule skew values
skew_value = regional_data.residential_skew_std*randn(NumHouses,1);
skew_value(skew_value < -regional_data.residential_skew_max) = -regional_data.residential_skew_max;
skew_value(skew_value > regional_data.residential_skew_max) = regional_data.residential_skew_max;

wh_skew_value = 3*regional_data.residential_skew_std*randn(NumHouses,1);
wh_skew_value(wh_skew_value < -6*regional_data.residential_skew_max) = -6*regional_data.residential_skew_max;
wh_skew_value(wh_skew_value > 6*regional_data.residential_skew_max) = 6*regional_data.residential_skew_max;

% scale this skew up to weeks
pp_skew_value = 128*regional_data.residential_skew_std*randn(NumHouses,1);
pp_skew_value(pp_skew_value < -128*regional_data.residential_skew_max) = -128*regional_data.residential_skew_max;
pp_skew_value(pp_skew_value > 128*regional_data.residential_skew_max) = 128*regional_data.residential_skew_max;

%% Start creating the output file - headers

%open the file handle
fHandle=fopen(FileOutput,'wt');

%Start the initial matter
fprintf(fHandle,'//IEEE 4-node file - generated %s\n',datestr(now,'yyyy-mm-dd HH:MM:SS'));
fprintf(fHandle,'//Generated with prefix ''%s''\n',BaseRecorderName);
fprintf(fHandle,'//Contained %d houses - [%d %d %d] in Weather Region %d\n',NumHouses,NumHousesPerPhase(1),NumHousesPerPhase(2),NumHousesPerPhase(3),WeatherRegion);
fprintf(fHandle,'//Run with mininum_timestep=%d and recorders set at %d\n',MinTimeStep,RecorderInterval);
if (Want_Controllers==1)
    fprintf(fHandle,'//Market ran with period=%d, price cap=%.2f, initial_price=%.2f, initial_std_dev=%.2f\n',MarketPeriod,MarketPriceCap,MarketInitialPrice,MarketInitialStdDev);
    fprintf(fHandle,'//       and capacity reference bid=%.2f and max capacity reference quantity=%.2f\n',MarketCapacityReferenceBid,MarketMaxCapacityReferenceQuantity);
    fprintf(fHandle,'//Controllers on all devices\n');
else
    fprintf(fHandle,'//No market\n');
end
if (PowerflowSolver==0)
    fprintf(fHandle,'//FBS Powerflow solver\n\n');
else
    fprintf(fHandle,'//NR Powerflow solver\n\n');
end

%Definitions
fprintf(fHandle,'#set suppress_repeat_messages=0\n');
fprintf(fHandle,'#set minimum_timestep=%d\n',MinTimeStep);
fprintf(fHandle,'#set profiler=1\n');
fprintf(fHandle,'#set randomseed=10\n');
if (PauseAtExit==1)
    fprintf(fHandle,'#set pauseatexit=1\n');
end
fprintf(fHandle,'#include "water_and_setpoint_schedule_v5.glm";\n');
fprintf(fHandle,'#include "appliance_schedules.glm";\n\n');
fprintf(fHandle,'clock {\n');
fprintf(fHandle,'\ttimezone %s;\n',TimeZone);
fprintf(fHandle,'\tstarttime ''%s'';\n',StartTime);
fprintf(fHandle,'\tstoptime ''%s'';\n',EndTime);
fprintf(fHandle,'}\n\n');
fprintf(fHandle,'module tape;\n');
fprintf(fHandle,'module climate;\n');
fprintf(fHandle,'module market;\n');
fprintf(fHandle,'module residential {\n');
fprintf(fHandle,'\timplicit_enduses NONE;\n');
fprintf(fHandle,'}\n');
fprintf(fHandle,'module powerflow {\n');
if (PowerflowSolver==0)
	fprintf(fHandle,'\tsolver_method FBS;\n');
else
	fprintf(fHandle,'\tsolver_method NR;\n');
	fprintf(fHandle,'\tNR_iteration_limit 50;\n');
end
fprintf(fHandle,'}\n\n');
fprintf(fHandle,'class auction {\n');
fprintf(fHandle,'\tdouble current_price_mean_24h;\n');
fprintf(fHandle,'\tdouble current_price_stdev_24h;\n');
fprintf(fHandle,'}\n\n');
fprintf(fHandle,'object climate {\n');
fprintf(fHandle,'\tname "WeatherData";\n');
fprintf(fHandle,'\ttmyfile "%s";\n',WeatherFile);
fprintf(fHandle,'\tinterpolate QUADRATIC;\n');
fprintf(fHandle,'}\n\n');
fprintf(fHandle,'#include "Test_Feeder_Line_Configurations.glm";\n\n');

%% Auction object
if (Want_Controllers==1)
    fprintf(fHandle,'object auction {\n');
    fprintf(fHandle,'\tname Market_1;\n');
    fprintf(fHandle,'\tperiod %d;\n',MarketPeriod);
    fprintf(fHandle,'\tunit kW;\n');
    if (MarketVerbose==1)
        fprintf(fHandle,'\tverbose TRUE;\n');
    end
    fprintf(fHandle,'\tspecial_mode NONE;\n');
    fprintf(fHandle,'\tprice_cap %f;\n',MarketPriceCap);
    fprintf(fHandle,'\tinit_price %f;\n',MarketInitialPrice);
    fprintf(fHandle,'\tinit_stdev %f;\n',MarketInitialStdDev);
    fprintf(fHandle,'\twarmup 0;\n');
    fprintf(fHandle,'\tcapacity_reference_object substation_transformer;\n');
    fprintf(fHandle,'\tcapacity_reference_property power_out_real;\n');
    fprintf(fHandle,'\tcapacity_reference_bid_price %f;\n',MarketCapacityReferenceBid);
    fprintf(fHandle,'\tmax_capacity_reference_bid_quantity %f;\n',MarketMaxCapacityReferenceQuantity);
    fprintf(fHandle,'\tcurve_log_file "%s_market_bids.csv";\n',BaseRecorderName);
    fprintf(fHandle,'\tcurve_log_info EXTRA;\n');
    fprintf(fHandle,'\ttransaction_log_file "%s_log.txt";\n',BaseRecorderName);
    fprintf(fHandle,'\tobject recorder {\n');
    fprintf(fHandle,'\t\tproperty "current_market.clearing_price,current_market.clearing_quantity,fixed_price,fixed_quantity";\n');
    fprintf(fHandle,'\t\tinterval %d;\n',RecorderInterval);
    fprintf(fHandle,'\t\tfile "%s_marketvalues.csv";\n',BaseRecorderName);
    fprintf(fHandle,'\t};\n');
    fprintf(fHandle,'}\n\n');
end

%% Main part of powerflow (4-node system with triplex)
fprintf(fHandle,'///////////////////////////////////////////\n');
fprintf(fHandle,'// BEGIN: IEEE-4 Feeder - main part\n');
fprintf(fHandle,'///////////////////////////////////////////\n\n');
fprintf(fHandle,'object node {\n');
fprintf(fHandle,'\tname node1;\n');
fprintf(fHandle,'\tphases "ABCN";\n');
fprintf(fHandle,'\tbustype SWING;\n');
fprintf(fHandle,'\tnominal_voltage 7200;\n');
fprintf(fHandle,'}\n\n');
fprintf(fHandle,'object overhead_line {\n');
fprintf(fHandle,'\tname ohl12;\n');
fprintf(fHandle,'\tphases "ABCN";\n');
fprintf(fHandle,'\tfrom node1;\n');
fprintf(fHandle,'\tto node2;\n');
fprintf(fHandle,'\tlength 2000;\n');
fprintf(fHandle,'\tconfiguration lc300;\n');
fprintf(fHandle,'}\n\n');
fprintf(fHandle,'object node {\n');
fprintf(fHandle,'\tname node2;\n');
fprintf(fHandle,'\tphases "ABCN";\n');
fprintf(fHandle,'\tnominal_voltage 7200;\n');
fprintf(fHandle,'}\n\n');
fprintf(fHandle,'object transformer {\n');
fprintf(fHandle,'\tname substation_transformer;\n');
fprintf(fHandle,'\tphases "ABCN";\n');
fprintf(fHandle,'\tfrom node2;\n');
fprintf(fHandle,'\tto node3;\n');
fprintf(fHandle,'\tconfiguration tc400;\n');
fprintf(fHandle,'}\n\n');
fprintf(fHandle,'object node {\n');
fprintf(fHandle,'\tname node3;\n');
fprintf(fHandle,'\tphases "ABCN";\n');
fprintf(fHandle,'\tnominal_voltage 2400;\n');
fprintf(fHandle,'}\n\n');
fprintf(fHandle,'object overhead_line {\n');
fprintf(fHandle,'\tname ohl34;\n');
fprintf(fHandle,'\tphases "ABCN";\n');
fprintf(fHandle,'\tfrom node3;\n');
fprintf(fHandle,'\tto node4;\n');
fprintf(fHandle,'\tlength 2500;\n');
fprintf(fHandle,'\tconfiguration lc300;\n');
fprintf(fHandle,'}\n\n');
fprintf(fHandle,'object node {\n');
fprintf(fHandle,'\tname node4;\n');
fprintf(fHandle,'\tphases "ABCN";\n');
fprintf(fHandle,'\tnominal_voltage 2400;\n');
fprintf(fHandle,'}\n\n');

%% Load object
fprintf(fHandle,'//Load object - for gen controller objects\n');
fprintf(fHandle,'object load {\n');
fprintf(fHandle,'\tname gen_control_connect;\n');
fprintf(fHandle,'\tparent node1;\n');
fprintf(fHandle,'\tnominal_voltage 7200.0;\n');
fprintf(fHandle,'\tphases ABCN;\n');
fprintf(fHandle,'\tobject recorder {\n');
fprintf(fHandle,'\t\tproperty "constant_power_A,constant_power_B,constant_power_C";\n');
fprintf(fHandle,'\t\tinterval %d;\n',RecorderInterval);
fprintf(fHandle,'\t\tfile "%s_generator_controller.csv";\n',BaseRecorderName);
fprintf(fHandle,'\t};\n');
fprintf(fHandle,'}\n\n');

%% Triplex portions
fprintf(fHandle,'//////////////////////////////////////////////\n');
fprintf(fHandle,'// BEGIN :Transformer and triplex_nodes\n');
fprintf(fHandle,'//////////////////////////////////////////////\n\n');
fprintf(fHandle,'//Triplex Transformers\n\n');
fprintf(fHandle,'object transformer {\n');
fprintf(fHandle,'\tname center_tap_1;\n');
fprintf(fHandle,'\tphases AS;\n');
fprintf(fHandle,'\tfrom node4;\n');
fprintf(fHandle,'\tto trip_node_AS;\n');
fprintf(fHandle,'\tconfiguration AS_config;\n');
fprintf(fHandle,'}\n\n');
fprintf(fHandle,'object transformer {\n');
fprintf(fHandle,'\tname center_tap_2;\n');
fprintf(fHandle,'\tphases BS;\n');
fprintf(fHandle,'\tfrom node4;\n');
fprintf(fHandle,'\tto trip_node_BS;\n');
fprintf(fHandle,'\tconfiguration BS_config;\n');
fprintf(fHandle,'}\n\n');
fprintf(fHandle,'object transformer {\n');
fprintf(fHandle,'\tname center_tap_3;\n');
fprintf(fHandle,'\tphases CS;\n');
fprintf(fHandle,'\tfrom node4;\n');
fprintf(fHandle,'\tto trip_node_CS;\n');
fprintf(fHandle,'\tconfiguration CS_config;\n');
fprintf(fHandle,'}\n\n');
fprintf(fHandle,'//Triplex nodes\n\n');
fprintf(fHandle,'object triplex_node {\n');
fprintf(fHandle,'\tname trip_node_AS;\n');
fprintf(fHandle,'\tphases AS;\n');
fprintf(fHandle,'\tnominal_voltage 120;\n');
fprintf(fHandle,'}\n\n');
fprintf(fHandle,'object triplex_node {\n');
fprintf(fHandle,'\tname trip_node_BS;\n');
fprintf(fHandle,'\tphases BS;\n');
fprintf(fHandle,'\tnominal_voltage 120;\n');
fprintf(fHandle,'}\n\n');
fprintf(fHandle,'object triplex_node {\n');
fprintf(fHandle,'\tname trip_node_CS;\n');
fprintf(fHandle,'\tphases CS;\n');
fprintf(fHandle,'\tnominal_voltage 120;\n');
fprintf(fHandle,'}\n\n');
fprintf(fHandle,'//Triplex meters\n\n');
fprintf(fHandle,'object triplex_meter {\n');
fprintf(fHandle,'\tname trip_meter_AS;\n');
fprintf(fHandle,'\tphases AS;\n');
fprintf(fHandle,'\tnominal_voltage 120;\n');
fprintf(fHandle,'}\n\n');
fprintf(fHandle,'object triplex_meter {\n');
fprintf(fHandle,'\tname trip_meter_BS;\n');
fprintf(fHandle,'\tphases BS;\n');
fprintf(fHandle,'\tnominal_voltage 120;\n');
fprintf(fHandle,'}\n\n');
fprintf(fHandle,'object triplex_meter {\n');
fprintf(fHandle,'\tname trip_meter_CS;\n');
fprintf(fHandle,'\tphases CS;\n');
fprintf(fHandle,'\tnominal_voltage 120;\n');
fprintf(fHandle,'}\n\n');
fprintf(fHandle,'//Triplex lines\n\n');
fprintf(fHandle,'object triplex_line {\n');
fprintf(fHandle,'\tname trip_line_AS;\n');
fprintf(fHandle,'\tphases AS;\n');
fprintf(fHandle,'\tfrom trip_node_AS;\n');
fprintf(fHandle,'\tto trip_meter_AS;\n');
fprintf(fHandle,'\tlength 10;\n');
fprintf(fHandle,'\tconfiguration triplex_line_configuration_1;\n');
fprintf(fHandle,'}\n\n');
fprintf(fHandle,'object triplex_line {\n');
fprintf(fHandle,'\tname trip_line_BS;\n');
fprintf(fHandle,'\tphases BS;\n');
fprintf(fHandle,'\tfrom trip_node_BS;\n');
fprintf(fHandle,'\tto trip_meter_BS;\n');
fprintf(fHandle,'\tlength 10;\n');
fprintf(fHandle,'\tconfiguration triplex_line_configuration_1;\n');
fprintf(fHandle,'}\n\n');
fprintf(fHandle,'object triplex_line {\n');
fprintf(fHandle,'\tname trip_line_CS;\n');
fprintf(fHandle,'\tphases CS;\n');
fprintf(fHandle,'\tfrom trip_node_CS;\n');
fprintf(fHandle,'\tto trip_meter_CS;\n');
fprintf(fHandle,'\tlength 10;\n');
fprintf(fHandle,'\tconfiguration triplex_line_configuration_1;\n');
fprintf(fHandle,'}\n\n');
fprintf(fHandle,'//////////////////////////////////////////////\n');
fprintf(fHandle,'// END: Pure Powerflow Portions\n');
fprintf(fHandle,'//////////////////////////////////////////////\n\n');

%% Print generator controllers

fprintf(fHandle,'/////////////////////////////////////////////\n');
fprintf(fHandle,'//Generator_controller objects\n');
fprintf(fHandle,'/////////////////////////////////////////////\n');

%Determine how many generator controllers there are
NumGenControllers=size(regional_data.GenControlInformation,1);

if (~isempty(NumGenControllers))
	if (NumGenControllers>1)
		for gVals=1:NumGenControllers

			%Print the generator controllers
			fprintf(fHandle,'object generator_controller {\n');
			fprintf(fHandle,'\tparent gen_control_connect;\n');
			fprintf(fHandle,'\tname generator_controller_%d;\n',gVals);
			fprintf(fHandle,'\tmarket Market_1;\n');
			fprintf(fHandle,'\tgenerator_rating %.2f;\n',regional_data.GenControlInformation{gVals}{1});
			fprintf(fHandle,'\tgenerator_state %s;\n',regional_data.GenControlInformation{gVals}{2});
			fprintf(fHandle,'\tbid_curve "%s";\n',regional_data.GenControlInformation{gVals}{3});
			fprintf(fHandle,'\tbid_delay 1;\n');
			fprintf(fHandle,'\tstartup_cost %.2f;\n',regional_data.GenControlInformation{gVals}{4});
			fprintf(fHandle,'\tshutdown_cost %.2f;\n',regional_data.GenControlInformation{gVals}{5});
			fprintf(fHandle,'\tminimum_runtime %.2f min;\n',regional_data.GenControlInformation{gVals}{6});
			fprintf(fHandle,'\tminimum_downtime %.2f min;\n',regional_data.GenControlInformation{gVals}{7});
			fprintf(fHandle,'\tamortization_factor %.2f 1/h;\n',regional_data.GenControlInformation{gVals}{8});
			fprintf(fHandle,'\tinput_unit_base 1000 kW;\n');
			fprintf(fHandle,'\tobject recorder {\n');
			fprintf(fHandle,'\t\tproperty "generator_output,generator_state,capacity_factor";\n');
			fprintf(fHandle,'\t\tinterval %d;\n',RecorderInterval);
			fprintf(fHandle,'\t\tfile "%s_gen_controller_%d.csv";\n',BaseRecorderName,gVals);
			fprintf(fHandle,'\t};\n');
			fprintf(fHandle,'}\n');
		end
	end
end
fprintf(fHandle,'\n');

%% Print houses

%Common variables
building_type = {'Single Family';'Apartment';'Mobile Home'};

%Slider settings
% limit slider randomization to Olypen style
slider_random = 0.45 + (0.2).*randn(NumHouses,1);
    sl1 = slider_random > regional_data.market_info;
slider_random(sl1) = regional_data.market_info;
    sl2 = slider_random < 0;
slider_random(sl2) = 0;

fprintf(fHandle,'/////////////////////////////////////////////\n');
fprintf(fHandle,'//Houses\n');
fprintf(fHandle,'/////////////////////////////////////////////\n');

%Loop through the houses and output them
for hVals=1:NumHouses

    %Print first parts of house values
    fprintf(fHandle,'object house {\n');
	fprintf(fHandle,'\tname house_%d;\n',hVals);
    if (HousePhaseAssignment(hVals)==1)
        fprintf(fHandle,'\tparent trip_meter_AS;\n');
    elseif (HousePhaseAssignment(hVals)==2)
        fprintf(fHandle,'\tparent trip_meter_BS;\n');
    else
        fprintf(fHandle,'\tparent trip_meter_CS;\n');
    end
    fprintf(fHandle,'\tgroupid Residential;\n');
	fprintf(fHandle,'\tschedule_skew %.0f;\n',skew_value(hVals));

    %Determine thermal integrity and floor area properties
        % Stolen from feeder_generator.m
        % Choose what type of building we are going to use
        % and set the thermal integrity of said building
        [size_a,size_b] = size(thermal_integrity);

        therm_int = ceil(size_a * size_b * rand(1));

        row_ti = mod(therm_int,size_a) + 1;
        col_ti = mod(therm_int,size_b) + 1;
        while ( thermal_integrity(row_ti,col_ti) < 1 )
            therm_int = ceil(size_a * size_b * rand(1));

            row_ti = mod(therm_int,size_a) + 1;
            col_ti = mod(therm_int,size_b) + 1;
        end

        thermal_integrity(row_ti,col_ti) = thermal_integrity(row_ti,col_ti) - 1;

        thermal_temp = regional_data.thermal_properties(row_ti,col_ti);

        %TODO check this variance on the floor area
        % As it is now, this will shift mean of low integrity
        % single family homes to a smaller square footage and vice
        % versa for high integrity homes-is NOT mathematically correct
        f_area = regional_data.floor_area(row_ti);
        story_rand = rand(1);
        height_rand = randi(2);
        fa_rand = rand(1);
        if (col_ti == 1) % SF homes
            floor_area = f_area + (f_area/2) * fa_rand * (row_ti - 4)/3;
            if (story_rand < regional_data.one_story(WeatherRegion))
                stories = 1;
            else
                stories = 2;
            end
        else
            floor_area = f_area + (f_area/2) * (0.5 - fa_rand); %+/- 50%
            stories = 1;
            height_rand = 0;
        end

        % Now also adjust square footage as a factor of whether
        % the load modifier (avg_house) rounded up or down
        floor_area = (1 + LargeVersusSmallValues(hVals)) * floor_area;

        if (floor_area > 4000)
            floor_area = 3800 + fa_rand*200;
        elseif (floor_area < 300)
            floor_area = 300 + fa_rand*100;
        end

        fprintf(fHandle,'\tfloor_area %.0f;\n',floor_area);
        fprintf(fHandle,'\tnumber_of_stories %.0f;\n',stories);
        ceiling_height = 8 + height_rand;
        fprintf(fHandle,'\tceiling_height %.0f;\n',ceiling_height);
        os_rand = regional_data.over_sizing_factor * (.8 + 0.4*rand);
        fprintf(fHandle,'\tover_sizing_factor %.1f;\n',os_rand);

        %TODO do I want to handle apartment walls differently?
        fprintf(fHandle,'\t//Thermal integrity -> %s %.0f\n',building_type{row_ti},col_ti);

        rroof = thermal_temp{1}(1)*(0.8 + 0.4*rand(1));
        fprintf(fHandle,'\tRroof %.2f;\n',rroof);

        rwall = thermal_temp{1}(2)*(0.8 + 0.4*rand(1));
        fprintf(fHandle,'\tRwall %.2f;\n',rwall);

        rfloor = thermal_temp{1}(3)*(0.8 + 0.4*rand(1));
        fprintf(fHandle,'\tRfloor %.2f;\n',rfloor);
        fprintf(fHandle,'\tglazing_layers %.0f;\n',thermal_temp{1}(4));
        fprintf(fHandle,'\tglass_type %.0f;\n',thermal_temp{1}(5));
        fprintf(fHandle,'\tglazing_treatment %.0f;\n',thermal_temp{1}(6));
        fprintf(fHandle,'\twindow_frame %.0f;\n',thermal_temp{1}(7));

        rdoor = thermal_temp{1}(8)*(0.8 + 0.4*rand(1));
        fprintf(fHandle,'\tRdoors %.2f;\n',rdoor);
        fprintf(fHandle,'\tRwindows 1.81;\n');

        airchange = thermal_temp{1}(9)*(0.8 + 0.4*rand(1));
        fprintf(fHandle,'\tairchange_per_hour %.2f;\n',airchange);

        c_COP = thermal_temp{1}(11) + rand(1)*(thermal_temp{1}(10) - thermal_temp{1}(11));
        fprintf(fHandle,'\tcooling_COP %.1f;\n',c_COP);

        init_temp = 68 + 4*rand(1);
        fprintf(fHandle,'\tair_temperature %.2f;\n',init_temp);
        fprintf(fHandle,'\tmass_temperature %.2f;\n',init_temp);

        % This is a bit of a guess from Rob's estimates
        mass_floor = 2.5 + 1.5*rand(1);
        fprintf(fHandle,'\ttotal_thermal_mass_per_floor_area %.3f;\n',mass_floor);

    %Heating type information
        heat_type = rand(1);
        cool_type = rand(1);
        h_COP = c_COP;
        ct = 'NONE';

        if (heat_type <= regional_data.perc_gas)
            fprintf(fHandle,'\theating_system_type GAS;\n');
            if (cool_type <= regional_data.perc_AC)
                fprintf(fHandle,'\tcooling_system_type ELECTRIC;\n');
                ct = 'ELEC';
            else
                fprintf(fHandle,'\tcooling_system_type NONE;\n');
            end
            ht = 'GAS';
        elseif (heat_type <= (regional_data.perc_gas + regional_data.perc_pump))
            fprintf(fHandle,'\theating_system_type HEAT_PUMP;\n');
            fprintf(fHandle,'\theating_COP %.1f;\n',h_COP);
            fprintf(fHandle,'\tcooling_system_type ELECTRIC;\n');
            fprintf(fHandle,'\tauxiliary_strategy DEADBAND;\n');
            fprintf(fHandle,'\tauxiliary_system_type ELECTRIC;\n');
            fprintf(fHandle,'\tmotor_model BASIC;\n');
            fprintf(fHandle,'\tmotor_efficiency AVERAGE;\n');
            ht = 'HP';
            ct = 'ELEC';
%         elseif (floor_area*ceiling_height > 12000 ) % No resistive homes over with large volumes
%             fprintf(fHandle,'\theating_system_type GAS;\n');
%             if (cool_type <= regional_data.perc_AC)
%                 fprintf(fHandle,'\tcooling_system_type ELECTRIC;\n');
%                 ct = 'ELEC';
%             else
%                 fprintf(fHandle,'\tcooling_system_type NONE;\n');
%             end
%             ht = 'GAS';
        else
            fprintf(fHandle,'\theating_system_type RESISTANCE;\n');
            if (cool_type <= regional_data.perc_AC)
                fprintf(fHandle,'\tcooling_system_type ELECTRIC;\n');
                fprintf(fHandle,'\tmotor_model BASIC;\n');
                fprintf(fHandle,'\tmotor_efficiency GOOD;\n');
                ct = 'ELEC';
            else
                fprintf(fHandle,'\tcooling_system_type NONE;\n');
            end
            ht = 'ELEC';
        end

    %Breaker values
        fprintf(fHandle,'\tbreaker_amps 1000;\n');
        fprintf(fHandle,'\thvac_breaker_rating 1000;\n');

    %Choose the heating and cooling schedule
        cooling_set = ceil(regional_data.no_cool_sch*rand(1));
        heating_set = ceil(regional_data.no_heat_sch*rand(1));

        % choose a cooling bin
        coolsp = regional_data.cooling_setpoint{row_ti};
        [no_cool_bins,junk] = size(coolsp);

        % see if we have that bin left
        cool_bin = randi(no_cool_bins);
        while (cool_sp(cool_bin,row_ti) < 1)
            cool_bin = randi(no_cool_bins);
        end
        cool_sp(cool_bin,row_ti) = cool_sp(cool_bin,row_ti) - 1;

        % choose a heating bin
        heatsp = regional_data.heating_setpoint{row_ti};
        [no_heat_bins,~] = size(heatsp);

        heat_bin = randi(no_heat_bins);
        heat_count = 1;

        % see if we have that bin left, then check to make sure
        % upper limit of chosen bin is not greater than lower limit
        % of cooling bin
        while (heat_sp(heat_bin,row_ti) < 1 || (heatsp(heat_bin,3) >= coolsp(cool_bin,4)))
            heat_bin = randi(no_heat_bins);

            % if we tried a few times, give up and take an extra
            % draw from the lowest bin
            if (heat_count > 20)
                heat_bin = 1;
                break;
            end

            heat_count = heat_count + 1;
        end
        heat_sp(heat_bin,row_ti) = heat_sp(heat_bin,row_ti) - 1;

        % randomly choose within the bin, then +/- one
        % degree to seperate the deadbands
        cool_night = (coolsp(cool_bin,3) - coolsp(cool_bin,4))*rand(1) + coolsp(cool_bin,4) + 1;
        heat_night = (heatsp(heat_bin,3) - heatsp(heat_bin,4))*rand(1) + heatsp(heat_bin,4) - 1;

        cool_night_diff = coolsp(cool_bin,2) * 2 * rand(1);
        heat_night_diff = heatsp(heat_bin,2) * 2 * rand(1);

	%Remaining house properties from sample Andy file
	fprintf(fHandle,'\thvac_power_factor 0.97;\n');
	fprintf(fHandle,'\tfan_type ONE_SPEED;\n');
	fprintf(fHandle,'\tnumber_of_doors 2;\n');

    %Controller writing
    if (Want_Controllers==1)

        fprintf(fHandle,'\tthermostat_deadband 0.001;\n');
        fprintf(fHandle,'\tdlc_offset 100;\n');


		% pull in the slider response level
		slider = slider_random(hVals);

		% set the pre-cool / pre-heat range to really small
		% to get rid of it.
		s_tstat = 2;
		hrh = -5+5*(1-slider);
		crh = 5-5*(1-slider);
		hrl = -0.005+0*(1-slider);
		crl = -0.005+0*(1-slider);

		hrh2 = -s_tstat - (1 - slider) * (3 - s_tstat);
		crh2 = s_tstat + (1 - slider) * (3 - s_tstat);
		hrl2 = -s_tstat - (1 - slider) * (3 - s_tstat);
		crl2 = s_tstat + (1 - slider) * (3 - s_tstat);

        if (strcmp(ht,'HP') ~= 0) % Control both therm setpoints
			fprintf(fHandle,'\tcooling_setpoint %.2f;\n',cool_night);
			fprintf(fHandle,'\theating_setpoint %.2f;\n',cool_night-3);
			fprintf(fHandle,'\tobject controller {\n');
			fprintf(fHandle,'\t\tschedule_skew %.0f;\n',skew_value(hVals));
			fprintf(fHandle,'\t\tbid_delay 1;\n');
			fprintf(fHandle,'\t\tname controller_%d;\n',hVals);
			fprintf(fHandle,'\t\tmarket Market_1;\n');
			fprintf(fHandle,'\t\tuse_override ON;\n');
			fprintf(fHandle,'\t\toverride override;\n');
			fprintf(fHandle,'\t\tbid_mode ON;\n');
			fprintf(fHandle,'\t\tcontrol_mode DOUBLE_RAMP;\n');
			fprintf(fHandle,'\t\tresolve_mode DEADBAND;\n');
			fprintf(fHandle,'\t\theating_range_high %.3f;\n',hrh);
			fprintf(fHandle,'\t\tcooling_range_high %.3f;\n',crh);
			fprintf(fHandle,'\t\theating_range_low %.3f;\n',hrl);
			fprintf(fHandle,'\t\tcooling_range_low %.3f;\n',crl);
			fprintf(fHandle,'\t\theating_ramp_high %.3f;\n',hrh2);
			fprintf(fHandle,'\t\tcooling_ramp_high %.3f;\n',crh2);
			fprintf(fHandle,'\t\theating_ramp_low %.3f;\n',hrl2);
			fprintf(fHandle,'\t\tcooling_ramp_low %.3f;\n',crl2);
			fprintf(fHandle,'\t\tcooling_base_setpoint cooling%d*%.2f+%.2f;\n',cooling_set,cool_night_diff,cool_night);
			fprintf(fHandle,'\t\theating_base_setpoint heating%d*%.2f+%.2f;\n',heating_set,heat_night_diff,heat_night);
			fprintf(fHandle,'\t\tperiod %d;\n',MarketPeriod);
			fprintf(fHandle,'\t\taverage_target current_price_mean_24h;\n');
			fprintf(fHandle,'\t\tstandard_deviation_target current_price_stdev_24h;\n');
			fprintf(fHandle,'\t\ttarget air_temperature;\n');
			fprintf(fHandle,'\t\theating_setpoint heating_setpoint;\n');
			fprintf(fHandle,'\t\theating_demand last_heating_load;\n');
			fprintf(fHandle,'\t\tcooling_setpoint cooling_setpoint;\n');
			fprintf(fHandle,'\t\tcooling_demand last_cooling_load;\n');
			fprintf(fHandle,'\t\tdeadband thermostat_deadband;\n');
			fprintf(fHandle,'\t\ttotal hvac_load;\n');
			fprintf(fHandle,'\t\tload hvac_load;\n');
			fprintf(fHandle,'\t\tstate power_state;\n');
			fprintf(fHandle,'\t};\n');
        elseif (strcmp(ht,'ELEC') ~= 0) % Control the heat, but check to see if we have AC
            if (strcmp(ct,'ELEC') ~= 0) % get to control just like a heat pump
				fprintf(fHandle,'\tcooling_setpoint %.2f;\n',cool_night);
				fprintf(fHandle,'\theating_setpoint %.2f;\n',cool_night-3);
				fprintf(fHandle,'\tobject controller {\n');
				fprintf(fHandle,'\t\tschedule_skew %.0f;\n',skew_value(hVals));
				fprintf(fHandle,'\t\tbid_delay 1;\n');
				fprintf(fHandle,'\t\tname controller_%d;\n',hVals);
				fprintf(fHandle,'\t\tmarket Market_1;\n');
				fprintf(fHandle,'\t\tuse_override ON;\n');
				fprintf(fHandle,'\t\toverride override;\n');
				fprintf(fHandle,'\t\tbid_mode ON;\n');
				fprintf(fHandle,'\t\tcontrol_mode DOUBLE_RAMP;\n');
				fprintf(fHandle,'\t\tresolve_mode DEADBAND;\n');
				fprintf(fHandle,'\t\theating_range_high %.3f;\n',hrh);
				fprintf(fHandle,'\t\tcooling_range_high %.3f;\n',crh);
				fprintf(fHandle,'\t\theating_range_low %.3f;\n',hrl);
				fprintf(fHandle,'\t\tcooling_range_low %.3f;\n',crl);
				fprintf(fHandle,'\t\theating_ramp_high %.3f;\n',hrh2);
				fprintf(fHandle,'\t\tcooling_ramp_high %.3f;\n',crh2);
				fprintf(fHandle,'\t\theating_ramp_low %.3f;\n',hrl2);
				fprintf(fHandle,'\t\tcooling_ramp_low %.3f;\n',crl2);
				fprintf(fHandle,'\t\tcooling_base_setpoint cooling%d*%.2f+%.2f;\n',cooling_set,cool_night_diff,cool_night);
				fprintf(fHandle,'\t\theating_base_setpoint heating%d*%.2f+%.2f;\n',heating_set,heat_night_diff,heat_night);
				fprintf(fHandle,'\t\tperiod %d;\n',MarketPeriod);
				fprintf(fHandle,'\t\taverage_target current_price_mean_24h;\n');
				fprintf(fHandle,'\t\tstandard_deviation_target current_price_stdev_24h;\n');
				fprintf(fHandle,'\t\ttarget air_temperature;\n');
				fprintf(fHandle,'\t\theating_setpoint heating_setpoint;\n');
				fprintf(fHandle,'\t\theating_demand last_heating_load;\n');
				fprintf(fHandle,'\t\tcooling_setpoint cooling_setpoint;\n');
				fprintf(fHandle,'\t\tcooling_demand last_cooling_load;\n');
				fprintf(fHandle,'\t\tdeadband thermostat_deadband;\n');
				fprintf(fHandle,'\t\ttotal hvac_load;\n');
				fprintf(fHandle,'\t\tload hvac_load;\n');
				fprintf(fHandle,'\t\tstate power_state;\n');
				fprintf(fHandle,'\t};\n');
            else % control only the heat
				fprintf(fHandle,'\tcooling_setpoint %.2f;\n',cool_night);
				fprintf(fHandle,'\theating_setpoint %.2f;\n',cool_night-3);
				fprintf(fHandle,'\tobject controller {\n');
				fprintf(fHandle,'\t\tschedule_skew %.0f;\n',skew_value(hVals));
				fprintf(fHandle,'\t\tbid_delay 1;\n');
				fprintf(fHandle,'\t\tname controller_%d;\n',hVals);
				fprintf(fHandle,'\t\tmarket Market_1;\n');
				fprintf(fHandle,'\t\tuse_override ON;\n');
				fprintf(fHandle,'\t\toverride override;\n');
				fprintf(fHandle,'\t\tbid_mode ON;\n');
				fprintf(fHandle,'\t\tcontrol_mode RAMP;\n');
				fprintf(fHandle,'\t\trange_high %.3f;\n',hrh);
				fprintf(fHandle,'\t\trange_low %.3f;\n',hrl);
				fprintf(fHandle,'\t\tramp_high %.3f;\n',hrh2);
				fprintf(fHandle,'\t\tramp_low %.3f;\n',hrl2);
				fprintf(fHandle,'\t\tbase_setpoint heating%d*%.2f+%.2f;\n',heating_set,heat_night_diff,heat_night);
				fprintf(fHandle,'\t\tperiod %d;\n',MarketPeriod);
				fprintf(fHandle,'\t\taverage_target current_price_mean_24h;\n');
				fprintf(fHandle,'\t\tstandard_deviation_target current_price_stdev_24h;\n');
				fprintf(fHandle,'\t\ttarget air_temperature;\n');
				fprintf(fHandle,'\t\tsetpoint heating_setpoint;\n');
				fprintf(fHandle,'\t\tdemand last_heating_load;\n');
				fprintf(fHandle,'\t\tdeadband thermostat_deadband;\n');
				fprintf(fHandle,'\t\ttotal hvac_load;\n');
				fprintf(fHandle,'\t\tload hvac_load;\n');
				fprintf(fHandle,'\t\tstate power_state;\n');
				fprintf(fHandle,'\t};\n');
            end
        elseif (strcmp(ct,'ELEC') ~= 0) % gas heat, but control the AC
			fprintf(fHandle,'\theating_setpoint heating%d*%.2f+%.2f;\n',heating_set,heat_night_diff,heat_night);
			fprintf(fHandle,'\tobject controller {\n');
			fprintf(fHandle,'\t\tschedule_skew %.0f;\n',skew_value(hVals));
			fprintf(fHandle,'\t\tbid_delay 1;\n');
			fprintf(fHandle,'\t\tname controller_%d;\n',hVals);
			fprintf(fHandle,'\t\tmarket Market_1;\n');
			fprintf(fHandle,'\t\tuse_override ON;\n');
			fprintf(fHandle,'\t\toverride override;\n');
			fprintf(fHandle,'\t\tbid_mode ON;\n');
			fprintf(fHandle,'\t\tcontrol_mode RAMP;\n');
			fprintf(fHandle,'\t\trange_high %.3f;\n',crh);
			fprintf(fHandle,'\t\trange_low %.3f;\n',crl);
			fprintf(fHandle,'\t\tramp_high %.3f;\n',crh2);
			fprintf(fHandle,'\t\tramp_low %.3f;\n',crl2);
			fprintf(fHandle,'\t\tbase_setpoint cooling%d*%.2f+%.2f;\n',cooling_set,cool_night_diff,cool_night);
			fprintf(fHandle,'\t\tperiod %d;\n',MarketPeriod);
			fprintf(fHandle,'\t\taverage_target current_price_mean_24h;\n');
			fprintf(fHandle,'\t\tstandard_deviation_target current_price_stdev_24h;\n');
			fprintf(fHandle,'\t\ttarget air_temperature;\n');
			fprintf(fHandle,'\t\tsetpoint cooling_setpoint;\n');
			fprintf(fHandle,'\t\tdemand last_cooling_load;\n');
			fprintf(fHandle,'\t\tdeadband thermostat_deadband;\n');
			fprintf(fHandle,'\t\ttotal hvac_load;\n');
			fprintf(fHandle,'\t\tload hvac_load;\n');
			fprintf(fHandle,'\t\tstate power_state;\n');
			fprintf(fHandle,'\t};\n');
        else % gas heat, no AC, so no control
			fprintf(fHandle,'\tcooling_setpoint cooling%d*%.2f+%.2f;\n',cooling_set,cool_night_diff,cool_night);
			fprintf(fHandle,'\theating_setpoint heating%d*%.2f+%.2f;\n',heating_set,heat_night_diff,heat_night);
        end
    end

    %Auxilliary load pieces

        % scale all of the end-use loads
        scalar1 = 324.9/8907 * floor_area^0.442;
        scalar2 = 0.8 + 0.4 * rand(1);
        scalar3 = 0.8 + 0.4 * rand(1);
        resp_scalar = scalar1 * scalar2;
        unresp_scalar = scalar1 * scalar3;

        % average size is 1.36 kW
        % Energy Savings through Automatic Seasonal Run-Time Adjustment of Pool Filter Pumps
        % Stephen D Allen, B.S. Electrical Engineering
        pool_pump_power = 1.36 + .36*rand(1);
        pool_pump_perc = rand(1);

        % average 4-12 hours / day -> 1/6-1/2 duty cycle
        % typically run for 2 - 4 hours at a time
        pp_dutycycle = 1/6 + (1/2 - 1/6)*rand(1);
        pp_period = 4 + 4*rand(1);
        pp_init_phase = rand(1);

%         fprintf(fHandle,'\tobject ZIPload {\n');
%         fprintf(fHandle,'\t\tname house_%d_resp\n',hVals);
%         fprintf(fHandle,'\t\t// Responsive load\n');
%         fprintf(fHandle,'\t\tschedule_skew %.0f;\n',skew_value);
%         fprintf(fHandle,'\t\tbase_power responsive_loads*%.2f;\n',resp_scalar);
%         fprintf(fHandle,'\t\theatgain_fraction %.3f;\n',regional_data.heat_fraction);
%         fprintf(fHandle,'\t\tpower_pf %.3f;\n',regional_data.p_pf);
%         fprintf(fHandle,'\t\tcurrent_pf %.3f;\n',regional_data.i_pf);
%         fprintf(fHandle,'\t\timpedance_pf %.3f;\n',regional_data.z_pf);
%         fprintf(fHandle,'\t\timpedance_fraction %f;\n',regional_data.zfrac);
%         fprintf(fHandle,'\t\tcurrent_fraction %f;\n',regional_data.ifrac);
%         fprintf(fHandle,'\t\tpower_fraction %f;\n',regional_data.pfrac);
%         fprintf(fHandle,'\t};\n');

        fprintf(fHandle,'\tobject ZIPload {\n');
        fprintf(fHandle,'\t\t// Unresponsive load\n');
        fprintf(fHandle,'\t\tname house_%d_unresp_load;\n',hVals);
        fprintf(fHandle,'\t\tschedule_skew %.0f;\n',skew_value(hVals));
        fprintf(fHandle,'\t\tbase_power unresponsive_loads*%.2f;\n',unresp_scalar);
        fprintf(fHandle,'\t\theatgain_fraction %.3f;\n',regional_data.heat_fraction);
        fprintf(fHandle,'\t\tpower_pf %.3f;\n',regional_data.p_pf);
        fprintf(fHandle,'\t\tcurrent_pf %.3f;\n',regional_data.i_pf);
        fprintf(fHandle,'\t\timpedance_pf %.3f;\n',regional_data.z_pf);
        fprintf(fHandle,'\t\timpedance_fraction %f;\n',regional_data.zfrac);
        fprintf(fHandle,'\t\tcurrent_fraction %f;\n',regional_data.ifrac);
        fprintf(fHandle,'\t\tpower_fraction %f;\n',regional_data.pfrac);
        fprintf(fHandle,'\t};\n');

        % pool pumps only on single-family homes
        if (pool_pump_perc < 2*regional_data.perc_poolpumps && no_pool_pumps >= 1 && row_ti == 1)
            fprintf(fHandle,'\tobject ZIPload {\n');
            fprintf(fHandle,'\t\tname house_%d_ppump;\n',hVals);
            fprintf(fHandle,'\t\t// Pool Pump\n');
            fprintf(fHandle,'\t\tschedule_skew %.0f;\n',pp_skew_value(hVals));
            fprintf(fHandle,'\t\tbase_power pool_pump_season*%.2f;\n',pool_pump_power);
            fprintf(fHandle,'\t\tduty_cycle %.2f;\n',pp_dutycycle);
            fprintf(fHandle,'\t\tphase %.2f;\n',pp_init_phase);
            fprintf(fHandle,'\t\tperiod %.2f;\n',pp_period);
            fprintf(fHandle,'\t\theatgain_fraction 0.0;\n');
            fprintf(fHandle,'\t\tpower_pf %.3f;\n',regional_data.p_pf);
            fprintf(fHandle,'\t\tcurrent_pf %.3f;\n',regional_data.i_pf);
            fprintf(fHandle,'\t\timpedance_pf %.3f;\n',regional_data.z_pf);
            fprintf(fHandle,'\t\timpedance_fraction %f;\n',regional_data.zfrac);
            fprintf(fHandle,'\t\tcurrent_fraction %f;\n',regional_data.ifrac);
            fprintf(fHandle,'\t\tpower_fraction %f;\n',regional_data.pfrac);
            fprintf(fHandle,'\t\tis_240 TRUE;\n');
            fprintf(fHandle,'\t};\n');
        end

		%water heaters
		heat_element = 3.0 + 0.5*randi(5);
		tank_set = 120 + 16*rand(1);
		therm_dead = 4 + 4*rand(1);
		tank_UA = 2 + 2*rand(1);
		water_sch = ceil(regional_data.no_water_sch*rand(1));
		water_var = 0.95 + rand(1) * 0.1; % +/-5% variability
		wh_size_test = rand(1);
		wh_size_rand = randi(3);

        if (heat_type > (1 - regional_data.wh_electric) && regional_data.use_wh == 1)
            fprintf(fHandle,'\tobject waterheater {\n');
            fprintf(fHandle,'\t\tschedule_skew %.0f;\n',wh_skew_value(hVals));
            fprintf(fHandle,'\t\theating_element_capacity %.1f kW;\n',heat_element);
            fprintf(fHandle,'\t\ttank_setpoint %.1f;\n',tank_set);
            fprintf(fHandle,'\t\ttemperature 132;\n');
            fprintf(fHandle,'\t\tthermostat_deadband %.1f;\n',therm_dead);
            fprintf(fHandle,'\t\tlocation INSIDE;\n');
            fprintf(fHandle,'\t\ttank_UA %.1f;\n',tank_UA);

            if (wh_size_test < regional_data.wh_size(1))
                fprintf(fHandle,'\t\tdemand small_%.0f*%.02f;\n',water_sch,water_var);
                    whsize = 20 + (wh_size_rand-1) * 5;
                fprintf(fHandle,'\t\ttank_volume %.0f;\n',whsize);
            elseif (wh_size_test < (regional_data.wh_size(1) + regional_data.wh_size(2)))
                if(floor_area < 2000)
                    fprintf(fHandle,'\t\tdemand small_%.0f*%.02f;\n',water_sch,water_var);
                else
                    fprintf(fHandle,'\t\tdemand large_%.0f*%.02f;\n',water_sch,water_var);
                end
                    whsize = 30 + (wh_size_rand - 1)*10;
                fprintf(fHandle,'\t\ttank_volume %.0f;\n',whsize);
            elseif (floor_area > 2000)
                    whsize = 50 + (wh_size_rand - 1)*10;
                fprintf(fHandle,'\t\tdemand large_%.0f*%.02f;\n',water_sch,water_var);
                fprintf(fHandle,'\t\ttank_volume %.0f;\n',whsize);
            else
                fprintf(fHandle,'\t\tdemand large_%.0f*%.02f;\n',water_sch,water_var);
                    whsize = 30 + (wh_size_rand - 1)*10;
                fprintf(fHandle,'\t\ttank_volume %.0f;\n',whsize);
            end
            fprintf(fHandle,'\t};\n');
        end

	%End house
    fprintf(fHandle,'}\n\n');

end

%% Print recorders

fprintf(fHandle,'object multi_recorder {\n');
fprintf(fHandle,'\tfile %s_transformer_power.csv;\n',BaseRecorderName);
fprintf(fHandle,'\tparent substation_transformer;\n');
fprintf(fHandle,'\tinterval %d;\n',RecorderInterval);
fprintf(fHandle,'\tproperty power_out_A.real,power_out_A.imag,power_out_B.real,power_out_B.imag,power_out_C.real,power_out_C.imag,power_out.real,house_1:total_load;\n');
fprintf(fHandle,'}\n');
fprintf(fHandle,'\n');
fprintf(fHandle,'object recorder {\n');
fprintf(fHandle,'\tparent house_1;\n');
fprintf(fHandle,'\tproperty total_load,air_temperature[degC],outdoor_temperature[degC],floor_area,Rroof,Rwall,Rfloor,Rdoors,heating_system_type;\n');
fprintf(fHandle,'\tinterval %d;\n',RecorderInterval);
fprintf(fHandle,'\tfile "%s_base_data_house.csv";\n',BaseRecorderName);
fprintf(fHandle,'}\n');

%close the file handle
fclose(fHandle);