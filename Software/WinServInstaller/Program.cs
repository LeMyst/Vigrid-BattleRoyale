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

            Console.WriteLine("");

            var process = new Process();
            var startinfo = new ProcessStartInfo("cmd.exe", "/C \"" + install_path + "\\UPDATE_SERVER.bat\"");
            startinfo.RedirectStandardOutput = true;
            startinfo.UseShellExecute = false;
            process.StartInfo = startinfo;
            process.OutputDataReceived += (sender, outp) => Console.WriteLine(outp.Data); // do whatever processing you need to do in this handler
            process.Start();
            process.BeginOutputReadLine();
            process.WaitForExit();

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
