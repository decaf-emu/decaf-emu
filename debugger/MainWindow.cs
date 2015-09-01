using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Diagnostics;
using System.Drawing;
using System.IO;
using System.Linq;
using System.Net;
using System.Net.Sockets;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;

namespace debugger
{
    public partial class MainWindow : Form
    {
        public class StateObject
        {
            // Client  socket.
            public Socket workSocket = null;
            // Size of inbound packet
            public int recvPacketSize = 0;
            // Total received data
            public int recvPacketCur = 0;
            // Buffer for incoming data
            public byte[] buffer = null;
        }

        public MainWindow()
        {
            InitializeComponent();
        }

        private static void StartListening()
        {
            Debug.WriteLine("StartListening");

            IPEndPoint localEndPoint = new IPEndPoint(IPAddress.Any, 11234);

            Socket listener = new Socket(
                AddressFamily.InterNetwork, 
                SocketType.Stream, 
                ProtocolType.Tcp);

            listener.Bind(localEndPoint);
            listener.Listen(100);

            StartAccept(listener);
        }

        private static void StartAccept(Socket listener)
        {
            listener.BeginAccept(new AsyncCallback(AcceptCallback), listener);
        }

        public static void AcceptCallback(IAsyncResult ar)
        {
            Debug.WriteLine("AcceptCallback");

            // Get the socket that handles the client request.
            Socket listener = (Socket)ar.AsyncState;
            Socket newSocket = listener.EndAccept(ar);

            // Create the state object.
            StateObject state = new StateObject();
            state.workSocket = newSocket;

            StartReadPacket(state);
            StartAccept(listener);
        }

        public static void StartReadPacket(StateObject state)
        {
            Debug.WriteLine("StartReadPacket");

            state.buffer = new byte[6];
            state.recvPacketCur = 0;
            state.recvPacketSize = 0;

            StartReadHeader(state);
        }

        public static void StartReadHeader(StateObject state)
        {
            Debug.WriteLine("StartReadHeader");

            state.workSocket.BeginReceive(
                state.buffer, state.recvPacketCur, 6 - state.recvPacketCur, 0,
                new AsyncCallback(ReadHeaderCallback), state);
        }

        public static void ReadHeaderCallback(IAsyncResult ar)
        {
            Debug.WriteLine("ReadHeaderCallback");

            // Retrieve the state object and the handler socket
            // from the asynchronous state object.
            StateObject state = (StateObject)ar.AsyncState;
            Socket handler = state.workSocket;

            // Read data from the client socket. 
            int bytesRead = handler.EndReceive(ar);

            if (bytesRead > 0)
            {
                state.recvPacketCur += bytesRead;
                if (state.recvPacketCur < 6)
                {
                    // Need more data for header
                    StartReadHeader(state);
                } else
                {
                    // We have all the header data...
                    state.recvPacketSize = BitConverter.ToUInt16(state.buffer, 0);

                    if (state.recvPacketSize > state.recvPacketCur)
                    {
                        byte[] oldBuffer = state.buffer;
                        state.buffer = new byte[state.recvPacketSize];
                        System.Array.Copy(oldBuffer, state.buffer, 6);

                        StartReadPayload(state);
                    }
                    else
                    {
                        HandlePacket(state.buffer);
                        StartReadPacket(state);
                    }
                }
            }
        }

        public static void StartReadPayload(StateObject state)
        {
            Debug.WriteLine("StartReadPayload");

            state.workSocket.BeginReceive(
                state.buffer, state.recvPacketCur,state.recvPacketSize - state.recvPacketCur, 0,
                new AsyncCallback(ReadPayloadCallback), state);
        }

        public static void ReadPayloadCallback(IAsyncResult ar)
        {
            Debug.WriteLine("ReadPayloadCallback");

            // Retrieve the state object and the handler socket
            // from the asynchronous state object.
            StateObject state = (StateObject)ar.AsyncState;
            Socket handler = state.workSocket;

            // Read data from the client socket. 
            int bytesRead = handler.EndReceive(ar);

            if (bytesRead > 0)
            {
                state.recvPacketCur += bytesRead;
                if (state.recvPacketCur < state.recvPacketSize)
                {
                    // Need more data for header
                    StartReadPayload(state);
                }
                else
                {
                    HandlePacket(state.buffer);
                    StartReadPacket(state);
                }
            }
        }

        const ushort PacketCmdPreLaunch = 1;
        const ushort PacketCmdBpHit = 2;
        const ushort PacketCmdPause = 3;
        const ushort PacketCmdResume = 4;
        const ushort PacketCmdAddBreakpoint = 5;
        const ushort PacketCmdRemoveBreakpoint = 6;

        private static string readString(BinaryReader rdr)
        {
            ulong strLen = rdr.ReadUInt64();
            return new string(rdr.ReadChars((int)strLen));
        }

        class DebugModuleInfo
        {
            string name;

            public void read(BinaryReader rdr)
            {
                name = readString(rdr);
            }
        };

        class DebugThreadInfo
        {
            string name;
            int curCoreId;
            uint attribs;
            uint state;

            uint cia;
            uint[] gpr = new uint[32];
            uint crf;

            public void read(BinaryReader rdr)
            {
                name = readString(rdr);
                curCoreId = rdr.ReadInt32();
                attribs = rdr.ReadUInt32();
                state = rdr.ReadUInt32();

                cia = rdr.ReadUInt32();
                for (int i = 0; i < 32; ++i)
                {
                    gpr[i] = rdr.ReadUInt32();
                }
                crf = rdr.ReadUInt32();
            }
        }

        class DebugPauseInfo
        {
            DebugModuleInfo[] modules;
            DebugThreadInfo[] threads;

            public void read(BinaryReader rdr)
            {
                ulong numModules = rdr.ReadUInt64();
                modules = new DebugModuleInfo[numModules];
                for (ulong i = 0; i < numModules; ++i) {
                    modules[i] = new DebugModuleInfo();
                    modules[i].read(rdr);
                }

                ulong numThreads = rdr.ReadUInt64();
                threads = new DebugThreadInfo[numThreads];
                for (ulong i = 0; i < numThreads; ++i) {
                    threads[i] = new DebugThreadInfo();
                    threads[i].read(rdr);
                }
            }
        };

        class DebugPacketPreLaunch
        {
            DebugPauseInfo info;

            public void read(BinaryReader rdr)
            {
                info = new DebugPauseInfo();
                info.read(rdr);
            }
        };

        public static void HandlePacket(byte[] data)
        {
            Debug.WriteLine("HandlePacket");

            BinaryReader rdr = new BinaryReader(new MemoryStream(data));

            ushort size = rdr.ReadUInt16();
            ushort cmd = rdr.ReadUInt16();
            ushort reqId = rdr.ReadUInt16();

            if (cmd == PacketCmdPreLaunch)
            {
                DebugPacketPreLaunch pak = new DebugPacketPreLaunch();
                pak.read(rdr);
            } else
            {
                Debug.WriteLine(data);
            }
        }

        private static void Send(Socket handler, byte[] data)
        {
            // Begin sending the data to the remote device.
            handler.BeginSend(data, 0, data.Length, 0,
                new AsyncCallback(SendCallback), handler);
        }

        private static void SendCallback(IAsyncResult ar)
        {
            try
            {
                // Retrieve the socket from the state object.
                Socket handler = (Socket)ar.AsyncState;

                // Complete sending the data to the remote device.
                int bytesSent = handler.EndSend(ar);
                Console.WriteLine("Sent {0} bytes to client.", bytesSent);
            }
            catch (Exception e)
            {
                Console.WriteLine(e.ToString());
            }
        }

        private void MainWindow_Load(object sender, EventArgs e)
        {
            StartListening();

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
