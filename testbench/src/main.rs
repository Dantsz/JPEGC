use plotters::prelude::*;
use serde::{Deserialize, Serialize};
use std::fs::metadata;
use std::fs::{self, DirEntry};
use std::path::{Path, PathBuf};
use std::process::Command;
use std::sync::{Arc, Mutex};
use std::time::Instant;
#[derive(Serialize, Deserialize, Debug)]

struct CompressionData {
    image_name: String,
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
    image_name: String,
    uncompressed_path: &Path,
    compressed_path: &Path,
    compression_elapsed_time: usize,
    decompression_elapsed_time: usize,
) -> CompressionData {
    let original = get_file_size(uncompressed_path).unwrap();
    let compressed = get_file_size(compressed_path).unwrap();
    CompressionData {
        image_name,
        pre_compression_size: original,
        post_compression_size: compressed,
        compression_ratio: (original as f64) / (compressed as f64),
        compression_elapsed_time,
        decompression_elapsed_time,
    }
}
fn process_entry(entry: DirEntry, images_dir: &PathBuf, jpecg: &str) -> CompressionData {
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
        entry.file_name().to_str().unwrap().to_owned(),
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
    compression_data
}
fn main() {
    let images_dir = Path::new(env!("OUT_DIR")).join("Release").join("report");
    let jpecg_executable_path = Path::new(env!("OUT_DIR")).join("Release").join("JPEGC.exe");
    let jpecg = jpecg_executable_path.as_os_str().to_str().unwrap();
    println!("Images : {:?}\nExecutable:{}\n", images_dir, jpecg);
    let pool = rayon::ThreadPoolBuilder::new().build().unwrap();
    println!("Running on {} threads", pool.current_num_threads());
    let all_report = Arc::try_unwrap(pool.scope(|s| {
        let compression_entries = Arc::new(Mutex::new(Vec::<CompressionData>::new()));
        if let Ok(entries) = fs::read_dir(images_dir.clone()) {
            for entry in entries {
                let report_entries = compression_entries.clone();
                let imgs_dir = images_dir.clone();
                if let Ok(entry) = entry {
                    if Path::new(&entry.path()).is_dir() {
                        s.spawn(move |_| {
                            let entry_data = process_entry(entry, &imgs_dir, jpecg);
                            (*report_entries).lock().unwrap().push(entry_data);
                        });
                    }
                }
            }
        }
        compression_entries
    }))
    .expect("Failed to compile final report.")
    .into_inner()
    .expect("");

    fs::write(
        images_dir.join("report.json"),
        serde_json::to_string_pretty(&all_report).expect(""),
    )
    .expect("Failed to write final report");

    let graph_path = images_dir.join("0.1.png");
    let root_drawing_area = BitMapBackend::new(&graph_path, (1024, 768)).into_drawing_area();

    root_drawing_area.fill(&WHITE).unwrap();

    let mut chart = ChartBuilder::on(&root_drawing_area)
        .build_cartesian_2d(-3.14..3.14, -1.2..1.2)
        .unwrap();

    chart
        .draw_series(LineSeries::new(
            (-314..314).map(|x| x as f64 / 100.0).map(|x| (x, x.sin())),
            &RED,
        ))
        .unwrap();
}
