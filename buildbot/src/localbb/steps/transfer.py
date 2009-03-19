
from buildbot.steps.transfer import FileUpload, SUCCESS
import urllib

class FileUploadOutput(FileUpload):
    description = ['uploading']
    descriptionDone = ['output']

    def __init__(self, *args, **kwargs):
        FileUpload.__init__(self, *args, **kwargs)
        self._dest = self.masterdest
        self._descDone = kwargs.get('descriptionDone')

    def start(self):
        self.masterdest = '%s/%s-output-%s'%(self.getProperty('buildername'),
                                             self.getProperty('buildnumber'),
                                             self._dest)
        return FileUpload.start(self)

    def finished(self, result):
        if self._descDone is None:
            self.descriptionDone = [
                    '<a href="builders/%s/builds/%s/output">output</a>'%(
                         urllib.quote(self.getProperty('buildername'), safe=''),
                         self.getProperty('buildnumber'))]
        self.step_status.setText(self.descriptionDone)
        if not self.cmd.rc:
            self.step_status.addURL(self._dest,
                    'builders/%s/builds/%s/output/%s'%(
                         urllib.quote(self.getProperty('buildername'), safe=''),
                         self.getProperty('buildnumber'),
                         self._dest))
        return FileUpload.finished(self, result)

