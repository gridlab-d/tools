
from twisted.application import service
from buildbot.master import BuildMaster
from localbb.sshagent import ServiceWrapper

from os import environ
environ['CVS_RSH'] = 'ssh'

basedir = r'/home/buildbot/projects/Gridlab-D/master'
configfile = r'master.cfg'

application = service.Application('buildmaster')
m = BuildMaster(basedir, configfile)
m = ServiceWrapper(m)
m.setServiceParent(application)

