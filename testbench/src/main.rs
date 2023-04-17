use serde::{Deserialize, Serialize};
use std::fs::metadata;
use std::fs::{self, DirEntry};
use std::path::{Path, PathBuf};
use std::process::Command;
use std::time::Instant;
#[derive(Serialize, Deserialize)]

struct CompressionData {
    pre_compression_size: usize,
    post_compression_size: usize,
    compression_ratio: f64,
    compression_elapsed_time: usize,
    decompression_elapsed_time: usize,
}
fn get_file_size(path: &Path) -> Option<usize> {
    let file_metadata = metadata(path).ok()?;
    let file_size = file_metadata.len() as usize;
    Some(file_size)
}
fn calculate_properties(
    uncompressed_path: &Path,
    compressed_path: &Path,
    compression_elapsed_time: usize,
    decompression_elapsed_time: usize,
) -> CompressionData {
    let original = get_file_size(uncompressed_path).unwrap();
    let compressed = get_file_size(compressed_path).unwrap();
    CompressionData {
        pre_compression_size: original,
        post_compression_size: compressed,
        compression_ratio: (original as f64) / (compressed as f64),
        compression_elapsed_time,
        decompression_elapsed_time,
    }
}
fn process_entry(entry: DirEntry, images_dir: &PathBuf, jpecg: &str) {
    let image_src = images_dir
        .join(entry.file_name())
        .join(format!("{}.bmp", entry.file_name().to_str().unwrap()));
    let image_dst = images_dir
        .join(entry.file_name())
        .join(format!("{}.bjpg", entry.file_name().to_str().unwrap()));
    let image_decompressed_dst = images_dir.join(entry.file_name()).join(format!(
        "{}_decompressed.bmp",
        entry.file_name().to_str().unwrap()
    ));
    let compressed_jpegc_output = images_dir.join(entry.file_name()).join(format!(
        "{}_compressing.log",
        entry.file_name().to_str().unwrap()
    ));
    let decompressed_jpegc_output = images_dir.join(entry.file_name()).join(format!(
        "{}_decompressing.log",
        entry.file_name().to_str().unwrap()
    ));
    let data_dst = images_dir.join(entry.file_name()).join(format!(
        "{}_stats.json",
        entry.file_name().to_str().unwrap()
    ));
    let comp_start = Instant::now();
    let jpegc_cmd = Command::new(jpecg)
        .args([&image_src, &image_dst])
        .output()
        .expect("");
    let comp_elapsed = comp_start.elapsed();
    let comp_elapsed_ms =
        comp_elapsed.as_secs() as usize * 1000 + comp_elapsed.subsec_millis() as usize;

    let decomp_start = Instant::now();
    let jpegc_decomp_cmd = Command::new(jpecg)
        .args([
            "--decompress",
            &image_dst.as_os_str().to_str().unwrap(),
            &image_decompressed_dst.as_os_str().to_str().unwrap(),
        ])
        .output()
        .expect("");
    let decomp_elapsed = decomp_start.elapsed();
    let decomp_elapsed_ms =
        decomp_elapsed.as_secs() as usize * 1000 + decomp_elapsed.subsec_millis() as usize;
    println!("Converted: {}", entry.file_name().to_string_lossy());
    fs::write(
        compressed_jpegc_output,
        String::from_utf8(jpegc_cmd.stdout).expect(""),
    )
    .expect("Failed to write image");
    fs::write(
        decompressed_jpegc_output,
        String::from_utf8(jpegc_decomp_cmd.stdout).expect(""),
    )
    .expect("Failed to write image");
    let compression_data = calculate_properties(
        &image_src.as_path(),
        &image_dst.as_path(),
        comp_elapsed_ms,
        decomp_elapsed_ms,
    );
    fs::write(
        data_dst,
        serde_json::to_string(&compression_data).expect("Failed to serialize compression data"),
    )
    .expect("Failed to write stats");
}
fn main() {
    let images_dir = Path::new(env!("OUT_DIR")).join("Release").join("images");
    let jpecg_executable_path = Path::new(env!("OUT_DIR")).join("Release").join("JPEGC.exe");
    let jpecg = jpecg_executable_path.as_os_str().to_str().unwrap();
    println!("Images : {:?}\nExecutable:{}\n", images_dir, jpecg);
    let pool = rayon::ThreadPoolBuilder::new().build().unwrap();
    println!("Running on {} threads", pool.current_num_threads());
    if let Ok(entries) = fs::read_dir(images_dir.clone()) {
        for entry in entries {
            if let Ok(entry) = entry {
                pool.install(|| process_entry(entry, &images_dir, jpecg));
            }
        }
    }
}
