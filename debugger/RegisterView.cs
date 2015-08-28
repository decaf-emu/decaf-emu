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
    public partial class RegisterView : Form
    {
        public RegisterView()
        {
            InitializeComponent();
        }

        private void RegisterView_Load(object sender, EventArgs e)
        {

        }

        private void RegisterView_Paint(object sender, PaintEventArgs e)
        {
            Font renderFont = new Font("Lucida Console", 8, FontStyle.Regular);
            Brush textBrush = new SolidBrush(Color.Black);

            Graphics g = e.Graphics;

            g.Clear(Color.White);

            int lineY = 2;

            string lineStr;

            // GPRs
            for (int i = 0; i < 16; ++i)
            {
                lineStr = String.Format("R{0:d2}  {1:X8}           R{2:d2}  {3:X8}", i, 0xFFFFFFFF, 16+i, 0xFFFFFFFF);
                g.DrawString(lineStr, renderFont, textBrush, 0, lineY);

                lineY += 13;
            }

            lineY += 13;

            lineStr = String.Format("LR   {0:X8}", 0x02000010);
            g.DrawString(lineStr, renderFont, textBrush, 0, lineY);
            lineY += 13;

            lineStr = String.Format("CTR  {0:X8}", 0x02000010);
            g.DrawString(lineStr, renderFont, textBrush, 0, lineY);
            lineY += 13;

            // CRFs
            lineY += 13;

            lineStr = String.Format("      - + Z O                 - + Z O");
            g.DrawString(lineStr, renderFont, textBrush, 0, lineY);

            for (int i = 0; i < 4; ++i)
            {
                lineY += 13;
                lineStr = String.Format("CRF{0}  0 0 0 0           CRF{1}  0 0 0 0", i, 4 + i);
                g.DrawString(lineStr, renderFont, textBrush, 0, lineY);
            }

        }
    }
}
