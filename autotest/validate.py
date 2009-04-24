import os, sys, time
import email, smtplib

	
def email_error():
	#end email_error()
	
def parse_outfile(filename):

def build_conf(confdir, glmname):
	#	fopen conf file
	#	set stdout
	#	set stderr
	#	set dump file
	#	set debug=1
	#	set warning=1
	#	close conf file
	# end build_conf


#	basedir ~ for the glmfile
#	testdir ~ for the 
def run_file(basedir, testdir, glmname):
	#	validate basedir
	#	validate testdir
	#	validate glmname
	#	point to gridlabd.conf in testdir
	#	set output to glmname + "_out.glm"
	#	set dump file to glmname + "_dump.glm"
	#	set stdout to glmname + "_stdout.txt"
	#	set stderr to glmname + "_stderr.txt"

	# 	mkdir(glmname)
	#	copy glmname.glm to testdir/glmname.glm
	#	cd testdir/glmname
	#	build conf file
	
	#	rv = exec("gridlabd "+
	
	#end run_file

def gld_validate(args):
	#	capture cwd
	#	if args = null, copy cwd to active dir
	#	open autotest log file
	logtime = localtime()
	logfilename = "autotest_"+str(logtime.year)+str(logtime.month)+str(logtime.day)+"_"+str(logtime.hour)+str(logtime.minute)+str(logtime.second)+".log"
	autotest_log = fopen(logfilename, "w");
	#	walk active dir and cache autotest directories
	#	print autotest dir list to output file
	
	#	for each path in autotest list
		#	get file list for dir
		#	for each file
			#	if it ends with ".glm"
					#	base dir is where the GLM file resides
				#	rv = run_file(base dir, autotest dir, glm file name - ".glm")
				#	if rv is not empty
					#	if strstr(file name, "_opt")
						#	fprintf warning
					#	else
						#	fprintf error
						#	add to error list
						#	++error
				
	#	if error > 0
		# where are the email settings fetched from?
		#	email_error(error_ct, error_list)
	#end gld_validate()


if __name__ == '__main__':
	gld_validate(sys.argv)
	#end main

