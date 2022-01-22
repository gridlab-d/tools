import warnings, re
import pandas as pd
import sys
from collections import OrderedDict


def findLoadandNodeParent(filename):
    try:
        dictSource = open(filename, 'r')
    except IOError:
        warnings.warn('An error occured trying to read the ' + filename + ' file.')
    else:
        lines = dictSource.readlines()
        loadDict = {}
        index = 0
        while index < len(lines):
            line = lines[index].split('//')[0].strip()
            if re.search("object load", line) != None:
                index += 1
                RealB = ''
                ReactiveB = ''
                RealA = ''
                ReactiveA = ''
                RealC = ''
                ReactiveC = ''
                while '}' not in lines[index]:
                    words = lines[index].split('//')[0].strip().rstrip(';').split()
                    if len(words) >= 2:
                        if words[0] == 'name':
                            name = words[1]
                        if words[0] == 'parent':
                            parent = words[1]
                        if words[0] == 'constant_power_B':
                            p = words[1].split('+')
                            RealB = p[0]
                            ReactiveB = p[1][:-1]
                        if words[0] == 'constant_power_A':
                            p = words[1].split('+')
                            RealA = p[0]
                            ReactiveA = p[1][:-1]
                        if words[0] == 'constant_power_C':
                            p = words[1].split('+')
                            RealC = p[0]
                            ReactiveC = p[1][:-1]
                    index += 1
                if not name in loadDict.keys():
                    loadDict[name] = { 'RealA' : RealA,
                                       'ReactiveA' : ReactiveA,
                                       'RealB' : RealB,
                                       'ReactiveB' : ReactiveB,
                                       'RealC' : RealC,
                                       'ReactiveC' : ReactiveC,
                                       'Node': parent }
            index += 1
        return loadDict

def _writeDictToExcel(loadDict, outputFilename):
    writer = pd.ExcelWriter(outputFilename, engine='xlsxwriter')
    data1 = pd.DataFrame.from_dict({i: loadDict[i].values() for i in loadDict.keys()},
                       orient='index', columns=[ 'RealA', 'ReactiveA', 'RealB', 'ReactiveB', 'RealC', 'ReactiveC', 'Node' ])
    data1.to_excel(writer)
    writer.save()


if __name__ == '__main__':
    if len(sys.argv) == 3:
        fileName = sys.argv[1]
        outputFileName = sys.argv[2]
    else:
        warnings.warn("Usage: python findLoadAndItsParentNode.py inputFileName outputFileName")
        exit()
    loadDict = findLoadandNodeParent(fileName)
    _writeDictToExcel(loadDict, outputFileName)
