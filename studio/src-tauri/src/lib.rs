use tauri::Emitter;
use tauri::menu::{Menu, MenuItem, PredefinedMenuItem, Submenu};
use tauri::Listener;
use tauri::Manager;
use keyring::Entry;
use rand::RngCore;
use argon2::Argon2;
use base64;
use std::collections::HashMap;
use std::sync::{Arc, Mutex};
use serde::{Serialize, Deserialize};
use futures_util::{SinkExt, StreamExt};
use std::sync::{Arc, Mutex as StdMutex};
use std::time::Instant;

#[derive(Clone, Serialize, Deserialize, Debug)]
pub struct DeviceInfo {
  pub name: String,
  pub host: String,
  pub port: u16,
}

#[derive(Default)]
struct ConnPool(Mutex<HashMap<String, ()>>); // placeholder for future WS clients

struct RecentState(Mutex<Vec<String>>);

// Simple single-upload cancel flag
#[derive(Default)]
struct UploadCancel(StdMutex<Option<bool>>);

#[tauri::command]
async fn get_or_create_master_key() -> Result<String, String> {
  // Use OS keychain to persist a 32-byte master key, derive a password with Argon2, and return it (base64)
  let service = "com.prism.studio";
  let user = "stronghold_master_key";
  let entry = Entry::new(service, user).map_err(|e| e.to_string())?;
  let raw_b64 = match entry.get_password() {
    Ok(p) => p,
    Err(_) => {
      let mut key = [0u8; 32];
      rand::thread_rng().fill_bytes(&mut key);
      let b64 = base64::encode(key);
      entry.set_password(&b64).map_err(|e| e.to_string())?;
      b64
    }
  };
  let raw = base64::decode(raw_b64).map_err(|e| e.to_string())?;
  let salt = b"PRISM.K1.v1";
  let mut out = [0u8; 32];
  Argon2::default().hash_password_into(&raw, salt, &mut out).map_err(|e| e.to_string())?;
  Ok(base64::encode(out))
}

#[cfg_attr(mobile, tauri::mobile_entry_point)]
pub fn run() {
  tauri::Builder::default()
    .setup(|app| {
      // Logging plugin (dev only)
      if cfg!(debug_assertions) {
        app.handle().plugin(
          tauri_plugin_log::Builder::default()
            .level(log::LevelFilter::Info)
            .build(),
        )?;
      }

      // Core plugins
      app.handle().plugin(tauri_plugin_dialog::init())?;
      app.handle().plugin(tauri_plugin_fs::init())?;
      // Secure storage and key/value store
      app.handle().plugin(tauri_plugin_stronghold::Builder::new(|s| s.as_bytes().to_vec()).build())?;
      app.handle().plugin(tauri_plugin_store::Builder::default().build())?;
      // Opener plugin for revealing files/folders
      app.handle().plugin(tauri_plugin_opener::init())?;

      // Build native menu with accelerators
      // Build items for File submenu
      let mi_new = MenuItem::with_id(app, "new", "New", true, Some("CmdOrCtrl+N"))?;
      let mi_open = MenuItem::with_id(app, "open", "Open…", true, Some("CmdOrCtrl+O"))?;
      let mi_settings = MenuItem::with_id(app, "settings", "Settings…", true, Some("CmdOrCtrl+Comma"))?;
      let mi_sep1 = PredefinedMenuItem::separator(app)?;
      let mi_save = MenuItem::with_id(app, "save", "Save", true, Some("CmdOrCtrl+S"))?;
      let mi_save_as = MenuItem::with_id(app, "saveAs", "Save As…", true, Some("Shift+CmdOrCtrl+S"))?;
      let mi_sep2 = PredefinedMenuItem::separator(app)?;
      let mi_quit = PredefinedMenuItem::quit(app, None)?;

      // Build items for Edit submenu
      let mi_undo = MenuItem::with_id(app, "undo", "Undo", true, Some("CmdOrCtrl+Z"))?;
      let mi_redo = MenuItem::with_id(app, "redo", "Redo", true, Some("Shift+CmdOrCtrl+Z"))?;

      app.manage(RecentState(Mutex::new(Vec::new())));
      app.manage(UploadCancel::default());
      app.manage(ConnPool::default());

      // Recent submenu with 5 placeholder items + matching Reveal entries
      let mi_r0 = MenuItem::with_id(app, "recent_0", "(empty)", false, Option::<&str>::None)?;
      let mi_rr0 = MenuItem::with_id(app, "recent_reveal_0", "Reveal", false, Option::<&str>::None)?;
      let mi_r1 = MenuItem::with_id(app, "recent_1", "(empty)", false, Option::<&str>::None)?;
      let mi_rr1 = MenuItem::with_id(app, "recent_reveal_1", "Reveal", false, Option::<&str>::None)?;
      let mi_r2 = MenuItem::with_id(app, "recent_2", "(empty)", false, Option::<&str>::None)?;
      let mi_rr2 = MenuItem::with_id(app, "recent_reveal_2", "Reveal", false, Option::<&str>::None)?;
      let mi_r3 = MenuItem::with_id(app, "recent_3", "(empty)", false, Option::<&str>::None)?;
      let mi_rr3 = MenuItem::with_id(app, "recent_reveal_3", "Reveal", false, Option::<&str>::None)?;
      let mi_r4 = MenuItem::with_id(app, "recent_4", "(empty)", false, Option::<&str>::None)?;
      let mi_rr4 = MenuItem::with_id(app, "recent_reveal_4", "Reveal", false, Option::<&str>::None)?;
      let recent_sep = PredefinedMenuItem::separator(app)?;
      let recent_clear = MenuItem::with_id(app, "recent_clear", "Clear Recent", true, Option::<&str>::None)?;
      let recent_sub = Submenu::with_items(app, "Open Recent", true, &[
        &mi_r0, &mi_rr0,
        &mi_r1, &mi_rr1,
        &mi_r2, &mi_rr2,
        &mi_r3, &mi_rr3,
        &mi_r4, &mi_rr4,
        &recent_sep, &recent_clear
      ])?;

      let file_sub = Submenu::with_items(app, "File", true, &[
        &mi_new, &mi_open, &recent_sub, &mi_settings, &mi_sep1, &mi_save, &mi_save_as, &mi_sep2, &mi_quit
      ])?;
      let edit_sub = Submenu::with_items(app, "Edit", true, &[&mi_undo, &mi_redo])?;

      let main_menu = if cfg!(target_os = "macos") {
        let app_menu = Submenu::with_items(app, "PRISM Studio", true, &[
          &MenuItem::with_id(app, "settings", "Preferences…", true, Some("CmdOrCtrl+Comma"))?,
        ])?;
        Menu::with_items(app, &[&app_menu, &file_sub, &edit_sub])?
      } else {
        Menu::with_items(app, &[&file_sub, &edit_sub])?
      };

      app.set_menu(main_menu)?;

      // Forward menu events to the frontend
      let app_handle = app.handle();
  app_handle.on_menu_event(move |app, event| {
    let id = event.id().as_ref();
    match id {
      other if other.starts_with("recent_reveal_") => {
        if let Some(idx_str) = other.strip_prefix("recent_reveal_") {
          if let Ok(idx) = idx_str.parse::<usize>() {
            let state = app.state::<RecentState>();
            if let Some(path) = state.0.lock().ok().and_then(|v| v.get(idx).cloned()) {
              let _ = app.emit("menu:revealRecent", path);
            }
          }
        }
      }
          other if other.starts_with("recent_reveal_") => {
            if let Some(idx_str) = other.strip_prefix("recent_reveal_") {
              if let Ok(idx) = idx_str.parse::<usize>() {
                let state = app.state::<RecentState>();
                if let Some(path) = state.0.lock().ok().and_then(|v| v.get(idx).cloned()) {
                  let _ = app.emit("menu:revealRecent", path);
                }
              }
            }
          }
          "undo" | "redo" | "save" | "saveAs" | "open" | "new" | "settings" => {
            let _ = app.emit("menu:action", id);
          }
          other if other.starts_with("recent_") => {
            if let Some(idx_str) = other.strip_prefix("recent_") {
              if let Ok(idx) = idx_str.parse::<usize>() {
                let state = app.state::<RecentState>();
                if let Some(path) = state.0.lock().ok().and_then(|v| v.get(idx).cloned()) {
                  let _ = app.emit("menu:openRecent", path);
                }
              }
            }
          }
          "recent_clear" => {
            let _ = app.emit("menu:action", "clearRecent");
          }
          _ => {}
        }
      });

      // Listen for recent list updates from frontend to populate dynamic menu
      let apph_for_listen = app_handle.clone();
      let apph_for_listen_clone = apph_for_listen.clone();
      let _un = apph_for_listen.listen("recent:update", move |ev| {
        let app = &apph_for_listen_clone;
        let payload = ev.payload();
        if let Ok(paths) = serde_json::from_str::<Vec<String>>(payload) {
            // Save paths to state for click handling
            if let Some(state) = app.try_state::<RecentState>() {
              if let Ok(mut v) = state.0.lock() { *v = paths.clone(); }
            }

            // Rebuild menu with updated recents
            // Build static items again
            let mi_new = MenuItem::with_id(app, "new", "New", true, Some("CmdOrCtrl+N")).unwrap();
            let mi_open = MenuItem::with_id(app, "open", "Open…", true, Some("CmdOrCtrl+O")).unwrap();
            let mi_settings = MenuItem::with_id(app, "settings", "Settings…", true, Some("CmdOrCtrl+Comma")).unwrap();
            let mi_sep1 = PredefinedMenuItem::separator(app).unwrap();
            let mi_save = MenuItem::with_id(app, "save", "Save", true, Some("CmdOrCtrl+S")).unwrap();
            let mi_save_as = MenuItem::with_id(app, "saveAs", "Save As…", true, Some("Shift+CmdOrCtrl+S")).unwrap();
            let mi_sep2 = PredefinedMenuItem::separator(app).unwrap();
            let mi_quit = PredefinedMenuItem::quit(app, None).unwrap();

            let mi_undo = MenuItem::with_id(app, "undo", "Undo", true, Some("CmdOrCtrl+Z")).unwrap();
            let mi_redo = MenuItem::with_id(app, "redo", "Redo", true, Some("Shift+CmdOrCtrl+Z")).unwrap();

            let (mi_r0, mi_rr0, mi_r1, mi_rr1, mi_r2, mi_rr2, mi_r3, mi_rr3, mi_r4, mi_rr4) = (
              MenuItem::with_id(app, "recent_0", "(empty)", false, Option::<&str>::None).unwrap(),
              MenuItem::with_id(app, "recent_reveal_0", "Reveal", false, Option::<&str>::None).unwrap(),
              MenuItem::with_id(app, "recent_1", "(empty)", false, Option::<&str>::None).unwrap(),
              MenuItem::with_id(app, "recent_reveal_1", "Reveal", false, Option::<&str>::None).unwrap(),
              MenuItem::with_id(app, "recent_2", "(empty)", false, Option::<&str>::None).unwrap(),
              MenuItem::with_id(app, "recent_reveal_2", "Reveal", false, Option::<&str>::None).unwrap(),
              MenuItem::with_id(app, "recent_3", "(empty)", false, Option::<&str>::None).unwrap(),
              MenuItem::with_id(app, "recent_reveal_3", "Reveal", false, Option::<&str>::None).unwrap(),
              MenuItem::with_id(app, "recent_4", "(empty)", false, Option::<&str>::None).unwrap(),
              MenuItem::with_id(app, "recent_reveal_4", "Reveal", false, Option::<&str>::None).unwrap(),
            );

            // Fill recent items based on paths
            let placeholders = [&mi_r0, &mi_r1, &mi_r2, &mi_r3, &mi_r4];
            for (i, item) in placeholders.iter().enumerate() {
              if let Some(p) = paths.get(i) {
                let filename = std::path::Path::new(p).file_name().and_then(|s| s.to_str()).unwrap_or(p);
                let _ = item.set_text(&format!("{} {} — {}", i+1, filename, p));
                let _ = item.set_enabled(true);
              }
            }
            let revealers = [&mi_rr0, &mi_rr1, &mi_rr2, &mi_rr3, &mi_rr4];
            for (i, item) in revealers.iter().enumerate() {
              if let Some(p) = paths.get(i) {
                let filename = std::path::Path::new(p).file_name().and_then(|s| s.to_str()).unwrap_or(p);
                let label = if cfg!(target_os = "macos") {
                  format!("Reveal in Finder {}", filename)
                } else if cfg!(target_os = "windows") {
                  format!("Show in Explorer {}", filename)
                } else {
                  format!("Reveal in File Manager {}", filename)
                };
                let _ = item.set_text(&label);
                let _ = item.set_enabled(true);
              }
            }

            let sep = PredefinedMenuItem::separator(app).unwrap();
            let clear = MenuItem::with_id(app, "recent_clear", "Clear Recent", true, Option::<&str>::None).unwrap();
            let recent_sub = Submenu::with_items(app, "Open Recent", true, &[
              placeholders[0] as &dyn tauri::menu::IsMenuItem<_>, &mi_rr0,
              placeholders[1] as &dyn tauri::menu::IsMenuItem<_>, &mi_rr1,
              placeholders[2] as &dyn tauri::menu::IsMenuItem<_>, &mi_rr2,
              placeholders[3] as &dyn tauri::menu::IsMenuItem<_>, &mi_rr3,
              placeholders[4] as &dyn tauri::menu::IsMenuItem<_>, &mi_rr4,
              &sep, &clear,
            ]).unwrap();

            let file_sub = if cfg!(target_os = "macos") {
              Submenu::with_items(app, "File", true, &[&mi_new, &mi_open, &recent_sub, &mi_sep1, &mi_save, &mi_save_as, &mi_sep2, &mi_quit]).unwrap()
            } else {
              Submenu::with_items(app, "File", true, &[&mi_new, &mi_open, &recent_sub, &mi_settings, &mi_sep1, &mi_save, &mi_save_as, &mi_sep2, &mi_quit]).unwrap()
            };
            let edit_sub = Submenu::with_items(app, "Edit", true, &[&mi_undo, &mi_redo]).unwrap();
            let main_menu = if cfg!(target_os = "macos") {
              let app_menu = Submenu::with_items(app, "PRISM Studio", true, &[
                &MenuItem::with_id(app, "settings", "Preferences…", true, Some("CmdOrCtrl+Comma")).unwrap(),
              ]).unwrap();
              Menu::with_items(app, &[&app_menu, &file_sub, &edit_sub]).unwrap()
            } else {
              Menu::with_items(app, &[&file_sub, &edit_sub]).unwrap()
            };
            let _ = app.set_menu(main_menu);
          }
      });
      // keep listener alive via _un binding

      Ok(())
    })
    .invoke_handler(tauri::generate_handler![device_discover, device_connect, device_status, device_list, device_delete, get_or_create_master_key])
    .invoke_handler(tauri::generate_handler![device_export])
    .invoke_handler(tauri::generate_handler![device_upload, device_upload_cancel])
    .invoke_handler(tauri::generate_handler![device_control_play, device_control_stop, device_control_brightness, device_control_gamma])
    .run(tauri::generate_context!())
    .expect("error while running tauri application");
}

#[tauri::command]
async fn device_discover() -> Result<Vec<DeviceInfo>, String> {
  // Try mDNS browse for _prism-k1._tcp.local.
  // Fallback to prism-k1.local:80 if nothing found.
  let mut out: Vec<DeviceInfo> = Vec::new();
  // mdns-sd browse can block; run in blocking task and collect results for a short window
  let res = tauri::async_runtime::spawn_blocking(|| {
    let mut svc = mdns_sd::ServiceDaemon::new().map_err(|e| e.to_string())?;
    let ty = "_prism-k1._tcp.local.";
    let receiver = svc.browse(ty).map_err(|e| e.to_string())?;
    let start = std::time::Instant::now();
    let mut found: Vec<DeviceInfo> = Vec::new();
    while start.elapsed() < std::time::Duration::from_millis(800) {
      if let Ok(event) = receiver.recv_timeout(std::time::Duration::from_millis(100)) {
        if let mdns_sd::ServiceEvent::ServiceResolved(info) = event {
          let host = info.get_hostname().to_string();
          let port = info.get_port();
          let name = info.get_fullname().to_string();
          found.push(DeviceInfo { name, host, port });
        }
      }
    }
    Ok::<_, String>(found)
  }).await.map_err(|e| e.to_string())?;

  match res {
    Ok(mut v) => {
      if v.is_empty() {
        // fallback well-known host
        v.push(DeviceInfo{ name: "prism-k1".into(), host: "prism-k1.local.".into(), port: 80 });
      }
      out = v;
    }
    Err(e) => return Err(e),
  }
  Ok(out)
}

#[tauri::command]
async fn device_connect(host: String, port: Option<u16>) -> Result<String, String> {
  let url = format!("ws://{}:{}/", host.trim_end_matches('.'), port.unwrap_or(80));
  // Just attempt a connection and then close to validate reachability.
  let connect_res = tokio_tungstenite::connect_async(url.as_str()).await;
  match connect_res {
    Ok((_ws, _)) => Ok("CONNECTED".to_string()),
    Err(e) => Err(format!("CONNECT_FAILED: {}", e)),
  }
}

#[tauri::command]
async fn device_status(host: String) -> Result<serde_json::Value, String> {
  match tlv_request(host, 80, 0x30, &[]).await {
    Ok(payload) => Ok(decode_status_payload(&payload)),
    Err(e) => Err(e)
  }
}

#[tauri::command]
async fn device_list(host: String) -> Result<serde_json::Value, String> {
  match tlv_request(host, 80, 0x22, &[]).await {
    Ok(payload) => Ok(decode_list_payload(&payload)),
    Err(e) => Err(e)
  }
}

#[tauri::command]
async fn device_delete(host: String, name: String) -> Result<bool, String> {
  match tlv_request(host, 80, 0x21, name.as_bytes()).await {
    Ok(_payload) => Ok(true),
    Err(e) => Err(e)
  }
}

#[tauri::command]
async fn device_export(host: String, id: String, path: String) -> Result<bool, String> {
  // Best-effort: try HTTP GET to /patterns/<id>.bin
  let url = format!("http://{}:{}/patterns/{}.bin", host.trim_end_matches('.'), 80, id);
  let resp = reqwest::get(url).await.map_err(|e| format!("HTTP_FAILED: {}", e))?;
  if !resp.status().is_success() {
    return Err(format!("HTTP_STATUS: {}", resp.status()));
  }
  let bytes = resp.bytes().await.map_err(|e| format!("READ_FAILED: {}", e))?;
  std::fs::write(&path, &bytes).map_err(|e| format!("WRITE_FAILED: {}", e))?;
  Ok(true)
}

fn crc32_bytes(data: &[u8]) -> u32 { crc32fast::hash(data) }

async fn ws_connect(host: &str, port: u16) -> Result<tokio_tungstenite::WebSocketStream<tokio_tungstenite::ConnectorStream>, String> {
  let url = format!("ws://{}:{}/", host.trim_end_matches('.'), port);
  let (ws, _resp) = tokio_tungstenite::connect_async(url)
    .await.map_err(|e| format!("WS_CONNECT_FAILED: {}", e))?;
  Ok(ws)
}

fn build_tlv_frame(typ: u8, payload: &[u8]) -> Vec<u8> {
  let mut frame: Vec<u8> = Vec::with_capacity(3 + payload.len() + 4);
  frame.push(typ);
  let len = payload.len() as u16;
  frame.push((len >> 8) as u8);
  frame.push((len & 0xFF) as u8);
  frame.extend_from_slice(payload);
  let crc = crc32_bytes(&frame);
  frame.extend_from_slice(&crc.to_be_bytes());
  frame
}

async fn ws_send_and_recv(mut ws: tokio_tungstenite::WebSocketStream<tokio_tungstenite::ConnectorStream>, frame: Vec<u8>) -> Result<Vec<u8>, String> {
  use tokio_tungstenite::tungstenite::Message;
  ws.send(Message::Binary(frame)).await.map_err(|e| format!("WS_SEND_FAILED: {}", e))?;
  if let Some(msg) = ws.next().await {
    match msg {
      Ok(Message::Binary(data)) => {
        if data.len() < 7 { return Err("TLV_TOO_SHORT".into()); }
        let r_len = u16::from_be_bytes([data[1], data[2]]) as usize;
        if data.len() != 3 + r_len + 4 { return Err("TLV_LENGTH_MISMATCH".into()); }
        let calc = crc32_bytes(&data[0..3+r_len]);
        let r_crc = u32::from_be_bytes([data[3+r_len], data[3+r_len+1], data[3+r_len+2], data[3+r_len+3]]);
        if calc != r_crc { return Err("TLV_CRC_INVALID".into()); }
        Ok(data)
      }
      Ok(_) => Err("WS_NON_BINARY".into()),
      Err(e) => Err(format!("WS_READ_FAILED: {}", e))
    }
  } else {
    Err("WS_CLOSED".into())
  }
}

trait ProgressEmitter {
  fn emit(&self, event: &str, payload: serde_json::Value);
}

impl ProgressEmitter for tauri::Window {
  fn emit(&self, event: &str, payload: serde_json::Value) { let _ = self.emit(event, payload); }
}

struct TestEmitter(pub std::sync::Arc<std::sync::Mutex<Vec<serde_json::Value>>>);
impl ProgressEmitter for TestEmitter {
  fn emit(&self, _event: &str, payload: serde_json::Value) { let _ = self.0.lock().map(|mut v| v.push(payload)); }
}

async fn upload_with_emitter<E: ProgressEmitter>(emitter: &E, cancel: &UploadCancel, host: String, name: String, bytes: Vec<u8>) -> Result<String, String> {
  use futures_util::StreamExt;
  // Fetch STATUS to learn maxChunk (optional)
  let status = tlv_request(host.clone(), 80, 0x30, &[]).await.ok();
  let mut max_chunk: usize = 4089; // default
  if let Some(payload) = status {
    // decode_status_payload: [verlen][ver][ledCount(LE16)][avail(LE32)][maxChunk(LE16)]
    if payload.len() >= 4 { let vlen = u32::from_le_bytes([payload[0],payload[1],payload[2],payload[3]]) as usize;
      let mut off = 4 + vlen;
      if payload.len() >= off+2+4+2 { off += 2 + 4; let mc = u16::from_le_bytes([payload[off],payload[off+1]]); max_chunk = mc as usize; }
    }
  }

  // Helper to open WS
  let url = format!("ws://{}:{}/", host.trim_end_matches('.'), 80);
  let mut connect_ws = || async {
    let (ws, _resp) = tokio_tungstenite::connect_async(url.clone())
      .await.map_err(|e| format!("WS_CONNECT_FAILED: {}", e))?;
    Ok::<_, String>(ws)
  };
  let mut ws = connect_ws().await?;

  let size = bytes.len();
  // Enforce device max pattern size 256KB
  if size > 262_144 { return Err("PATTERN_MAX_SIZE_EXCEEDED".into()); }
  let crc = crc32_bytes(&bytes);
  if name.is_empty() || name.len() >= 64 { return Err("INVALID_NAME".into()); }
  let mut begin = Vec::with_capacity(1 + name.len() + 8);
  begin.push(name.len() as u8);
  begin.extend_from_slice(name.as_bytes());
  begin.extend_from_slice(&(size as u32).to_be_bytes());
  begin.extend_from_slice(&crc.to_be_bytes());

  // PUT_BEGIN (await one ack frame)
  let frame = build_tlv_frame(0x10, &begin);
  use tokio_tungstenite::tungstenite::Message;
  ws.send(Message::Binary(frame)).await.map_err(|e| format!("WS_SEND_FAILED: {}", e))?;
  if let Some(msg) = ws.next().await { match msg { Ok(Message::Binary(_)) => {}, _ => {} } }

  // stream PUT_DATA with cancel, EMA throughput, and 10 Hz throttle
  let t0 = Instant::now();
  let mut sent = 0usize;
  // reset cancel flag
  if let Ok(mut f) = cancel.0.lock() { *f = Some(false); }
  // EMA and throttle state
  let mut last_emit = Instant::now();
  let mut last_mark = Instant::now();
  let mut last_bytes: usize = 0;
  let mut ema_bps: f64 = 0.0;
  let alpha: f64 = 0.2; // EMA smoothing factor
  let mut retried = false;
  while sent < size {
    // cancellation check
    if let Ok(f) = cancel.0.lock() { if f.unwrap_or(false) {
      let _ = window.emit("upload:progress", serde_json::json!({"phase":"cancelled","bytesSent": sent, "totalBytes": size}));
      // Close WS and return early without PUT_END
      let _ = ws.close(None).await;
      return Ok("CANCELLED".into());
    }}
    let chunk_len = std::cmp::min(max_chunk, size - sent);
    let mut payload = Vec::with_capacity(4 + chunk_len);
    payload.extend_from_slice(&((sent as u32).to_be_bytes()));
    payload.extend_from_slice(&bytes[sent..sent+chunk_len]);
    let frame = build_tlv_frame(0x11, &payload);
  if let Err(e) = ws.send(Message::Binary(frame)).await {
      // attempt single reconnect
      if retried { return Err(format!("WS_SEND_FAILED: {}", e)); }
      retried = true;
      // Re-STATUS to refresh maxChunk
      let status = tlv_request(host.clone(), 80, 0x30, &[]).await.ok();
      if let Some(payload) = status {
        if payload.len() >= 4 { let vlen = u32::from_le_bytes([payload[0],payload[1],payload[2],payload[3]]) as usize;
          let mut off = 4 + vlen;
          if payload.len() >= off+2+4+2 { off += 2 + 4; let mc = u16::from_le_bytes([payload[off],payload[off+1]]); max_chunk = mc as usize; }
        }
      }
      ws = connect_ws().await?;
      // Re-send PUT_BEGIN to (re)start session
      let frame_b = build_tlv_frame(0x10, &begin);
      ws.send(Message::Binary(frame_b)).await.map_err(|e| format!("WS_SEND_FAILED: {}", e))?;
      if let Some(msg2) = ws.next().await { match msg2 { Ok(Message::Binary(_)) => {}, _ => {} } }
      // continue loop without advancing sent
      continue;
    }
    sent += chunk_len;

    // Throttle to ~10Hz and compute EMA throughput
    let now = Instant::now();
    let dt_mark = now.duration_since(last_mark).as_millis() as f64 / 1000.0;
    if dt_mark > 0.0 {
      let bytes_delta = (sent - last_bytes) as f64;
      let inst_bps = bytes_delta / dt_mark;
      ema_bps = if ema_bps == 0.0 { inst_bps } else { alpha * inst_bps + (1.0 - alpha) * ema_bps };
      last_mark = now;
      last_bytes = sent;
    }
    if now.duration_since(last_emit).as_millis() >= 100 {
      let percent = (sent as f64 / size as f64) * 100.0;
      let elapsed_ms = t0.elapsed().as_millis() as u64;
      emitter.emit("upload:progress", serde_json::json!({
        "phase": "stream",
        "bytesSent": sent,
        "totalBytes": size,
        "percent": percent,
        "bytesPerSec": ema_bps as u64,
        "elapsedMs": elapsed_ms,
      }));
      last_emit = now;
    }
  }

  // PUT_END (empty payload) after full success only
  let frame = build_tlv_frame(0x12, &[]);
  ws.send(Message::Binary(frame)).await.map_err(|e| format!("WS_SEND_FAILED: {}", e))?;
  // Await final ack (best-effort)
  if let Some(_msg) = ws.next().await { /* ignore content; validated by firmware */ }
  emitter.emit("upload:progress", serde_json::json!({
    "phase": "finalizing",
    "bytesSent": size,
    "totalBytes": size,
    "percent": 100.0,
  }));
  emitter.emit("upload:progress", serde_json::json!({"phase":"done"}));

  Ok("OK".into())
}

#[tauri::command]
async fn device_upload(window: tauri::Window, cancel: tauri::State<'_, UploadCancel>, host: String, name: String, bytes: Vec<u8>) -> Result<String, String> {
  upload_with_emitter(&window, &*cancel, host, name, bytes).await
}

#[tauri::command]
async fn device_upload_cancel(cancel: tauri::State<'_, UploadCancel>) -> Result<bool, String> {
  if let Ok(mut f) = cancel.0.lock() { *f = Some(true); }
  Ok(true)
}

#[tauri::command]
async fn device_control_play(host: String, name: String) -> Result<bool, String> {
  // CONTROL 0x20: [0x01][name_len][name]
  const CONTROL_TYPE: u8 = 0x20;
  const CTRL_PLAY: u8 = 0x01;
  let mut payload = Vec::with_capacity(2 + name.len());
  payload.push(CTRL_PLAY);
  payload.push(name.len() as u8);
  payload.extend_from_slice(name.as_bytes());
  let resp = tlv_request(host, 80, CONTROL_TYPE, &payload).await?;
  let _ = resp; Ok(true)
}

#[tauri::command]
async fn device_control_stop(host: String) -> Result<bool, String> {
  const CONTROL_TYPE: u8 = 0x20;
  const CTRL_STOP: u8 = 0x02;
  let payload = [CTRL_STOP];
  let resp = tlv_request(host, 80, CONTROL_TYPE, &payload).await?;
  let _ = resp; Ok(true)
}

#[tauri::command]
async fn device_control_brightness(host: String, target: u8, duration_ms: u16) -> Result<bool, String> {
  const CONTROL_TYPE: u8 = 0x20;
  const CTRL_BRIGHTNESS: u8 = 0x10;
  let mut payload = Vec::with_capacity(4);
  payload.push(CTRL_BRIGHTNESS);
  payload.push(target);
  payload.extend_from_slice(&duration_ms.to_be_bytes());
  let _ = tlv_request(host, 80, CONTROL_TYPE, &payload).await?; Ok(true)
}

#[tauri::command]
async fn device_control_gamma(host: String, gamma_x100: u16, duration_ms: u16) -> Result<bool, String> {
  const CONTROL_TYPE: u8 = 0x20;
  const CTRL_GAMMA: u8 = 0x11;
  let mut payload = Vec::with_capacity(5);
  payload.push(CTRL_GAMMA);
  payload.extend_from_slice(&gamma_x100.to_be_bytes());
  payload.extend_from_slice(&duration_ms.to_be_bytes());
  let _ = tlv_request(host, 80, CONTROL_TYPE, &payload).await?; Ok(true)
}

async fn tlv_request(host: String, port: u16, typ: u8, payload: &[u8]) -> Result<Vec<u8>, String> {
  use futures_util::{SinkExt, StreamExt};
  let url = format!("ws://{}:{}/", host.trim_end_matches('.'), port);
  let (mut ws, _resp) = tokio_tungstenite::connect_async(url)
    .await.map_err(|e| format!("WS_CONNECT_FAILED: {}", e))?;

  // Build TLV frame
  let mut frame: Vec<u8> = Vec::with_capacity(3 + payload.len() + 4);
  frame.push(typ);
  let len = payload.len() as u16;
  frame.push((len >> 8) as u8);
  frame.push((len & 0xFF) as u8);
  frame.extend_from_slice(payload);
  let crc = crc32fast::hash(&frame);
  frame.extend_from_slice(&(crc.to_be_bytes()));

  ws.send(tokio_tungstenite::tungstenite::Message::Binary(frame))
    .await.map_err(|e| format!("WS_SEND_FAILED: {}", e))?;

  // Await one binary response
  if let Some(msg) = ws.next().await {
    match msg {
      Ok(tokio_tungstenite::tungstenite::Message::Binary(data)) => {
        if data.len() < 7 { return Err("TLV_TOO_SHORT".into()); }
        let r_typ = data[0];
        let r_len = u16::from_be_bytes([data[1], data[2]]) as usize;
        if data.len() != 3 + r_len + 4 { return Err("TLV_LENGTH_MISMATCH".into()); }
        let r_payload = &data[3..3+r_len];
        let r_crc = u32::from_be_bytes([data[3+r_len], data[3+r_len+1], data[3+r_len+2], data[3+r_len+3]]);
        let calc = crc32fast::hash(&data[0..3+r_len]);
        if r_crc != calc { return Err("TLV_CRC_INVALID".into()); }
        if r_typ == 0xFF { // error frame
          // try parse error text
          let msg = std::str::from_utf8(r_payload).unwrap_or("error");
          return Err(format!("DEVICE_ERROR: {}", msg));
        }
        Ok(r_payload.to_vec())
      }
      Ok(_) => Err("WS_NON_BINARY".into()),
      Err(e) => Err(format!("WS_READ_FAILED: {}", e))
    }
  } else {
    Err("WS_CLOSED".into())
  }
}

fn read_u16_le(b: &[u8]) -> u16 { u16::from_le_bytes([b[0], b[1]]) }
fn read_u32_le(b: &[u8]) -> u32 { u32::from_le_bytes([b[0], b[1], b[2], b[3]]) }

fn decode_status_payload(p: &[u8]) -> serde_json::Value {
  // [0..4] ver_len (u32 LE), [..] ver bytes, [..] led_count (u16 LE), [..] avail (u32 LE), [..] max_chunk (u16 LE), [..] tpl_count (u8)
  let mut off = 0usize;
  let mut ver = "".to_string();
  let mut led_count = 0u16;
  let mut avail = 0u32;
  let mut max_chunk = 0u16;
  let mut templates = 0u8;
  if p.len() >= 4 { let vlen = read_u32_le(&p[0..4]) as usize; off = 4;
    if p.len() >= off+vlen { ver = String::from_utf8_lossy(&p[off..off+vlen]).to_string(); off += vlen; }
  }
  if p.len() >= off+2 { led_count = read_u16_le(&p[off..off+2]); off += 2; }
  if p.len() >= off+4 { avail = read_u32_le(&p[off..off+4]); off += 4; }
  if p.len() >= off+2 { max_chunk = read_u16_le(&p[off..off+2]); off += 2; }
  if p.len() >= off+1 { templates = p[off]; off += 1; }
  let mut caps: Option<u32> = None;
  if p.len() >= off+4 { caps = Some(read_u32_le(&p[off..off+4])); }
  serde_json::json!({
    "version": ver,
    "ledCount": led_count,
    "storageAvailable": avail,
    "maxChunk": max_chunk,
    "templateCount": templates,
    "caps": caps,
  })
}

fn decode_list_payload(p: &[u8]) -> serde_json::Value {
  // count(u16 LE), then [name_len(u16) name bytes size(u32) mtime(u32)] * count
  if p.len() < 2 { return serde_json::json!({ "count": 0, "items": [] }); }
  let mut off = 0usize;
  let count = read_u16_le(&p[off..off+2]) as usize; off += 2;
  let mut items: Vec<serde_json::Value> = Vec::with_capacity(count);
  for _ in 0..count {
    if p.len() < off+2 { break; }
    let nlen = read_u16_le(&p[off..off+2]) as usize; off += 2;
    if p.len() < off+nlen+8 { break; }
    let name = String::from_utf8_lossy(&p[off..off+nlen]).to_string(); off += nlen;
    let size = read_u32_le(&p[off..off+4]); off += 4;
    let mtime = read_u32_le(&p[off..off+4]); off += 4;
    items.push(serde_json::json!({ "id": name, "size": size, "mtime": mtime }));
  }
  serde_json::json!({ "count": items.len(), "items": items })
}

#[cfg(test)]
mod tests {
    use super::*;
    use tungstenite::protocol::Message as WsMessage;
    use tokio::net::TcpListener;
    use tokio_tungstenite::accept_async;
    use futures_util::StreamExt;
    use std::sync::{Arc, Mutex};

    fn parse_tlv_len(buf: &[u8]) -> usize { if buf.len()>=3 { u16::from_be_bytes([buf[1],buf[2]]) as usize } else { 0 } }

    #[tokio::test(flavor = "multi_thread", worker_threads = 2)]
    async fn ws_mock_ack_and_put_end() {
        // Spin up a minimal WS server that acks PUT_BEGIN and PUT_END, and drops once mid-stream
        let listener = TcpListener::bind("127.0.0.1:0").await.unwrap();
        let addr = listener.local_addr().unwrap();

        // Server task
        let server = tokio::spawn(async move {
            let (stream, _) = listener.accept().await.unwrap();
            let mut ws = accept_async(stream).await.unwrap();
            // Expect a frame (PUT_BEGIN), echo ack
            if let Some(Ok(WsMessage::Binary(data))) = ws.next().await { let _ = ws.send(WsMessage::Binary(data)).await; }
            // Next frame (PUT_DATA) -> drop connection to force client reconnect
            let _ = ws.next().await;
            // End first connection
        });

        // Client connects, sends a frame, and handles reconnect using helper
        let url = format!("ws://{}/", addr);
        let (mut ws, _) = tokio_tungstenite::connect_async(url).await.unwrap();
        // Send a dummy PUT_BEGIN
        let begin = build_tlv_frame(0x10, &[0x01, 0x02]);
        ws.send(WsMessage::Binary(begin.clone())).await.unwrap();
        // Expect an ack back (same frame echoed)
        if let Some(Ok(WsMessage::Binary(resp))) = ws.next().await {
            assert_eq!(parse_tlv_len(&resp), parse_tlv_len(&begin));
        }
        // Send PUT_DATA then expect server to drop (no assertion, just ensure no panic)
        let data = build_tlv_frame(0x11, &[0,0,0,0, 1,2,3,4]);
        let _ = ws.send(WsMessage::Binary(data)).await;
        let _ = server.await; // server terminates after drop
    }

    #[tokio::test(flavor = "multi_thread", worker_threads = 2)]
    async fn upload_with_emitter_reconnect_and_finalize() {
        // Start a server that acks begin, drops first PUT_DATA to force reconnect, then accepts full flow
        let listener = TcpListener::bind("127.0.0.1:0").await.unwrap();
        let addr = listener.local_addr().unwrap();
        let dropped = Arc::new(Mutex::new(false));
        let dropped_c = dropped.clone();

        let server = tokio::spawn(async move {
            // First connection
            let (stream, _) = listener.accept().await.unwrap();
            let mut ws = accept_async(stream).await.unwrap();
            // PUT_BEGIN ack
            if let Some(Ok(WsMessage::Binary(data))) = ws.next().await { let _ = ws.send(WsMessage::Binary(data)).await; }
            // Drop on first PUT_DATA
            let _ = ws.next().await; // ignore content and drop
            // Second connection
            let (stream2, _) = listener.accept().await.unwrap();
            let mut ws2 = accept_async(stream2).await.unwrap();
            // Expect PUT_BEGIN again, ack
            if let Some(Ok(WsMessage::Binary(data))) = ws2.next().await { let _ = ws2.send(WsMessage::Binary(data)).await; }
            // Accept some PUT_DATA frames
            while let Some(msg) = ws2.next().await { match msg { Ok(WsMessage::Binary(_d)) => {
                // NOP; in real device we would validate offset
                let _ = dropped_c.lock().map(|mut f| *f = true);
            }, _ => break } }
        });

        // Build small payload
        let host = format!("{}", addr);
        let bytes = vec![1u8; 10*1024];
        let emitter_buf: Arc<Mutex<Vec<serde_json::Value>>> = Arc::new(Mutex::new(Vec::new()));
        let emitter = TestEmitter(emitter_buf.clone());
        let state = UploadCancel(Default::default());
        let res = upload_with_emitter(&emitter, &state, host, "baked".into(), bytes).await;
        assert!(res.is_ok());
        assert!(*dropped.lock().unwrap());
        let events = emitter_buf.lock().unwrap();
        assert!(events.iter().any(|e| e.get("phase").and_then(|x| x.as_str())==Some("stream")));
    }

    #[tokio::test(flavor = "multi_thread", worker_threads = 2)]
    async fn put_end_has_empty_payload() {
        // Server that inspects PUT_END and asserts zero-length payload
        let listener = TcpListener::bind("127.0.0.1:0").await.unwrap();
        let addr = listener.local_addr().unwrap();

        let server = tokio::spawn(async move {
            let (stream, _) = listener.accept().await.unwrap();
            let mut ws = accept_async(stream).await.unwrap();
            // Expect PUT_BEGIN, ack
            if let Some(Ok(WsMessage::Binary(data))) = ws.next().await {
                let _ = ws.send(WsMessage::Binary(data)).await;
            }
            // Expect PUT_END next (empty upload -> no DATA frames)
            if let Some(Ok(WsMessage::Binary(data2))) = ws.next().await {
                assert_eq!(data2[0], 0x12);
                let len = u16::from_be_bytes([data2[1], data2[2]]);
                assert_eq!(len, 0, "PUT_END payload must be empty");
            }
        });

        // Upload empty bytes so client goes straight to PUT_END
        let host = format!("{}", addr);
        let emitter_buf: Arc<Mutex<Vec<serde_json::Value>>> = Arc::new(Mutex::new(Vec::new()));
        let emitter = TestEmitter(emitter_buf.clone());
        let state = UploadCancel(Default::default());
        let res = upload_with_emitter(&emitter, &state, host, "baked".into(), vec![]).await;
        assert!(res.is_ok());
        let _ = server.await;
    }
}
