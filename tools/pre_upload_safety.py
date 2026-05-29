Import("env")

import time


READY_TOKEN = "READY_FOR_UPLOAD"
COMMAND = b"prepare_upload\n"


def _configured_port():
    for value in (env.subst("$UPLOAD_PORT"), env.GetProjectOption("upload_port", "")):
        if value and "$" not in value and value.lower() != "none":
            return value
    return None


def _autodetect_single_port():
    try:
        from serial.tools import list_ports
    except Exception:
        return None

    ports = list(list_ports.comports())
    preferred = []
    for port in ports:
        text = f"{port.device} {port.description} {port.hwid}".lower()
        if any(key in text for key in ("esp", "jtag", "usb serial", "cp210", "ch340", "xiao")):
            preferred.append(port.device)

    if len(preferred) == 1:
        return preferred[0]
    if len(ports) == 1:
        return ports[0].device
    return None


def _send_prepare_command(port):
    try:
        import serial

        ser = serial.Serial()
        ser.port = port
        ser.baudrate = int(env.GetProjectOption("monitor_speed", 115200))
        ser.timeout = 0.1
        ser.write_timeout = 0.5
        ser.dtr = False
        ser.rts = False
        ser.open()
    except Exception as exc:
        print(f"[UPLOAD-SAFETY] Could not open {port}: {exc}")
        return False

    try:
        time.sleep(0.2)
        ser.reset_input_buffer()
        ser.write(COMMAND)
        ser.flush()

        deadline = time.time() + 3.0
        seen = ""
        while time.time() < deadline:
            data = ser.read(ser.in_waiting or 1)
            if not data:
                continue
            seen += data.decode("utf-8", errors="replace")
            if READY_TOKEN in seen:
                print(f"[UPLOAD-SAFETY] Motor pins released on {port}")
                return True

        print(f"[UPLOAD-SAFETY] No {READY_TOKEN} response on {port}; continuing upload")
        return False
    finally:
        ser.close()


def prepare_upload_safely(source, target, env):
    port = _configured_port() or _autodetect_single_port()
    if not port:
        print("[UPLOAD-SAFETY] No single upload port found; continuing upload")
        return

    _send_prepare_command(port)


env.AddPreAction("upload", prepare_upload_safely)
