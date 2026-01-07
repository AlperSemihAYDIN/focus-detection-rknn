# Installation Guide

## Prerequisites Check

### 1. Verify RKNN Runtime
```bash
ssh root@<DEVICE_IP> "ls -l /usr/lib/librknnrt.so"
```
Expected output: `/usr/lib/librknnrt.so -> librknnrt.so.X.X.X`

If missing, install RKNN runtime for your board.

### 2. Verify FFmpeg
```bash
ssh root@<DEVICE_IP> "ffmpeg -version | head -1"
```
Expected: `ffmpeg version 7.x.x` or similar

### 3. Test Camera Connection
```bash
ssh root@<DEVICE_IP> "timeout 5 ffmpeg -rtsp_transport tcp -i rtsp://admin:admin123@<CAMERA_IP>:554/stream1 -f null -"
```
Should show video stream info without errors.

## Step-by-Step Installation

### Step 1: Copy Files
```bash
# Create directory
ssh root@<DEVICE_IP> "mkdir -p /root/focus"

# Copy all files
scp focus_rtsp root@<DEVICE_IP>:/root/focus/
scp model_640_fp16.rknn root@<DEVICE_IP>:/root/focus/
scp focus_rtsp.c root@<DEVICE_IP>:/root/focus/
```

### Step 2: Configure Camera IP
Edit `focus.service` on your PC:
```bash
nano focus.service
```

Change this line:
```ini
Environment="RTSP_URL=rtsp://admin:admin123@192.168.1.21:554/stream1"
```
Replace `192.168.1.21` with your camera's IP address.

### Step 3: Install Service
```bash
# Copy service file
scp focus.service root@<DEVICE_IP>:/etc/systemd/system/

# Reload systemd
ssh root@<DEVICE_IP> "systemctl daemon-reload"

# Enable service (auto-start on boot)
ssh root@<DEVICE_IP> "systemctl enable focus.service"

# Start service
ssh root@<DEVICE_IP> "systemctl start focus.service"
```

### Step 4: Install Log Rotation (Optional)
```bash
# Install logrotate if not present
# (Download .deb for your architecture first)
scp logrotate_*.deb root@<DEVICE_IP>:/tmp/
ssh root@<DEVICE_IP> "dpkg -i /tmp/logrotate_*.deb"

# Install rotation config
scp focus-logrotate root@<DEVICE_IP>:/etc/logrotate.d/focus
ssh root@<DEVICE_IP> "chmod 644 /etc/logrotate.d/focus"
```

### Step 5: Verify Installation
```bash
# Check service status
ssh root@<DEVICE_IP> "systemctl status focus.service"

# Should show: Active: active (running)

# Watch live results
ssh root@<DEVICE_IP> "tail -f /root/focus/result.txt"
```

You should see JSON lines appearing every 200ms:
```json
{"timestamp":"2026-01-07T12:30:45Z","sharp":0.999512,"blur":0.000351}
```

## Recompiling (if needed)

If you modify `focus_rtsp.c`:
```bash
ssh root@<DEVICE_IP> "cd /root/focus && gcc -o focus_rtsp focus_rtsp.c -I/usr/include/rknn -L/usr/lib -lrknnrt -lm -O2"
ssh root@<DEVICE_IP> "systemctl restart focus.service"
```

## Multi-Device Deployment

For deploying to multiple devices with different camera IPs:

1. Keep the same binary and model files
2. Only change `focus.service` for each device
3. Use a deployment script:
```bash
#!/bin/bash
DEVICES=("192.168.1.1" "192.168.1.2" "192.168.1.3")
CAMERAS=("192.168.1.20" "192.168.1.21" "192.168.1.22")

for i in "${!DEVICES[@]}"; do
    DEVICE_IP="${DEVICES[$i]}"
    CAMERA_IP="${CAMERAS[$i]}"
    
    # Update service file
    sed "s/192.168.1.21/$CAMERA_IP/g" focus.service > focus_temp.service
    
    # Deploy
    scp focus_temp.service root@$DEVICE_IP:/etc/systemd/system/focus.service
    ssh root@$DEVICE_IP "systemctl daemon-reload && systemctl restart focus.service"
done
```

## Uninstallation
```bash
ssh root@<DEVICE_IP> "systemctl stop focus.service"
ssh root@<DEVICE_IP> "systemctl disable focus.service"
ssh root@<DEVICE_IP> "rm /etc/systemd/system/focus.service"
ssh root@<DEVICE_IP> "rm -rf /root/focus"
ssh root@<DEVICE_IP> "rm /etc/logrotate.d/focus"
ssh root@<DEVICE_IP> "systemctl daemon-reload"
```
