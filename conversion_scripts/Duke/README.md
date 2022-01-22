# Convert Duke CYME model to gridlab-d
Codes in this folder are written in Python 2

## cymeToGridlab_duke.py
1. This script is based on the cymeToGridlab.py in the parent directory conversion_scripts. 
2. It generates a glm file for a given feeder.
3. It needs the omf library to run as it draws a graph for the glm file generated and run the power flow in the end (later half of function _tests()). If comment that part of the code out, omf library is not necessary. 
4. Instead of installing omf, add the path to omf/omf to PYTHONPATH is a easier way to go.

## CompareModelResults.py
1. This script is used to compare the output csv files from the CYME model and gridlab-d.
2. It generate a csv file with comparison results.

## LineCurrentStat.py
1. This script is used to find currents for underground cable and overhead line. It reads two excel files with information on underground cable rated currents and overhead line rated currents, respectively, and a gridlab-d glm file.
2. It generate a excel file with currents for each line and phase.

## AddPostfixToGLMFileNameProperty.py
This script adds a postfix to all names (the name property) in a glm file. The postfix is mostly the feeder name so if we combine multiple glm files, there won't be any duplicate names.

## findLoadAndItsParentNode.py
1. This script finds all loads and the parent node of each load in a given glm file.
2. The output file list all loads and their parent nodes, and the active and reactive power of each phase for each load.
