using System;
using System.Collections.Generic;
using System.Data;
using System.IO;
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

namespace TrustBase_Admin_Console
{
    /// <summary>
    /// Interaction logic for MainWindow.xaml
    /// </summary>
    public partial class MainWindow : Window
    {
        public MainWindow()
        {
            InitializeComponent();
            DataColumn TimeColumn = new DataColumn("User");
            DataColumn ProcessIDColumn = new DataColumn("Time");
            DataColumn ErrorColumn = new DataColumn("Excuse");

            exceptions.Columns.Add(TimeColumn);
            exceptions.Columns.Add(ProcessIDColumn);
            exceptions.Columns.Add(ErrorColumn);

            this.dataGrid.DataContext = exceptions.DefaultView;

            foreach(TrustBaseExceptionRecord exception in getExceptions()) {
                DataRow TBException = exceptions.NewRow();
                TBException["User"] = exception.user;
                TBException["Time"] = exception.time.ToShortDateString() + " - " + exception.time.ToLongTimeString();
                TBException["Excuse"] = exception.excuse;
                exceptions.Rows.Add(TBException);
            }
        }

        /**
         * Model for the DataGrid display
        **/
        private DataTable exceptions = new DataTable();


        private void addExceptionButton(object sender, RoutedEventArgs e)
        {

            if (MessageBox.Show("An exception will be added", "Exception Add", MessageBoxButton.OKCancel) ==
                MessageBoxResult.OK)
            {
                AddException("I wanted less security");
            }
        }

        private static TrustBaseExceptionRecord[] getExceptions()
        {
            string[] lines = System.IO.File.ReadAllLines("exceptions.tb");
            TrustBaseExceptionRecord[] answer = new TrustBaseExceptionRecord[lines.Length];
            for(int i = 0; i < lines.Length; i++)
            {
                string[] segments = lines[i].Split('@');
                answer[i] = new TrustBaseExceptionRecord(
                        segments[0],
                        new DateTime(long.Parse(segments[1])),
                        segments[2]
                    );
            }
            return answer;
        }

        private void AddException(string excuse)
        {
            using (FileStream fs = new FileStream("exceptions.tb", FileMode.Append))
            {
                using (TextWriter tw = new StreamWriter(fs))
                {
                    tw.WriteLine(System.Security.Principal.WindowsIdentity.GetCurrent().Name 
                        + "@" + System.DateTime.Now.Ticks + "@" + excuse);
                    tw.Flush();
                    fs.SetLength(fs.Position);
                }
            }
            updateExceptions();
        }

        private void updateExceptions()
        {
            exceptions.Clear();
            foreach (TrustBaseExceptionRecord exception in getExceptions())
            {
                DataRow TBException = exceptions.NewRow();
                TBException["User"] = exception.user;
                TBException["Time"] = exception.time.ToShortDateString() + " - " + exception.time.ToLongTimeString();
                TBException["Excuse"] = exception.excuse;
                exceptions.Rows.Add(TBException);
            }
        }

        private struct TrustBaseExceptionRecord
        {
            public string user;
            public DateTime time;
            public string excuse;
            
            public TrustBaseExceptionRecord(string user, DateTime time, string excuse)
            {
                this.user = user;
                this.time = time;
                this.excuse = excuse;
            }
        }
    }
}
