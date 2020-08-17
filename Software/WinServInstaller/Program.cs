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
            if (!GetYN("Is SteamCMD installed already?"))
                return;
            if (!GetYN("Is the Git repository cloned?"))
                return;
            if (!GetYN("Is the steam user already authenticated through SteamCMD?"))
                return;

            string steamcmd_path = GetPath("Enter the directory of your steamcmd.exe install").TrimEnd('\\'); ;
            string gitlab_path = GetPath("Enter the directory of your gitlab repository").TrimEnd('\\'); ;

            string install_path = GetPath("Enter the directory to install the server into").TrimEnd('\\');

            string server_name = "[DayZ] Battle Royale " + GetInput("Enter the subname of the server");
            int port = -1;
            string server_port;
            do
            {
                server_port = GetInput("Enter the port of the server");
            } while (!int.TryParse(server_port, out port));
            int query_port = -1;
            do
            {
                server_port = GetInput("Enter the query port of the server");
            } while (!int.TryParse(server_port, out query_port));

            string git_install_path;
            do
            {
                string path = GetInput("Enter your git install path (leave blank for default)");
                if(path.Trim() != "")
                {
                    git_install_path = path;
                }
                else
                {
                    git_install_path = @"C:\Program Files\Git\bin";
                }
            } while (!Directory.Exists(git_install_path));
            string server_ip;
            string path = GetInput("Enter your server IP (leave blank for default)");
            if (path.Trim() != "")
            {
                server_ip = path;
            }
            else
            {
                server_ip = "127.0.0.1";
            }

            string APIKey = GetInput("Enter your DayZBR API Key");

            string steam_user = GetInput("Enter steam username");
            string steam_pass = GetInput("Enter steam password");

            string run_dayzbr_bat = Properties.Resources.RUN_DAYZBR
                .Replace("##SERVERPATH##", install_path)
                .Replace("##SERVERNAME##", server_name)
                .Replace("##PORT##", port.ToString());
            File.WriteAllText(install_path + "\\RUN_DAYZBR.bat", run_dayzbr_bat);

            string update_server = Properties.Resources.UPDATE_SERVER
                .Replace("##GITINSTALLPATH##", git_install_path)
                .Replace("##STEAMCMD##", steamcmd_path)
                .Replace("##SERVERPATH##", install_path)
                .Replace("##GITLABPATH##", gitlab_path)
                .Replace("##STEAMUSER##", steam_user)
                .Replace("##STEAMPASS##", steam_pass);
            File.WriteAllText(install_path + "\\UPDATE_SERVER.bat", update_server);

            if (!Directory.Exists($"{install_path}\\profiles"))
                Directory.CreateDirectory($"{install_path}\\profiles");

            Console.WriteLine("");

            //--- TODO: handle JSON configs in the profile folder (done in the UPDATE_SERVER.bat file as it needs to run every restart)
            var process = new Process();
            var startinfo = new ProcessStartInfo("cmd.exe", "/C \"" + install_path + "\\UPDATE_SERVER.bat\"");
            startinfo.RedirectStandardOutput = true;
            startinfo.RedirectStandardError = true;
            startinfo.UseShellExecute = false;
            process.StartInfo = startinfo;
            process.ErrorDataReceived += (sender, outp) => Console.WriteLine(outp.Data);
            process.OutputDataReceived += (sender, outp) => Console.WriteLine(outp.Data); // do whatever processing you need to do in this handler
            process.Start();
            process.BeginOutputReadLine();
            process.BeginErrorReadLine();
            process.WaitForExit();

            Console.WriteLine("");
            Console.WriteLine("");
            Console.WriteLine("Install Complete --- Configuring...");
            Console.WriteLine("");
            Console.WriteLine("");

            string serverDZ = File.ReadAllText($"{install_path}\\serverDZ.cfg");

            if (!Directory.Exists($"{install_path}\\profiles"))
                Directory.CreateDirectory($"{install_path}\\profiles");

            //TODO: pull configs via git

            //--- TODO: update serverDZ.cfg
            /*
             * ======== new data ====
             
                steamQueryPort = 2403;
                networkRangeClose = 20;			// network bubble distance for spawn of close objects with items in them (f.i. backpacks), set in meters, default value if not set is 20
                networkRangeNear = 150;			// network bubble distance for spawn (despawn +10%) of near inventory items objects, set in meters, default value if not set is 150
                networkRangeFar = 1000;			// network bubble distance for spawn (despawn +10%) of far objects (other than inventory items), set in meters, default value if not set is 1000
                networkRangeDistantEffect = 4000; // network bubble distance for spawn of effects (currently only sound effects), set in meters, default value if not set is 4000

                defaultVisibility=1375;			// highest terrain render distance on server (if higher than "viewDistance=" in DayZ client profile, clientside parameter applies)
                defaultObjectViewDistance=1375;	// highest object render distance on server (if higher than "preferredObjectViewDistance=" in DayZ client profile, clientside parameter applies)


            ==== replace data 
                hostname = "[Beta] DayZ Battle Royale - Chicago #2";  // Server name
                password = "PASSWORD HERE";              // Password to connect to the server
                passwordAdmin = "PASSWORD HERE";         // Password to become a server admin

                instanceId = 2;             // DayZ server instance id, to identify the number of instances per box and their storage folders with persistence files
                class Missions
                {
                    class DayZ
                    {
                        template="BattleRoyale.ChernarusPlusGloom";
                    };
                };
             
             */


            //handle unique server BR configs
            if (!Directory.Exists($"{install_path}\\profiles\\BattleRoyale"))
                Directory.CreateDirectory($"{install_path}\\profiles\\BattleRoyale");
            
            File.WriteAllText($"{install_path}\\profiles\\BattleRoyale\\api_settings.json", Properties.Resources.API_Settings.Replace("##APIKEY##", APIKey));
            File.WriteAllText($"{install_path}\\profiles\\BattleRoyale\\server_settings.json", Properties.Resources.Server_Settings.Replace("##IPADDR##", server_ip).Replace("##QUERYPORT##", query_port.ToString());



            Console.WriteLine("");
            Console.WriteLine("Install Complete! Press Any Key to Quit");
            Console.ReadKey();
        }
        static bool GetYN(string question)
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
        static string GetInput(string message)
        {
            Console.Write(message);
            Console.Write(": ");
            return Console.ReadLine();
        }
        static string GetPath(string message)
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
