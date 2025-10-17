// src-tauri/src/commands/device_stop.rs (PR TEMPLATE)
#[tauri::command]
pub async fn device_stop(host: String) -> Result<(), String> {
  // TODO: send TLV STOP
  Ok(())
}
