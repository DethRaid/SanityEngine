using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;

using ClangSharp;
using ClangSharp.Interop;

namespace SanityEngine.Codegen.NET
{
    class ClangSharpCodegenProgram
    {
        static int Main(string[] args)
        {
            Console.WriteLine("HELLO HUMAN");

            var returnValue = 0;
            if(args.Length != 2)
            {
                Console.Error.WriteLine("Incorrect number of arguments!");
                Console.Error.WriteLine("Usage:");
                Console.Error.WriteLine("\tSanityEngine.Codegen.NET <C++ Directory> <Output Directory>");
                returnValue = -1;
            }
            else
            {
                GenerateRuntimeClasses(args[0], args[1]);
            }

            Console.WriteLine("REMAIN INDOORS");

            return returnValue;
        }

        private static void GenerateRuntimeClasses(string cppDirectory, string outputDirectory)
        {
            Console.WriteLine("Examining directory '{0}' for header files", cppDirectory);
            var headerFiles = Directory.GetFiles(cppDirectory, "*.hpp", SearchOption.AllDirectories);

            foreach(var headerFile in headerFiles)
            {
                Console.WriteLine("Examining file {0}", headerFile);
                CXTranslationUnit.TryParse(
                    CXIndex.Create(), 
                    headerFile, 
                    default, 
                    default, 
                    CXTranslationUnit_Flags.CXTranslationUnit_SkipFunctionBodies | CXTranslationUnit_Flags.CXTranslationUnit_SingleFileParse,
                    out var translationUnit);

                new TranslationUnitDecl();

                foreach(var cursor in translationUnit.TranslationUnitDecl.CursorChildren)
                {
                    Console.WriteLine("Examining class {0}", classCompilation.GetDisplayName());
                    var numRuntimeClassAttributes = classCompilation.Attributes.Where(attribute => attribute.Name == "sanity::runtime_class").Count();
                    if(numRuntimeClassAttributes == 0)
                    {
                        Console.WriteLine("Class has no attributes :(");
                        continue;
                    }

                    if(numRuntimeClassAttributes > 1)
                    {
                        Console.Error.WriteLine(
                            "Class {0} in file {1} has {2} [sanity::runtime_class] attributes. No more than one is allowed",
                            classCompilation.Name,
                            headerFile,
                            numRuntimeClassAttributes);
                        continue;
                    }

                    // if numRuntimeClassAttributes == 1
                    GenerateRuntimeClass(classCompilation);
                }

            }
        }
    }
}
