namespace BuildNDeploy {
    partial class Form1 {
        /// <summary>
        ///  Required designer variable.
        /// </summary>
        private System.ComponentModel.IContainer components = null;

        /// <summary>
        ///  Clean up any resources being used.
        /// </summary>
        /// <param name="disposing">true if managed resources should be disposed; otherwise, false.</param>
        protected override void Dispose(bool disposing) {
            if (disposing && (components != null)) {
                components.Dispose();
            }
            base.Dispose(disposing);
        }

        #region Windows Form Designer generated code

        /// <summary>
        ///  Required method for Designer support - do not modify
        ///  the contents of this method with the code editor.
        /// </summary>
        private void InitializeComponent() {
            runbuild = new Button();
            button1 = new Button();
            logtext = new TextBox();
            checkBox1 = new CheckBox();
            SuspendLayout();
            // 
            // runbuild
            // 
            runbuild.Anchor = AnchorStyles.Bottom | AnchorStyles.Right;
            runbuild.Location = new Point(1172, 614);
            runbuild.Name = "runbuild";
            runbuild.Size = new Size(217, 32);
            runbuild.TabIndex = 0;
            runbuild.Text = "run build on wsl2";
            runbuild.UseVisualStyleBackColor = true;
            runbuild.Click += runbuild_Click;
            // 
            // button1
            // 
            button1.Anchor = AnchorStyles.Bottom | AnchorStyles.Right;
            button1.Location = new Point(929, 614);
            button1.Name = "button1";
            button1.Size = new Size(237, 32);
            button1.TabIndex = 1;
            button1.Text = "deploy to mpc";
            button1.UseVisualStyleBackColor = true;
            button1.Click += button1_Click;
            // 
            // logtext
            // 
            logtext.Anchor = AnchorStyles.Top | AnchorStyles.Bottom | AnchorStyles.Left | AnchorStyles.Right;
            logtext.BackColor = SystemColors.InfoText;
            logtext.Font = new Font("Lucida Console", 11.25F, FontStyle.Regular, GraphicsUnit.Point);
            logtext.ForeColor = SystemColors.Window;
            logtext.Location = new Point(12, 12);
            logtext.Multiline = true;
            logtext.Name = "logtext";
            logtext.ScrollBars = ScrollBars.Vertical;
            logtext.Size = new Size(1377, 584);
            logtext.TabIndex = 2;
            logtext.WordWrap = false;
            // 
            // checkBox1
            // 
            checkBox1.Anchor = AnchorStyles.Bottom | AnchorStyles.Left;
            checkBox1.AutoSize = true;
            checkBox1.Location = new Point(12, 614);
            checkBox1.Name = "checkBox1";
            checkBox1.Size = new Size(270, 23);
            checkBox1.TabIndex = 3;
            checkBox1.Text = "Show Log (journalctl -u inmusic-mpc -f)";
            checkBox1.UseVisualStyleBackColor = true;
            checkBox1.CheckedChanged += checkBox1_CheckedChanged;
            // 
            // Form1
            // 
            AutoScaleDimensions = new SizeF(8F, 19F);
            AutoScaleMode = AutoScaleMode.Font;
            ClientSize = new Size(1401, 650);
            Controls.Add(checkBox1);
            Controls.Add(logtext);
            Controls.Add(button1);
            Controls.Add(runbuild);
            Name = "Form1";
            Text = "Form1";
            Load += Form1_Load;
            ResumeLayout(false);
            PerformLayout();
        }

        #endregion

        private Button runbuild;
        private Button button1;
        private TextBox logtext;
        private CheckBox checkBox1;
    }
}