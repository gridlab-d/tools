import sys
import warnings
import csv
import math

if len(sys.argv) != 4:
    warnings.warn("Usage: CompareModelResults.py CYMEmodelResult GridLABDmodelResult outputFilename")
    exit()
else:
    cymeFile = sys.argv[1]
    gridlabdFile = sys.argv[2]
    outputFilename = sys.argv[3]
    cymeDict = {}
    gridLabDDict = {}
    outputDict = {}
    goodCymeKeys = []
    goodGridLABDKeys = []
    matchedNodes = {}
    unMatchedNodes = {}
    delKey = []
    try:
        # cyme = open(cymeFile, 'r')
        with open(cymeFile, 'r') as cymeFile:
            cymeLines = csv.reader(cymeFile)
            for row in cymeLines:  # type: object
                if row[0] != 'Node Id\n' and row[0] != '':
                #     pass
                # else:
                    cymeDict[row[0]] = {'A' : row[5], 'B' : row[6], 'C' : row[7]}
    except IOError:
        warnings.warn('An error occurred trying to read the CYME result file.')
        exit()
    # else:
    #     pass
        # cymeLines = cymeFile.readlines()
        # for index in range(len(cymeLines)):
        #     words = cymeLines[index].split()
    try:
        with open(gridlabdFile, 'r') as gridlabdFile:
            gridlabdLines = csv.reader(gridlabdFile)
            for row in gridlabdLines:
                if len(row) == 7:
                    gridLabDDict[row[0]] = {'Ar' : row[1], 'Ai' : row[2], 'Br' : row[3], 'Bi' : row[4], 'Cr' : row[5], 'Ci' : row[6]}
    except IOError:
        warnings.warn('An error occured trying to read the GridLABD result file.')
        exit()
    for nodeName, nodeValue in gridLabDDict.iteritems():
        try:
            ar = float(nodeValue['Ar'])
            ai = float(nodeValue['Ai'])
            br = float(nodeValue['Br'])
            bi = float(nodeValue['Bi'])
            cr = float(nodeValue['Cr'])
            ci = float(nodeValue['Ci'])
            gridLabDDict[nodeName]['A'] = math.sqrt(ar**2 + ai**2)
            gridLabDDict[nodeName]['B'] = math.sqrt(br**2 + bi**2)
            gridLabDDict[nodeName]['C'] = math.sqrt(cr**2 + ci**2)
        except Exception:
            delKey.append(nodeName)
            continue
            # del gridLabDDict[nodeName]
            # gridLabDDict[nodeName]['A'] = ''
            # gridLabDDict[nodeName]['B'] = ''
            # gridLabDDict[nodeName]['C'] = ''
        # key = ''.join(c for c in nodeName if c.isdigit())
        foundKey = ""
        for key in cymeDict.keys():
            if key in nodeName:
                foundKey = key
                break;
        # if key != '' and cymeDict.has_key(key):
        if foundKey:
            goodCymeKeys.append(foundKey)
            goodGridLABDKeys.append(nodeName)
            aDiff = bDiff = cDiff = -1
            if cymeDict[key]['A'] != "":
                aDiff = abs(float(cymeDict[key]['A'])*1000 - gridLabDDict[nodeName]['A'])/float(cymeDict[key]['A'])/1000
                gridLabDDict[nodeName]['aDiff'] = aDiff
            else:
                gridLabDDict[nodeName]['aDiff'] = ''
            if cymeDict[key]['B'] != "":
                bDiff = abs(float(cymeDict[key]['B'])*1000 - gridLabDDict[nodeName]['B'])/float(cymeDict[key]['B'])/1000
                gridLabDDict[nodeName]['bDiff'] = bDiff
            else:
                gridLabDDict[nodeName]['bDiff'] = ''
            if cymeDict[key]['C'] != "":
                cDiff = abs(float(cymeDict[key]['C'])*1000 - gridLabDDict[nodeName]['C'])/float(cymeDict[key]['C'])/1000
                gridLabDDict[nodeName]['cDiff'] = cDiff
            else:
                gridLabDDict[nodeName]['cDiff'] = ''
            if aDiff > 0.0002 or bDiff > 0.0002 or cDiff > 0.0002:
                unMatchedNodes[nodeName] = key
            else:
                matchedNodes[nodeName] = key
    for key in delKey:
        del gridLabDDict[key]
    with open(outputFilename, 'wb') as csvfile:
        output = csv.writer(csvfile, delimiter=',')
        output.writerow(['GridLABD Node ID', 'voltA_real', 'voltA_imag', 'voltB_real', 'voltB_imag', 'voltC_real', 'voltC_imag', '', 'VA', 'VB', 'VC', '', 'CYME ID', 'VA', 'VB', 'VC', '', 'VA diff', 'VB diff', 'VC diff'])
        output.writerow(['matched nodes:'])
        for key, value in matchedNodes.iteritems():
            output.writerow([key, gridLabDDict[key]['Ar'], gridLabDDict[key]['Ai'], gridLabDDict[key]['Br'], gridLabDDict[key]['Bi'], gridLabDDict[key]['Cr'], gridLabDDict[key]['Ci'], '', gridLabDDict[key]['A'], gridLabDDict[key]['B'], gridLabDDict[key]['C'], '', value, cymeDict[value]['A'], cymeDict[value]['B'], cymeDict[value]['C'], '', gridLabDDict[key]['aDiff'], gridLabDDict[key]['bDiff'], gridLabDDict[key]['cDiff']])
        output.writerow([])
        output.writerow(['unMatched nodes:'])
        for key, value in unMatchedNodes.iteritems():
            output.writerow([key, gridLabDDict[key]['Ar'], gridLabDDict[key]['Ai'], gridLabDDict[key]['Br'], gridLabDDict[key]['Bi'], gridLabDDict[key]['Cr'], gridLabDDict[key]['Ci'], '', gridLabDDict[key]['A'], gridLabDDict[key]['B'], gridLabDDict[key]['C'], '', value,
                             cymeDict[value]['A'], cymeDict[value]['B'], cymeDict[value]['C'], '',
                             gridLabDDict[key]['aDiff'], gridLabDDict[key]['bDiff'], gridLabDDict[key]['cDiff']])
        output.writerow([])
        output.writerow(['unique nodes in gridlabd results:'])
        uniqueGridLabDNodes = []
        for key, value in gridLabDDict.iteritems():
            if key not in goodGridLABDKeys:
                output.writerow([key, gridLabDDict[key]['Ar'], gridLabDDict[key]['Ai'], gridLabDDict[key]['Br'], gridLabDDict[key]['Bi'], gridLabDDict[key]['Cr'], gridLabDDict[key]['Ci'], '', value['A'], value['B'], value['C'], '', '', '', '', '', '', '', '', ''])
                uniqueGridLabDNodes.append(key)
        output.writerow([])
        output.writerow(['unique nodes in cyme:'])
        uniqueCYMENodes = []
        for key, value in cymeDict.iteritems():
            if key not in goodCymeKeys:
                output.writerow([key, '', '', '', '', '', '', '', '', '', '', '', value['A'], value['B'], value['C'], '', '', '', '', ''])
                uniqueCYMENodes.append(key)
    statisticDict = {}
    keyValuePairs = [ pairs for key in matchedNodes.keys() for pairs in gridLabDDict[key].items()]
    keyValuePairs.extend([ pairs for key in unMatchedNodes.keys() for pairs in gridLabDDict[key].items()])
    voltageA = [ pair[1] for pair in keyValuePairs if pair[0] == 'aDiff' and pair[1] != '']
    statisticDict['a'] = {}
    statisticDict['a']['max'] = max(voltageA)
    statisticDict['a']['min'] = min(voltageA)
    statisticDict['a']['average'] = sum(voltageA)/float(len(voltageA))
    voltageB = [ pair[1] for pair in keyValuePairs if pair[0] == 'bDiff' and pair[1] != '']
    statisticDict['b'] = {}
    statisticDict['b']['max'] = max(voltageB)
    statisticDict['b']['min'] = min(voltageB)
    statisticDict['b']['average'] = sum(voltageB)/float(len(voltageB))
    voltageC = [ pair[1] for pair in keyValuePairs if pair[0] == 'cDiff' and pair[1] != '']
    statisticDict['c'] = {}
    statisticDict['c']['max'] = max(voltageC)
    statisticDict['c']['min'] = min(voltageC)
    statisticDict['c']['average'] = sum(voltageC)/float(len(voltageC))
    print "There are {nodeNumber} nodes in GridLAB-D model exit in CYME model.\n" \
          "Voltage A: \n" \
          "\tmax: {VAMax:.10%}" \
          "\tmin: {VAMin:.10%}" \
          "\taverage: {VAAverage:.10%}" \
          "\nVoltage B: \n" \
          "\tmax: {VBMax:.10%}" \
          "\tmin: {VBMin:.10%}" \
          "\taverage: {VBAverage:.10%}" \
          "\nVoltage C: \n" \
          "\tmax: {VCmax:.10%}" \
          "\tmin: {VCMin:.10%}" \
          "\taverage: {VCAverage:.10%}" \
          "".format(nodeNumber=len(matchedNodes)+len(unMatchedNodes), VAMax=statisticDict['a']['max'], VAMin=statisticDict['a']['min'],
                    VAAverage=statisticDict['a']['average'], VBMax=statisticDict['b']['max'],
                    VBMin=statisticDict['b']['min'], VBAverage=statisticDict['b']['average'],
                    VCmax=statisticDict['c']['max'], VCMin=statisticDict['c']['min'],
                    VCAverage=statisticDict['c']['average'])
    print "There is/are {uniqueG} node(s) in GridLAB-D model that is/are not in CYME model as the following:\n{nodes}"\
        .format(uniqueG=len(uniqueGridLabDNodes), nodes="\n")
        # .format(uniqueG=len(uniqueGridLabDNodes), nodes="\n".join(uniqueGridLabDNodes))
    print "There is/are {uniqueC} node(s) in CYME model that is/are not in GridLAB-D model as the following:\n{nodes}"\
        .format(uniqueC=len(uniqueCYMENodes), nodes="\n")
        # .format(uniqueC=len(uniqueCYMENodes), nodes="\n".join(uniqueCYMENodes))