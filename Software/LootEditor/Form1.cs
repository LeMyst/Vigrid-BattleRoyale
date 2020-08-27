using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;
using System.IO;
using Newtonsoft.Json;

namespace LootEditor
{
    public partial class Form1 : Form
    {
        Dictionary<string, Category> categories = new Dictionary<string, Category>();
        Dictionary<string, CategoryEntries> entries = new Dictionary<string, CategoryEntries>();
        Dictionary<string, List<Control>> entry_controls = new Dictionary<string, List<Control>>();

        public Form1()
        {
            InitializeComponent();
        }

        private void button1_Click(object sender, EventArgs e)
        {
            FolderBrowserDialog fbd = new FolderBrowserDialog();
            if (fbd.ShowDialog() == DialogResult.OK)
            {
                textBox1.Text = fbd.SelectedPath;
                LoadSettings(textBox1.Text);
            }
        }


        
        private void InvokeUI(Action a)
        {
            this.BeginInvoke(new MethodInvoker(a));
        }

        void LoadSettings(string path)
        {
            categories.Clear();
            entries.Clear();
            foreach(KeyValuePair<string, List<Control>> kvp in entry_controls)
            {
                foreach(Control ctrl in kvp.Value)
                {
                    try
                    {
                        ctrl.Parent.Controls.Remove(ctrl);
                    }
                    catch { } //may run into some issues doing the above so we'll do it here
                }
            }
            entry_controls.Clear();
            if (!Directory.Exists(path + @"\Loot"))
            {
                MessageBox.Show("Invalid Path!");
                return;
            }
            if (!Directory.Exists(path + @"\Loot\Categories"))
            {
                MessageBox.Show("Invalid Path!");
                return;
            }
            if (!Directory.Exists(path + @"\Loot\Entries"))
            {
                MessageBox.Show("Invalid Path!");
                return;
            }

            foreach (string file in Directory.EnumerateFiles(path + @"\Loot\Categories", "*.json"))
            {
                string category_name = Path.GetFileNameWithoutExtension(file).ToLower();
                Category category = JsonConvert.DeserializeObject<Category>(File.ReadAllText(file));
                categories.Add(category_name, category);
            }
            foreach (string file in Directory.EnumerateFiles(path + @"\Loot\Entries", "*.json"))
            {
                string entry_name = Path.GetFileNameWithoutExtension(file).ToLower();
                Entry entry = JsonConvert.DeserializeObject<Entry>(File.ReadAllText(file));

                if(!entries.ContainsKey(entry.category.ToLower()))
                {
                    entries.Add(entry.category.ToLower(), new CategoryEntries());
                }
                entries[entry.category.ToLower()].entries.Add(entry_name, entry);
            }


            Task.Run(() =>
            {
                foreach (KeyValuePair<string, CategoryEntries> entry in entries)
                {
                    if (!categories.ContainsKey(entry.Key))
                    {
                        MessageBox.Show("OH NO! UNKNOWN CATEGORY!\nCategory: " + entry.Key + "\nExample Entry: " + entry.Value.entries.ElementAt(0));
                    }

                    TabPage new_page = new TabPage(entry.Key);
                    new_page.UseVisualStyleBackColor = true;
                    new_page.AutoScroll = true;
                    InvokeUI(() => {
                        tabControl1.TabPages.Add(new_page);
                    });
                    
                    tabControl1.Invalidate();

                    FlowLayoutPanel flp = new FlowLayoutPanel();
                    flp.Dock = DockStyle.None;
                    flp.AutoSize = true;
                    flp.AutoSizeMode = AutoSizeMode.GrowAndShrink;
                    flp.Location = new Point(0, 10);
                    flp.Size = new Size(1500, 50);
                    InvokeUI(() =>
                    {
                        new_page.Controls.Add(flp);

                    });

                    tabControl1.Selected += (s2, e2) =>
                    {
                        if (tabControl1.SelectedTab == new_page)
                        {
                            if (new_page.Controls[0].Controls.Count > 0)
                                return;

                            Task.Run(() =>
                            {

                                #region Category Control Actions
                                Label label = new Label();
                                label.Text = "Category Weight: ";
                                InvokeUI(() =>
                                {
                                    flp.Controls.Add(label);
                                });

                                TextBox textbox = new TextBox();
                                textbox.Text = categories[entry.Key].weight.ToString();
                                textbox.LostFocus += (s, e) =>
                                {
                                    float value = 0;
                                    if (!float.TryParse(textbox.Text, out value))
                                    {
                                        textbox.Text = categories[entry.Key].weight.ToString();
                                        return;
                                    }
                                    categories[entry.Key].weight = value;
                                };
                                InvokeUI(() =>
                                {
                                    flp.Controls.Add(textbox);
                                });


                                Button reset_btn = new Button();
                                reset_btn.Width = 80;
                                reset_btn.Text = "Reset Weights";
                                reset_btn.Click += (s, e) =>
                                {
                                    foreach(Control ctrl in flp.Controls)
                                    {
                                        if(ctrl is TextBox txtbx)
                                        {
                                            txtbx.Text = "1.0";
                                            txtbx.Focus(); //triggers the action to update in the dictionaries
                                        }
                                    }
                                    reset_btn.Focus();
                                };
                                InvokeUI(() =>
                                {
                                    flp.Controls.Add(reset_btn);
                                });
                                Button add_item_btn = new Button();
                                add_item_btn.Width = 80;
                                add_item_btn.Text = "Add Item";
                                add_item_btn.Click += (s, e) =>
                                {
                                    string input = Microsoft.VisualBasic.Interaction.InputBox("New Name: ", "Rename").ToLower();
                                    if (input == "")
                                        return;

                                    if (input.ToCharArray().Intersect(Path.GetInvalidFileNameChars()).Count() > 0)
                                    {
                                        MessageBox.Show("Invalid Name! Use only file-safe characters!");
                                        return;
                                    }

                                    if(entry.Value.entries.ContainsKey(input))
                                    {
                                        MessageBox.Show("Invalid Name! An item with that name already exists!");
                                        return;
                                    }

                                    Entry new_entry = new Entry();
                                    new_entry.category = entry.Key;
                                    new_entry.needs_ammo = 0;
                                    new_entry.needs_attachments = 0;
                                    new_entry.needs_magazines = 0;
                                    new_entry.weight = 1;
                                    new_entry.styles = new string[0];
                                    new_entry.spawn_with = new string[0];

                                    entry.Value.entries.Add(input, new_entry);
                                    entry_controls.Add(input, CreateControls(flp, new KeyValuePair<string, Entry>(input, new_entry), entry));

                                    UpdatePercentChanceToSpawn(entry.Key);
                                };
                                InvokeUI(() =>
                                {
                                    flp.Controls.Add(add_item_btn);
                                });

                                flp.SetFlowBreak(add_item_btn, true); //new line
                                #endregion

                                foreach (KeyValuePair<string, Entry> kvp in entry.Value.entries)
                                {
                                    entry_controls.Add(kvp.Key, CreateControls(flp, kvp, entry));
                                }
                                UpdatePercentChanceToSpawn(entry.Key);
                            });

                        }
                    };
                    
                }
            });
            
            
        }

        private void UpdatePercentChanceToSpawn(string category)
        {
            
            if(entries.ContainsKey(category))
            {
                float total_weight = 0;
                foreach (var val in entries[category].entries)
                {
                    total_weight += val.Value.weight;
                }

                foreach (var val in entries[category].entries)
                {
                    float weight = val.Value.weight;
                    float percent = (100 * weight) / total_weight;

                    foreach(Control ctrl in entry_controls[val.Key])
                    {
                        if(ctrl is Label lbl)
                        {
                            if(lbl.Text.StartsWith("Spawn Chance: "))
                            {
                                InvokeUI(() =>
                                {
                                    lbl.Text = "Spawn Chance: " + percent.ToString("N4") + "%";
                                });
                            }
                        }
                    }
                }
            }
        }
        private List<Control> CreateControls(FlowLayoutPanel flp, KeyValuePair<string, Entry> kvp, KeyValuePair<string, CategoryEntries> entry)
        {
            List<Control> all_controls = new List<Control>();
            Entry loot = kvp.Value;

            Label item_name = new Label();
            all_controls.Add(item_name);
            item_name.Width = 80;
            item_name.Text = kvp.Key;
            InvokeUI(() =>
            {
                flp.Controls.Add(item_name);
            });

            Label spawn_chance = new Label();
            all_controls.Add(spawn_chance);
            spawn_chance.Width = 150;
            spawn_chance.Text = "Spawn Chance: 0%";
            InvokeUI(() =>
            {
                flp.Controls.Add(spawn_chance);
            });

            Label weight = new Label();
            all_controls.Add(weight);
            weight.Width = 60;
            weight.TextAlign = ContentAlignment.MiddleRight;
            weight.Text = "Weight: ";
            InvokeUI(() =>
            {
                flp.Controls.Add(weight);
            });


            TrackBar slider = new TrackBar(); //defined here so the textbox can interact with it
            TextBox winput = new TextBox();
            all_controls.Add(winput);
            winput.Width = 150;
            winput.Text = loot.weight.ToString();
            winput.LostFocus += (s, e) =>
            {
                float value = 0;
                if (!float.TryParse(winput.Text, out value))
                {
                    winput.Text = loot.weight.ToString();
                    return;
                }
                loot.weight = value;
                int slider_val = (int)(loot.weight * 1000);
                if(slider_val > slider.Maximum)
                { 
                    slider.Maximum = slider_val; 
                }
                slider.Value = slider_val;
                UpdatePercentChanceToSpawn(entry.Key);
            };
            winput.KeyDown += (s, e) =>
            {
                if (e.KeyCode == Keys.Enter)
                {
                    float value = 0;
                    if (!float.TryParse(winput.Text, out value))
                    {
                        winput.Text = loot.weight.ToString();
                        return;
                    }
                    loot.weight = value;
                    int slider_val = (int)(loot.weight * 1000);
                    if (slider_val > slider.Maximum)
                    {
                        slider.Maximum = slider_val;
                    }
                    slider.Value = slider_val;
                    UpdatePercentChanceToSpawn(entry.Key);
                }
            };
            InvokeUI(() =>
            {
                flp.Controls.Add(winput);
            });

            all_controls.Add(slider);
            slider.Maximum = 10000 < (int)(loot.weight * 1000) ? (int)(loot.weight * 1000) : 10000;
            slider.Value = (int)(loot.weight * 1000);
            slider.TickStyle = TickStyle.None;
            slider.AutoSize = false;
            slider.Size = new Size(200, 25);
            slider.ValueChanged += (s, e) =>
            {
                if(slider.Enabled)
                {
                    //handle because it's uyser input
                    float new_weight = Convert.ToSingle(slider.Value) / 1000.0f;
                    loot.weight = new_weight;
                    winput.Text = new_weight.ToString();
                    UpdatePercentChanceToSpawn(entry.Key);
                }
            };
            InvokeUI(() =>
            {
                flp.Controls.Add(slider);
            });

            CheckBox needs_mags = new CheckBox();
            all_controls.Add(needs_mags);
            needs_mags.Width = 110;
            needs_mags.Text = "Spawn with Mags";
            needs_mags.Checked = loot.needs_magazines == 1;
            needs_mags.CheckedChanged += (s, e) =>
            {
                if (needs_mags.Checked)
                    loot.needs_magazines = 1;
                else
                    loot.needs_magazines = 0;
            };
            InvokeUI(() =>
            {
                flp.Controls.Add(needs_mags);
            });

            CheckBox needs_ammo = new CheckBox();
            all_controls.Add(needs_ammo);
            needs_ammo.Width = 120;
            needs_ammo.Text = "Spawn with Ammo";
            needs_ammo.Checked = loot.needs_ammo == 1;
            needs_ammo.CheckedChanged += (s, e) =>
            {
                if (needs_ammo.Checked)
                    loot.needs_ammo = 1;
                else
                    loot.needs_ammo = 0;
            };
            InvokeUI(() =>
            {
                flp.Controls.Add(needs_ammo);
            });

            CheckBox needs_atch = new CheckBox();
            all_controls.Add(needs_atch);
            needs_atch.Width = 150;
            needs_atch.Text = "Spawn with Attachments";
            needs_atch.Checked = loot.needs_attachments == 1;
            needs_atch.CheckedChanged += (s, e) =>
            {
                if (needs_atch.Checked)
                    loot.needs_attachments = 1;
                else
                    loot.needs_attachments = 0;
            };
            InvokeUI(() =>
            {
                flp.Controls.Add(needs_atch);
            });

            Label styles_label = new Label();
            all_controls.Add(styles_label);
            styles_label.Width = 45;
            styles_label.TextAlign = ContentAlignment.MiddleRight;
            styles_label.Text = "Styles: ";
            InvokeUI(() =>
            {
                flp.Controls.Add(styles_label);
            });

            ListBox styles = new ListBox();
            all_controls.Add(styles);
            foreach (string item in loot.styles)
            {
                styles.Items.Add(item);
            }
            styles.Height = 50;
            MenuItem add_style = new MenuItem("Add");
            add_style.Click += (s, e) =>
            {
                string new_style = Microsoft.VisualBasic.Interaction.InputBox("CfgWeapons Class Name: ", "Add Style");
                if (new_style == "")
                    return;

                styles.Items.Add(new_style);
                List<string> styles_list = loot.styles.ToList();
                styles_list.Add(new_style);
                loot.styles = styles_list.ToArray();
            };
            MenuItem remove_style = new MenuItem("Remove");
            remove_style.Click += (s, e) =>
            {
                int index = styles.SelectedIndex;
                if (index == -1)
                    return;

                styles.Items.RemoveAt(index);
                List<string> styles_list = loot.styles.ToList();
                styles_list.RemoveAt(index);
                loot.styles = styles_list.ToArray();
            };
            styles.ContextMenu = new ContextMenu(new MenuItem[] { add_style, remove_style });
            InvokeUI(() =>
            {
                flp.Controls.Add(styles);
            });

            Label spawnwith_label = new Label();
            all_controls.Add(spawnwith_label);
            spawnwith_label.Width = 80;
            spawnwith_label.TextAlign = ContentAlignment.MiddleRight;
            spawnwith_label.Text = "Spawn With: ";
            InvokeUI(() =>
            {
                flp.Controls.Add(spawnwith_label);
            });

            ListBox spawnwith = new ListBox();
            all_controls.Add(spawnwith);
            foreach (string item in loot.spawn_with)
            {
                spawnwith.Items.Add(item);
            }
            spawnwith.Height = 50;
            MenuItem add_spawnwith = new MenuItem("Add");
            add_spawnwith.Click += (s, e) =>
            {
                string new_spawn = Microsoft.VisualBasic.Interaction.InputBox("Class Name: ", "Add Spawn With");
                if (new_spawn == "")
                    return;

                spawnwith.Items.Add(new_spawn);
                List<string> spawnwith_list = loot.spawn_with.ToList();
                spawnwith_list.Add(new_spawn);
                loot.spawn_with = spawnwith_list.ToArray();
            };
            MenuItem remove_spawnwith = new MenuItem("Remove");
            remove_spawnwith.Click += (s, e) =>
            {
                int index = spawnwith.SelectedIndex;
                if (index == -1)
                    return;

                spawnwith.Items.RemoveAt(index);
                List<string> spawnwith_list = loot.spawn_with.ToList();
                spawnwith_list.RemoveAt(index);
                loot.spawn_with = spawnwith_list.ToArray();
            };
            spawnwith.ContextMenu = new ContextMenu(new MenuItem[] { add_spawnwith, remove_spawnwith });
            InvokeUI(() =>
            {
                flp.Controls.Add(spawnwith);
            });

            Button rename_item = new Button();
            all_controls.Add(rename_item);
            rename_item.Text = "Rename Item";
            rename_item.Width = 80;
            rename_item.Click += (s, e) =>
            {
                string input = Microsoft.VisualBasic.Interaction.InputBox("New Name: ", "Rename").ToLower();
                if (input == "")
                    return;

                if (input.ToCharArray().Intersect(Path.GetInvalidFileNameChars()).Count() > 0)
                {
                    MessageBox.Show("Invalid Name! Use only file-safe characters!");
                    return;
                }
                item_name.Text = input;
                //insert our 
                entry.Value.entries.Add(input, kvp.Value);
                entry.Value.entries.Remove(kvp.Key);
            };
            InvokeUI(() =>
            {
                flp.Controls.Add(rename_item);
            });

            Button remove_item_btn = new Button();
            all_controls.Add(remove_item_btn);
            remove_item_btn.Text = "Remove Item";
            remove_item_btn.Width = 80;
            remove_item_btn.Click += (s, e) =>
            {
                //TODO: delete controls
                try
                {
                    foreach (Control ctrl in entry_controls[kvp.Key])
                    {
                        try
                        {
                            InvokeUI(() =>
                            {
                                flp.Controls.Remove(ctrl);
                            });
                        }
                        catch { }
                    }
                    entry.Value.entries.Remove(kvp.Key);
                }
                catch { }

                UpdatePercentChanceToSpawn(entry.Key);
            };
            InvokeUI(() =>
            {
                flp.Controls.Add(remove_item_btn);
            });

            Button duplicate_item_btn = new Button();
            all_controls.Add(duplicate_item_btn);
            duplicate_item_btn.Text = "Duplicate Item";
            duplicate_item_btn.Width = 90;
            duplicate_item_btn.Click += (s, e) =>
            {
                Entry new_entry = new Entry();
                new_entry.category = kvp.Value.category;
                new_entry.needs_ammo = kvp.Value.needs_ammo;
                new_entry.needs_attachments = kvp.Value.needs_attachments;
                new_entry.needs_magazines = kvp.Value.needs_magazines;
                new_entry.weight = kvp.Value.weight;
                new_entry.styles = new string[kvp.Value.styles.Length];
                new_entry.spawn_with = new string[kvp.Value.spawn_with.Length];
                Array.Copy(kvp.Value.styles, 0, new_entry.styles, 0, kvp.Value.styles.Length);
                Array.Copy(kvp.Value.spawn_with, 0, new_entry.spawn_with, 0, kvp.Value.spawn_with.Length);


                string entry_name = kvp.Key;
                do
                {
                    entry_name += "_copy";
                } while (entry.Value.entries.ContainsKey(entry_name));
                entry.Value.entries.Add(entry_name, new_entry);
                entry_controls.Add(entry_name, CreateControls(flp, new KeyValuePair<string, Entry>(entry_name, new_entry), entry));

                UpdatePercentChanceToSpawn(entry.Key);
            };
            InvokeUI(() =>
            {
                flp.Controls.Add(duplicate_item_btn);
            });


            flp.SetFlowBreak(duplicate_item_btn, true);

            return all_controls;
        }


        private void button2_Click(object sender, EventArgs e)
        {
            //TODO: delete all existing json files (rename feature)

            foreach (var kvp in categories)
            {
                string file = textBox1.Text + @"\Loot\Categories\" + kvp.Key + ".json";
                string json = JsonConvert.SerializeObject(kvp.Value, Formatting.Indented);
                File.WriteAllText(file, json);
            }
            foreach(var kvp in entries)
            {
                foreach(var kvp2 in kvp.Value.entries)
                {
                    string file = textBox1.Text + @"\Loot\Entries\" + kvp2.Key + ".json";
                    string json = JsonConvert.SerializeObject(kvp2.Value, Formatting.Indented);
                    File.WriteAllText(file, json);
                }
            }
        }

        private void button3_Click(object sender, EventArgs e)
        {

            LoadSettings(textBox1.Text);
        }
    }


    class Category
    {
        public float weight { get; set; }
    }
    class CategoryEntries
    {
        public Dictionary<string, Entry> entries = new Dictionary<string, Entry>();
    }
    class Entry
    {
        public string category { get; set; }
        public float weight { get; set; }
        public string[] styles { get; set; }
        public string[] spawn_with { get; set; }
        public int needs_magazines { get; set; }
        public int needs_ammo { get; set; }
        public int needs_attachments { get; set; }
    }
}
