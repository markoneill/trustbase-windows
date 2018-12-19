using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Linq;
using System.Text;
using System.Text.RegularExpressions;
using System.Threading.Tasks;
using Windows.Storage;

namespace PluginManager
{
    class Config
    {
        private string ConfigLocation;
        private List<Addon> addons;
        private List<Plugin> plugins;
        private Aggregation aggregation;

        /* Simple Model Classes */
        public class Addon
        {
            public string Name { get; set; }
            public string Description { get; set; }
            public string Version { get; set; }
            public string Type { get; set; }
            public string Path { get; set; }

            public Addon()
            {
                this.Name = "";
                this.Description = "";
                this.Version = "";
                this.Type = "";
                this.Path = "";
            }
            public Addon(string name, string description, string version, string type, string path)
            {
                this.Name = name;
                this.Description = description;
                this.Version = version;
                this.Type = type;
                this.Path = path;
            }
        }

        public class Plugin
        {
            public string Name { get; set; }
            public string Description { get; set; }
            public string Version { get; set; }
            public string Type { get; set; }
            public string Handler { get; set; }
            public string Path { get; set; }
            public int OpenSSL { get; set; }
            public bool Enabled { get; set; }

            public Plugin()
            {
                this.Name = "";
                this.Description = "";
                this.Version = "";
                this.Type = "";
                this.Handler = "";
                this.Path = "";
                this.OpenSSL = 0;
                this.Enabled = true;
            }
            public Plugin(string name, string description, string version, string type, string handler, string path, int openssl, bool enabled)
            {
                this.Name = name;
                this.Description = description;
                this.Version = version;
                this.Type = type;
                this.Handler = handler;
                this.Path = path;
                this.OpenSSL = openssl;
                this.Enabled = true;
            }
        }

        public class Aggregation
        {
            public double CongressThreshold;
            public List<Plugin> CongressGroup;
            public List<Plugin> NecessaryGroup;

            public Aggregation()
            {
                this.CongressThreshold = 0;
                this.CongressGroup = new List<Plugin>();
                this.NecessaryGroup = new List<Plugin>();
            }
        }

        public Config()
        {
            this.addons = new List<Addon>();
            this.plugins = new List<Plugin>();
            this.aggregation = new Aggregation();
        }



        public async void LoadSettings(Uri uri)
        {
            /* Immitation Enums */
            const int ADDON = 0;
            const int PLUGIN = 1;
            const int AGGREGATE = 2;

            List<string> options = new List<string>
            {
                "name",
                "addons",
                "plugins",
                "aggregate"
            };

            try
            {
                /* Read Configuration File */
                //Uri uri = new System.Uri("ms-appx:///trustbase.cfg");
                StorageFile file = await StorageFile.GetFileFromApplicationUriAsync(uri);
                string contents = await FileIO.ReadTextAsync(file);
                ConfigurationParser parser = new ConfigurationParser();
                parser.Load(contents);


                /* Ignore Comments */
                /*
                string commentPattern = @"\/\*(\*(?!\/)|[^*])*\*\/";
                Regex rgx = new Regex(commentPattern);
                string result = rgx.Replace(contents, "");
                Debug.WriteLine(result);*/

                /* Find the bounds of the individual settings: addons, plugins, aggregate */
                int addonStart = contents.IndexOf("addons");
                int pluginStart = contents.IndexOf("plugins");
                int aggregateStart = contents.IndexOf("aggregate");

                /*
                int option = ADDON;
                int maxVal = addonStart;

                if (pluginStart > maxVal) {
                    option = PLUGIN;
                    maxVal = pluginStart;
                }

                if (aggregateStart > maxVal)
                
                

                

                /* Load Plugins */
                /* Load Addons */
                /* Load Aggregate */

                // Deserialize obeject into 
            }
            catch (Exception)
            {
                // Timestamp not found
                Debug.WriteLine("\nRROR: Could not Read Timestamp\n");
            }
        }
    }
}
