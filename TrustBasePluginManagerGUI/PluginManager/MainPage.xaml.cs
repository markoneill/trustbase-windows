using System;
using System.Collections.Generic;
using System.IO;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using Windows.UI.Notifications;
using Microsoft.Toolkit.Uwp.Notifications;
using Windows.Storage;
using System.Diagnostics;
using System.Threading.Tasks;
using Windows.Storage.Pickers;
using System.Text.RegularExpressions;
using System.Threading;

// The Blank Page item template is documented at https://go.microsoft.com/fwlink/?LinkId=402352&clcid=0x409

namespace PluginManager
{

    /// <summary>
    /// An empty page that can be used on its own or navigated to within a Frame.
    /// </summary>
    public sealed partial class MainPage : Page
    {
        private static Semaphore _pool;

        private const int DEFAULT_NUM_OPTIONS = 3;

        private ConfigGramer configuration;

        private bool pluginExpandState;
        public static List<PluginItem> pluginList;
        public List<PluginItem> VisiblePluginList;

        private bool addonExpandState;
        public static List<AddonItem> addonList;
        public List<AddonItem> VisibleAddonList;

        public Aggregation PublicAggregation;

        private const string CONFIG_LOCATION = "./trustbase.cfg";
        public string ThresholdValue;

        async void WriteTimestamp()
        {
            Debug.WriteLine("\nTrying!!");

            StorageFolder localFolder = ApplicationData.Current.LocalFolder;
            Debug.WriteLine("Local Folder: " + localFolder.ToString());

            Windows.Globalization.DateTimeFormatting.DateTimeFormatter formatter = new Windows.Globalization.DateTimeFormatting.DateTimeFormatter("longtime");
            Debug.WriteLine("Created DateTime: " + formatter.Format(DateTimeOffset.Now));

            StorageFile configFile = await localFolder.CreateFileAsync("dataFile.txt", CreationCollisionOption.ReplaceExisting);
            Debug.WriteLine("Created Sample file: " + configFile.ToString());

            await FileIO.WriteTextAsync(configFile, formatter.Format(DateTimeOffset.Now));
            Debug.WriteLine("Finished Writing the timestamp!\n");
        }

        async void ReadTimestamp()
        {
            try
            {

                Uri uri = new System.Uri("ms-appx:///trustbase.cfg");
                StorageFile file = await StorageFile.GetFileFromApplicationUriAsync(uri);
                string contents = await Windows.Storage.FileIO.ReadTextAsync(file);
                //Debug.WriteLine(contents);

                //Object addons = ({ });


                // Deserialize obeject into 
            }
            catch (Exception)
            {
                // Timestamp not found
                Debug.WriteLine("\nERROR: Could not Read Timestamp\n");
            }
        }

        public MainPage()
        {
            configuration = new ConfigGramer();
            pluginExpandState = false;
            addonExpandState = false;
            _pool = new Semaphore(1, 1);
            InitializeComponent();
            LoadSettings();
            DebugModify.IsEnabled = false;
        }

        private void LoadAddons(ConfigGramer configuration)
        {
            addonList = new List<AddonItem>();

            ConfigGramer.Item result = configuration.GetItem(
                new ASTQualifier(ItemType.CONFIGURATION),
                new ASTQualifier(ItemType.SETTING_LIST),
                new ASTQualifier(ItemType.SETTING, "addons"),
                new ASTQualifier(ItemType.VALUE),
                new ASTQualifier(ItemType.LIST),
                new ASTQualifier(ItemType.VALUE_LIST)
            );

            if (result.IsEmpty())
            {
                return;
            }

            ConfigGramer._ValueList valueList = (ConfigGramer._ValueList)result.value;
            List<ConfigGramer._Value> list = valueList.Value;

            ASTQualifier[] qualifiers =
            {
                new ASTQualifier(ItemType.SETTING),
                new ASTQualifier(ItemType.VALUE),
                new ASTQualifier(ItemType.SCALAR_VALUE),
                new ASTQualifier(ItemType.STRING)
            };

            for (int i = 0; i < list.Count; i++)
            {
                ConfigGramer.Item listItem = configuration.GetItem(
                    result,
                    new ASTQualifier(ItemType.VALUE, i.ToString()),
                    new ASTQualifier(ItemType.GROUP),
                    new ASTQualifier(ItemType.SETTING_LIST)
                );

                AddonItem addon = new AddonItem();

                qualifiers[3].itemType = ItemType.STRING;
                qualifiers[0].itemQualifier = "name";
                addon.Name = configuration.GetItem(listItem, qualifiers).GetString();

                qualifiers[0].itemQualifier = "description";
                addon.Description = configuration.GetItem(listItem, qualifiers).GetString();

                qualifiers[0].itemQualifier = "version";
                addon.Version = configuration.GetItem(listItem, qualifiers).GetString();

                qualifiers[0].itemQualifier = "type";
                addon.Type = configuration.GetItem(listItem, qualifiers).GetString();

                qualifiers[0].itemQualifier = "path";
                addon.Path = configuration.GetItem(listItem, qualifiers).GetString();

                qualifiers[0].itemQualifier = "enabled";
                qualifiers[3].itemType = ItemType.BOOLEAN;
                addon.Enabled = configuration.GetItem(listItem, qualifiers).GetBoolean();

                addonList.Add(addon);
            }

            if (addonList.Count > DEFAULT_NUM_OPTIONS)
            {
                ExpandAddonListButton.Visibility = Visibility.Visible;
                ExpandAddonListButton.Content = "▲";
                VisibleAddonList = addonList.GetRange(0, 3);
            }
            else
            {
                ExpandAddonListButton.Visibility = Visibility.Collapsed;
                VisibleAddonList = addonList;
            }

            AddonOptionsListView.ItemsSource = VisibleAddonList;

        }

        private void LoadPlugins(ConfigGramer configuration)
        {
            pluginList = new List<PluginItem>();

            ConfigGramer.Item result = configuration.GetItem(
                new ASTQualifier(ItemType.CONFIGURATION),
                new ASTQualifier(ItemType.SETTING_LIST),
                new ASTQualifier(ItemType.SETTING, "plugins"),
                new ASTQualifier(ItemType.VALUE),
                new ASTQualifier(ItemType.LIST),
                new ASTQualifier(ItemType.VALUE_LIST)
            );

            if (result.IsEmpty())
            {
                return;
            }

            ConfigGramer._ValueList valueList = (ConfigGramer._ValueList)result.value;
            List<ConfigGramer._Value> list = valueList.Value;

            ASTQualifier[] qualifiers =
            {
                new ASTQualifier(ItemType.SETTING),
                new ASTQualifier(ItemType.VALUE),
                new ASTQualifier(ItemType.SCALAR_VALUE),
                new ASTQualifier(ItemType.STRING)
            };

            for (int i = 0; i < list.Count; i++)
            {
                ConfigGramer.Item listItem = configuration.GetItem(
                    result,
                    new ASTQualifier(ItemType.VALUE, i.ToString()),
                    new ASTQualifier(ItemType.GROUP),
                    new ASTQualifier(ItemType.SETTING_LIST)
                );

                PluginItem plugin = new PluginItem();

                qualifiers[3].itemType = ItemType.STRING;
                qualifiers[0].itemQualifier = "name";
                plugin.Name = configuration.GetItem(listItem, qualifiers).GetString();

                qualifiers[0].itemQualifier = "description";
                plugin.Description = configuration.GetItem(listItem, qualifiers).GetString();

                qualifiers[0].itemQualifier = "version";
                plugin.Version = configuration.GetItem(listItem, qualifiers).GetString();

                qualifiers[0].itemQualifier = "type";
                plugin.Type = configuration.GetItem(listItem, qualifiers).GetString();

                qualifiers[0].itemQualifier = "handler";
                plugin.Handler = configuration.GetItem(listItem, qualifiers).GetString();

                qualifiers[0].itemQualifier = "path";
                plugin.Path = configuration.GetItem(listItem, qualifiers).GetString();

                qualifiers[0].itemQualifier = "enabled";
                qualifiers[3].itemType = ItemType.BOOLEAN;
                plugin.Enabled = configuration.GetItem(listItem, qualifiers).GetBoolean();
                plugin.Updated = false;

                // TODO load necessary and congress groups membership from
                pluginList.Add(plugin);
            }

            if (pluginList.Count > DEFAULT_NUM_OPTIONS)
            {
                ExpandPluginListButton.Visibility = Visibility.Visible;
                ExpandPluginListButton.Content = "▼"; // ▲
                VisiblePluginList = pluginList.GetRange(0, 3);
            }
            else
            {
                ExpandPluginListButton.Visibility = Visibility.Collapsed;
                VisiblePluginList = pluginList;
            }

            ConfigurationSettingsGridView.ItemsSource = VisiblePluginList;
        }

        private void LoadAggregation(ConfigGramer configuration)
        {
            ConfigGramer.Item result = configuration.GetItem(
                new ASTQualifier(ItemType.CONFIGURATION),
                new ASTQualifier(ItemType.SETTING_LIST),
                new ASTQualifier(ItemType.SETTING, "aggregation"),
                new ASTQualifier(ItemType.VALUE),
                new ASTQualifier(ItemType.GROUP),
                new ASTQualifier(ItemType.VALUE),
                new ASTQualifier(ItemType.SETTING_LIST)
               );

            if (result.IsEmpty()) //checks to make sure we have an aggregation setting with values in it
            {
                return;
            }
            
            ConfigGramer._SettingList valueList = (ConfigGramer._SettingList)result.value;
            ASTQualifier[] qualifiers =
            {
                new ASTQualifier(ItemType.SETTING),
                new ASTQualifier(ItemType.VALUE),
                new ASTQualifier(ItemType.SCALAR_VALUE),
                new ASTQualifier(ItemType.STRING)
            };
            Aggregation aggregation = new Aggregation();
            for (int i = 0; i < valueList.Count; i++)
            {
                
                switch (valueList.Value[i].Name.Value)
                {

                    case "congress_threshold":
                        qualifiers[3].itemType = ItemType.FLOAT;
                        qualifiers[0].itemQualifier = "congress_threshold";
                        aggregation.Threshold = (configuration.GetItem(result, qualifiers).GetFloat()*100).ToString();
                       
                        break;
                    case "sufficient"://{PluginManager.ConfigGramer._Value} {PluginManager.ConfigGramer._Group} {PluginManager.ConfigGramer._SettingList}

                        loadSufficientGroup(configuration.GetItem(
                                                result,
                                                new ASTQualifier(ItemType.SETTING, "sufficient"),
                                                new ASTQualifier(ItemType.VALUE),
                                                new ASTQualifier(ItemType.GROUP),
                                                new ASTQualifier(ItemType.SETTING_LIST)
                                            ), ref aggregation);
                       
                        break;
                }
            }
       
            PublicAggregation = aggregation;
            NumberTextBox.Text = aggregation.Threshold;
        }

        private void loadSufficientGroup(ConfigGramer.Item item, ref Aggregation aggregation)
        {
            ASTQualifier[] qualifiers =
                  {
                new ASTQualifier(ItemType.SETTING),
                new ASTQualifier(ItemType.VALUE),
                new ASTQualifier(ItemType.LIST),
                new ASTQualifier(ItemType.VALUE_LIST)
            };
            if ( item.context == ItemType.SETTING_LIST)
            {
                ConfigGramer._SettingList sufficientList = (ConfigGramer._SettingList)item.value;
                for (int i = 0; i < sufficientList.Count; i++)
                {
                    switch (sufficientList.Value[i].Name.Value)
                    {
                        case "congress_group":
                            qualifiers[0].itemQualifier = "congress_group";
                            aggregation.Congress = configuration.GetItem(item, qualifiers).GetStringList();
                            break;
                        case "necessary_group":
                            qualifiers[0].itemQualifier = "necessary_group";
                            aggregation.NecessaryGroup = configuration.GetItem(item, qualifiers).GetStringList();
                            break;
                        default:
                            return;
                    }
                }
            }
        }

        private void SyncPluginsWithAggregation()
        {
            foreach(PluginItem p in pluginList)
            {
                p.CongressMember = PublicAggregation.Congress.Contains(p.Name);
                p.NecessaryMember = PublicAggregation.NecessaryGroup.Contains(p.Name);
            }
        }

        public async void LoadSettings()
        {
            try
            {
                /*
                Uri uri = new System.Uri("ms-appx:///trustbase.cfg");
                StorageFile file = await StorageFile.GetFileFromApplicationUriAsync(uri);
                string contents = await FileIO.ReadTextAsync(file);
                //*/
                Windows.Storage.StorageFolder storageFolder =
                    Windows.Storage.ApplicationData.Current.LocalFolder;
                StorageFile configFile =
                    await storageFolder.GetFileAsync("trustbase.cfg");
                string contents = await FileIO.ReadTextAsync(configFile);

                configuration.Load(contents);

                LoadAddons(configuration);
                LoadPlugins(configuration);
                LoadAggregation(configuration);
                SyncPluginsWithAggregation();
                DebugModify.IsEnabled = false;

                //this.ConfigurationSettingsGridView.ItemsSource = pluginList;
            }
            catch (Exception)
            {
                Debug.WriteLine("And you failed : :'(");
            }
            
        }

        public void ReadConfigurationSettings()
        {
            try
            {
                string content = "1 - ";
                //Windows.Storage.ApplicationDataContainer localSettings = Windows.Storage.ApplicationData.Current.LocalSettings;

                this.SendNotification("Debug", content);
            } catch (Exception e)
            {
                this.SendNotification("Debug", "Oops, something wrong happened");
                Debug.WriteLine(e.ToString());
            }
            /*
            StreamReader r = new StreamReader("file.json"));

            dynamic array = JsonConvert.DeserializeObject(json);
            foreach (var item in array)
            {
                Console.WriteLine("{0} {1}", item.temp, item.vcc);
            }*/
        }
        private async void saveFile(String fileContents)
        {
            Windows.Storage.Pickers.FileSavePicker savePicker = new FileSavePicker();
            
            // Dropdown of file types the user can save the file as
            //savePicker.FileTypeChoices.Add("Config", new List<string>() { ".cfg" });
            // Default file name if the user does not type one in or select a file to replace
            //savePicker.SuggestedFileName = "trustbase";
            string notificationTitle = "Success";
            string notificationMessage = "You have successfully updated your settings.";
            try
            {
                Windows.Storage.StorageFolder storageFolder =
                    Windows.Storage.ApplicationData.Current.LocalFolder;
                Windows.Storage.StorageFile configFile =
                    await storageFolder.CreateFileAsync("trustbase.cfg",
                        Windows.Storage.CreationCollisionOption.ReplaceExisting);
                Windows.Storage.Streams.IBuffer buffer = Windows.Security.Cryptography.CryptographicBuffer.ConvertStringToBinary(
                        fileContents, Windows.Security.Cryptography.BinaryStringEncoding.Utf8);
                await Windows.Storage.FileIO.WriteBufferAsync(configFile, buffer);

                //await Windows.Storage.FileIO.WriteTextAsync(sampleFile, fileContents);
                // TODO call the PluginManagerFileWriter.exe with elivated privleges & give it path and fileContents to write
                /*Process saver = new Process();
                saver.StartInfo.
                var process = Process.Start(pathToProgram, argsString);

                process.WaitForExit();

                var exitCode = process.ExitCode;*/
            }
            catch (UnauthorizedAccessException ex)
            {
                notificationTitle = "Unable to write file";
                notificationMessage = ex.Message;
            }
            catch (Exception ex1)
            {
                notificationTitle = "Unknown Error";
                notificationMessage = ex1.Message;
            }
            this.SendNotification(notificationTitle, notificationMessage);
        }

        private async void SaveButton_Click(object sender, RoutedEventArgs e)
        {
            if (_pool.WaitOne(0))
            {

                /* TODO: Do Stuff */
                String newConfig = configuration.MakeConfiguration(addonList, pluginList, PublicAggregation);
                //Uri uri = new System.Uri("ms-appx:///trustbase.cfg");
                //StorageFile file = await StorageFile.GetFileFromApplicationUriAsync(uri);
                //Stream outputStream = await file.OpenStreamForWriteAsync();
                // TODO: write out newConfig
                //outputStream.WriteAsync(newConfig, 0, newConfig.Count());C:\Users\ilab\Source\Repos\PluginManager\PluginManager\trustbase.cfg
                await Task.Run(() => saveFile(newConfig));

                /* one way to maybe do it
                newConfig.Add(configuration.makeConfiguration(addonList, pluginList));
                System.IO.File.WriteAllLines(CONFIG_LOCATION, newConfig);
                */

                /* Notify when settings have been saved */

                _pool.Release();
            }
        }

        private void SendNotification(string title, string content)
        {
            string logo = "file:///" + Path.GetFullPath("Assets/TrustBaseIcon.ico");

            // Construct the visuals of the toast
            ToastVisual visual = new ToastVisual()
            {
                BindingGeneric = new ToastBindingGeneric() {
                    Children = {
                        new AdaptiveText() { Text = title },
                        new AdaptiveText() {  Text = content },
                    },
                    AppLogoOverride = new ToastGenericAppLogo() {
                        Source = logo,
                        HintCrop = ToastGenericAppLogoCrop.Circle
                    }
                }
            };

            // Now we can construct the final toast content
            ToastContent toastContent = new ToastContent() { Visual = visual };

            // And create the toast notification
            ToastNotification toast = new ToastNotification(toastContent.GetXml());
            ToastNotificationManager.CreateToastNotifier().Show(toast);
        }

        private void DebugNotifications_Click(object sender, RoutedEventArgs e)
        {
            SendNotification("Sample Notification", "This is a sample notification to let you know stuff");
        }

        private void DebugModify_Click(object sender, RoutedEventArgs e)
        {
            this.WriteTimestamp();

            /*
            string title = "Trust Base Debug Message";
            string content = "";

            for (int i = 0; i < this.pluginList.Count; i++)
            {
                content += this.pluginList[i].ToString() + "\n";
            }

            this.SendNotification(title, content);
            */


            //this.ReadConfigurationSettings();
            //this.pluginList[0].PluginEnabled = !this.pluginList[0].PluginEnabled;
        }

        private void ExpandAddonListButton_Click(object sender, RoutedEventArgs e)
        {
            addonExpandState = !addonExpandState;
            if (addonExpandState)
            {
                // TODONE: Update private pluginList
                // DONE: C# used a shallow copy by default

                ExpandAddonListButton.Visibility = Visibility.Visible;
                ExpandAddonListButton.Content = "▲";
                VisiblePluginList = pluginList;
            }
            else
            {

                // TODONE: Update private pluginList
                // DONE: C# used a shallow copy by default

                ExpandAddonListButton.Visibility = Visibility.Visible;
                ExpandAddonListButton.Content = "▼";
                if (pluginList != null) { 
                    VisiblePluginList = pluginList.GetRange(0, DEFAULT_NUM_OPTIONS);
                }
            }

            ConfigurationSettingsGridView.ItemsSource = VisibleAddonList;
        }

        private void ExpandPluginListButton_Click(object sender, RoutedEventArgs e)
        {
            pluginExpandState = !pluginExpandState;
            if (pluginExpandState)
            {
                // TODONE: Update private pluginList
                // DONE: C# used a shallow copy by default

                ExpandPluginListButton.Visibility = Visibility.Visible;
                ExpandPluginListButton.Content = "▲";
                VisiblePluginList = pluginList;
            }
            else
            {

                // TODONE: Update private pluginList
                // DONE: C# used a shallow copy by default

                ExpandPluginListButton.Visibility = Visibility.Visible;
                ExpandPluginListButton.Content = "▼";
                if (pluginList != null)
                {
                    VisiblePluginList = pluginList.GetRange(0, DEFAULT_NUM_OPTIONS);
                }
            }

            ConfigurationSettingsGridView.ItemsSource = VisiblePluginList;
        }

        private void ThresholdTextBox_Left(object sender, RoutedEventArgs e)
        {
            TextBox field = (TextBox)sender;
            String text = field.Text;
            Regex floatRegex = new Regex(@"^[0-9]+[\.]{0,1}[0-9]*$");
            if (floatRegex.IsMatch(text))
            {
                if(Double.Parse(text) >100)
                {
                    field.Text = "100";
                }
                PublicAggregation.Threshold = field.Text;
            }
            else
            {
                field.Text = PublicAggregation.Threshold;
            }
        }
    }
}
