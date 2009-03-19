
from twisted.application import service
from buildbot.slave.bot import BuildSlave
from localbb.sshagent import ServiceWrapper

from os import environ
environ['CVS_RSH'] = 'ssh'

basedir = r'/home/buildbot/projects/Gridlab-D/slaves/linux-full'
buildmaster_host = 'localhost'
port = 9989
slavename = 'lilith.pnl.gov'
passwd = 'startmeup'
keepalive = 600
usepty = 1
umask = None

application = service.Application('buildslave')
s = BuildSlave(buildmaster_host, port, slavename, passwd, basedir,
               keepalive, usepty, umask=umask)
s = ServiceWrapper(s)
s.setServiceParent(application)

