#!/usr/bin/python
import logging
import sys
def temp_errlog(exctype, value, tb):
    log.warning('My Error Information')
    log.warning('Type: '+ str(exctype))
    log.warning('Value: '+ str(value))
    log.warning('Traceback:'+ str(tb))

import os


sys.path.append(os.path.dirname(sys.path[0]))
from trusthub_python import *
import socks
import socket
import ssl
from OpenSSL import crypto
import json
import hashlib
import time
import random
from toast_notification import ToastNotifier
import threading

logging.basicConfig(filename=os.path.join(sys.path[0],'MITM.log'),level=logging.DEBUG)

class mitmPlugin(TrustHubPlugin):
    proxy_cert = 'mitmproxy-ca-cert.pem' #PEM format certificate
    notification_icon = "TrustBaseIcon.ico"
    visited_sites = {}
    def _init_(self):
        pass
    
    def initialize(self):
        self.toaster = ToastNotifier(os.path.join(sys.path[0],self.notification_icon))
        self.timer = None
        try:
            mitm_cert_file = open(os.path.join(sys.path[0],self.proxy_cert), "rb").read()
            mitm_cert = crypto.load_certificate(crypto.FILETYPE_PEM, mitm_cert_file)
            self.mitm_digest = mitm_cert.digest("sha256")
        except IOError:
            logging.exception("Error reading Cert File")
            return INIT_FAIL
        except:
            logging.exception("OpenSSL crypto Error")
            return INIT_FAIL
        logging.info("Test Python Plugin initialized!")
        
        return INIT_SUCCESS
    
    def query(self, host, port, raw_certs):
        logging.info("queried! checking " + str(host) + " certificate for MITM")
        cert_chain = convert_tls_certificates_to_x509_list(raw_certs)
        for cert in cert_chain: 
            if self.mitm_digest == cert.digest("sha256"):
                logging.info(str(host) + ' is being MITM by loaded cert')
                if host not in self.visited_sites or self.visited_sites[host] <= time.time():
                    self.visited_sites[host] = time.time()+(8*60*60) # 8 hours
                    self.toaster.show_toast(host)
                return RESPONSE_VALID
        return RESPONSE_ABSTAIN
    
    def finalize(self):
        logging.info("MITM Python Plugin finalized!")
        self.toaster.endProcess()
        return

myPlugin = mitmPlugin()
setPlugin(myPlugin)

# vim: tabstop=8 expandtab shiftwidth=4 softtabstop=4

