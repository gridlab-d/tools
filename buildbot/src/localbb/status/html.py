
from buildbot.status.web import base
from twisted.web import static, html
import os
import urllib

class OutputList(base.HtmlResource):
    def __init__(self, buildname, buildnumber, name='output'):
        base.HtmlResource.__init__(self)
        self.outputname = name
        self.buildname = buildname
        self.buildnumber = buildnumber
        self.emptychildren = 0
        self.title = 'Output of build %s'%self.buildnumber

    def body(self, request):
        prefix = '%s-%s-'%(self.buildnumber, self.outputname)
        files = [f[len(prefix):] for f in
                 os.listdir(self.buildname)
                 if f.startswith(prefix)]
        links = ['<li><a href="%s">%s</a></li>'%(
                 urllib.quote(request.childLink(f)),
                 html.escape(f)) for f in files]
        return '<ul>\n%s\n</ul>\n'%'\n'.join(links)

    def getChild(self, path, request):
        if not path and request.prepath:
            return self
        if path == self.outputname:
            request.prepath.pop()
            return self
        return static.File('%s/%s-%s-%s'%(self.buildname,
                                               self.buildnumber,
                                               self.outputname,
                                               path))

