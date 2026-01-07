# Field Technician Guide

Quick reference for camera lens adjustment and system troubleshooting.

## Daily Operation

### Check System Status
```bash
ssh root@<DEVICE_IP> "systemctl status focus.service"
```
✅ Look for: `Active: active (running)`

### View Live Results
```bash
ssh root@<DEVICE_IP> "tail -f /root/focus/result.txt"
```
Press CTRL+C to exit.

## Camera Adjustment

### Understanding Output
```json
{"timestamp":"2026-01-07T12:30:45Z","sharp":0.999512,"blur":0.000351}
```

- **sharp > 0.9** → Camera is IN FOCUS ✅
- **blur > 0.9** → Camera is OUT OF FOCUS ❌

### Lens Adjustment Procedure
1. Start monitoring: `ssh root@<IP> "tail -f /root/focus/result.txt"`
2. Adjust camera lens manually
3. Watch `sharp` and `blur` values change in real-time
4. Stop when `sharp > 0.95` consistently
5. Press CTRL+C to exit monitoring

## Changing Camera IP

### Step 1: Stop Service
```bash
ssh root@<DEVICE_IP> "systemctl stop focus.service"
```

### Step 2: Edit Service File
On your PC, edit `focus.service`:
```ini
Environment="RTSP_URL=rtsp://admin:admin123@<NEW_CAMERA_IP>:554/stream1"
```

### Step 3: Upload and Restart
```bash
scp focus.service root@<DEVICE_IP>:/etc/systemd/system/
ssh root@<DEVICE_IP> "systemctl daemon-reload"
ssh root@<DEVICE_IP> "systemctl start focus.service"
```

### Step 4: Verify
```bash
ssh root@<DEVICE_IP> "systemctl status focus.service"
ssh root@<DEVICE_IP> "tail -f /root/focus/result.txt"
```

## Troubleshooting

### No Output in result.txt
```bash
# 1. Check if service is running
ssh root@<DEVICE_IP> "systemctl status focus.service"

# 2. If stopped, start it
ssh root@<DEVICE_IP> "systemctl start focus.service"

# 3. Check logs for errors
ssh root@<DEVICE_IP> "journalctl -u focus.service -n 50"
```

### Service Keeps Restarting
```bash
# Check what's wrong
ssh root@<DEVICE_IP> "journalctl -u focus.service -n 50"

# Common issues:
# - Camera IP wrong → Update focus.service
# - Camera not reachable → Check network cable
# - Model file missing → Check /root/focus/model_640_fp16.rknn exists
```

### Camera Connection Failed
```bash
# Test camera connection
ssh root@<DEVICE_IP> "timeout 5 ffmpeg -rtsp_transport tcp -i rtsp://admin:admin123@<CAMERA_IP>:554/stream1 -f null -"

# If fails:
# 1. Verify camera IP: ping <CAMERA_IP>
# 2. Check credentials (admin/admin123)
# 3. Check network cable connection
# 4. Check camera is powered on
```

### Disk Full
```bash
# Check disk space
ssh root@<DEVICE_IP> "df -h /root"

# Manual cleanup (if logrotate not working)
ssh root@<DEVICE_IP> "rm /root/focus/result.txt.*.gz"
ssh root@<DEVICE_IP> "echo '' > /root/focus/result.txt"
ssh root@<DEVICE_IP> "systemctl restart focus.service"
```

## Service Commands Quick Reference

| Command | Description |
|---------|-------------|
| `systemctl start focus.service` | Start the service |
| `systemctl stop focus.service` | Stop the service |
| `systemctl restart focus.service` | Restart the service |
| `systemctl status focus.service` | Check service status |
| `systemctl enable focus.service` | Enable auto-start on boot |
| `systemctl disable focus.service` | Disable auto-start |
| `journalctl -u focus.service -f` | Watch live logs |
| `tail -f /root/focus/result.txt` | Watch live results |

## Emergency Reset

If everything fails:
```bash
ssh root@<DEVICE_IP> "systemctl stop focus.service"
ssh root@<DEVICE_IP> "rm /root/focus/result.txt"
ssh root@<DEVICE_IP> "systemctl start focus.service"
ssh root@<DEVICE_IP> "tail -f /root/focus/result.txt"
```

## Contact Support
If issues persist, collect this information:
```bash
ssh root@<DEVICE_IP> "systemctl status focus.service > /tmp/debug.log"
ssh root@<DEVICE_IP> "journalctl -u focus.service -n 100 >> /tmp/debug.log"
ssh root@<DEVICE_IP> "ls -lh /root/focus/ >> /tmp/debug.log"
scp root@<DEVICE_IP>:/tmp/debug.log ./
```
Send `debug.log` to support.
