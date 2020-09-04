using System;

namespace SanityEngine.Editor
{
    /// <summary>
    /// Application class for the Sanity Engine world editor
    /// </summary>
    class Application
    {
        static void Main(string[] _)
        {
            Console.WriteLine("Hello World!");

            Sanity.SanityEngine engine = new();

            engine.Tick(0.5);
        }
    }
}
