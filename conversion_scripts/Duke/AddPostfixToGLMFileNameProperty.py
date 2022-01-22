import sys
import warnings

if len(sys.argv) != 4:
    warnings.warn("Usage: AddPostfixToGLMFileNameProperty.py inputFilename postfix outputFilename")
    exit()
else:
    inputFilename = sys.argv[1]
    postfix = sys.argv[2]
    outputFilename = sys.argv[3]
    try:
        input = open(inputFilename, 'r')
    except IOError:
        warnings.warn('An error occured trying to read the input file.')
    else:
        nameDict = {}
        outputStr = ""
        inputLines = input.readlines()
        for index in range(len(inputLines)):
            if index <= 41:
                outputStr += inputLines[index]
            else:
                words = inputLines[index].split()
                if len(words) >= 2 and words[0] == 'name':
                    oldName = words[1][:-1]
                    if oldName not in nameDict.keys():
                        newName = oldName + '_' + postfix
                        nameDict[oldName] = newName
                        outputStr += ('\t' + words[0] + ' ' + newName + ';\n')
                    else:
                        outputStr += ('\t' + words[0] + ' ' + nameDict[oldName] + ';\n')
                elif len(words) >= 2 and words[1][:-1] in nameDict.keys():
                    outputStr += ('\t' + words[0] + ' ' + nameDict[words[1][:-1]] + ';\n')
                else:
                    outputStr += inputLines[index]
        input.close()
        try:
            output = open(outputFilename, 'w')
        except IOError:
            warnings.warn('An error occured trying to open the output file.')
        else:
            output.write(outputStr)
            output.close()