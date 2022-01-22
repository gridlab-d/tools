# Convert ComEd CYME model to gridlab-d
Codes in this folder are written in Python 3

## cymeToGridlab_comEd2.py
1. This script is based on the cymeToGridlab.py in the parent directory conversion_scripts. 
2. It generates five glm files for each feeder: headers, equipments, network, capacitors and loads.
3. It does not depend on the omf library anymore.

## CompareModelResults.py
1. This script is used to compare the output csv files from the CYME model and gridlab-d.
2. It generate a csv file with comparison results.