
from twisted.application import service
from buildbot.slave.bot import BuildSlave

basedir = r'D:\temp\gridlab_svn\tools\buildbot2\projects\Gridlab-D\slaves\win32-full'
buildmaster_host = 'lilith.pnl.gov'
port = 9989
slavename = 'ego.pnl.gov'
passwd = 'startmeup'
keepalive = 600
usepty = 1
umask = None

application = service.Application('buildslave')
s = BuildSlave(buildmaster_host, port, slavename, passwd, basedir,
               keepalive, usepty, umask=umask)
s.setServiceParent(application)

