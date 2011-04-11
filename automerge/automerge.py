#!/usr/bin/python
import sys
from optparse import OptionParser
import os
import pysvn
from pysvn import Client,Revision
from subprocess import call,Popen,PIPE
from lastrev import startrev



update = 2
build = 4
autotest=8

def notify (event_dict):
	if(event_dict['kind'] == pysvn.node_kind.file):
		print "A\t",event_dict['path']


def ssl_trust_prompt(td):
	"""docstring for ssl_trust_prompt"""
	print 'Failures:',td['failures']
	return True,td['failures'],True
	
def doAutotest(path):
	"""docstring for doAutotest"""
	currentDir = os.getcwd()
	os.chdir(path)
	retcode = call(['python', 'validate.py', '.'])
	os.chdir(currentDir)
	
	return retcode
	
def doCompile(path):
	"""docstring for doCompile"""
	currentDir = os.getcwd()
	os.chdir(path)
	
	retcode = call(['MSBuild', 'gridlabd.sln', '/t:Rebuild',
                    '/p:Platform=Win32;Configuration=Release', '/nologo'])
	
	os.chdir(currentDir)
	
	return retcode
	
def getSourceTree(path,url,client,options):
	"""docstring for getSourceTree"""
	if(os.path.exists(path)):
		if(options.verbose):
			print "Updating",path
		client.update(path)
	else:
		# if not, check it out
		if(options.verbose):
			print "Checking out",path
		client.checkout(url,path)
		
def doMerge(path,endrev,options):
	"""docstring for doMerge"""
	conflicts = False
	conflict_list = []
	retcode = 0
	
	if(options.verbose):
		print "svn merge -r",str(startrev) + ":" + str(endrev),"--non-interactive --ignore-all-space",options.branchUrl,path
	
	p = Popen(['svn','merge','-r',str(startrev) + ":" + str(endrev),
			   '--non-interactive', '--extensions','--ignore-space-change',
			   options.branchUrl,path],shell=True,stdin=PIPE,stdout=PIPE)
	
	line = p.stdout.readline()
	
	while(line != ''):
		line = line.rstrip()
		if(options.verbose>1):
			if(not line.startswith('C')):
				print line
		
		if(line.startswith('C')): # Conflict detected
			conflicts = True
			conflict_list.append(line)
			if(options.verbose):
				print line

		line = p.stdout.readline()
	if(conflicts):
		retcode = -1
	return (retcode,conflict_list)
	
def printList(l):
	"""Prints out the contents of a list"""
	for i in l:
		print i
	
def test_skip(skip_flags,flag):
	return (skip_flags == '0' or not (skip_flags & flag))

def main(options):
	"""docstring for main"""
	client = Client()
	client.callback_ssl_server_trust_prompt = ssl_trust_prompt
	if(options.verbose>1):
		client.callback_notify = notify
	retcode = 0
	# check out branch from subversion
	if(test_skip(options.branch_flags, update)):
		getSourceTree(options.workingDir+os.sep+"branch",options.branchUrl,client,options)
	elif(options.verbose):
		print "Skipping branch update"
	
	# compile branch
	if(test_skip(options.branch_flags,build)):
		print "Compiling branch"
		retcode = doCompile(options.workingDir+os.sep+"branch"+os.sep+"VS2005")
	elif(options.verbose):
		print "Skipping branch compile"
		
	# If compile successful, Run autotest
	if(retcode != 0):
		print "Failed to compile branch"
		return retcode
	
	if(test_skip(options.branch_flags,autotest)):
		print "Running branch autotest"
		retcode = doAutotest(options.workingDir+os.sep+"branch")
	elif(options.verbose):
		print "Skipping branch autotest"
	
	if(retcode != 0):
		print "Failed branch autotest"
		return retcode
	
	if(test_skip(options.trunk_flags,update)):
		getSourceTree(options.workingDir+os.sep+"trunk",options.trunkUrl,client,options)
	elif(options.verbose):
		print "Skipping trunk update"
	
	# Determine the revision number of the head of the branch
	if(options.endrev == 'HEAD'):
		if(options.verbose):
			print "Getting last revision number of branch"
		messages = client.log(branchUrl,revision_end=Revision(pysvn.opt_revision_kind.number,startrev))
		endrev = messages[0].revision.number
		options.endrev = str(endrev)
	
	if(options.verbose):
		print "Revisions to merge",options.startrev+":"+options.endrev
	
	# Merge changes from branch to trunk
	if(not options.skip_merge):
		if(options.verbose):
			print "Merging branch changes to trunk working directory"
		(retcode,conflicts) = doMerge(options.workingDir+os.sep+"trunk",endrev,options)
	elif(options.verbose):
		print "Skipping merge"
	
	# How do we tell if there were merge errors?!?
	if(retcode !=0):
		print "Merge to working directory failed with conflicts."
		printList(conflicts)
		return retcode
	
	# Compile trunk
	if(test_skip(options.trunk_flags,build)):
		print "Compiling trunk"
		retcode = doCompile(options.workingDir+os.sep+"trunk"+os.sep+"VS2005")
	elif(options.verbose):
		print "Skipping trunk compile"
	
	if(retcode != 0):
		print "Failed to compile merged trunk"
		return retcode
	
	# If compile successful, run autotest
	if(test_skip(options.trunk_flags,autotest)):
		print "Running trunk autotest"
		retcode = doAutotest(options.workingDir+os.sep+"trunk")
	elif(options.verbose):
		print "Skipping trunk autotest"
	
	if(retcode != 0):
		print "Failed in merged autotest"
		return retcode
	
	# TODO: If successful, commit.
	if(not options.skip_commit):
		pass
	
	# Write out new lastrev.py file
	fp = open("lastrev.py",'w')
	fp.write("startrev = " + str(endrev)+"\n")
	fp.write("prevrev = " + str(startrev))
	fp.close()
	
def skip_callback2(option, opt, value, parser):
	"""docstring for skip_callback2"""
	value = value.replace(',','|')
	
	attr = getattr(parser.values,option.dest)
		
	setattr(parser.values,option.dest,eval(str(attr) + "|"+value))


if __name__ == '__main__':
	skip_flags = 0
	trunkUrl = "https://gridlab-d.svn.sourceforge.net/svnroot/gridlab-d/trunk"
	branchUrl = "https://gridlab-d.svn.sourceforge.net/svnroot/gridlab-d/branch/2.2"
	workingDir = "automerge"
	
	parser = OptionParser()
	parser.add_option("--skip-br",action='callback', callback=skip_callback2, 
	                  dest='branch_flags',nargs=1, type='string',default='0',
	                  help='Skips branch actions. CSV consisting of update, build, autotest')
	parser.add_option("--skip-tr",action='callback', callback=skip_callback2, 
	                  dest='trunk_flags',nargs=1, type='string',default='0',
	                  help='Skips trunk actions. CSV consisting of update, build, autotest')
	parser.add_option("--skip-merge", action='store_true', dest='skip_merge',
	                  default=False, help="Option to skip merge step")
	parser.add_option("--skip-commit",action='store_true',dest='skip_commit',
					  default=False,help='Skip the commit step')
	parser.add_option("-v",action='count', dest='verbose',default=0,
	                  help='Verbose output')
	
	parser.add_option('--trunk-url',action='store',type='string',dest='trunkUrl',
	                  default=trunkUrl,help='URL of the trunk SVN tree')
	parser.add_option('--branch-url',action='store',type='string',dest='branchUrl',
	                  default=branchUrl,help='URL of the branch SVN tree')
	parser.add_option('--working-dir', action='store',type='string',dest='workingDir',
					  default=workingDir,help='Location of the working directory')

	# TODO: Add options to specify the start/end revisions
	(options,args) = parser.parse_args()
	
	retcode = main(options)
	sys.exit(retcode)
