// src-tauri/src/lib.rs (PR TEMPLATE)
#![cfg_attr(all(not(debug_assertions), target_os = "windows"), windows_subsystem = "windows")]

mod commands;
use commands::*;

fn main() {
  tauri::Builder::default()
    .invoke_handler(tauri::generate_handler![
      device_upload, device_play, device_stop, device_set_param
    ])
    .run(tauri::generate_context!())
    .expect("error while running tauri application");
}
