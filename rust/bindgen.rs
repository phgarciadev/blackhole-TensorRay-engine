extern crate bindgen;

/// Este é o script que lê os cabeçalhos C e cospe código Rust.
/// Magia? Não, só um parser glorificado.
fn main() {
    // O único arquivo que importa. Se não tá no wrapper.h, não existe.
    const WRAPPER_HEADER: &str = "wrapper.h";

    println!("// Gerado automaticamente por `rust/bindgen.rs`. NÃO EDITE MANUALMENTE.");
    println!("// Se editar, eu vou saber. E vou te caçar.");
    println!("#![allow(non_upper_case_globals)]");
    println!("#![allow(non_camel_case_types)]");
    println!("#![allow(non_snake_case)]");
    println!("#![allow(dead_code)]");

    let bindings = bindgen::Builder::default()
        .header(WRAPPER_HEADER)
        // Incluindo os caminhos para os headers. Que bagunça...
        .clang_arg("-I../include")
        .clang_arg("-I../engine")
        .clang_arg("-I../gui")
        .clang_arg("-I../math")
        // Seguindo a cartilha do kernel: nada de std.
        .use_core()
        .ctypes_prefix("cty")
        // Para não poluir o namespace global.
        .prepend_enum_name(false)
        // Funções e tipos permitidos. Começou com 'bhs_', a gente deixa entrar.
        .allowlist_function("bhs_.*")
        .allowlist_type("bhs_.*")
        .allowlist_var("BHS_.*")
        .parse_callbacks(Box::new(bindgen::CargoCallbacks::new()))
        .generate()
        .expect("Falha miserável na geração dos bindings. Verifique os headers C.");

    // Joga tudo na saída padrão. Você que se vire pra redirecionar.
    println!("{}", bindings.to_string());
}
