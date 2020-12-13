using SharpCompress.Archives;
using SharpCompress.Common;
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace WinServInstaller
{
    class BRServerInstaller
    {
        private ObjectStore S3 { get; }

        public BRServerInstaller(ObjectStore s3_obj)
        {
            S3 = s3_obj;
        }

        public void InstallSteamCMD(string SteamCMD_Path)
        {
            if (SteamCMD_Path.EndsWith("/"))
                SteamCMD_Path = SteamCMD_Path.TrimEnd('/');
            Directory.CreateDirectory(SteamCMD_Path);

            //download latest steamcmd 7z from object store
            string latest_steamcmd_key = S3.GetLatestKey("server-install/steam-cmd");
            S3.WriteToFile("steamcmd.7z", latest_steamcmd_key);
            extract_to_folder("steamcmd.7z", SteamCMD_Path);
            //first time run steamcmd (downloads all the necessary files then quits)
            Process proc = new Process
            {
                StartInfo = new ProcessStartInfo
                {
                    FileName = SteamCMD_Path + "/steamcmd.exe",
                    Arguments = "+quit"
                }
            };
            proc.Start();
            proc.WaitForExit();
        }
        public void VerifySteamLogin(string SteamCMD_Path, string SteamUser, string SteamPass)
        {
            if (SteamCMD_Path.EndsWith("/"))
                SteamCMD_Path = SteamCMD_Path.TrimEnd('/');

            Process proc = new Process
            {
                StartInfo = new ProcessStartInfo
                {
                    FileName = SteamCMD_Path + "/steamcmd.exe",
                    Arguments = "+login " + SteamUser + " " + SteamPass + " +quit"
                }
            };
            proc.Start();
            proc.WaitForExit();
        }
        public void InstallServer(string SteamCMD_Path, string SteamUser, string SteamPass, string Server_Path, string AppId)
        {
            if (SteamCMD_Path.EndsWith("/"))
                SteamCMD_Path = SteamCMD_Path.TrimEnd('/');

            if (Server_Path.EndsWith("/"))
                Server_Path = Server_Path.TrimEnd('/');
            Directory.CreateDirectory(Server_Path);

            Process proc = new Process
            {
                StartInfo = new ProcessStartInfo
                {
                    FileName = SteamCMD_Path + "/steamcmd.exe",
                    Arguments = 
                        "+login " + SteamUser + " " + SteamPass + 
                        " +force_install_dir \"" + Server_Path + "\"" +
                        " +app_update " + AppId + " validate" +
                        " +quit"
                }
            };
            proc.Start();
            proc.WaitForExit();
        }

        public void InstallWritePatcher(string Server_Path)
        {
            if (Server_Path.EndsWith("/") || Server_Path.EndsWith("\\"))
                Server_Path = Server_Path.TrimEnd('/', '\\');
            Directory.CreateDirectory(Server_Path + "\\WritePatcher");

            //download & extract latest writepatcher application files
            string latest_writepatcher_key = S3.GetLatestKey("server-install/copy-to-root/WritePatcher");
            S3.WriteToFile("writepatcher.7z", latest_writepatcher_key);
            extract_to_folder("writepatcher.7z", Server_Path + "\\WritePatcher");
            //download patch server batch file
            S3.WriteToFile(Server_Path + "\\PATCH_SERVER.bat", "server-install/copy-to-root/PATCH_SERVER.bat");
        }

        public void InstallServerConfig(string Server_Path)
        {
            if (Server_Path.EndsWith("/") || Server_Path.EndsWith("\\"))
                Server_Path = Server_Path.TrimEnd('/', '\\');

            S3.WriteToFile(Server_Path + "\\serverDZ.cfg", "server-install/templates/serverDZ.cfg");
        }

        public void InstallGitForWindows()
        {
            string latest_git_for_win = S3.GetLatestKey("server-install/git-for-win");
            S3.WriteToFile("gitforwin.7z", latest_git_for_win);
            extract_to_folder("gitforwin.7z", "gitforwin");
            Process proc = new Process
            {
                StartInfo = new ProcessStartInfo
                {
                    FileName = "gitforwin\\installer.exe"
                }
            };
            proc.Start();
            proc.WaitForExit();
        }
        public void InstallBatchTemplates(string Server_Path)
        {
            if (Server_Path.EndsWith("/") || Server_Path.EndsWith("\\"))
                Server_Path = Server_Path.TrimEnd('/', '\\');

            S3.WriteToFile(Server_Path + "\\RUN_DAYZBR.bat", "server-install/templates/RUN_DAYZBR.bat");
            S3.WriteToFile(Server_Path + "\\UPDATE_SERVER.bat", "server-install/templates/UPDATE_SERVER.bat");
        }
        public void CloneGitRepo(string Git_Install_Path, string Get_Repo_Path, string git_branch)
        {
            if (Get_Repo_Path.EndsWith("/") || Get_Repo_Path.EndsWith("\\"))
                Get_Repo_Path = Get_Repo_Path.TrimEnd('/', '\\');
            Directory.CreateDirectory(Get_Repo_Path);

            //if this exists, assume git repo already cloned
            Process proc;
            string git_exe = Git_Install_Path + "\\git.exe";
            if (!Directory.Exists(Get_Repo_Path + "\\.git"))
            {
                proc = new Process
                {
                    StartInfo = new ProcessStartInfo
                    {
                        FileName = git_exe,
                        Arguments = "-C \"" + Get_Repo_Path + "\" clone https://gitlab.desolationredux.com/DayZ/DayZBR-Mod/BattleRoyale.git ."
                    }
                };
                proc.Start();
                proc.WaitForExit();
            }
            proc = new Process
            {
                StartInfo = new ProcessStartInfo
                {
                    FileName = git_exe,
                    Arguments = "-C \"" + Get_Repo_Path + "\" checkout " + git_branch
                }
            };
            proc.Start();
            proc.WaitForExit();
            proc = new Process
            {
                StartInfo = new ProcessStartInfo
                {
                    FileName = git_exe,
                    Arguments = "-C \"" + Get_Repo_Path + "\" pull"
                }
            };
            proc.Start();
            proc.WaitForExit();
        }
        public void InstallSettings(string Server_Path)
        {
            if (Server_Path.EndsWith("/") || Server_Path.EndsWith("\\"))
                Server_Path = Server_Path.TrimEnd('/', '\\');

            string profile_dir = Server_Path + "\\profiles";
            Directory.CreateDirectory(profile_dir);

            string[] s3_keys = S3.GetAllkeys("server-install/profiles/copy-to-profiles");
            int i = 1;
            foreach(string key in s3_keys)
            {
                S3.WriteToFile($"profiles_{i}.7z", key);
                extract_to_folder($"profiles_{i}.7z", profile_dir);
                i++;
            }
        }




        //--- updating template files
        public void UpdateSettings(string Server_Path,string ip, string query_port, string mission_name, string unlock_skins, string use_api, string api_endpoint, string api_key, string bans_api_key)
        {
            if (Server_Path.EndsWith("/") || Server_Path.EndsWith("\\"))
                Server_Path = Server_Path.TrimEnd('/', '\\');

            string settings_dir = Server_Path + "\\profiles\\BattleRoyale";

            //--- server_settings
            string file_contents = File.ReadAllText(settings_dir + "\\server_settings.json");
            file_contents = file_contents.Replace("##QUERY_PORT##", query_port);
            file_contents = file_contents.Replace("##IP_ADDR##", ip);
            File.WriteAllText(settings_dir + "\\server_settings.json", file_contents);

            //--- general_settings
            file_contents = File.ReadAllText(settings_dir + "\\general_settings.json");
            file_contents = file_contents.Replace("##MISSION_NAME##", mission_name);
            File.WriteAllText(settings_dir + "\\general_settings.json", file_contents);

            //--- api_settings
            file_contents = File.ReadAllText(settings_dir + "\\api_settings.json");
            file_contents = file_contents.Replace("##API_ENDPOINT##", api_endpoint);
            file_contents = file_contents.Replace("##API_KEY##", api_key);
            file_contents = file_contents.Replace("##BANS_API_KEY##", bans_api_key);
            file_contents = file_contents.Replace("##USE_API##", use_api);
            file_contents = file_contents.Replace("##UNLOCK_SKINS##", unlock_skins);
            File.WriteAllText(settings_dir + "\\api_settings.json", file_contents);
        }
        public void UpdateUpdateBatch(string Server_Path, string steamcmd_path, string world_name, string git_repo_path, string cf_ws_id, string cot_ws_id, string ex_ws_id, string ex_lic_ws_id, string ex_veh_ws_id, string ex_cor_ws_id, string br_ws_id, string extra_ws_ids, string git_install_path)
        {
            if (Server_Path.EndsWith("/") || Server_Path.EndsWith("\\"))
                Server_Path = Server_Path.TrimEnd('/', '\\');

            string file_contents = File.ReadAllText(Server_Path + "\\UPDATE_SERVER.bat");

            file_contents = file_contents.Replace("##STEAMCMD_PATH##", steamcmd_path);
            file_contents = file_contents.Replace("##INSTALL_PATH##", Server_Path);
            file_contents = file_contents.Replace("##WORLD_NAME##", world_name);
            file_contents = file_contents.Replace("##GIT_REPO_PATH##", git_repo_path);
            file_contents = file_contents.Replace("##CF_WORKSHOP_ID##", cf_ws_id);
            file_contents = file_contents.Replace("##COT_WORKSHOP_ID##", cot_ws_id);
            file_contents = file_contents.Replace("##EXPANSION_WORKSHOP_ID##", ex_ws_id);
            file_contents = file_contents.Replace("##EXPANSION_LICENSED_WORKSHOP_ID##", ex_lic_ws_id);
            file_contents = file_contents.Replace("##EXPANSION_VEHICLES_WORKSHOP_ID##", ex_veh_ws_id);
            file_contents = file_contents.Replace("##EXPANSION_CORE_WORKSHOP_ID##", ex_cor_ws_id);
            file_contents = file_contents.Replace("##BATTLEROYALE_WORKSHOP_ID##", br_ws_id);

            string[] extra_workshop_ids = extra_ws_ids.Split(';');
            string steamcmd_extra_ws_downloads = "";
            foreach(string workshop_id in extra_workshop_ids)
            {
                steamcmd_extra_ws_downloads += "\r\n +workshop_download_item 221100 " + workshop_id + " ^";
            }

            file_contents = file_contents.Replace("##EXTRA_WORKSHOP_MODS_DOWNLOAD_ITEM_VALUE##", steamcmd_extra_ws_downloads);

            string robocopy_extra_ws_copies = "";
            foreach(string workshop_id in extra_workshop_ids)
            {
                robocopy_extra_ws_copies += "robocopy \"%ServerPath%\\%WorkshopSubpath%\\" + workshop_id + "\" \"%ServerPath%\\@" + workshop_id + "\" /MIR /NJS /NJH /NDL \r\n";
            }
            
            file_contents = file_contents.Replace("##EXTRA_WORKSHOP_MODS_COPY_CONTENT_VALUE##", robocopy_extra_ws_copies);

            string xcopy_extra_ws_copies = "";
            foreach (string workshop_id in extra_workshop_ids)
            {
                xcopy_extra_ws_copies += "xcopy /s \"%ServerPath%\\@" + workshop_id + "\\Keys\\*.*\" \"%ServerPath%\\keys\\\" /K /D /H /Y" + "\r\n";
            }
            file_contents = file_contents.Replace("##EXTRA_WORKSHOP_MODS_COPY_KEYS_VALUE##", xcopy_extra_ws_copies);


            file_contents = file_contents.Replace("##GIT_INSTALL_PATH##", git_install_path);

            File.WriteAllText(Server_Path + "\\UPDATE_SERVER.bat", file_contents);
        }
        public void UpdateRunBatch(string Server_Path, string server_name, string extra_mods, string server_port)
        {
            if (Server_Path.EndsWith("/") || Server_Path.EndsWith("\\"))
                Server_Path = Server_Path.TrimEnd('/', '\\');

            string file_contents = File.ReadAllText(Server_Path + "\\RUN_DAYZBR.bat");

            //generate mod list
            string[] new_mod_workshop_ids = extra_mods.Split(';');
            string new_mod_lines = "";
            foreach (string workshop_id in new_mod_workshop_ids)
            {
                new_mod_lines += @"%ServerPath%\@" + workshop_id + ";";  //all new mods are stored in folder @[workshopid] in root
            }
            string mod_list = @"%ServerPath%\@CF;%ServerPath%\@Community-Online-Tools;%ServerPath%\@DayZ-Expansion-;%ServerPath%\@DayZ-Expansion-Licensed-;%ServerPath%\@DayZ-Expansion-Vehicles-;%ServerPath%\@DayZ-Expansion-Core-;" + new_mod_lines + @"%ServerPath%\@BattleRoyale";


            file_contents = file_contents.Replace("##SERVER_NAME##", server_name);
            file_contents = file_contents.Replace("##INSTALL_PATH##", Server_Path);
            file_contents = file_contents.Replace("##MOD_LIST##", mod_list);
            file_contents = file_contents.Replace("##SERVER_PORT##", server_port);

            File.WriteAllText(Server_Path + "\\RUN_DAYZBR.bat", file_contents);
        }
        public void UpdateConfig(string Server_Path, string server_name, string password, string password_admin, string max_players, string query_port, string lighting_value, string world_name)
        {
            if (Server_Path.EndsWith("/") || Server_Path.EndsWith("\\"))
                Server_Path = Server_Path.TrimEnd('/', '\\');

            string file_contents = File.ReadAllText(Server_Path + "\\serverDZ.cfg");

            file_contents = file_contents.Replace("##SERVER_NAME##", server_name);
            file_contents = file_contents.Replace("##PASSWORD##", password);
            file_contents = file_contents.Replace("##PASSWORD_ADMIN##", password_admin);
            file_contents = file_contents.Replace("##MAX_PLAYERS##", max_players);
            file_contents = file_contents.Replace("##STEAM_QUERY_PORT##", query_port);
            file_contents = file_contents.Replace("##LIGHTING_CONFIG##", lighting_value);
            file_contents = file_contents.Replace("##WORLD_NAME##", world_name);

            File.WriteAllText(Server_Path + "\\serverDZ.cfg", file_contents);
        }
        
        public void RunUpdate(string Server_Path)
        {
            if (Server_Path.EndsWith("/") || Server_Path.EndsWith("\\"))
                Server_Path = Server_Path.TrimEnd('/', '\\');

            Process.Start(Server_Path + "\\UPDATE_SERVER.bat").WaitForExit();
        }

        //extracts an archive to folder (overwriting existing files if necessary)
        private void extract_to_folder(string zipfile, string path)
        {
            using (var archive = ArchiveFactory.Open(zipfile))
            {
                foreach(var entry in archive.Entries)
                {
                    if(!entry.IsDirectory)
                    {
                        entry.WriteToDirectory(path, new ExtractionOptions() { ExtractFullPath = true, Overwrite = true });
                    }
                }
            }
        }
    }
}
