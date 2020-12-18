using System;
using System.Collections.Generic;
using System.IO;

namespace UnlockWritePatcher
{
    class Program
    {
        static void Main(string[] args)
        {
            Console.Title = "DayZ Write Patcher";
            //parse arguments
            string exe = "DayZServer_x64.exe";
            string pattern1 = "83 FF 01 0F 84 95 00 00 00";
            string pattern2 = "0F 84 95 00 00 00 48 8D 0D ? ? ? ?";
            int byteoff1 = 2;
            int byteoff2 = 1;
            bool backup = true;

            foreach (var arg in args)
            {
                string[] parts = arg.Split('=');
                if(parts.Length > 1)
                {
                    string prefix = parts[0];
                    string suffix = string.Join('=', parts, 1, parts.Length-1);
                    
                    switch(prefix)
                    {
                        case "exe": exe = suffix; break;
                        case "pattern1": pattern1 = suffix; break;
                        case "pattern2": pattern2 = suffix; break;
                        case "byteoff1": byteoff1 = int.Parse(suffix); break;
                        case "byteoff2": byteoff2 = int.Parse(suffix); break;
                        case "backup": backup = (suffix == "true"); break;
                    }
                }
            }


            byte[] file_data = File.ReadAllBytes(exe);
            int index_pattern_1 = FindPattern(pattern1, file_data);
            if (index_pattern_1 == -1)
            {
                Console.WriteLine("Failed to find Pattern1");
                return;
            }
            int index_pattern_2 = FindPattern(pattern2, file_data);
            if (index_pattern_2 == -1)
            {
                Console.WriteLine("Failed to find Pattern1");
                return;
            }
            index_pattern_1 += byteoff1;
            index_pattern_2 += byteoff2;

            

            Console.WriteLine("Patching DayZ...");

            Console.Write("Changing 0x" + file_data[index_pattern_1].ToString("X2"));
            Console.WriteLine(" To 0x78");
            Console.Write("Changing 0x" + file_data[index_pattern_2].ToString("X"));
            Console.WriteLine(" To 0x85");

            if (backup)
            {
                string backup_name = exe + ".bak";
                /*while (File.Exists(backup_name))
                    backup_name += ".bak";*/

                File.WriteAllBytes(backup_name, file_data);
            }

            file_data[index_pattern_1] = 0x78; //0x1 => 0x78
            file_data[index_pattern_2] = 0x85; //0x84 => 0x85

            File.WriteAllBytes(exe, file_data);
            Console.WriteLine("Patched Successfully!");
        }
        static int FindPattern(string pattern_string, byte[] file_data)
        {
            string[] parts = pattern_string.Split(' ');
            List<int> bytes_l = new List<int>();
            List<char> mask_l = new List<char>();
            foreach(string hex in parts)
            {
                if (hex.Contains("?"))
                {
                    mask_l.Add('?');
                    bytes_l.Add(0);
                }
                else
                {
                    mask_l.Add('x');
                    bytes_l.Add(Convert.ToInt32(hex, 16));
                }
            }
            int[] bytes = bytes_l.ToArray();
            char[] mask = mask_l.ToArray();

            for(int i = 0; i < file_data.Length; i++)
            {
                if (CheckPatternMask(bytes, mask, file_data, i))
                {
                    return i;
                }
            }
            return -1;
        }
        static bool CheckPatternMask(int[] bytes, char[] mask, byte[] file_data, int index)
        {
            for(int i = 0; i < bytes.Length; i++)
            {
                if (mask[i] == '?')
                    continue;
                if (bytes[i] != file_data[index + i])
                    return false;
            }
            return true;
        }

    }
}
