# Very simple test script
# Runs a GLM file in a different folder and copies the results
# into a separate output folder, just because
#
# Basically a nonsense script, but shows an example of how you may run GridLAB-D from Python
# Written for Python 3.x and a Windows environment.
#
# Created September 21, 2017 by Frank Tuffner
# Updated August 13, 2020 for posting to GridLAB-D repository

#Import necessary libraries - subprocess actually runs commands
#os is used to append the path
import subprocess, os, shutil

#Variables (just for convenience)
#Relative path (just folder name) - to specify a fully-resolved path, the "curr_file_loc" variable needs adjusting
#GLMFile = name of the GLM to run, in GLMFolder
#GLMFolder = location of the GLM
#GLDFolder = location of "single-folder" GridLAB-D (all copied into same folder)
#OutputFolder = location where to copy the result (voltage_A.csv in this case) and append a run number.
GLMFile = "glm_to_run.glm"
GLMFolder = "glm_files"
GLDFolder = "gld_build"
OutputFolder = "output_folder"

#Nonsense variable - realistically should be built off a file table or similar.
NumRuns = 4

#Find out where we are - base path information
curr_file_loc = os.path.dirname(os.path.realpath(__file__))

#Copy in the current path
curr_env = os.environ.copy()

#Append the GLD portion
curr_env["PATH"] = curr_env["PATH"] + ";" + (curr_file_loc + "\\" + GLDFolder)
curr_env["GLPATH"] = curr_file_loc + "\\" + GLDFolder

#Define the GridLAB-D command - this will be the same for all instances
GLDCommand="gridlabd " + '"' + (curr_file_loc + "\\" + GLMFolder + "\\" + GLMFile) + '"'

#Run the loops
for i in range(0,NumRuns,1):
	
	#Generate GLM/player file here
	print("File generated!")
	
	#Run the GLM here
	proc_val = subprocess.Popen(GLDCommand, env=curr_env, shell=True)
	
	#Wait for it to finish (consequence of Popen
	proc_val.wait()
	
	#Move/rename the output file 
	shutil.move((curr_file_loc + "\\voltage_A_out.csv"),(curr_file_loc + "\\" + OutputFolder + "\\voltage_A_out_" + str(i) + ".csv"))

print("All done")