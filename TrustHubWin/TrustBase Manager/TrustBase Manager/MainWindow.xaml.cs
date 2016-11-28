using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Navigation;
using System.Windows.Shapes;
using System.Management;
using System.Runtime.InteropServices;
using System.Reflection;
using System.Management.Instrumentation;
using System.Data;
using System.Diagnostics;
using System.Windows.Forms;
using System.Collections.Specialized;

namespace TrustBase_Manager
{
    /// <summary>
    /// Interaction logic for TrustBaseLog GUI
    /// </summary>
    public partial class MainWindow : Window
    {
        System.Collections.Generic.Dictionary<string, DateTime> deduplicator = new Dictionary<string, DateTime>();

        private System.Windows.Forms.ContextMenu trayIconContextMenu;

        private string DELIMETER = "|";

        [DllImport("user32.dll")]
        public static extern int SendMessage(IntPtr hWnd, uint Msg, int wParam, int lParam);

        /**
         * Model for the DataGrid display
         **/
        private DataTable events = new DataTable();

        /**
         * Utility to read events from the Windows Event Log
         **/
        EventLog myLog = new EventLog();

        /**
         * Tray Icon for TrustBase
         **/
        private NotifyIcon trayIcon;

        /**
         * Represents whether the enable notifications box is checked
         **/
        private bool enableNotifications = false;

        /**
         * Main Window Constructor
         * 
         * Called on application run.
         * Defaults to invisible window with no taskbar presence
         * Assigns a log event handler for the "TrustBaseLog"
         * Sets up a datagrid display of log information
         * Locks down datagrid so it cannot be resized or rearranged
         * Adds a tray icon
         **/
        public MainWindow()
        {

            //======================
            // DEFAULT TO INVISIBLE
            //======================

            this.ShowInTaskbar = false;
            this.Visibility = Visibility.Hidden;
            //this.ResizeMode = ResizeMode.CanMinimize;

            InitializeComponent();
            this.ResizeMode = ResizeMode.CanResize;
            //=========================
            //SET UP LOG EVENT HANDLER
            //=========================

            myLog.Log = "TrustBaseLog";
            myLog.EnableRaisingEvents = true;
            myLog.EntryWritten += notify;
            myLog.Source = "TrustBase Manager";

            //=========================
            // SET UP LOG DATA DISPLAY
            //=========================
            DataColumn TimeColumn = new DataColumn("Time");
            DataColumn ProcessIDColumn = new DataColumn("ProcessID");
            DataColumn ErrorColumn = new DataColumn("Error");

            events.Columns.Add(TimeColumn);
            events.Columns.Add(ProcessIDColumn);
            events.Columns.Add(ErrorColumn);

            events.Rows.Clear();
            foreach (EventLogEntry entry in myLog.Entries)
            {
                DataRow TBevent = events.NewRow();
                TBevent["Time"] = entry.TimeGenerated.ToShortDateString() + " - " + entry.TimeGenerated.ToLongTimeString();
                TBevent["ProcessID"] = entry.Source;
                TBevent["Error"] = entry.Message;
                events.Rows.Add(TBevent);
            }

            dataGrid.DataContext = events.DefaultView;
            //dataGrid.AutoGenerateColumns = true;
            dataGrid.CanUserAddRows = false;
            dataGrid.CanUserReorderColumns = false;
            dataGrid.CanUserResizeColumns = false;
            dataGrid.RowHeaderWidth = 0;

            //=================
            //SET UP TRAY ICON
            //=================

            this.trayIcon = new System.Windows.Forms.NotifyIcon();
            this.trayIcon.Icon = new System.Drawing.Icon(@"C:\\Users\\Joshua Reynolds\\Pictures\\TrustBaseIcon.ico");
            this.trayIcon.Text = "TrustBase";
            this.trayIcon.Visible = true;
            this.trayIcon.Click += this.TrayIconClicked;

            this.trayIconContextMenu = new System.Windows.Forms.ContextMenu();

            this.trayIconContextMenu.MenuItems.Add("Open TrustBase Manager");
            this.trayIconContextMenu.MenuItems.Add("Allow/Block TrustBase notifications");

            this.trayIcon.ContextMenu = this.trayIconContextMenu;

            //====================
            //ADD EVENT LISTENERS
            //====================
            this.Closing += this.closing;
            this.checkBox.Click += CheckBox_Click;

            //====================
            //DISPLAY UAC SHIELD
            //====================

        }

        private void CheckBox_Click(object sender, RoutedEventArgs e)
        {
            System.Windows.Controls.CheckBox box = (System.Windows.Controls.CheckBox) e.Source;
            this.enableNotifications = box.IsChecked.Value;
        }

        /**
         * Destructor for Main Window.
         * 
         * Disables events on the windows log.
         * Hides and disposes of the Tray Icon.
         *  (otherwise a ghost icon remains in the
         *  taskbar).
         **/
        ~MainWindow()
        {
            myLog.EnableRaisingEvents = false;
            trayIcon.Dispose();
        }

        /**
         * Manage Exceptions Button Click Event Handler
         * 
         * Requires Elevated permisison from UAC
         * NOT IMPLEMENTED YET
         **/
        private void Manage_Exceptions_Button_Click(object sender, RoutedEventArgs e)
        {
            myLog.WriteEntry("chrome.exe|An unauthorized user person tried to add TrustBase Exceptions");
        }

        /**
         * Refresh the Log
         * 
         * Clears the log and repopulates it
         * 
         **/
        private void Refresh_Button_Click(object sender, RoutedEventArgs e)
        {
            events.Rows.Clear();
            foreach (EventLogEntry entry in myLog.Entries)
            {
                string message;
                string process;
                if(entry.Message.Contains(DELIMETER))
                {
                    int delimeterPosition = entry.Message.IndexOf(DELIMETER);
                    message = entry.Message.Substring(delimeterPosition + 1);
                    process = entry.Message.Substring(0, delimeterPosition);
                }
                else
                {
                    process = "Unspecified";
                    message = entry.Message;
                }
                
                

                DataRow TBevent = events.NewRow();
                TBevent["Time"] = entry.TimeGenerated.ToShortDateString() + " - " + entry.TimeGenerated.ToLongTimeString();
                TBevent["ProcessID"] = process;
                TBevent["Error"] = message;
                events.Rows.Add(TBevent);
            }
        }




        /**
        * Notify the User
        *    
        * Do nothing if notifications are disabled
        * Grab most recent event
        * Parse Event Data
        * Decide whether to notify (Is this a recent repeat?)
        * 
        * 
        * //TODO Persist nofification enabled state 
        * 
        **/
        private void notify(object sender, EntryWrittenEventArgs e)
        {            
            if (!this.enableNotifications)
            {
                return;
            }


            //============================
            // GET WRITTEN EVENT
            //============================
            EventLogEntry most_recent = null;

            foreach (EventLogEntry entry in myLog.Entries)
            {
                if(System.DateTime.Now.Ticks - entry.TimeWritten.Ticks < TimeSpan.TicksPerSecond * 8)
                {
                    most_recent = entry;
                }
            }

            //============================
            // GENERATE NOTIFICATION TEXT
            //============================
            string message;
            ToolTipIcon icon;

            if (most_recent.Message.Contains(DELIMETER))
            {
                int delimeterPosition = most_recent.Message.IndexOf(DELIMETER);
                message = most_recent.Message.Substring(delimeterPosition + 1);
                string process = most_recent.Message.Substring(0, delimeterPosition);
                message = "Trustbase blocked a connection made by " + process + ". " + message;
                icon = ToolTipIcon.Error;
            }
            else
            {
                message = most_recent.Message;
                icon = ToolTipIcon.Info;
            }

            //=========================================================
            //DO NOT NOTIFY FOR THE SAME PROGRAM BLOCKED SEVERAL TIMES
            //=========================================================
            if (!deduplicator.Keys.Contains<string>(most_recent.Source))
            {
                deduplicator.Add(most_recent.Source, DateTime.Now);
                trayIcon.ShowBalloonTip(9000, "TrustBase", message, icon);
            }
            else
            {
                DateTime last_notification;
                deduplicator.TryGetValue(most_recent.Source, out last_notification);
                if(DateTime.Now.Ticks - last_notification.Ticks > TimeSpan.TicksPerMinute)
                {
                    deduplicator.Remove(most_recent.Source);
                    deduplicator.Add(most_recent.Source, DateTime.Now);
                    trayIcon.ShowBalloonTip(9000, "TrustBase", message, icon);
                }
            }
            
        }

        /**
         * Make the GUI disappear.
         * 
         * Leaves a Notification Area Icon
         * 
         * */
        private void MakeInvisible()
        {
            this.events.Rows.Clear();
            this.ShowInTaskbar = false;
            this.Visibility = Visibility.Collapsed;
        }

        /**
         * Make the Log GUI appear
         * 
         * 
         * */
        private void MakeVisible()
        {
            this.ShowInTaskbar = true;
            this.Visibility = Visibility.Visible;
            //this.ResizeMode = ResizeMode.CanMinimize;
            this.Refresh_Button_Click(null, null);
            this.Focus();
        }


        /**
         * TrayIconClicked Event
         * 
         * Make the GUI visible
         * 
         * */
        private void TrayIconClicked(object sender, EventArgs e)
        {
            this.MakeVisible();
        }



        /**
         * Closing Event
         * 
         * This happens when the user clicks the X button on the window
         * The close command is revoked and the window becomes invisible
         * 
         **/
        private void closing(object sender, System.ComponentModel.CancelEventArgs e)
        {
            this.MakeInvisible();
            e.Cancel = true;
        }

        /**
         * Parse Event 
         * 
         * Parses a Trustbase event log to extract relevant data
         * 
         * 
         **/
        private TrustBaseEvent parseEvent(EventLogEntry entry)
        {
            var result = new TrustBaseEvent();
            result.Exception = entry.Message;
            result.GUID = new Guid();
            //result.ProcessName = result.GUID;
            result.TimeWritten = entry.TimeWritten;
            return result;
        }

        /**
         * 
         * Relevant Data from a TrustBaseLog event entry
         * 
         **/
        private struct TrustBaseEvent
        {
            public string Exception;
            public Guid GUID;
            public string ProcessName;
            public DateTime TimeWritten;
        }

        //[Flags]
        //public enum SHGSI : uint
        //{
        //    SHGSI_ICONLOCATION = 0,
        //    SHGSI_ICON = 0x000000100,
        //    SHGSI_SYSICONINDEX = 0x000004000,
        //    SHGSI_LINKOVERLAY = 0x000008000,
        //    SHGSI_SELECTED = 0x000010000,
        //    SHGSI_LARGEICON = 0x000000000,
        //    SHGSI_SMALLICON = 0x000000001,
        //    SHGSI_SHELLICONSIZE = 0x000000004
        //}

        //[StructLayoutAttribute(LayoutKind.Sequential, CharSet = CharSet.Unicode)]
        //public struct SHSTOCKICONINFO
        //{
        //    public UInt32 cbSize;
        //    public IntPtr hIcon;
        //    public Int32 iSysIconIndex;
        //    public Int32 iIcon;
        //    [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 260)]
        //    public string szPath;
        //}

        //[DllImport("Shell32.dll", SetLastError = false)]
        //public static extern Int32 SHGetStockIconInfo(SHSTOCKICONID siid, SHGSI uFlags, ref SHSTOCKICONINFO psii);

        //private delegate bool EnumWindowsProc(IntPtr hWnd, IntPtr lParam);

        //[DllImport("user32.dll")]
        //public static extern bool GetWindowRect(IntPtr hWnd, out RECT lpRect);

        //[DllImport("user32.dll", CharSet = CharSet.Auto, SetLastError = true)]
        //private static extern bool EnumWindows(EnumWindowsProc callback, IntPtr extraData);

        //[DllImport("user32.dll")]
        //static extern IntPtr GetForegroundWindow();

        //[StructLayout(LayoutKind.Sequential)]
        //public struct RECT
        //{
        //    public int Left;
        //    public int Top;
        //    public int Right;
        //    public int Bottom;
        //}

        //public enum SHSTOCKICONID : uint
        //{
        //    SIID_DOCNOASSOC = 0,
        //    SIID_DOCASSOC = 1,
        //    SIID_APPLICATION = 2,
        //    SIID_FOLDER = 3,
        //    SIID_FOLDEROPEN = 4,
        //    SIID_DRIVE525 = 5,
        //    SIID_DRIVE35 = 6,
        //    SIID_DRIVEREMOVE = 7,
        //    SIID_DRIVEFIXED = 8,
        //    SIID_DRIVENET = 9,
        //    SIID_DRIVENETDISABLED = 10,
        //    SIID_DRIVECD = 11,
        //    SIID_DRIVERAM = 12,
        //    SIID_WORLD = 13,
        //    SIID_SERVER = 15,
        //    SIID_PRINTER = 16,
        //    SIID_MYNETWORK = 17,
        //    SIID_FIND = 22,
        //    SIID_HELP = 23,
        //    SIID_SHARE = 28,
        //    SIID_LINK = 29,
        //    SIID_SLOWFILE = 30,
        //    SIID_RECYCLER = 31,
        //    SIID_RECYCLERFULL = 32,
        //    SIID_MEDIACDAUDIO = 40,
        //    SIID_LOCK = 47,
        //    SIID_AUTOLIST = 49,
        //    SIID_PRINTERNET = 50,
        //    SIID_SERVERSHARE = 51,
        //    SIID_PRINTERFAX = 52,
        //    SIID_PRINTERFAXNET = 53,
        //    SIID_PRINTERFILE = 54,
        //    SIID_STACK = 55,
        //    SIID_MEDIASVCD = 56,
        //    SIID_STUFFEDFOLDER = 57,
        //    SIID_DRIVEUNKNOWN = 58,
        //    SIID_DRIVEDVD = 59,
        //    SIID_MEDIADVD = 60,
        //    SIID_MEDIADVDRAM = 61,
        //    SIID_MEDIADVDRW = 62,
        //    SIID_MEDIADVDR = 63,
        //    SIID_MEDIADVDROM = 64,
        //    SIID_MEDIACDAUDIOPLUS = 65,
        //    SIID_MEDIACDRW = 66,
        //    SIID_MEDIACDR = 67,
        //    SIID_MEDIACDBURN = 68,
        //    SIID_MEDIABLANKCD = 69,
        //    SIID_MEDIACDROM = 70,
        //    SIID_AUDIOFILES = 71,
        //    SIID_IMAGEFILES = 72,
        //    SIID_VIDEOFILES = 73,
        //    SIID_MIXEDFILES = 74,
        //    SIID_FOLDERBACK = 75,
        //    SIID_FOLDERFRONT = 76,
        //    SIID_SHIELD = 77,
        //    SIID_WARNING = 78,
        //    SIID_INFO = 79,
        //    SIID_ERROR = 80,
        //    SIID_KEY = 81,
        //    SIID_SOFTWARE = 82,
        //    SIID_RENAME = 83,
        //    SIID_DELETE = 84,
        //    SIID_MEDIAAUDIODVD = 85,
        //    SIID_MEDIAMOVIEDVD = 86,
        //    SIID_MEDIAENHANCEDCD = 87,
        //    SIID_MEDIAENHANCEDDVD = 88,
        //    SIID_MEDIAHDDVD = 89,
        //    SIID_MEDIABLURAY = 90,
        //    SIID_MEDIAVCD = 91,
        //    SIID_MEDIADVDPLUSR = 92,
        //    SIID_MEDIADVDPLUSRW = 93,
        //    SIID_DESKTOPPC = 94,
        //    SIID_MOBILEPC = 95,
        //    SIID_USERS = 96,
        //    SIID_MEDIASMARTMEDIA = 97,
        //    SIID_MEDIACOMPACTFLASH = 98,
        //    SIID_DEVICECELLPHONE = 99,
        //    SIID_DEVICECAMERA = 100,
        //    SIID_DEVICEVIDEOCAMERA = 101,
        //    SIID_DEVICEAUDIOPLAYER = 102,
        //    SIID_NETWORKCONNECT = 103,
        //    SIID_INTERNET = 104,
        //    SIID_ZIPFILE = 105,
        //    SIID_SETTINGS = 106,
        //    SIID_DRIVEHDDVD = 132,
        //    SIID_DRIVEBD = 133,
        //    SIID_MEDIAHDDVDROM = 134,
        //    SIID_MEDIAHDDVDR = 135,
        //    SIID_MEDIAHDDVDRAM = 136,
        //    SIID_MEDIABDROM = 137,
        //    SIID_MEDIABDR = 138,
        //    SIID_MEDIABDRE = 139,
        //    SIID_CLUSTEREDDRIVE = 140,
        //    SIID_MAX_ICONS = 175
        //}

        ////MAYBE WE CAN ASSOCIATE
        //[DllImport("msi.dll", CharSet = CharSet.Unicode)]
        //static extern Int32 MsiGetProductInfo(string product, string property,
        //[Out] StringBuilder valueBuf, ref Int32 len);

        //[DllImport("msi.dll", SetLastError = true)]
        //static extern int MsiEnumProducts(int iProductIndex,
        //    StringBuilder lpProductBuf);

        //static void thing(string[] args)
        //{
        //    StringBuilder sbProductCode = new StringBuilder(39);
        //    int iIdx = 0;

        //    while (0 == MsiEnumProducts(iIdx++, sbProductCode))
        //    {
        //        Int32 productNameLen = 512;
        //        StringBuilder sbProductName = new StringBuilder(productNameLen);

        //        MsiGetProductInfo(sbProductCode.ToString(),
        //            "ProductName", sbProductName, ref productNameLen);

        //        //if (sbProductName.ToString().Contains("Visual Studio"))
        //        //{
        //        //    Int32 installDirLen = 1024;
        //        //    StringBuilder sbInstallDir = new StringBuilder(installDirLen);

        //        //    MsiGetProductInfo(sbProductCode.ToString(),
        //        //        "InstallLocation", sbInstallDir, ref installDirLen);

        //        //    //Console.WriteLine("ProductName {0}: {1}",
        //        //    //    sbProductName, sbInstallDir);
        //        //}
        //    }
        //}

        //private void OverLayActiveWindow()
        //{
        //    //System.Threading.Thread.Sleep(5000);

        //    //int processID = 2452;

        //    // Process process = Process.GetProcessById(processID);
        //    //Process thisProcess = process;// Process.GetCurrentProcess();
        //    System.IntPtr main_window = GetForegroundWindow(); //thisProcess.MainWindowHandle;
        //    Size windowsize = GetControlSize(main_window);
        //    Point upperLeft = GetControlLocation(main_window);

        //    Window overlay = new Window();
        //    RECT windowRect;
        //    GetWindowRect(main_window, out windowRect);
        //    overlay.Left = windowRect.Left;
        //    overlay.Top = windowRect.Top;
        //    overlay.Title = "TRUSTBASE ALERT!";
        //    overlay.ShowInTaskbar = true;
        //    overlay.Height = windowsize.Height;
        //    overlay.Width = windowsize.Width;
        //    overlay.Content = new ContentControl();
        //    overlay.Background = Brushes.Red;
        //    overlay.Show();
        //    overlay.Topmost = true;
        //    overlay.Focus();
        //}

        //public static Size GetControlSize(IntPtr hWnd)
        //{
        //    RECT pRect;
        //    Size cSize = new Size();
        //    // get coordinates relative to window
        //    GetWindowRect(hWnd, out pRect);

        //    cSize.Width = pRect.Right - pRect.Left;
        //    cSize.Height = pRect.Bottom - pRect.Top;

        //    return cSize;
        //}

        //public static Point GetControlLocation(IntPtr hWnd)
        //{
        //    RECT pRect;
        //    Point upperLeft = new Point();
        //    // get coordinates relative to window
        //    GetWindowRect(hWnd, out pRect);

        //    upperLeft.X = pRect.Left;
        //    upperLeft.Y = pRect.Top;

        //    return upperLeft;
        //}

    }
}
