use std::process::{Command};

fn main() {
    compile_cpp_engine();

    generate_rust_bindings();

    compile_rust_editor();
}

fn compile_cpp_engine() {
    let working_directory_buffer = std::env::current_dir().expect("bro wtf can't read your own cwd");
    let working_directory = working_directory_buffer.to_str().expect("cwd must be strinyboi");

    println!("Working directory: {}", working_directory);

    let cpp_compile_output = Command::new(r#"E:\Program Files (x86)\Microsoft Visual Studio\2019\Preview\MSBuild\Current\Bin\msbuild.exe"#)
        .current_dir(working_directory)
        .arg("-target:Sanity.Engine")
        .arg("SanityEngine/SanityEngine.vcxproj")
        .output()
        .expect("Could not compile C++ code");

    let stdout_message = std::str::from_utf8(cpp_compile_output.stdout.as_slice());
    println!("{:#?}", stdout_message);
}

fn generate_rust_bindings() {
    unimplemented!()
}

fn compile_rust_editor() {
    unimplemented!()
}
