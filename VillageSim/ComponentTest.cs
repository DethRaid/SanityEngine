using System;
using System.Runtime.InteropServices;
using SanityEngine;

namespace VillageSim
{
    // ReSharper disable once UnusedMember.Global
    [ComVisible(true)]
    [ClassInterface(ClassInterfaceType.None)]
    public class ComponentTest : GameplayComponent
    {
        public override void Tick(float deltaTime)
        {
            Console.WriteLine("C# ticking!");
        }
    }
}
