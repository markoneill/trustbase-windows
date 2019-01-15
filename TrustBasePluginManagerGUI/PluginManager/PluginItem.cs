using System;
using System.ComponentModel;
using System.Collections.Generic;
using System.Runtime.CompilerServices;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace PluginManager
{
    public class PluginItem : INotifyPropertyChanged
    {
        private bool enabled;
        private bool congressMember;
        private bool necessaryMember;
        private string version;
        private bool updated = false;

        public string Name { get; set; }
        public string Description { get; set; }
        public string Type { get; set; }
        public string Handler { get; set; }
        public string Path { get; set; }
        public bool? CongressMember {
            get => congressMember && enabled;
            set {
                congressMember = value ?? false;
                if (!congressMember && !necessaryMember)
                {
                    enabled = false;
                    OnMultiplePropertyChanged();
                }
                else if (congressMember && !enabled)
                {
                    enabled = true;
                    OnMultiplePropertyChanged();
                }
                else
                {
                    OnPropertyChanged();
                }
            }
        }
        public bool? NecessaryMember {
            get => necessaryMember && enabled;
            set {
                necessaryMember = value ?? false;
                if (!congressMember && !necessaryMember)
                {
                    enabled = false;
                    OnMultiplePropertyChanged();
                }
                else if (necessaryMember && !enabled)
                {
                    enabled = true;
                    OnMultiplePropertyChanged();
                }
                else
                {
                    OnPropertyChanged();
                }
            }
        }
        public string PrettyName { get => "\"" + Name + "\""; }
        /*
        public string PrettyDescription { get => "\"" + Description + "\""; }
        public string PrettyType { get => "\"" + Type + "\""; }
        public string PrettyHandler { get => "\"" + Handler + "\""; }
        public string PrettyPath { get => "\"" + Path + "\""; }
        */
        public string PrettyVersion { get => version; }

        public event PropertyChangedEventHandler PropertyChanged = delegate { };

        public PluginItem() { }

        public PluginItem(string name, string description, bool enabled)
        {
            this.Name = name;
            this.Description = description;
            this.enabled = enabled;
        }

        // TODONE each set needs to update "updated" to true
        public bool Enabled {
            get => this.enabled;
            set {
                if (enabled == value)
                {
                    return;
                }
                updated = true;
                enabled = value;
                if (enabled && (!congressMember && !necessaryMember))
                {
                    congressMember = true;
                    OnMultiplePropertyChanged();
                }
                else if (!enabled && (congressMember || necessaryMember)) {
                    congressMember = false;
                    necessaryMember = false;
                    OnMultiplePropertyChanged();
                }
                else
                {
                    OnPropertyChanged();
                }
                if (enabled && !Handler.Equals("native"))
                {
                    foreach(AddonItem item in MainPage.addonList)
                    {
                        if (item.Type.Equals(this.Handler))
                        {
                            item.Enabled = true;
                            break;
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
            this.PropertyChanged(this, new PropertyChangedEventArgs(propertyName));
        }

        private void OnMultiplePropertyChanged()
        {
            this.PropertyChanged(this, new PropertyChangedEventArgs("CongressMember"));
            this.PropertyChanged(this, new PropertyChangedEventArgs("NecessaryMember"));
            this.PropertyChanged(this, new PropertyChangedEventArgs("Enabled"));
        }


        public override string ToString()
        {
            return base.ToString() + (updated ? ": *" : ": ") + Name + ", " + (Enabled ? "enabled" : "disabled");
        }


    }


}
