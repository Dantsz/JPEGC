use serde::{Deserialize, Serialize};
use std::fs;
use std::fs::metadata;
use std::path::Path;
use std::process::Command;
#[derive(Serialize, Deserialize)]

struct CompressionData {
    pre_compression_size: usize,
    post_compression_size: usize,
    compression_ratio: f64,
}
fn get_file_size(path: &Path) -> Option<usize> {
    let file_metadata = metadata(path).ok()?;
    let file_size = file_metadata.len() as usize;
    Some(file_size)
}
fn calculate_properties(uncompressed_path: &Path, compressed_path: &Path) -> CompressionData {
    let original = get_file_size(uncompressed_path).unwrap();
    let compressed = get_file_size(compressed_path).unwrap();
    CompressionData {
        pre_compression_size: original,
        post_compression_size: compressed,
        compression_ratio: (original as f64) / (compressed as f64),
    }
}

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
                let data_dst = images_dir.join(entry.file_name()).join(format!(
                    "{}_stats.json",
                    entry.file_name().to_str().unwrap()
                ));
                let jpegc_cmd = Command::new(jpecg)
                    .args([&image_src, &image_dst])
                    .output()
                    .expect("");

                println!("Converted: {}", entry.file_name().to_string_lossy());
                fs::write(jpegc_output, String::from_utf8(jpegc_cmd.stdout).expect(""))
                    .expect("Failed to write image");
                let compression_data =
                    calculate_properties(&image_src.as_path(), &image_dst.as_path());
                fs::write(
                    data_dst,
                    serde_json::to_string(&compression_data)
                        .expect("Failed to serialize compression data"),
                )
                .expect("Failed to write stats");
            }
        }
    }
}
