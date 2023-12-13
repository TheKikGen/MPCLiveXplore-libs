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
            button2 = new Button();
            button3 = new Button();
            SuspendLayout();
            // 
            // runbuild
            // 
            runbuild.Anchor = AnchorStyles.Bottom | AnchorStyles.Right;
            runbuild.Location = new Point(1026, 485);
            runbuild.Margin = new Padding(3, 2, 3, 2);
            runbuild.Name = "runbuild";
            runbuild.Size = new Size(190, 25);
            runbuild.TabIndex = 0;
            runbuild.Text = "run build on wsl2";
            runbuild.UseVisualStyleBackColor = true;
            runbuild.Click += runbuild_Click;
            // 
            // button1
            // 
            button1.Anchor = AnchorStyles.Bottom | AnchorStyles.Right;
            button1.Location = new Point(617, 485);
            button1.Margin = new Padding(3, 2, 3, 2);
            button1.Name = "button1";
            button1.Size = new Size(207, 25);
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
            logtext.Location = new Point(10, 9);
            logtext.Margin = new Padding(3, 2, 3, 2);
            logtext.Multiline = true;
            logtext.Name = "logtext";
            logtext.ScrollBars = ScrollBars.Vertical;
            logtext.Size = new Size(1205, 462);
            logtext.TabIndex = 2;
            logtext.WordWrap = false;
            // 
            // checkBox1
            // 
            checkBox1.Anchor = AnchorStyles.Bottom | AnchorStyles.Left;
            checkBox1.AutoSize = true;
            checkBox1.Location = new Point(10, 484);
            checkBox1.Margin = new Padding(3, 2, 3, 2);
            checkBox1.Name = "checkBox1";
            checkBox1.Size = new Size(240, 19);
            checkBox1.TabIndex = 3;
            checkBox1.Text = "Show Log (journalctl -u inmusic-mpc -f)";
            checkBox1.UseVisualStyleBackColor = true;
            checkBox1.CheckedChanged += checkBox1_CheckedChanged;
            // 
            // button2
            // 
            button2.Anchor = AnchorStyles.Bottom | AnchorStyles.Right;
            button2.Location = new Point(404, 484);
            button2.Margin = new Padding(3, 2, 3, 2);
            button2.Name = "button2";
            button2.Size = new Size(207, 25);
            button2.TabIndex = 4;
            button2.Text = "deploy 1 to mpc";
            button2.UseVisualStyleBackColor = true;
            button2.Click += button2_Click;
            // 
            // button3
            // 
            button3.Anchor = AnchorStyles.Bottom | AnchorStyles.Right;
            button3.Location = new Point(830, 484);
            button3.Margin = new Padding(3, 2, 3, 2);
            button3.Name = "button3";
            button3.Size = new Size(190, 25);
            button3.TabIndex = 5;
            button3.Text = "run 1 build on wsl2";
            button3.UseVisualStyleBackColor = true;
            button3.Click += button3_Click;
            // 
            // Form1
            // 
            AutoScaleDimensions = new SizeF(7F, 15F);
            AutoScaleMode = AutoScaleMode.Font;
            ClientSize = new Size(1226, 513);
            Controls.Add(button3);
            Controls.Add(button2);
            Controls.Add(checkBox1);
            Controls.Add(logtext);
            Controls.Add(button1);
            Controls.Add(runbuild);
            Margin = new Padding(3, 2, 3, 2);
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
        private Button button2;
        private Button button3;
    }
}