
import os
from subprocess import Popen, PIPE, STDOUT

def ServiceWrapper(service, sshkey=None):
	cls = service.__class__

	class ServiceWrapper(cls):
		def startSSHAgent(self):
			p = Popen(['ssh-agent', '-c'], stdout=PIPE, stderr=PIPE)
			p.wait()
			if p.returncode:
				raise Exception('ssh-agent: error %d: %s'%(p.returncode, p.stderr.read()))
			for line in p.stdout:
				if line.startswith('setenv'):
					parts = line.split()
					os.environ[parts[1]] = parts[2][:-1]
			args = ['ssh-add']
			if sshkey is not None:
				args.append(str(sshkey))
			p = Popen(args, stdout=PIPE, stderr=STDOUT)
			p.wait()
			if p.returncode != 0:
				raise Exception('ssh-add: error %d: %s'%(p.returncode, p.stdout.read()))

		def stopSSHAgent(self):
			p = Popen(['ssh-agent', '-k'], stdout=PIPE, stderr=STDOUT)
			p.wait()

		def startService(self):
			self.startSSHAgent()
			cls.startService(self)

		def stopService(self):
			cls.stopService(self)
			self.stopSSHAgent()

	service.__class__ = ServiceWrapper
	return service

