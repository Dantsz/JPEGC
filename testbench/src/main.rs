use std::fs;
use std::path::Path;
use std::process::Command;
fn main() {
    let images_dir = Path::new(env!("OUT_DIR")).join("Release").join("images");
    let jpecg_executable_path = Path::new(env!("OUT_DIR")).join("Release").join("JPEGC.exe");
    let jpecg = jpecg_executable_path.as_os_str().to_str().unwrap();
    println!("Images : {:?}\nExecutable:{}\n", images_dir, jpecg);
    if let Ok(entries) = fs::read_dir(images_dir.clone()) {
        for entry in entries {
            if let Ok(entry) = entry {
                let image_src = images_dir
                    .join(entry.file_name())
                    .join(format!("{}.bmp", entry.file_name().to_str().unwrap()));
                let image_dst = images_dir
                    .join(entry.file_name())
                    .join(format!("{}.bjpg", entry.file_name().to_str().unwrap()));
                let jpegc_output = images_dir
                    .join(entry.file_name())
                    .join(format!("{}.log", entry.file_name().to_str().unwrap()));
                let jpegc_cmd = Command::new(jpecg)
                    .args([image_src, image_dst])
                    .output()
                    .expect("");
                println!("Converted: {}", entry.file_name().to_string_lossy());
                fs::write(jpegc_output, String::from_utf8(jpegc_cmd.stdout).expect(""));
            }
        }
    }
}
