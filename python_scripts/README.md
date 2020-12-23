# python_scripts
Tool Scripts for GridLAB-D in Python

## GlmParser

1) The class 'GlmParser' parses the .glm file, ignoring all comments;
2) UFLS GFA devices can be added to 'load' and 'triplex_load' objects, using the class 'GlmParser';
3) A member function of the class 'GlmParser' can add parallel cables (e.g., defined in the CYME model) into the GLD model.

## JsonExporter
Exports the .json file for running GridLAB-D with Helics and NS-3. A set of inveters is specified as the endpoints.

## GldSmn
Runs GridLAB-D and save the results, with respect to a given set of PVs (of which the Q_Out is evaluated from -1.0 p.u. to +1.0 p.u.).

## CsvExtractor
This was created for the transactive algorithm of the Duke RDS project. It extracts the interested values (e.g., voltage changes) from the results collected using GldSmn. The extracted information is packaged using the pickle module. 

## Simple_GLD_Run_Example

Provides an extremely simplified example of a Python 3.x script (in Windows) to run GridLAB-D and copy results.
The script is extremely basic and basically meaningless, but shows the mechanics needed.
