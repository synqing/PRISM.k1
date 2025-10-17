// src-tauri/src/commands/device_upload.rs (PR TEMPLATE)
use tauri::State;

#[tauri::command]
pub async fn device_upload(host: String, name: String, bytes: Vec<u8>) -> Result<(), String> {
  // TODO: ws connect, PUT_BEGIN/DATA/END with CRC-32 chunking (STATUS.maxChunk)
  Ok(())
}
