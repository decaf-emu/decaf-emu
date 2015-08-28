namespace debugger
{
    partial class AssemblyView
    {
        /// <summary>
        /// Required designer variable.
        /// </summary>
        private System.ComponentModel.IContainer components = null;

        /// <summary>
        /// Clean up any resources being used.
        /// </summary>
        /// <param name="disposing">true if managed resources should be disposed; otherwise, false.</param>
        protected override void Dispose(bool disposing)
        {
            if (disposing && (components != null))
            {
                components.Dispose();
            }
            base.Dispose(disposing);
        }

        #region Windows Form Designer generated code

        /// <summary>
        /// Required method for Designer support - do not modify
        /// the contents of this method with the code editor.
        /// </summary>
        private void InitializeComponent()
        {
            this.scrollBar = new System.Windows.Forms.VScrollBar();
            this.SuspendLayout();
            // 
            // scrollBar
            // 
            this.scrollBar.Dock = System.Windows.Forms.DockStyle.Right;
            this.scrollBar.Location = new System.Drawing.Point(591, 0);
            this.scrollBar.Name = "scrollBar";
            this.scrollBar.Size = new System.Drawing.Size(17, 409);
            this.scrollBar.TabIndex = 0;
            this.scrollBar.ValueChanged += new System.EventHandler(this.scrollBar_ValueChanged);
            // 
            // AssemblyView
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(608, 409);
            this.ControlBox = false;
            this.Controls.Add(this.scrollBar);
            this.DoubleBuffered = true;
            this.FormBorderStyle = System.Windows.Forms.FormBorderStyle.SizableToolWindow;
            this.Name = "AssemblyView";
            this.Text = "Assembly View";
            this.Load += new System.EventHandler(this.AssemblyView_Load);
            this.Paint += new System.Windows.Forms.PaintEventHandler(this.pictureBox1_Paint);
            this.MouseWheel += new System.Windows.Forms.MouseEventHandler(this.pictureBox1_MouseWheel);
            this.ResumeLayout(false);

        }

        #endregion
        private System.Windows.Forms.VScrollBar scrollBar;
    }
}

