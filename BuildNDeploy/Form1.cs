

using Microsoft.Extensions.Configuration;
using Microsoft.VisualBasic.Logging;
using Renci.SshNet;
using Renci.SshNet.Common;
using System.Configuration;
using System.Diagnostics;
using System.Net.Sockets;
using System.Runtime.Intrinsics.X86;
using System.Security.AccessControl;
using static System.Windows.Forms.VisualStyles.VisualStyleElement.StartPanel;


namespace BuildNDeploy {
    public partial class Form1 : Form {

        private readonly IConfiguration config;
        public Form1() {

            InitializeComponent();

            config = new ConfigurationBuilder()
            .AddJsonFile("appsettings.json", optional: false, reloadOnChange: true)
            .Build();
        }

        void log(string info) {
            this.Invoke((Func<string, bool>)DoGuiAccess, info);
        }
        bool DoGuiAccess(string info) {
            logtext.Text = $"{logtext.Text}\r\n{info}";
            logtext.SelectionStart = logtext.Text.Length;
            logtext.SelectionLength = 0;
            logtext.ScrollToCaret();
            return true;

        }
        public void BashC(string cmd) {
            log($"start bashCommand: {cmd}");
            var escapedArgs = cmd.Replace("\"", "\\\"").Replace("\r\n", "\n");
            try {
                var process = new Process {
                    StartInfo = new ProcessStartInfo {
                        FileName = "bash",
                        Arguments = $"-c \"{escapedArgs}\"",
                        RedirectStandardOutput = true,
                        RedirectStandardError = true,
                        UseShellExecute = false,
                        CreateNoWindow = true
                    },
                    EnableRaisingEvents = true
                };
                process.OutputDataReceived += (sender, e) => { log(e.Data + ""); };
                process.ErrorDataReceived += (sender, e) => { log(e.Data + ""); };

                process.Start();
                process.BeginOutputReadLine();
                process.BeginErrorReadLine();

            } catch (Exception e) {
                log($"Command {cmd} failed {e}");
            }

        }

        private void runbuild_Click(object sender, EventArgs e) {
            var src = config["Iamforce2_src_path_wsl"];

            BashC(@$"
                cd {src}
                ./wsl2_mk
            ");
        }

        private void button1_Click(object sender, EventArgs e) {

            
           
            

            SshClient sshclient = new SshClient(config["mpc_ip"], "root");
            sshclient.Connect();
            SshCommand sc = sshclient.CreateCommand("systemctl stop inmusic-mpc; systemctl status inmusic-mpc");
            sc.Execute();
            log(sc.Result.Replace("\n", "\r\n") + "\r\n");

            var bins = Directory.EnumerateFiles(config["Iamforce2_bin_path_windows"], "*.so");

            foreach (var bin in bins) {
                log(bin.ToString());

                var filename = Path.GetFileName(bin);
                var remotepath = $"{config["Iamforce2_remote_dir"]}/{filename}";

                var scp = new ScpClient(config["mpc_ip"], "root");
                scp.Connect();
                scp.Uploading += delegate (object sender, ScpUploadEventArgs e) {
                    log($"uploaded {e.Filename} bytes {e.Uploaded} from {e.Size}");
                };

                var file = new FileInfo(bin);
                scp.Upload(file, remotepath);
            }

            sc = sshclient.CreateCommand("systemctl start inmusic-mpc; systemctl status inmusic-mpc");
            sc.Execute();
            log(sc.Result.Replace("\n", "\r\n") + "\r\n");

        }

        private void Form1_Load(object sender, EventArgs e) {
            log(@"*** CHECK PATHES IN C\Users\.....\AppData\Local\BuildNDeploy\.....\user.config ***" + "\r\n");

            log($"Iamforce2_src_path_wsl = {config["Iamforce2_src_path_wsl"]} \r\n");
            log($"Iamforce2_src_path_windows  = {config["Iamforce2_src_path_windows"]} \r\n");
            log($"Iamforce2_bin_path_windows = {config["Iamforce2_bin_path_windows"]} \r\n");
            log($"Iamforce2_remote_dir = {config["Iamforce2_remote_dir"]} \r\n");

            log("*******");
        }


        Process logprocess;
        public void Bashlog(string cmd) {


            try {
                logprocess = new Process {
                    StartInfo = new ProcessStartInfo {
                        FileName = "ssh ",
                        Arguments = $"root@{config["mpc_ip"]} \"{cmd}\"",
                        RedirectStandardOutput = true,
                        RedirectStandardError = true,
                        UseShellExecute = false,
                        CreateNoWindow = true
                    },
                    EnableRaisingEvents = true
                };
                logprocess.OutputDataReceived += (sender, e) => { log(e.Data + ""); };
                logprocess.ErrorDataReceived += (sender, e) => { log(e.Data + ""); };

                logprocess.Start();
                logprocess.BeginOutputReadLine();
                logprocess.BeginErrorReadLine();

            } catch (Exception e) {
                log($"Command {cmd} failed {e}");
            }
            //return process
        }


        private void checkBox1_CheckedChanged(object sender, EventArgs e) {
            if (checkBox1.Checked) {
                Bashlog("journalctl -u inmusic-mpc -f");
            }
            else {
                logprocess?.Kill();
            }
        }
    }
}