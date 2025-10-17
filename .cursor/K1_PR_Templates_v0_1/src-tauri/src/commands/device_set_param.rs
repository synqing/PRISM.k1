// src-tauri/src/commands/device_set_param.rs (PR TEMPLATE)
#[tauri::command]
pub async fn device_set_param(host: String, id: u16, value: f32) -> Result<(), String> {
  // TODO: send TLV SET_PARAM
  Ok(())
}
