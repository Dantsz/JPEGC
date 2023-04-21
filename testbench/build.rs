use fs_extra::{copy_items, dir};

use std::{
    env, fs, io,
    path::{Path, PathBuf},
    process::Command,
};
fn compile_JPECG(out_dir: &str) -> Result<String, io::Error> {
    let configure_message = Command::new("cmake")
        .args(&["--preset=defaultvcpkg", "."])
        .current_dir("../")
        .output()
        .expect("Failed to configure CMAKE")
        .stdout;
    let compile_message = Command::new("cmake")
        .args(&["--build", "--preset=defaultvcpkg", "--config Release"])
        .current_dir("../")
        .output()
        .expect("Failed to compile JPECG")
        .stdout;
    std::fs::write(
        Path::new(out_dir).join("jpeg_compilation.log"),
        format!(
            "Configuring : {:?}\nCompilation: {:?}",
            String::from_utf8(configure_message),
            String::from_utf8(compile_message)
        ),
    )
    .expect("");
    Ok(Path::new(&env::current_dir()?.parent().unwrap())
        .join(r#"builds\defaultvcpkg\JPEGC\Release"#)
        .as_os_str()
        .to_str()
        .unwrap()
        .to_owned())
}
fn setup_test_images(images_dir: &str, dst_path: &Path) {
    fs::create_dir_all(dst_path).expect("");
    if let Ok(entries) = fs::read_dir(images_dir) {
        for entry in entries {
            if let Ok(entry) = entry {
                if Path::new(entry.file_name().to_str().unwrap())
                    .extension()
                    .unwrap()
                    == "bmp"
                {
                    let image_dir_path_buf = dst_path.join(
                        Path::new(entry.file_name().to_str().unwrap())
                            .file_stem()
                            .unwrap(),
                    );
                    let image_dir_path = image_dir_path_buf.as_os_str();
                    fs::create_dir_all(image_dir_path).expect("");
                    fs::copy(entry.path(), image_dir_path_buf.join(entry.file_name())).expect(
                        &format!(
                            "Failed to copy image: {} as {}",
                            entry.path().to_str().unwrap(),
                            image_dir_path_buf.join(entry.file_name()).to_str().unwrap()
                        ),
                    );
                }
            }
        }
    }
}
fn main() {
    let out_dir_os = env::var_os("OUT_DIR").unwrap();
    let out_dir = out_dir_os.to_str().unwrap();
    let manifest_dir = std::env::var("CARGO_MANIFEST_DIR")
        .map(PathBuf::from)
        .unwrap();

    let compiled_dir = compile_JPECG(out_dir).expect("");

    let options = dir::CopyOptions::new().copy_inside(true).overwrite(true);

    copy_items(&vec![compiled_dir], out_dir, &options).expect("");
    setup_test_images(
        manifest_dir
            .parent()
            .unwrap()
            .join("images")
            .to_str()
            .unwrap(),
        Path::new(out_dir).join("Release").join("report").as_path(),
    );
}
