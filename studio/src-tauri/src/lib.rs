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

#[derive(Clone, Serialize, Deserialize, Debug)]
pub struct DeviceInfo {
  pub name: String,
  pub host: String,
  pub port: u16,
}

#[derive(Default)]
struct ConnPool(Mutex<HashMap<String, ()>>); // placeholder for future WS clients

struct RecentState(Mutex<Vec<String>>);

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
  if p.len() >= off+1 { templates = p[off]; }
  serde_json::json!({
    "version": ver,
    "ledCount": led_count,
    "storageAvailable": avail,
    "maxChunk": max_chunk,
    "templateCount": templates,
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
    #[test]
    fn test_basic() {
        assert_eq!(2 + 2, 4);
    }
}
