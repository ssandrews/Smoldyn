use cmake::Config;

fn main() {
    // build libsmodlyn_static.a
    let dst = Config::new("../..")
        .define("OPTION_PYTHON", "OFF")
        .define("OPTION_STATIC", "ON")
        .define("OPTION_USE_OPENGL", "OFF")
        .define("OPTION_USE_LIBTIFF", "OFF")
        .build();

    cxx_build::bridge("src/lib.rs")
        .include("../../source")
        .include("include")
        .include(format!("{}/build", dst.display()))
        .compile("simulation");

    println!("cargo:rerun-if-changed=include/smolrs.h");
    // cxx_build's rerun-if-changed replaces cargo's default, so list the C/C++ sources too.
    println!("cargo:rerun-if-changed=../../source");
    println!("cargo:rerun-if-changed=../../CMakeLists.txt");
    println!("cargo:rustc-link-search=native={}/build", dst.display());
    println!("cargo:rustc-link-lib=static=smoldyn_static");
}
