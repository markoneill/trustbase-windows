from __future__ import absolute_import
from __future__ import print_function
from __future__ import unicode_literals

__all__ = ['ToastNotifier']

# #############################################################################
# ########## Libraries #############
# ##################################
# standard library
import logging
import threading
import os,  ctypes
from pkg_resources import Requirement
from pkg_resources import resource_filename
from Queue import Empty
from time import sleep

# 3rd party modules
from win32api import GetModuleHandle
from win32api import PostQuitMessage
from win32con import COLOR_WINDOW
from win32con import CS_HREDRAW
from win32con import CS_VREDRAW
from win32con import CW_USEDEFAULT
from win32con import IDI_APPLICATION
from win32con import IMAGE_ICON
from win32con import LR_DEFAULTSIZE
from win32con import LR_LOADFROMFILE
from win32con import WM_LBUTTONUP
from win32con import WM_RBUTTONUP
from win32con import WM_DESTROY
from win32con import WM_USER
from win32con import WS_OVERLAPPED
from win32con import WS_SYSMENU
from win32gui import CreateWindow
from win32gui import DestroyWindow
from win32gui import LoadIcon
from win32gui import LoadImage
from win32gui import NIF_ICON
from win32gui import NIIF_ICON_MASK
from win32gui import NIF_INFO
from win32gui import NIF_MESSAGE
from win32gui import NIF_TIP
from win32gui import NIM_ADD
from win32gui import NIM_DELETE
from win32gui import NIM_MODIFY
from win32gui import PumpWaitingMessages
from win32gui import RegisterClass
from win32gui import UnregisterClass
from win32gui import Shell_NotifyIcon
from win32gui import UpdateWindow
from win32gui import WNDCLASS

from multiprocessing import Queue, Process

# ############################################################################
# ########### Prevate functions ####
# ##################################

def startSubprocess(icon_path, MessageQ):
    n = NotificationSubprocess(icon_path, MessageQ)
    n.event_loop()

# ############################################################################
# ########### Classes ##############
# ##################################


class ToastNotifier(object):
    '''
    Specifyes an interface for sowing a Windows tost
    '''
    def __init__(self, IconPath):
        self.msgQ = Queue()
        self._thread1 = threading.Thread(target=startSubprocess, args=(IconPath, self.msgQ,))
        self._thread1.start()
    
    def show_toast(self,ToastMessage):
        self.msgQ.put(ToastMessage)
        
    def endProcess(self):
        self.msgQ.put("kys") 
    
class Mbox(object):
    def __init__(self):
        self.visible = 0
        
    def show(self, title, text, style):
        if self.visible:
            self.destroy()
        self.window = ctypes.windll.user32.MessageBoxW(0,text,title,style)
        self.visible = 1
    
    def destroy(self):
        self.visible = 0


class NotificationSubprocess(object):
    '''Create a Windows 10  toast notification.
    adapted from: https://github.com/jithurjacob/Windows-10-Toast-Notifications
    '''
    def __init__(self, icon_path, msg_q):
        """Initialize."""
        self.visible = 0
        self.log = []
        self._thread = None
        self.msg_q = msg_q
        self.message_box = Mbox()
        message_map = {
            WM_DESTROY: self.onDestroy,
            WM_USER+20 : self.onTaskbarNotify,
        }
        wc = WNDCLASS()
        hinst = wc.hInstance = GetModuleHandle(None)
        icon_flags = LR_LOADFROMFILE | LR_DEFAULTSIZE
        try:
            self.hicon = LoadImage(hinst,  os.path.realpath(icon_path),
                              IMAGE_ICON, 0, 0, icon_flags)
        except Exception as e:
            print(str(e))
            self.hicon = LoadIcon(0, IDI_APPLICATION)
        wc.lpszClassName = str("Trustbase_notification") #lol
        wc.style = CS_VREDRAW | CS_HREDRAW
        #wc.hCursor = win32gui.LoadCursor( 0, win32con.IDC_ARROW )
        wc.hbrBackground = COLOR_WINDOW
        wc.lpfnWndProc = message_map # could also specify a wndproc.
        classAtom = RegisterClass(wc)
        # Create the Window.
        style = WS_OVERLAPPED | WS_SYSMENU
        self.hwnd = CreateWindow( classAtom, "MITM_alert", style, \
                    0, 0, CW_USEDEFAULT, CW_USEDEFAULT, \
                    0, 0, hinst, None)
        UpdateWindow(self.hwnd)
        self.notification_id = 1
        self.show()
            
    def show(self, nid =None):
        """Display the taskbar icon"""
        flags = NIF_ICON | NIF_MESSAGE
        if nid is None:
            nid = (self.hwnd, 0, flags, WM_USER+20, self.hicon)
        if self.visible:
            self.hide()
        Shell_NotifyIcon(NIM_ADD, nid)
        self.visible = 1
		
    def hide(self):
        """Hide the taskbar icon"""
        if self.visible:
            nid = (self.hwnd, 0)
            Shell_NotifyIcon(NIM_DELETE, nid)
        self.visible = 0

    def event_loop(self):
        while(1):
            try:
                host = self.msg_q.get(False)     # non-blocking reed from queue throws queue.Empty
                if host == "kys":
                    return
                self.show_toast(host)
            except Empty:
                pass
            #PumpWaitingMessages()
            
    def show_toast(self, host= "this website"):  #this function now only logs the websites for the user to see
        self.log.insert(0,host)
        if len(self.log) > 20:
            del self.log[20:]
        #flags = NIF_ICON | NIF_MESSAGE | NIF_INFO
        #nid = (self.hwnd,0, flags, WM_USER+20, self.hicon, "tool tip", "Your traffic to {} is being monitored by your employer\nClick on the system tray icon to see a complete list".format(host), 9, "TrustBase", NIIF_ICON_MASK)
        #self.show(nid)
        
    def onDestroy(self):
        self.message_box.destroy()
        self.hide()
        win32gui.PostQuitMessage(0) 

    def onTaskbarNotify(self, hwnd, msg, wparam, lparam):
        if lparam == WM_LBUTTONUP:
            self.show_history()
        return 1
		
    def show_history(self):
        if self._thread != None and self._thread.is_alive():
            pass # learn how to close the active message box
        if len(self.log) == 0:
            self._thread = threading.Thread(target =self.message_box.show, args=("TrustBase Log", 'No traffic has been monitored by your employer this session', 0))
        else:
            reverse = self.log[::-1]
            self._thread = threading.Thread(target =self.message_box.show, args=("TrustBase Log", 'Your traffic to the following sites has been monitored by your employer:\n- '+'\n- '.join(reverse), 0))
        self._thread.start()
'''        
def main():
    n = ToastNotifier("TrustBaseIcon.ico")
    for i in range(1, 5):
        n.showTost("site " + str(i))
        sleep(2)

    
if __name__ == '__main__':
    #freeze_support()
    main()'''
