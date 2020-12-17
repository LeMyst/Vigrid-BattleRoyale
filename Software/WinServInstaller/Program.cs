using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.IO;
using System.Diagnostics;

namespace WinServInstaller
{
    class Program
    {
        static void Main(string[] args)
        {
            Console.Title = "DayZBR Windows Server Installer";
            //read s3 config file
            string access_key, secret_key, service;
            string[] data_lines;
            if (File.Exists("s3cfg.ini"))
                data_lines = File.ReadAllLines("s3cfg.ini");
            else
                data_lines = new string[] { };

            if (data_lines.Length > 0)
                access_key = data_lines[0].Trim();
            else
                access_key = GetInput("Enter S3 Access Key").Trim();
            if (data_lines.Length > 1)
                secret_key = data_lines[1].Trim();
            else
                secret_key = GetInput("Enter S3 Secret Key").Trim();
            if (data_lines.Length > 2)
                service = data_lines[2].Trim();
            else
                service = GetInput("Enter Service URL (Leave blank for VULTR)").Trim();
            //make object store controller
            ObjectStore s3;
            if (service == "")
                s3 = new ObjectStore(access_key, secret_key);
            else
                s3 = new ObjectStore(access_key, secret_key, service);
            //test and write config
            try
            {
                Console.Write("Testing S3 Connection...");
                s3.TestConnection();
                if (access_key != "")
                {
                    List<string> lines = new List<string>();
                    lines.Add(access_key);
                    if(secret_key != "")
                    {
                        lines.Add(secret_key);
                        if(service != "")
                        {
                            lines.Add(service);
                        }
                    }
                    File.WriteAllLines("s3cfg.ini", lines);
                }
                Console.WriteLine("Pass");
            } catch(Exception ex)
            {
                Console.WriteLine(ex.ToString());
                goto end;
            }

            //set our bucket
            s3.SetBucket("dayzbr");

            //initializes settings
            ServerInstallSettings settings = new ServerInstallSettings(s3);

            //create server installer object
            BRServerInstaller installer = new BRServerInstaller(s3);

            //--- step 0: install dependencies
            if (!File.Exists(settings.GIT_INSTALL_PATH + "\\git.exe"))
            {
                Console.WriteLine("Please follow the installer instructions to install Git for Windows...");
                Console.WriteLine("!!!IMPORTANT!!! - If you choose an alternative installation path, you need to EXIT this server installer NOW. BEFORE you finish installing Git!!");
                installer.InstallGitForWindows();
            }
            //--- step 1: steamcmd & git setup
            Console.WriteLine("Installing SteamCMD. Please wait...");
            installer.InstallSteamCMD(settings.STEAMCMD_PATH); //install & firsttime setup of steamcmd
            
            Console.WriteLine("If prompted, please enter your steamguard code into SteamCMD");
            Console.WriteLine("Waiting for user to complete input in SteamCMD...");
            installer.VerifySteamLogin(settings.STEAMCMD_PATH, settings.STEAM_USER, settings.STEAM_PASS); //login verification (steamguard)

            Console.WriteLine("Cloning Git Repo. Please wait...");
            installer.CloneGitRepo(settings.GIT_INSTALL_PATH, settings.GIT_REPO_PATH, settings.GIT_BRANCH_NAME); //clone and checkout branch

            //--- step 2: blank dayz server install
            Console.WriteLine("Installing DayZ Server. Please wait...");
            installer.InstallServer(settings.STEAMCMD_PATH, settings.STEAM_USER, settings.STEAM_PASS, settings.SERVER_PATH, "223350");

            //--- step 3: install write patcher application & batch file
            Console.WriteLine("Installing DayZ Server Application Patcher. Please wait...");
            installer.InstallWritePatcher(settings.SERVER_PATH);

            //--- step 4: install template files 
            Console.WriteLine("Installing BR Template Files. Please wait...");
            installer.InstallServerConfig(settings.SERVER_PATH);
            installer.InstallBatchTemplates(settings.SERVER_PATH);
            installer.InstallSettings(settings.SERVER_PATH);

            //--- step 5: update template files
            installer.UpdateConfig(settings.SERVER_PATH, settings.SERVER_NAME, settings.SERVER_PASSWORD, settings.PASSWORD_ADMIN, settings.MAX_PLAYERS, settings.STEAM_QUERY_PORT, settings.LIGHTING_CONFIG_VALUE, settings.WORLD_NAME);
            installer.UpdateRunBatch(settings.SERVER_PATH, settings.SERVER_NAME, settings.EXTRA_MODS, settings.SERVER_PORT);
            installer.UpdateUpdateBatch(settings.SERVER_PATH, settings.STEAMCMD_PATH, settings.WORLD_NAME, settings.GIT_REPO_PATH, settings.CF_WORKSHOP_ID, settings.COT_WORKSHOP_ID, settings.EXP_WORKSHOP_ID, settings.EXP_LIC_WORKSHOP_ID, settings.EXP_VEH_WORKSHOP_ID, settings.EXP_COR_WORKSHOP_ID, settings.BR_WORKSHOP_ID, settings.EXTRA_MODS, settings.GIT_INSTALL_PATH);
            installer.UpdateSettings(settings.SERVER_PATH, settings.IP_ADDR, settings.SERVER_PORT, settings.STEAM_QUERY_PORT, settings.WORLD_NAME, settings.UNLOCK_SKINS, settings.USE_API, settings.API_ENDPOINT, settings.API_KEY, settings.BANS_API_KEY);

            Console.WriteLine("Running Update. Please wait...");
            installer.RunUpdate(settings.SERVER_PATH);
end: 
            Console.WriteLine("");
            Console.WriteLine("Install Complete! Press Any Key to Quit");
            Console.ReadKey();
        }



        public static bool GetYN(string question)
        {
            Console.Write(question);
            Console.Write("(y/N): ");
            string res = Console.ReadLine();
            if(res.Length > 0)
            {
                if (res.ToLower().Substring(0, 1) == "y")
                    return true;
            }
            return false;
        }
        public static string GetInput(string message)
        {
            Console.Write(message);
            Console.Write(": ");
            return Console.ReadLine();
        }
        public static string GetPath(string message)
        {
        retry:
            try
            {
                Console.Write(message);
                Console.Write(": ");
                string proper_path = Console.ReadLine();
                if (Directory.Exists(proper_path))
                {
                    return proper_path;
                }
                else
                {
                    Console.WriteLine("Invalid Path! You must enter an existing directory!");
                    goto retry;
                }
            } catch(Exception ex)
            {
                Console.WriteLine(ex.Message);
                goto retry;
            }
        }
    }
}
