using CppAst;
using System;
using System.IO;
using System.Linq;

namespace SanityEngine.Codegen.NET
{
    class CppAsNetCodegenProgram
    {
        private static void GenerateRuntimeClasses(string cppDirectory, string outputDirectory)
        {
            Console.WriteLine("Examining directory '{0}' for header files", cppDirectory);
            var headerFiles = Directory.GetFiles(cppDirectory, "*.hpp", SearchOption.AllDirectories);

            foreach(var headerFile in headerFiles)
            {
                Console.WriteLine("Examining file {0}", headerFile);
                var compilation = CppAst.CppParser.ParseFile(headerFile);

                foreach(var classCompilation in compilation.Classes)
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

        private static void GenerateRuntimeClass(CppClass classCompilation)
        {
            Console.WriteLine("Generating a runtime class for C++ class {0}", classCompilation.GetDisplayName());
            var runtimeClassString = string.Format("runtimeclass {0}\n{", classCompilation.Name);
            foreach(var function in classCompilation.Functions)
            {
                if(function.Visibility == CppVisibility.Public)
                {
                    var returnTypeName = ToMidlType(function.ReturnType);
                }
            }
        }

        private static string ToMidlType(CppType cppType)
        {
            switch(cppType.TypeKind)
            {
                case CppTypeKind.Primitive:
                    return ToMidlPrimitiveType(cppType.GetDisplayName());
                    
                case CppTypeKind.Pointer:
                    break;
                case CppTypeKind.Reference:
                    break;
                case CppTypeKind.Array:
                    break;
                case CppTypeKind.Qualified:
                    break;
                case CppTypeKind.Function:
                    break;
                case CppTypeKind.Typedef:
                    break;
                case CppTypeKind.StructOrClass:
                    break;
                case CppTypeKind.Enum:
                    break;
                case CppTypeKind.TemplateParameterType:
                    break;
                case CppTypeKind.Unexposed:
                    break;
            }

            throw new ArgumentException(string.Format("{0} is not a MIDL-compatible type", cppType));
        }

        private static string ToMidlPrimitiveType(string displatName)
        {
            return "";
        }
    }
}
