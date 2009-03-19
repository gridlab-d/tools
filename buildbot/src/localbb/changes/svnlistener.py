#!/usr/bin/env python

from twisted.internet.protocol import Factory,Protocol
from twisted.internet import reactor
from twisted.protocols.basic import LineOnlyReceiver
from twisted.python import log

from buildbot.changes.svnpoller import SVNPoller


class SVNListenerProtocol(LineOnlyReceiver):
    hostname = None
    peername = None
    delimiter = '\n'
    
    def connectionMade(self):
        peer = self.transport.getPeer()
        host = self.transport.getHost()
        self.peername = '%s:%s'%(peer.host, peer.port)
        self.hostname = '%s:%s'%(host.host, host.port)
        log.msg("CVSListenerProtocol: connection made from %s to %s"%
                 (self.peername, self.hostname))
        
    def lineReceived(self,line):
        line = line.rstrip('\r')
        #print "Data:%s:" % data
        if line == self.factory.signature:
            if self.factory.delayed and self.factory.delayed.active():
                self.factory.delayed.reset(self.factory.delay)
            elif self.factory.poller and not self.factory.poller.working:
                log.msg("SVNListenerProtocol: initiating SVN check on behalf of %s"%self.peername)
                self.factory.delayed = reactor.callLater(self.factory.delay,
                                               self.factory.poller.checkcvs)
        else:
            if len(line) > len(self.factory.signature):
                line = '%s...'%line[:len(self.factory.signature) + 10]
            log.msg("SVNListenerProtocol: invalid signature sent from %s: %s"%
                     (self.peername, line))
        self.transport.loseConnection()
           
        
class SVNListenerFactory(Factory):
    protocol = SVNListenerProtocol
    delayed = None
    
    def __init__(self, poller, signature, commitStableTimer=30):
        assert isinstance(poller, SVNPoller)
        self.poller = poller
        self.signature = signature
        self.delay = commitStableTimer


def startSVNListener(poller,signature,port):
    reactor.listenTCP(port,SVNListenerFactory(poller,signature))
    
