from buildbot.process import factory
from buildbot.steps.source import SVN
from buildbot.steps.shell import Configure, Compile, ShellCommand
from buildbot.steps.python_twisted import Trial
from buildbot.steps.transfer import FileUpload
from localbb.steps.transfer import FileUploadOutput

#==================================== Build Factory Functions ====================================
def gen_win32_nightly_factory(svnURL):
	win32_nightly2_factory = factory.BuildFactory()
	win32_nightly2_factory.addStep(SVN, workdir=r'build',svnurl=svnURL,username='buildbot',password="V3hUTh2d")
	win32_nightly2_factory.addStep(ShellCommand,
			   workdir=r'build',
			   name='update_revision',
			   haltOnFailure=False,
			   description=['update revision'],
			   descriptionDone=['update_revision'],
			   command=['bash', 'utilities/gen_rev'])
	win32_nightly2_factory.addStep(ShellCommand,
			   workdir=r'build',
			   name='update_version',
			   haltOnFailure=False,
			   description=['update version'],
			   descriptionDone=['update_version'],
			   command=['bash', 'checkpkgs'])
	win32_nightly2_factory.addStep(ShellCommand,
			   workdir=r'build',
			   name='clean_autotest',
			   haltOnFailure=False,
			   description=['clean autotest'],
			   descriptionDone=['clean_autotest'],
			   command=['bash', 'for i in `find . -type d -name autotest`; do for j in `find $i -name \*.dll`; do rm $j; done; done'])
	win32_nightly2_factory.addStep(Compile, workdir=r'build\\VS2005',
			   description=['32-bit compile'],
			   descriptionDone=['32-bit compile'],
			   command=['MSBuild', 'gridlabd.sln', '/t:Rebuild',
						'/p:Platform=Win32;Configuration=Release', '/nologo'])
	win32_nightly2_factory.addStep(ShellCommand,
			   workdir=r'build',
			   name='autotest',
			   haltOnFailure=True, 
			   description=['Testing Gridlabd'],
			   timeout=60*60,
			   descriptionDone=['autotest'],
			   command=['python', 'validate.py', '.'])
	win32_nightly2_factory.addStep(ShellCommand,
			   workdir=r'build\\VS2005',
			   name='installer',
			   haltOnFailure=True,
			   description=['building installer'],
			   descriptionDone=['installer'],
			   command=['python','build_installer.py', 'gridlabd-win32'])
	win32_nightly2_factory.addStep(ShellCommand,
			   workdir=r'build\\VS2005',
			   name='copy_installer',
			   haltOnFailure=True,
			   description=['copy installer'],
			   descriptionDone=['copy installer'],
			   command=['python','copy_installer.py', 'gridlabd-win32','..\\..\\releases'])
	win32_nightly2_factory.addStep(ShellCommand,
			   workdir=r'build\\VS2005',
			   name='sf_updater',
			   haltOnFailure=True,
			   description=['updating sourceforge'],
			   descriptionDone=['sf_update'],
			   command=['python','update_sourceforge.py']) # Deletes files older than 5 days.
	win32_nightly2_factory.addStep(ShellCommand,
			   workdir=r'build',
			   name='upload_installer',
			   haltOnFailure=False,
			   description=['Upload nightly builds to Sourceforge'],
			   descriptionDone=['upload_installer'],
			   command=['rsync_wrapper.bat','../releases/','frs.sourceforge.net','/home/frs/project/g/gr/gridlab-d/gridlab-d/Nightly\ Win32'])
	win32_nightly2_factory.addStep(FileUploadOutput,
			   blocksize=64*1024,
			   workdir=r'build\\VS2005\\Win32\\Release',
			   slavesrc='gridlabd-win32-nightly.exe',
			   masterdest='gridlabd-win32-nightly.exe')
	win32_nightly2_factory.addStep(ShellCommand,
			   workdir=r'build',
			   name='doxygen',
			   haltOnFailure=True,
			   description=['Build Current Documentation'],
			   descriptionDone=['doxygen'],
			   command=['doxygen.exe', 'doxygen\\gridlabd.conf'])
	win32_nightly2_factory.addStep(ShellCommand,
			   workdir=r'build',
			   name='copy_docs',
			   haltOnFailure=False,
			   description=['Copy nightly build documentation'],
			   descriptionDone=['copy_docs'],
			   command=['rsync_wrapper.bat','../documents/html/*','web.sourceforge.net','htdocs/doxygen'])
	win32_nightly2_factory.addStep(ShellCommand,
			   workdir=r'build',
			   name='generate_troubleshooting',
			   haltOnFailure=False,
			   description=['generate troubleshooting'],
			   descriptionDone=['generate_troubleshooting'],
			   command=['bash', 'generate_troubleshooting.sh'])
	win32_nightly2_factory.addStep(ShellCommand,
			   workdir=r'build',
			   name='copy_troubleshooting',
			   haltOnFailure=False,
			   description=['Copy troubleshooting documentation'],
			   descriptionDone=['copy_troubleshooting'],
			   command=['rsync_wrapper.bat','../documents/troubleshooting/*','web.sourceforge.net','htdocs/troubleshooting'])

	return win32_nightly2_factory

def gen_win32_full_factory (svnURL):
	win32_full2_factory = factory.BuildFactory()
	win32_full2_factory.addStep(SVN, workdir=r'build',svnurl=svnURL,username='buildbot',password="V3hUTh2d")
	win32_full2_factory.addStep(Compile, workdir=r'build\\VS2005',
			   description=['32-bit compile'],
			   descriptionDone=['32-bit compile'],
			   command=['MSBuild', 'gridlabd.sln', '/t:Rebuild',
						'/p:Platform=Win32;Configuration=Release', '/nologo'])
	win32_full2_factory.addStep(ShellCommand,
			   workdir=r'build\\VS2005',
			   name='installer',
			   haltOnFailure=True,
			   description=['building installer'],
			   descriptionDone=['installer'],
			   command=['iscc.exe', '/Fgridlabd-release', 'gridlabd.iss'])
	win32_full2_factory.addStep(FileUploadOutput,
			   blocksize=64*1024,
			   workdir=r'build\\VS2005\\Win32\\Release',
			   slavesrc='gridlabd-release.exe',
			   masterdest='gridlabd-release.exe')
	return win32_full2_factory

def gen_x64_full_factory (svnURL):
	x64_full_factory = factory.BuildFactory()
	x64_full_factory.addStep(SVN, workdir=r'build',svnurl=svnURL,username='buildbot',password="V3hUTh2d")
	x64_full_factory.addStep(Compile, workdir=r'build\\VS2005',
			   description=['64-bit compile'],
			   descriptionDone=['64-bit compile'],
			   command=['build_x64.bat'])
	x64_full_factory.addStep(ShellCommand,
			   workdir=r'build\\VS2005',
			   name='installer',
			   haltOnFailure=True,
			   description=['building installer'],
			   descriptionDone=['installer'],
			   command=['iscc.exe', '/Fgridlabd_x64-release', 'gridlabd-64.iss'])
	x64_full_factory.addStep(FileUploadOutput,
			   blocksize=64*1024,
			   workdir=r'build\\VS2005\\x64\\Release',
			   slavesrc='gridlabd_x64-release.exe',
			   masterdest='gridlabd_x64-release.exe')
	return x64_full_factory

def gen_x64_nightly_factory(svnURL):
	x64_nightly_factory = factory.BuildFactory()
	x64_nightly_factory.addStep(SVN, workdir=r'build',svnurl=svnURL,username='buildbot',password="V3hUTh2d")
	x64_nightly_factory.addStep(ShellCommand,
			   workdir=r'build',
			   name='update_revision',
			   haltOnFailure=False,
			   description=['update revision'],
			   descriptionDone=['update_revision'],
			   command=['bash', 'utilities/gen_rev'])
	x64_nightly_factory.addStep(ShellCommand,
			   workdir=r'build',
			   name='update_version',
			   haltOnFailure=False,
			   description=['update version'],
			   descriptionDone=['update_version'],
			   command=['bash', 'checkpkgs'])
	x64_nightly_factory.addStep(ShellCommand,
			   workdir=r'build',
			   name='clean_autotest',
			   haltOnFailure=False,
			   description=['clean autotest'],
			   descriptionDone=['clean_autotest'],
			   command=['bash', 'for i in `find . -type d -name autotest`; do for j in `find $i -name \*.dll`; do rm $j; done; done'])
	x64_nightly_factory.addStep(Compile, workdir=r'build\\VS2005',
			   description=['64-bit compile'],
			   descriptionDone=['64-bit compile'],
			   command=['build_x64.bat'])
	x64_nightly_factory.addStep(ShellCommand,
			   workdir=r'build',
			   name='autotest',
			   haltOnFailure=True, 
			   description=['Testing Gridlabd'],
			   descriptionDone=['autotest'],
			   timeout=60*60,
			   command=['python', 'validate.py', '.'])
	x64_nightly_factory.addStep(ShellCommand,
			   workdir=r'build\\VS2005',
			   name='installer',
			   haltOnFailure=True,
			   description=['building installer'],
			   descriptionDone=['installer'],
			   command=['python','build_installer.py', 'gridlabd-x64','gridlabd-64.iss'])
	x64_nightly_factory.addStep(ShellCommand,
			   workdir=r'build\\VS2005',
			   name='copy_installer',
			   haltOnFailure=True,
			   description=['copy installer'],
			   descriptionDone=['copy installer'],
			   command=['python','copy_installer.py', 'gridlabd-x64','..\\..\\releases'])
	x64_nightly_factory.addStep(ShellCommand,
			   workdir=r'build\\VS2005',
			   name='sf_updater',
			   haltOnFailure=True,
			   description=['updating sourceforge'],
			   descriptionDone=['sf_update'],
			   command=['python','update_sourceforge.py','--prefix=gridlabd-x64-']) # Deletes files older than 5 days.
	x64_nightly_factory.addStep(ShellCommand,
			   workdir=r'build',
			   name='upload_installer',
			   haltOnFailure=False,
			   description=['Upload nightly builds to Sourceforge'],
			   descriptionDone=['upload_installer'],
			   command=['rsync_wrapper.bat','../releases/','frs.sourceforge.net','/home/frs/project/g/gr/gridlab-d/gridlab-d/Nightly\ x64'])
	x64_nightly_factory.addStep(FileUploadOutput,
			   blocksize=64*1024,
			   workdir=r'build\\VS2005\\x64\\Release',
			   slavesrc='gridlabd-x64-nightly.exe',
			   masterdest='gridlabd-x64-nightly.exe')
	return x64_nightly_factory

def gen_rh_nightly_factory (svnURL):
	linux_rh_nightly_factory = factory.BuildFactory()
	linux_rh_nightly_factory.addStep(SVN, workdir=r'build',svnurl=svnURL,username='buildbot',password="V3hUTh2d")
	linux_rh_nightly_factory.addStep(ShellCommand,
			   workdir=r'build',
			   name='clean_autotest',
			   haltOnFailure=False,
			   description=['clean autotest'],
			   descriptionDone=['clean_autotest'],
			   command=['bash', 'for i in `find . -type d -name autotest`; do for j in `find $i -name \*.dll`; do rm $j; done; done'])
	linux_rh_nightly_factory.addStep(ShellCommand, workdir=r'build', name='autoreconf',
			   description=['autoreconf'], descriptionDone=['autoreconf'],
			   command=['autoreconf', '-is'])
	linux_rh_nightly_factory.addStep(Configure, workdir=r'build', command=['./configure','--prefix=/tmp/gridlabd','--exec-prefix=/tmp/gridlabd'])
	linux_rh_nightly_factory.addStep(Compile, workdir=r'build', command=['make', 'over'])
	linux_rh_nightly_factory.addStep(ShellCommand, name='make_install',
			   description='make_install',descriptionDone='make_install',
			   workdir=r'build', command=['make', 'install'])
	linux_rh_nightly_factory.addStep(ShellCommand,
			   workdir=r'build',
			   name='autotest',
			   haltOnFailure=True, 
			   description=['Testing Gridlabd'],
			   timeout=60*120,
			   descriptionDone=['autotest'],
			   command=['python', 'validate.py','--idir=/tmp/gridlabd', '.'])
	linux_rh_nightly_factory.addStep(ShellCommand, name='rpm_prep',
			   description='rpm_prep',descriptionDone='rpm_prep',
			   workdir=r'build', command=['make', 'rpm-prep'])
	linux_rh_nightly_factory.addStep(ShellCommand, name='make_rpm',
			   description='make_rpm',descriptionDone='make_rpm',
			   workdir=r'build', command=['make', 'dist-rpm'])
	linux_rh_nightly_factory.addStep(ShellCommand, workdir=r'build', name='cleanup_validate',
			   description=['cleanup_validate'], descriptionDone=['cleanup_validate'],
			   command=['rm', '-rf','/tmp/gridlabd/*'])
	# TODO: Add upload of RPM file to Sourceforge once the install has been independantly tested.

	return linux_rh_nightly_factory

def gen_rh_full_factory (svnURL):
	linux_rh_full_factory = factory.BuildFactory()
	linux_rh_full_factory.addStep(SVN, workdir=r'build',svnurl=svnURL,username='buildbot',password="V3hUTh2d")
	linux_rh_full_factory.addStep(ShellCommand, workdir=r'build', name='autoreconf',
			   description=['autoreconf'], descriptionDone=['autoreconf'],
			   command=['autoreconf', '-is'])
	linux_rh_full_factory.addStep(Configure, workdir=r'build')
	linux_rh_full_factory.addStep(Compile, workdir=r'build', command=['make', 'over'])

	return linux_rh_full_factory
