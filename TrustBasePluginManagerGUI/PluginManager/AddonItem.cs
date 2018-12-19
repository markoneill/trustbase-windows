using System;
using System.ComponentModel;
using System.Collections.Generic;
using System.Runtime.CompilerServices;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace PluginManager
{
    public class AddonItem : INotifyPropertyChanged
    {
        private bool enabled;
        private string version;
        private bool updated;

        public string Name { get; set; }
        public string Description { get; set; }
        public string Type { get; set; }
        public string Path { get; set; }
        /*
        public string PrettyName { get => "\"" + Name + "\""; }
        public string PrettyDescription { get => "\"" + Description + "\""; }
        public string PrettyVersion { get => "\"" + version + "\""; }
        public string PrettyType { get => "\"" + Type + "\""; }
        public string PrettyPath { get => "\"" + Path + "\""; }
        */
        public string PrettyVersion { get => version; }

        public event PropertyChangedEventHandler PropertyChanged = delegate { };

        public AddonItem() { }

        public bool Enabled 
        {
            get => this.enabled;
            set {
                if(enabled == value)
                {
                    return;
                }
                updated = true;
                this.enabled = value;
                this.OnPropertyChanged();
                if (!enabled)
                {       // TODO: make the plugin list and addon list into its own class
                    foreach (PluginItem item in MainPage.pluginList)
                    {
                        if (item.Handler.Equals(this.Type))
                        {
                            item.Enabled = false;
                        }
                    }
                }
            }
        }

        public bool Updated {
            get => this.updated;
            set { this.updated = value; }
        }

        public string Version {
            get {
                if (version != "")
                {
                    return "Version " + version;
                }
                else
                {
                    return "";
                }
            }
            set {
                version = value;
                OnPropertyChanged();
            }
        }

        protected void OnPropertyChanged([CallerMemberName] string propertyName = null)
        {
            PropertyChanged(this, new PropertyChangedEventArgs(propertyName));
        }

    }
}
