using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;

namespace debugger
{
    public partial class MainWindow : Form
    {
        public MainWindow()
        {
            InitializeComponent();
        }

        private void MainWindow_Load(object sender, EventArgs e)
        {
            AssemblyView asmView = new AssemblyView();
            asmView.MdiParent = this;
            asmView.Dock = DockStyle.Left;
            asmView.Show();

            RegisterView regView = new RegisterView();
            regView.MdiParent = this;
            regView.Dock = DockStyle.Right;
            regView.Show();
        }
    }
}
