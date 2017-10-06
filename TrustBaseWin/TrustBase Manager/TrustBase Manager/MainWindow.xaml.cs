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
using System.IO;
using System.Reflection;
using System.Management.Instrumentation;
using System.Data;
using System.Diagnostics;
using System.Windows.Forms;
using System.Collections.Specialized;
using System.Security.Principal;

namespace TrustBase_Manager
{


    /// <summary>
    /// Interaction logic for TrustBaseLog GUI
    /// </summary>
    public partial class MainWindow : Window
    {
        const int RESPONSE_ERROR = -1;
        const int RESPONSE_VALID = 1;
        const int RESPONSE_INVALID = 0;

        static bool IsRunningAsAdmin
        {
            get
            {
                return WindowsIdentity.GetCurrent().Owner
                  .IsWellKnown(WellKnownSidType.BuiltinAdministratorsSid);
            }
        }

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
        private bool enableBlockNotifications = false;
        private bool enableAcceptNotifications = false;

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
            DataColumn ProcessColumn = new DataColumn("Process");
            DataColumn HostnameColumn = new DataColumn("Hostname");
            DataColumn MessageColumn = new DataColumn("Message");

            events.Columns.Add(TimeColumn);
            events.Columns.Add(ProcessColumn);
            events.Columns.Add(HostnameColumn);
            events.Columns.Add(MessageColumn);

            events.Rows.Clear();
            refreshLog();

            dataGrid.DataContext = events.DefaultView;
            //dataGrid.AutoGenerateColumns = true;
            dataGrid.CanUserAddRows = false;
            dataGrid.CanUserReorderColumns = false;
            dataGrid.CanUserResizeColumns = false;
            dataGrid.RowHeaderWidth = 0;

            //=================
            //NOTIFICATION CHECK
            //=================
            enableBlockNotifications = (bool)this.notifyCheckBox.IsChecked;
            enableAcceptNotifications = (bool)this.notifyAcceptCheckBox.IsChecked;

            //=================
            //SET UP TRAY ICON
            //=================

            this.trayIcon = new System.Windows.Forms.NotifyIcon();
            this.trayIcon.Icon = Properties.Resources.TrustBaseIcon;
            this.trayIcon.Text = "TrustBase";
            this.trayIcon.Visible = true;
            this.trayIcon.Click += this.TrayIconClicked;

            this.trayIconContextMenu = new System.Windows.Forms.ContextMenu();

            this.trayIconContextMenu.MenuItems.Add("Open TrustBase Manager");
            //this.trayIconContextMenu.MenuItems.Add("Allow/Block TrustBase notifications");

            this.trayIcon.ContextMenu = this.trayIconContextMenu;

            //====================
            //ADD EVENT LISTENERS
            //====================
            this.Closing += this.closing;
            this.notifyCheckBox.Click += CheckBox_Click;

            //====================
            //CHECK IF RUNNING AS ADMIN
            //====================
            if(IsRunningAsAdmin)
            {
                this.SimulateLogMenuItem.IsEnabled = true;
                this.ClearLogMenuItem.IsEnabled = true;

                //check if log exists. Creating this can only be done as admin.
                if (!EventLog.SourceExists("TrustBase Manager"))
                    EventLog.CreateEventSource("TrustBase Manager", "TrustBaseLog");
            }
            else
            {
                this.SimulateLogMenuItem.IsEnabled = false;
                this.ClearLogMenuItem.IsEnabled = false;
            }

            //====================
            //DISPLAY UAC SHIELD
            //====================

        }

        private void CheckBox_Click(object sender, RoutedEventArgs e)
        {
            System.Windows.Controls.CheckBox box = (System.Windows.Controls.CheckBox) e.Source;
            this.enableBlockNotifications = box.IsChecked.Value;
        }

        private void notifyAcceptCheckBox_Click(object sender, RoutedEventArgs e)
        {
            System.Windows.Controls.CheckBox box = (System.Windows.Controls.CheckBox)e.Source;
            this.enableAcceptNotifications = box.IsChecked.Value;
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
        //private void Manage_Exceptions_Button_Click(object sender, RoutedEventArgs e)
        //{
        //}

        /**
         * Refresh button clicked
         * 
         **/
        private void Refresh_Button_Click(object sender, RoutedEventArgs e)
        {
            refreshLog();
        }
        /**
         * Refresh menu item clicked
         * 
         **/
        private void RefreshLogMenuItem_Click(object sender, RoutedEventArgs e)
        {
            refreshLog();
        }
        /**
         * simulate log entry menu item clicked
         * Requires Elevated permisison from UAC
         **/
        private void SimulateLogMenuItem_Click(object sender, RoutedEventArgs e)
        {
            myLog.WriteEntry("chrome.exe|fake.com|TrustBase blocked a connection",EventLogEntryType.Error,0, RESPONSE_INVALID);
        }
        /**
         * clear log menu item clicked
         * Requires Elevated permisison from UAC
         **/
        private void ClearLogMenuItem_Click(object sender, RoutedEventArgs e)
        {
            myLog.Clear();
            deduplicator.Clear();
            refreshLog();
        }

        /**
         * Refresh the Log
         * 
         * Clears the log and repopulates it
         * 
         **/
        private void refreshLog()
        {
            events.Rows.Clear();
            foreach (EventLogEntry entry in myLog.Entries)
            {
                TrustBaseEvent parsedEvent = parseEvent(entry);

                DataRow TBevent = events.NewRow();
                TBevent["Time"] = parsedEvent.TimeWritten.ToShortDateString() + " - " + parsedEvent.TimeWritten.ToLongTimeString();
                TBevent["Process"] = parsedEvent.ProcessName;
                TBevent["Hostname"] = parsedEvent.HostName;
                TBevent["Message"] = parsedEvent.Message;

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
            if (!this.enableBlockNotifications && !this.enableAcceptNotifications)
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
            ToolTipIcon icon;

            TrustBaseEvent parsedEvent = parseEvent(most_recent);

            if(parsedEvent.connectionBlocked && !this.enableBlockNotifications)
            {
                return;
            }
            if (!parsedEvent.connectionBlocked && !this.enableAcceptNotifications)
            {
                return;
            }

            if (parsedEvent.type == EventLogEntryType.Error)
            {
                icon = ToolTipIcon.Error;
            }
            else if (parsedEvent.type == EventLogEntryType.Warning)
            {
                icon = ToolTipIcon.Warning;
            }
            else if (parsedEvent.type == EventLogEntryType.Information)
            {
                icon = ToolTipIcon.Info;
            }
            else
            {
                icon = ToolTipIcon.None;
            }

            //=========================================================
            //DO NOT NOTIFY FOR THE SAME PROGRAM BLOCKED SEVERAL TIMES
            //=========================================================
            if (!deduplicator.Keys.Contains<string>(parsedEvent.ProcessName))
            {
                deduplicator.Add(parsedEvent.ProcessName, DateTime.Now);
                trayIcon.ShowBalloonTip(9000, "TrustBase", parsedEvent.Message + "\n" + parsedEvent.HostName + "\n" + parsedEvent.ProcessName, icon);
            }
            else
            {
                DateTime last_notification;
                deduplicator.TryGetValue(parsedEvent.ProcessName, out last_notification);
                if(DateTime.Now.Ticks - last_notification.Ticks > TimeSpan.TicksPerMinute)
                {
                    deduplicator.Remove(parsedEvent.ProcessName);
                    deduplicator.Add(parsedEvent.ProcessName, DateTime.Now);
                    trayIcon.ShowBalloonTip(9000, "TrustBase", parsedEvent.Message + "\n" + parsedEvent.HostName + "\n" + parsedEvent.ProcessName, icon);
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

            result.Message = "Unspecified";
            result.ProcessName = "Unspecified";
            result.HostName = "Unspecified";

            if (entry.Message.Contains(DELIMETER))
            {
                int delimeterPosition = entry.Message.IndexOf(DELIMETER);
                result.ProcessName = entry.Message.Substring(0, delimeterPosition);

                string hostnameAndmessage = entry.Message.Substring(delimeterPosition + 1);
                if (hostnameAndmessage.Contains(DELIMETER))
                {
                    delimeterPosition = hostnameAndmessage.IndexOf(DELIMETER);
                    result.HostName = hostnameAndmessage.Substring(0, delimeterPosition);
                    result.Message = hostnameAndmessage.Substring(delimeterPosition + 1);
                }
                else
                {
                    result.Message = hostnameAndmessage;
                }
            }
            else
            {
                result.Message = entry.Message;
            }

            result.connectionBlocked = entry.CategoryNumber == RESPONSE_INVALID;
            result.type = entry.EntryType;
            result.GUID = new Guid();
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
            public string Message;
            public Guid GUID;
            public string ProcessName;
            public string HostName;
            public DateTime TimeWritten;
            public bool connectionBlocked;
            public EventLogEntryType type;
        }

    }
}
