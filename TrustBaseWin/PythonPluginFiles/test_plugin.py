#!/usr/bin/python
from trusthub_python import *

class testPlugin(TrustHubPlugin):
    def _init_(self):
        pass
    
    def initialize(self):
        sys.stdout = Python_Plugin_Logger()
        print "Test Python Plugin initialized!"
        return INIT_SUCCESS
    
    def query(self, host, port, raw_certs):
        print "Test Python Plugin query!"
        #cert_chain = convert_tls_certificates_to_x509_list(raw_certs)
        return RESPONSE_VALID
    
    def finalize(self):
        print "Test Python Plugin finalized!"
        return

myPlugin = testPlugin()
setPlugin(myPlugin)


# vim: tabstop=8 expandtab shiftwidth=4 softtabstop=4

