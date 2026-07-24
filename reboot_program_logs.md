# Reboot Programs and Log Reference

This document describes all programs launched at reboot, their responsibilities, and the logs they produce.

## Reboot Startup Programs

### User Crontab (helper programs)

- `@reboot record_odas.py`
  - Manages passive audio recording.
- `@reboot cd /mnt/CHATAK_VM && python3 cors_http_server.py`
  - Starts an HTTP server on the flash drive so files are browsable.
- `@reboot /home/chatak/ChatakGUI/wifi_deamon.py`
  - Scans and connects to known Wi-Fi networks.

### Sudo Crontab (USB accessories: flash drive and microphone)

- `@reboot /usr/bin/python3 /home/chatak/ChatakGUI/asound_check.py`
  - Verifies microphone hardware credentials and updates ALSA config if needed.
- `@reboot /usr/bin/python3 /home/chatak/ChatakGUI/usb_mount.py`
  - Ensures the flash drive mounts correctly.

## ClassifierLogs

JSON output from the SST module, used for building and testing.

---

## record_odas.py Logs

Program: `@reboot record_odas.py`  
Purpose: manages passive recording.

### Log categories

1. **Startup and housekeeping**
   - Creates a timestamped log file (`cronDDMMYY_HHMM.log`).
   - Deletes old cron log files from the `logs` directory.
   - Logs errors encountered while deleting old logs.

2. **Process checks**
   - Checks whether ODAS (`odaslive` with `yammnetterminal_sensitive.cfg`) is already running.
   - Logs failures when checking running processes (for example if `ps -aux` fails).

3. **Mount operations**
   - Logs success/failure mounting `/mnt/CHATAK_VM`.
   - Logs unreadable mount issues.
   - Attempts `fsck` and remount if needed.
   - Logs failures during mount checks or after `fsck`.

4. **Recording events**
   - Logs recording start: `Recording with ODAS to <path>`.
   - Uses `record.txt` to determine recording duration.
   - Uses `pause.txt` to determine pause interval.

5. **Sleep/wake cycle**
   - Logs `Device is sleeping` when outside wake window.
   - Re-reads `wake.txt` and `sleep.txt` dynamically.

6. **Error handling**
   - Logs exceptions from process checks, mount operations, or recording with timestamp and message.

### Example log entries

- `2026-07-24 16:47:12 - Recording with ODAS to /mnt/CHATAK_VM/Passive_Audio`
- `2026-07-24 16:47:17 - CHATAK_VM not mounted. Skipping recording.`
- `2026-07-24 16:47:22 - Device is sleeping`
- `2026-07-24 16:47:25 - Mount failed after fsck: [error details]`
- `2026-07-24 16:47:30 - Process check failed: [error details]`

---

## wifi_deamon.py Logs

Program: `@reboot wifi_deamon.py`  
Purpose: manages Wi-Fi connectivity.

### Log categories

1. **Startup and housekeeping**
   - Creates a timestamped log file (`wifi_daemonDDMMYY_HHMM.log`).
   - Deletes old Wi-Fi daemon logs from the `logs` directory.
   - Logs errors encountered while deleting old logs.

2. **Connection status**
   - `Wi-Fi is connected to <SSID>` when connected.
   - `No Wi-Fi connection. Scanning available networks...` when disconnected.

3. **Network scanning**
   - `Found networks: <list>` after `nmcli` scan.
   - `Found networks: None` if no SSIDs are visible.

4. **Profile management**
   - `Profile for <SSID> already exists, skipping add.`
   - `Adding new profile for <SSID>...`
   - `Profile added for <SSID>`
   - `Failed to add profile for <SSID>`

5. **Connection attempts**
   - `Attempting to bring up connection <SSID>...`
   - `Connected to <SSID>`
   - `Failed to connect to <SSID>`

6. **wpa_supplicant errors**
   - `Error reading wpa_supplicant.conf: <error>`

7. **Daemon lifecycle**
   - `Wi-Fi daemon stopped by user` (for example on Ctrl+C).

### Example log entries

- `2026-07-24 16:51:02 - No Wi-Fi connection. Scanning available networks...`
- `2026-07-24 16:51:03 - Found networks: HomeNet, OfficeWiFi`
- `2026-07-24 16:51:03 - SSID HomeNet is in wpa_supplicant and visible.`
- `2026-07-24 16:51:03 - Profile for HomeNet already exists, skipping add.`
- `2026-07-24 16:51:03 - Attempting to bring up connection HomeNet...`
- `2026-07-24 16:51:05 - Connected to HomeNet`
- `2026-07-24 16:52:15 - Wi-Fi is connected to HomeNet`

---

## cors_http_server.py Logs

Program: `@reboot cors_http_server.py`  
Purpose: serves files from `/mnt/CHATAK_VM` with CORS enabled.

### Log categories

1. **Startup**
   - Startup message:
     `Serving HTTP on 0.0.0.0 port 8001, serving only /mnt/CHATAK_VM directory...`
   - Confirms bind address `0.0.0.0` and port `8001`.

2. **Request handling**
   - Requests are handled by `CORSRequestHandler`.
   - All request paths are translated to serve only from `/mnt/CHATAK_VM`.

3. **CORS headers**
   - Adds `Access-Control-Allow-Origin: *` to all responses.

4. **MIME type corrections**
   - `.css` served as `text/css`.
   - `.js` served as `application/javascript`.
   - Other files use default MIME detection.

5. **Server lifecycle**
   - Runs continuously using `httpd.serve_forever()`.
   - Primary explicit startup log; request logs appear as standard HTTP server entries (if enabled).

### Example log entries

- `Serving HTTP on 0.0.0.0 port 8001, serving only /mnt/CHATAK_VM directory...`
- `127.0.0.1 - - [24/Jul/2026 16:55:12] "GET /file.css HTTP/1.1" 200 -`

---

## asound_check.py Logs

Program: `@reboot asound_check.py`  
Purpose: validates and updates ALSA configuration for ReSpeaker devices.

### Log categories

1. **Startup and housekeeping**
   - Creates a timestamped log file (`sudocronDDMMYY_HHMM.log`).
   - Deletes old `sudocron` logs from the `logs` directory.
   - Logs errors encountered while deleting old logs.

2. **Device detection**
   - Runs `arecord -l` to list capture devices.
   - Searches for lines containing `ReSpeaker` or `ArrayUAC`.
   - Extracts `card` and `device` IDs when found.
   - Logs `ReSpeaker device not found in arecord -l output.` when not found.

3. **Configuration update**
   - Writes `/etc/asound.conf` with `pcm.multi_capture` mapped to detected `hw:<card>,<device>`.
   - Logs `Updated /etc/asound.conf with hw:<card>,<device>` after update.

4. **Error handling**
   - Logs exceptions in cleanup, detection, or config update with timestamp and message.

### Example log entries

- `2026-07-24 16:55:12 - ReSpeaker device not found in arecord -l output.`
- `2026-07-24 16:55:18 - Updated /etc/asound.conf with hw:2,0`
- `2026-07-24 16:55:20 - Could not delete sudocron240726_1645.log: [error details]`

---

## usb_mount.py Logs

Program: `@reboot usb_mount.py`  
Purpose: ensures `/mnt/CHATAK_VM` is mounted and available.

### Log categories

1. **Startup and housekeeping**
   - Creates a timestamped log file (`sudocronDDMMYY_HHMM.log`).
   - Deletes old `sudocron` logs from the `logs` directory.
   - Logs errors encountered while deleting old logs.

2. **Mount point setup**
   - `Created mount point /mnt/CHATAK_VM` if missing.
   - Confirms mount point readiness.

3. **Device information**
   - Logs `blkid /dev/sda1` output (UUID/filesystem).
   - Logs `Could not parse UUID or TYPE from blkid output.` if parsing fails.

4. **fstab management**
   - `Adding entry to /etc/fstab...` when UUID entry is missing.
   - `Entry already exists in /etc/fstab.` when present.

5. **Mount operations**
   - Runs `mount -a`.
   - Logs `USB mounted at /mnt/CHATAK_VM` when successful.

6. **Error handling**
   - Logs exceptions in mount-point creation, blkid parsing, fstab update, or mount operations.

### Example log entries

- `2026-07-24 16:58:12 - Created mount point /mnt/CHATAK_VM`
- `2026-07-24 16:58:13 - blkid output: /dev/sda1: UUID="abcd-1234" TYPE="ext4"`
- `2026-07-24 16:58:14 - Adding entry to /etc/fstab...`
- `2026-07-24 16:58:15 - USB mounted at /mnt/CHATAK_VM`
- `2026-07-24 16:58:16 - Could not parse UUID or TYPE from blkid output.`

---

This logging setup provides timestamped visibility into startup cleanup, device detection, mount state, connectivity behavior, recording operations, and runtime errors across all reboot services.
