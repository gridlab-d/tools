import warnings, re, sys
import pandas as pd
from collections import OrderedDict

def _readUndergroundCableGLMFile(filename):
    try:
        dictSource = open(filename, 'r')
    except IOError:
        warnings.warn('An error occured trying to read the ' + filename + ' file.')
    else:
        lines = dictSource.readlines()
        overheadLineDict = {}
        undergroundLineDict = {}
        lineConfigurationDict = {}
        index = 0
        while index < len(lines):
            line = lines[index].split('//')[0].strip()
            if re.search("object overhead_line", line) != None and re.search("overhead_line_conductor", line) == None:
                index += 1
                while '}' not in lines[index]:
                    words = lines[index].split('//')[0].strip().rstrip(';').split()
                    if len(words) >= 2:
                        if words[0] == 'name':
                            name = words[1]
                        if words[0] == 'configuration':
                            configuration = words[1]
                        if words[0] == 'phases':
                            phases = words[1]
                    index += 1
                if not overheadLineDict.has_key(words[1]):
                    overheadLineDict[name] = { 'configuration' : configuration, 'phases' : phases}
            elif re.search("object underground_line", line) != None:
                index += 1
                while '}' not in lines[index]:
                    words = lines[index].split('//')[0].strip().rstrip(';').split()
                    if len(words) >= 2:
                        if words[0] == 'name':
                            name = words[1]
                        if words[0] == 'configuration':
                            configurations = words[1].rsplit('ph', 2)
                            configuration = configurations[0]
                            phases = configurations[1]
                    index += 1
                if not undergroundLineDict.has_key(name):
                    undergroundLineDict[name] = { 'configuration' : configuration, 'phases' : phases}
            elif re.search("object line_configuration", line) != None:
                index += 1
                while '}' not in lines[index]:
                    words = lines[index].split('//')[0].strip().rstrip(';').split()
                    if len(words) >= 2:
                        if words[0] == 'name':
                            name = words[1]
                        if words[0] == 'conductor_A':
                            conductor_A = words[1]
                        if words[0] == 'conductor_B':
                            conductor_B = words[1]
                        if words[0] == 'conductor_C':
                            conductor_C = words[1]
                    index += 1
                if not lineConfigurationDict.has_key(name):
                    lineConfigurationDict[name] = {'conductor_A': conductor_A, 'conductor_B': conductor_B, 'conductor_C': conductor_C}
            index += 1
    return overheadLineDict, undergroundLineDict, lineConfigurationDict

def _readCurrents(filename):
    try:
        dictSource = pd.read_excel(filename, sheet_name='Sheet1')
        # dictSource = xl.parse('Sheet1')
    except IOError as ex:
        warnings.warn('An error occured trying to read th ' + filename + ' file.')
    else:
        columns = list(dictSource)
        return dictSource.set_index(columns[0])[columns[1]].to_dict()

def _findOverheadLineCurrent(overheadLineDict, lineConfigurationDict, overheadLineRatedCurrentsDict):
    overheadLineCurrentDict = {}
    for name, items in overheadLineDict.items():
        if not overheadLineCurrentDict.has_key(name):
            overheadLineCurrentDict[name] = {}
        config = items['configuration']
        phases = items['phases']
        for phase, conductor in lineConfigurationDict[config].items():
            phs = phase.rsplit('_')[1]
            current = overheadLineRatedCurrentsDict[conductor]
            if phs in phases:
                overheadLineCurrentDict[name][phs] = current
            else:
                overheadLineCurrentDict[name][phs] = ''
    return overheadLineCurrentDict

def _findUndergroundLineCurrent(undergroundLineDict, undergroundLineRatedCurrentsDict):
    undergroundLineCurrentDict = {}
    keyNotFound = []
    for name, items in undergroundLineDict.items():
        if not undergroundLineCurrentDict.has_key(name):
            undergroundLineCurrentDict[name] = {}
        config = items['configuration']
        if undergroundLineRatedCurrentsDict.has_key(config):
            current = undergroundLineRatedCurrentsDict[config]
        else:
            keyNotFound.append(config)
        for phs in 'ABC':
            if phs in items['phases']:
                undergroundLineCurrentDict[name][phs] = current
            else:
                undergroundLineCurrentDict[name][phs] = ''
    return undergroundLineCurrentDict, keyNotFound


def _writeDictToExcel(overheadLineCurrentDict, undergroundLineCurrentDict, outputFilename):
    writer = pd.ExcelWriter(outputFilename, engine='xlsxwriter')
    data1 = pd.DataFrame.from_dict({i: OrderedDict(sorted(overheadLineCurrentDict[i].items())).values()
                           for i in overheadLineCurrentDict.keys()},
                       orient='index', columns=['Phase A [A]', 'Phase B [A]', 'Phase C [A]'])
    data1.to_excel(writer, 'Overhead Line')
    data2 = pd.DataFrame.from_dict({i: OrderedDict(sorted(undergroundLineCurrentDict[i].items())).values()
                           for i in undergroundLineCurrentDict.keys()},
                       orient='index', columns=['Phase A [A]', 'Phase B [A]', 'Phase C [A]'])
    data2.to_excel(writer, 'Underground Line')
    writer.save()

if __name__ == '__main__':
    if len(sys.argv) == 4:
        udgcurrent = sys.argv[1]
        ohlcurrent = sys.argv[2]
        filename = sys.argv[3]
    else:
        warnings.warn("Usage: python LineCurrentStat.py UndergroundCableRatedCurrents OverheadLineRatedCurrents glmFile")
        exit()
    undergroundLineRatedCurrentsDict = _readCurrents(udgcurrent)
    overheadLineRatedCurrentsDict = _readCurrents(ohlcurrent)
    overheadLineDict, undergroundLineDict, lineConfigurationDict = _readUndergroundCableGLMFile(filename)
    overheadLineCurrentDict = _findOverheadLineCurrent(overheadLineDict, lineConfigurationDict, overheadLineRatedCurrentsDict)
    undergroundLineCurrentDict, keyNotFound = _findUndergroundLineCurrent(undergroundLineDict, undergroundLineRatedCurrentsDict)
    outputFilename = '_'.join(filename.split('_')[:3]) + '_lineCurrentStat.xlsx'
    _writeDictToExcel(overheadLineCurrentDict, undergroundLineCurrentDict, outputFilename)