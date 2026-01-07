# Real-time Focus Detection System for Rockchip RK3566

AI-powered blur/sharp detection system running on Rockchip RK3566 NPU using RKNN Runtime.

## Features
- ⚡ Real-time inference at 5 FPS on NPU
- 📹 RTSP camera support (H.264)
- 🎯 MobileNetV2-based binary classifier (640x640)
- 📊 JSON output format
- 🔄 Automatic log rotation
- 🚀 Systemd service integration
- 🌐 Multi-camera support via environment variables

## Hardware Requirements
- **Board**: Rockchip RK3566 (or similar with RKNN support)
- **Camera**: IP camera with RTSP stream (H.264, 1280x720 recommended)
- **Storage**: 500MB minimum
- **Network**: Ethernet connection to camera

## Software Requirements
- RKNN Runtime (librknnrt.so)
- FFmpeg with RTSP support
- Logrotate (optional, for log management)

## Quick Start

### 1. Install
```bash
# Copy files to device
scp -r focus_release/ root@<DEVICE_IP>:/root/focus/

# Install systemd service
scp focus_release/focus.service root@<DEVICE_IP>:/etc/systemd/system/
ssh root@<DEVICE_IP> "systemctl daemon-reload"
```

### 2. Configure Camera
Edit `focus.service` and change the RTSP URL:
```ini
Environment="RTSP_URL=rtsp://admin:admin123@<CAMERA_IP>:554/stream1"
```

### 3. Start Service
```bash
ssh root@<DEVICE_IP> "systemctl enable focus.service"
ssh root@<DEVICE_IP> "systemctl start focus.service"
```

### 4. Monitor Results
```bash
ssh root@<DEVICE_IP> "tail -f /root/focus/result.txt"
```

## Output Format
Each line in `/root/focus/result.txt` is a JSON object:
```json
{"timestamp":"2026-01-07T12:30:45Z","sharp":0.999512,"blur":0.000351}
```

- `sharp > 0.9`: Image is in focus
- `blur > 0.9`: Image is blurry

## Model Details
- **Architecture**: MobileNetV2
- **Input Size**: 640×640×3 (RGB)
- **Quantization**: FP16
- **Classes**: 2 (blur, sharp)
- **Framework**: TensorFlow → ONNX → RKNN

## Log Management
Automatic log rotation configured:
- Daily rotation or when file exceeds 100MB
- Keeps last 7 days
- Compressed archives (.gz)

## Service Management
```bash
# Start
systemctl start focus.service

# Stop
systemctl stop focus.service

# Restart
systemctl restart focus.service

# Status
systemctl status focus.service

# View logs
journalctl -u focus.service -f
```

## Troubleshooting

### Service won't start
```bash
# Check logs
journalctl -u focus.service -n 50

# Check RKNN runtime
ls -l /usr/lib/librknnrt.so

# Check FFmpeg
ffmpeg -version
```

### No camera connection
- Verify RTSP URL is correct
- Check network connectivity: `ping <CAMERA_IP>`
- Test RTSP stream: `ffmpeg -i rtsp://... -frames:v 1 test.jpg`

### No output in result.txt
- Verify service is running: `systemctl status focus.service`
- Check file permissions: `ls -l /root/focus/result.txt`
- Restart service: `systemctl restart focus.service`

## File Structure
```
focus_release/
├── model_640_fp16.rknn      # RKNN model file
├── focus_rtsp.c              # Source code
├── focus_rtsp                # Compiled binary
├── focus.service             # Systemd service
├── focus-logrotate           # Log rotation config
├── README.md                 # This file
├── INSTALLATION.md           # Detailed setup guide
└── TECHNICIAN_GUIDE.md       # Field technician manual
```

## Performance
- Inference time: ~200ms per frame
- Frame rate: 5 FPS
- Memory usage: ~150MB
- CPU usage: <5% (NPU handles inference)

## License
MIT License - see LICENSE file

## Contributing
Pull requests welcome! Please ensure all tests pass before submitting.

## Credits
- Model trained on Roboflow blur/sharp dataset
- RKNN Runtime by Rockchip
- Deployment optimized for embedded systems
