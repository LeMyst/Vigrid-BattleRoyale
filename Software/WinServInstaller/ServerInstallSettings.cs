using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Reflection;

namespace WinServInstaller
{
    class ServerInstallSettings
    {
        private ObjectStore S3;
        private Dictionary<string, string> s3_settings;

        public string STEAMCMD_PATH { get; set; }
        public string STEAM_USER { get; set; }
        public string STEAM_PASS { get; set; }
        public string SERVER_PATH { get; set; }
        public string GIT_INSTALL_PATH { get; set; }
        public string GIT_REPO_PATH { get; set; }
        public string GIT_BRANCH_NAME { get; set; }
        public string SERVER_NAME { get; set; }
        public string SERVER_PORT { get; set; }
        public string STEAM_QUERY_PORT { get; set; }
        public string SERVER_PASSWORD { get; set; }
        public string PASSWORD_ADMIN { get; set; }
        public string MAX_PLAYERS { get; set; }
        public string WORLD_NAME { get; set; }
        public string LIGHTING_CONFIG_VALUE { get; set; }

        public string CF_WORKSHOP_ID { get; set; }
        public string COT_WORKSHOP_ID { get; set; }
        public string EXP_WORKSHOP_ID { get; set; }
        public string EXP_LIC_WORKSHOP_ID { get; set; }
        public string EXP_VEH_WORKSHOP_ID { get; set; }
        public string EXP_COR_WORKSHOP_ID { get; set; }
        public string BR_WORKSHOP_ID { get; set; }
        public string EXTRA_MODS { get; set; } //list of extra mods workshop ids
        
        public string BANS_API_KEY { get; set; }
        public string API_ENDPOINT { get; set; }
        public string API_KEY { get; set; }
        public string USE_API { get; set; }
        public string UNLOCK_SKINS { get; set; }
        public string IP_ADDR { get; set; }


        //git install path

        //TODO: these should be given default values



        public ServerInstallSettings(ObjectStore s3_store)
        {
            S3 = s3_store;
            //parse s3 stored defaults (for secrets)
            S3.WriteToFile("s3_settings_file.txt", "server-install/templates/S3_Settings_Template.txt");
            s3_settings = new Dictionary<string, string>();
            string[] s3lines = System.IO.File.ReadAllLines("s3_settings_file.txt");
            foreach (string s3line in s3lines)
            {
                string[] parts = s3line.Split('=');
                string key = parts[0];
                string value = string.Join("=",parts, 1, (parts.Length-1));
                s3_settings.Add(key, value);
            }

            //try to load from disk
            if (System.IO.File.Exists("install_settings.ini"))
            {
                string[] lines = System.IO.File.ReadAllLines("install_settings.ini");
                if(lines.Length != this.GetType().GetProperties().Length)
                {
                    Console.WriteLine("Install Settings File Corrupt!");
                    CreateSettings();
                    return;
                }
                foreach(string line in lines)
                {
                    string property = line.Substring(0, line.IndexOf("="));
                    SetStringPropertyByName(property, line.Substring(line.IndexOf("=") + 1));
                }
                Console.WriteLine("Loading settings from disk!");
            }
            else
            {
                CreateSettings();
            }
        }
        private void SetStringPropertyByName(string name, string value)
        {
            if (value == "")
            {
                Console.WriteLine("Corrupt Property '" + name + "'");
                value = Program.GetInput("Enter the correct value for this property");
            }

            this.GetType().InvokeMember(name,
                    BindingFlags.Instance | BindingFlags.Public | BindingFlags.SetProperty,
                    Type.DefaultBinder, this, new object[] { value });
        }
        private void SaveSettings()
        {
            List<string> lines = new List<string>();
            PropertyInfo[] properties = this.GetType().GetProperties();
            foreach(PropertyInfo property in properties)
            {
                lines.Add(property.Name + "=" + property.GetValue(this));
            }
            System.IO.File.WriteAllLines("install_settings.ini", lines);
        }
        private void CreateSettings()
        {
            Console.WriteLine("== Path Settings");
            STEAMCMD_PATH = Program.GetInput("Enter SteamCMD Install Folder (Path will be created if it does not exist)");
            SERVER_PATH = Program.GetInput("Enter Server Install Folder (Path will be created if it does not exist)");
            GIT_REPO_PATH = Program.GetInput("Enter BattleRoyale Git Path (Path will be created if it does not exist)");
            GIT_INSTALL_PATH = Program.GetInput("Enter your git.exe Path (blank for Default)");
            if (GIT_INSTALL_PATH == "")
                GIT_INSTALL_PATH = @"C:\Program Files\Git\bin";

            Console.WriteLine("== Steam Settings");
            STEAM_USER = Program.GetInput("Enter Steam Username");
            STEAM_PASS = Program.GetInput("Enter Steam Password");

            Console.WriteLine("== Git Settings");
            GIT_BRANCH_NAME = Program.GetInput("Enter Git Branch (blank for master)");
            if (GIT_BRANCH_NAME == "")
                GIT_BRANCH_NAME = "master";

            Console.WriteLine("== Server Settings");
            SERVER_NAME = Program.GetInput("Enter Server Name");
            SERVER_PORT = Program.GetInput("Enter Server Port (blank for 2302)");
            if (SERVER_PORT == "")
                SERVER_PORT = "2302";
            STEAM_QUERY_PORT = Program.GetInput("Enter Server Query Port (blank for 2303)");
            if (STEAM_QUERY_PORT == "")
                STEAM_QUERY_PORT = "2303";
            SERVER_PASSWORD = Program.GetInput("Enter Server Password (write 'S3' for s3 default)");
            if (SERVER_PASSWORD == "S3")
            {
                SERVER_PASSWORD = s3_settings["SERVER_PASSWORD"];
            }
            PASSWORD_ADMIN = Program.GetInput("Enter Server Admin Password (write 'S3' for s3 default)");
            if(PASSWORD_ADMIN == "S3")
            {
                PASSWORD_ADMIN = s3_settings["PASSWORD_ADMIN"];
            }
            MAX_PLAYERS = Program.GetInput("Enter Max Players (blank for 60)");
            if (MAX_PLAYERS == "")
                MAX_PLAYERS = "60";
            WORLD_NAME = Program.GetInput("Enter World name (blank for ChernarusPlusGloom)");
            if (WORLD_NAME == "")
                WORLD_NAME = "ChernarusPlusGloom";
            LIGHTING_CONFIG_VALUE = Program.GetInput("Enter Lighting Value (blank for 0)");
            if (LIGHTING_CONFIG_VALUE == "")
                LIGHTING_CONFIG_VALUE = "0";

            Console.WriteLine("== Mod Settings");
            CF_WORKSHOP_ID = Program.GetInput("Enter CF Workshop id (blank for 1559212036)");
            if (CF_WORKSHOP_ID == "")
                CF_WORKSHOP_ID = "1559212036";
            COT_WORKSHOP_ID = Program.GetInput("Enter COT Workshop id (blank for 1564026768)");
            if (COT_WORKSHOP_ID == "")
                COT_WORKSHOP_ID = "1564026768";
            COT_WORKSHOP_ID = Program.GetInput("Enter COT Workshop id (blank for 1564026768)");
            if (COT_WORKSHOP_ID == "")
                COT_WORKSHOP_ID = "1564026768";
            EXP_WORKSHOP_ID = Program.GetInput("Enter Expansion Workshop id (blank for 2116151222)");
            if (EXP_WORKSHOP_ID == "")
                EXP_WORKSHOP_ID = "2116151222";
            EXP_LIC_WORKSHOP_ID = Program.GetInput("Enter Expansion-Licensed Workshop id (blank for 2116157322)");
            if (EXP_LIC_WORKSHOP_ID == "")
                EXP_LIC_WORKSHOP_ID = "2116157322";
            EXP_VEH_WORKSHOP_ID = Program.GetInput("Enter Expansion-Vehicles Workshop id (blank for 2291785437)");
            if (EXP_VEH_WORKSHOP_ID == "")
                EXP_VEH_WORKSHOP_ID = "2291785437";
            EXP_COR_WORKSHOP_ID = Program.GetInput("Enter Expansion-Core Workshop id (blank for 2291785308)");
            if (EXP_COR_WORKSHOP_ID == "")
                EXP_COR_WORKSHOP_ID = "2291785308";
            BR_WORKSHOP_ID = Program.GetInput("Enter BattleRoyale Workshop id (blank for 2160516972)");
            if (BR_WORKSHOP_ID == "")
                BR_WORKSHOP_ID = "2160516972";

            EXTRA_MODS = Program.GetInput("Enter extra mod workshop ids (delimited by ';')").Trim(' ',';');

            Console.WriteLine("=== BattleRoyale Settings");
            BANS_API_KEY = Program.GetInput("Enter Banlist API Key");
            API_ENDPOINT = Program.GetInput("Enter API Endpoint (blank for https://dayzbr.dev)");
            if (API_ENDPOINT == "")
                API_ENDPOINT = "https://dayzbr.dev";
            API_KEY = Program.GetInput("Enter API Key (write 'S3' for s3 default)");
            if (API_KEY == "S3")
            {
                API_KEY = s3_settings["API_KEY"];
            }
            USE_API = Program.GetInput("Use Api? (0 for no, blank for default: 1)");
            if (USE_API == "")
                USE_API = "1";
            UNLOCK_SKINS = Program.GetInput("Unlock All Skins? (1 for yes, blank for default: 0");
            if (UNLOCK_SKINS == "")
                UNLOCK_SKINS = "0";
            IP_ADDR = Program.GetInput("Enter Server IP (blank for default '127.0.0.1')");
            if (IP_ADDR == "")
                IP_ADDR = "127.0.0.1";


            SaveSettings();
        }
    }
}
