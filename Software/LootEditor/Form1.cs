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


                                flp.SetFlowBreak(reset_btn, true); //new line

                                foreach (KeyValuePair<string, Entry> kvp in entry.Value.entries)
                                {
                                    Entry loot = kvp.Value;

                                    Label item_name = new Label();
                                    item_name.Width = 80;
                                    item_name.Text = kvp.Key;
                                    InvokeUI(() =>
                                    {
                                        flp.Controls.Add(item_name);
                                    });

                                    Label weight = new Label();
                                    weight.Width = 80;
                                    weight.Text = "Weight";
                                    InvokeUI(() =>
                                    {
                                        flp.Controls.Add(weight);
                                    });

                                    TextBox winput = new TextBox();
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
                                    };
                                    InvokeUI(() =>
                                    {
                                        flp.Controls.Add(winput);
                                    });

                                    CheckBox needs_mags = new CheckBox();
                                    needs_mags.Width = 150;
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
                                    needs_ammo.Width = 150;
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
                                    styles_label.Width = 80;
                                    styles_label.TextAlign = ContentAlignment.MiddleRight;
                                    styles_label.Text = "Styles";
                                    InvokeUI(() =>
                                    {
                                        flp.Controls.Add(styles_label);
                                    });

                                    ListBox styles = new ListBox();
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
                                    spawnwith_label.Width = 80;
                                    spawnwith_label.Text = "Spawn With";
                                    InvokeUI(() =>
                                    {
                                        flp.Controls.Add(spawnwith_label);
                                    });

                                    ListBox spawnwith = new ListBox();
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

                                    flp.SetFlowBreak(spawnwith, true);
                                }


                            });

                        }
                    };
                    
                }
            });
            
            
        }



        private void button2_Click(object sender, EventArgs e)
        {
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
