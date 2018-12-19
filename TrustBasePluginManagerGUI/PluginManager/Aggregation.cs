using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Runtime.CompilerServices;

namespace PluginManager
{
    public class Aggregation : INotifyPropertyChanged
    {
        private bool updated = false;
        private string threshold;
        private List<string> congress = new List<string>();
        private List<string> necessaryGroup = new List<string>();

        public List<string> Congress {
            get => congress;
            set {
                congress = value;
                updated = true;
                OnPropertyChanged();
            }
        }
        public List<string> NecessaryGroup {
            get => necessaryGroup;
            set {
                necessaryGroup = value;
                updated = true;
                OnPropertyChanged();
            }

        }
        public string Threshold {
            get => this.threshold;
            set {
                updated = true;
                threshold = value;
                OnPropertyChanged();
            }
        }

        public bool Updated {
            get => this.updated;
            set { this.updated = value; }
        }

// not sure why we have this in AddonItems but I put it in here incase we need it
public event PropertyChangedEventHandler PropertyChanged = delegate { };

        protected void OnPropertyChanged([CallerMemberName] string propertyName = null)
        {
            PropertyChanged(this, new PropertyChangedEventArgs(propertyName));
        }

        internal void setGroups(List<PluginItem> pluginItems)
        {
            necessaryGroup.Clear();
            congress.Clear();
            foreach(PluginItem p in pluginItems)
            {
                if (p.NecessaryMember ?? false)
                {
                    necessaryGroup.Add(p.Name);
                }
                if (p.CongressMember ?? false)
                {
                    congress.Add(p.Name);
                }
            }
            updated = true;
        }
    }
}