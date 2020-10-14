// use std::process::{Command};
use std::path::PathBuf;

fn main() {
    compile_cpp_engine();

    generate_rust_bindings();

    compile_rust_editor();
}

fn compile_cpp_engine() {
    let working_directory_buffer = std::env::current_dir().expect("bruh wtf can't read your own cwd");
    let working_directory = working_directory_buffer.to_str().expect("cwd must be string");

    println!("Working directory: {}", working_directory);

    // TODO: Figure out how to make msbuild work
    // Command::new(r#"E:\Program Files (x86)\Microsoft Visual Studio\2019\Preview\MSBuild\Current\Bin\msbuild.exe"#)
    //     .current_dir(working_directory)
    //     .arg(r#"E:\Documents\SanityEngine\SanityEngine\SanityEngine.vcxproj"#)
    //     .arg(r#"/t:Build"#)
    //     .arg(r#"/p:VcpkgEnableManifest=true"#)
    //     .arg(r#"/p:Configuration="Debug""#)
    //     .arg(r#"/p:Platform="x64""#)
    //     .arg(r#"/maxCpuCount:20"#)
    //     .spawn()
    //     .expect("Could not compile C++ code");

    let dlls_to_copy = vec![
        "assimp-vc142-mtd.dll",
        "glfw3.dll",
        "zlibd1.dll",
        "Sanity.Engine.dll",
    ];

    for dll in dlls_to_copy {
        let source_path = format!(r#"E:\Documents\SanityEngine\x64\Debug\{dllname}"#, dllname=dll);
        let destination_path = format!(r#"E:\Documents\SanityEngine\sanity_editor\target\debug\{dllname}"#, dllname=dll);
        if std::fs::copy(source_path, destination_path).is_err() {
            println!("Could not copy DLL {dllname} to destination", dllname=dll);
        }
    }

    generate_rust_bindings();
}

fn generate_rust_bindings() {
    let bindings = bindgen::Builder::default()

        // Language arguments
        .clang_arg(r#"-std=c++20"#)

        // Environment setup, e.g. include paths and preprocessor tokens
        .clang_arg(r#"-IE:\Documents\SanityEngine\SanityEngine\extern\rex\include"#)
        .clang_arg(r#"-IE:\Documents\SanityEngine\vcpkg_installed\x86-windows\include"#)
        .clang_arg("-DRX_API=RX_EXPORT")
        .clang_arg("-DSANITY_EXPORT_API")
        .clang_arg("-DWIN32")
        .clang_arg("-D_WINDOWS")
        .clang_arg("-DTRACY_ENABLE")
        .clang_arg("-DRX_DEBUG")
        .clang_arg("-DGLM_ENABLE_EXPERIMENTAL")
        .clang_arg("-D_CRT_SECURE_NO_WARNINGS")
        .clang_arg("-DGLM_FORCE_LEFT_HANDED")
        .clang_arg("-DNOMINMAX")
        .clang_arg("-DWIN32_LEAN_AND_MEAN")
        .clang_arg("-DGLFW_DLL")

        // Cargo setup
        .parse_callbacks(Box::new(bindgen::CargoCallbacks))

        // Sanity Engine header files that need bindings
        .header("../SanityEngine/src/sanity_engine.hpp")
        .whitelist_type("SanityEngine")

        .generate()
        .expect("Failed to generate bindings");

    let out_path = PathBuf::from("sanity_editor/generated_files/sanity");
    bindings.write_to_file(out_path.join("engine.rs"))
        .expect("Couldn't write bindings to disk");
}

fn compile_rust_editor() {
    unimplemented!()
}
