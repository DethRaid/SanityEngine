use std::process::{Command};
use std::io::Error;

fn main() {
    compile_cpp_engine();

    generate_rust_bindings();

    compile_rust_editor();
}

fn compile_cpp_engine() {
    let working_directory_buffer = std::env::current_dir().expect("bruh wtf can't read your own cwd");
    let working_directory = working_directory_buffer.to_str().expect("cwd must be strinyboi");

    println!("Working directory: {}", working_directory);

    let cpp_compile_output = Command::new(r#"E:\Program Files (x86)\Microsoft Visual Studio\2019\Preview\MSBuild\Current\Bin\msbuild.exe"#)
        .current_dir(working_directory)
        .arg(r#"E:\Documents\SanityEngine\SanityEngine\SanityEngine.vcxproj"#)
        .arg(r#"/t:Build"#)
        .arg(r#"/p:VcpkgEnableManifest=true"#)
        .output()
        .expect("Could not compile C++ code");

    let stdout_message = std::str::from_utf8(cpp_compile_output.stdout.as_slice()).unwrap();
    println!("{}", stdout_message);

    let dlls_to_copy = vec![
        "assimp-vc142-mtd.dll",
        "Sanity.Engine.dll",
        "glfw3.dll"
    ];

    for dll in dlls_to_copy {
        let source_path = format!(r#"E:\Documents\SanityEngine\x64\Debug\{dllname}"#, dllname=dll);
        let destination_path = format!(r#"E:\Documents\SanityEngine\sanity_editor\target\debug\{dllname}"#, dllname=dll);
        if std::fs::copy(source_path, destination_path).is_err() {
            println!("Could not copy DLL {dllname} to destination", dllname=dll);
        }
    }
}

fn generate_rust_bindings() {
    unimplemented!()
}

fn compile_rust_editor() {
    unimplemented!()
}
