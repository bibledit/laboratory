using System;
using System.Windows.Forms;
using System.Timers;
using System.Threading.Tasks;
using System.Net;
using System.IO;
using System.Net.Sockets;
using System.Diagnostics;
using System.Text.RegularExpressions;

namespace Bibledit
{

    public partial class Form1 : Form
    {

        private System.Timers.Timer aTimer;

        public Form1()
        {
            InitializeComponent();

            // Create a timer with a one-second interval.
            aTimer = new System.Timers.Timer(1000);
            // Hook up the Elapsed event for the timer. 
            aTimer.Elapsed += OnTimedEvent;
            aTimer.AutoReset = false;
            aTimer.Start();

            Console.WriteLine("Hello World.");

            foreach (var oldProcess in Process.GetProcessesByName("server"))
            {
                oldProcess.Kill();
            }

            Process process = new Process();
            process.StartInfo.WorkingDirectory = @"C:\bibledit-windows";
            process.StartInfo.FileName = @"C:\bibledit-windows\server.exe";
            process.StartInfo.UseShellExecute = false;
            process.StartInfo.RedirectStandardOutput = true;

            process.EnableRaisingEvents = true;

            process.OutputDataReceived += new DataReceivedEventHandler((sender, e) =>
            {
                //Console.WriteLine(e.Data);
                String number = Regex.Match(e.Data, @"\d+$").Value;
                if (!String.IsNullOrWhiteSpace(number))
                {
                    Console.WriteLine(number);
                }
            });
            process.Start();
            // Asynchronously read the standard output of the spawned process.
            // This raises OutputDataReceived events for each line of output.
            process.BeginOutputReadLine();
            //process.WaitForExit();
            //process.Close();


        }

        ~Form1()
        {
            aTimer.Stop();
            aTimer.Dispose();
        }

        private void OnTimedEvent(Object source, ElapsedEventArgs e)
        {
            try
            {
                // Connect to the local Bibledit client server.
                TcpClient socket = new TcpClient();
                socket.Connect("localhost", 9876);
                // Fetch the link that indicates to open an external website.
                NetworkStream ns = socket.GetStream();
                StreamWriter sw = new StreamWriter(ns);
                sw.WriteLine("GET /assets/external HTTP/1.1");
                sw.WriteLine("");
                sw.Flush();
                // Read the response from the local Bibledit client server.
                String response;
                StreamReader sr = new StreamReader(ns);
                do
                {
                    response = sr.ReadLine();
                    // Check for a URL to open.
                    if ((response != null) && (response.Length > 4) && (response.Substring(0, 4) == "http"))
                    {
                        // Open the URL in default web browser.
                        System.Diagnostics.Process.Start(response);
                    }
                }
                while (response != null);
                // Close connection.
                socket.Close();
            }
            catch (Exception ex)
            {
                Console.WriteLine(ex.Message);
            }
            // Restart the timeout.
            aTimer.Stop();
            aTimer.Start();
        }

    }
}
