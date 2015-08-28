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
    public partial class AssemblyView : Form
    {
        public AssemblyView()
        {
            InitializeComponent();

            scrollBar.Minimum = 0x02000000 / 4;
            scrollBar.Maximum = 0x03000000 / 4;
            scrollBar.Value = 0x02000100 / 4;
        }

        private void pictureBox1_Paint(object sender, PaintEventArgs e)
        {
            Font renderFont = new Font("Lucida Console", 8, FontStyle.Regular);
            Brush lineBrush = new SolidBrush(Color.FromArgb(255, 100, 100, 100));
            Brush addrBrush = new SolidBrush(Color.Gray);
            Brush textBrush = new SolidBrush(Color.Black);
            Brush selBgBrush = new SolidBrush(Color.Black);
            Brush selAddrBrush = new SolidBrush(Color.White);
            Pen linePen = new Pen(Color.LightGray);

            Graphics g = e.Graphics;
            
            g.Clear(Color.White);

            SizeF addrStrSize = g.MeasureString("FFFFFFFF", renderFont);
            SizeF statStrSize = g.MeasureString("∙∙∙", renderFont);
            SizeF dataStrSize = g.MeasureString("FF", renderFont);
            SizeF insStrSize = g.MeasureString("PSQ_ST R4, R4, R4, R4, R4, R4, R4, R4", renderFont);

            float dataStrWidth = (dataStrSize.Width * 4);

            float addrX = 0;
            float div1X = addrX + addrStrSize.Width;
            float statX = div1X + 2;
            float dataX = statX + statStrSize.Width;
            float data0X = dataX;
            float data1X = dataX + (dataStrSize.Width * 1);
            float data2X = dataX + (dataStrSize.Width * 2);
            float data3X = dataX + (dataStrSize.Width * 3);
            float div2X = dataX + 3 + dataStrWidth;
            float insX = div2X + 3;
            float div3X = insX + insStrSize.Width;
            float cmtX = div3X + 2;

            int startAddr = scrollBar.Value * 4;
            int curAddr = startAddr;
            for (int i = 0; i < 100; i++, curAddr += 4)
            {
                string lineVal = String.Format("{0:X8}", curAddr);

                int lineY = 2 + i * 13;

                if (curAddr == 0x02000120)
                {
                    g.FillRectangle(selBgBrush, 0, lineY, addrStrSize.Width, addrStrSize.Height);
                    g.DrawString(lineVal, renderFont, selAddrBrush, addrX, 1 + lineY);
                } else
                {
                    g.DrawString(lineVal, renderFont, addrBrush, addrX, 1 + lineY);
                }

                lineVal = String.Format(" ∙ ");
                g.DrawString(lineVal, renderFont, textBrush, statX, 1 + lineY);

                uint lineData = 0x12345678;
                lineVal = String.Format("{0:X2}", (lineData << 0) & 0xFF);
                g.DrawString(lineVal, renderFont, textBrush, data0X, 1 + lineY);
                lineVal = String.Format("{0:X2}", (lineData << 8) & 0xFF);
                g.DrawString(lineVal, renderFont, textBrush, data1X, 1 + lineY);
                lineVal = String.Format("{0:X2}", (lineData << 16) & 0xFF);
                g.DrawString(lineVal, renderFont, textBrush, data2X, 1 + lineY);
                lineVal = String.Format("{0:X2}", (lineData << 24) & 0xFF);
                g.DrawString(lineVal, renderFont, textBrush, data3X, 1 + lineY);

                string insText = "psq_st f6, r5, 0";
                lineVal = insText.ToUpper();
                g.DrawString(lineVal, renderFont, textBrush, insX, 1 + lineY);
            }

            g.DrawLine(linePen, div1X, 0, div1X, this.Height);
            g.DrawLine(linePen, div2X, 0, div2X, this.Height);
            g.DrawLine(linePen, div3X, 0, div3X, this.Height);
        }

        private void pictureBox1_MouseWheel(object sender, MouseEventArgs e)
        {
            if (e.Delta != 0)
            {
                int newValue = scrollBar.Value - (e.Delta / 120);
                if (newValue < scrollBar.Minimum) {
                    newValue = scrollBar.Minimum;
                }
                if (newValue > scrollBar.Maximum) {
                    newValue = scrollBar.Maximum;
                }
                scrollBar.Value = newValue;
            }
        }

        private void scrollBar_ValueChanged(object sender, EventArgs e)
        {
            this.Invalidate();
        }

        private void AssemblyView_Load(object sender, EventArgs e)
        {

        }
    }
}
