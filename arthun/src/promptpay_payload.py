import re, time, serial
from typing import Optional
from serial.tools import list_ports

# ---------- EMV TLV helpers ----------
def tlv(tag: str, value: str) -> str:
    return f"{tag}{len(value):02d}{value}"

def crc16_ccitt(data: bytes, poly=0x1021, init=0xFFFF) -> int:
    crc = init
    for b in data:
        crc ^= (b << 8)
        for _ in range(8):
            crc = ((crc << 1) ^ poly) & 0xFFFF if (crc & 0x8000) else ((crc << 1) & 0xFFFF)
    return crc

def format_promptpay_mobile(phone: str) -> str:
    d = re.sub(r"\D", "", phone)
    if d.startswith("0"):  # 08x...
        return "0066" + d[1:]
    if d.startswith("66"):
        return "00" + d
    if len(d) == 9:       # ลืม 0 ข้างหน้า
        return "0066" + d
    raise ValueError("รูปแบบเบอร์ไม่ถูกต้อง")

def build_promptpay_payload(phone: str, amount: Optional[float] = None, static: bool = True) -> str:
    p = ""
    p += tlv("00", "01")
    p += tlv("01", "11" if static else "12")

    aid = "A000000677010111"
    mobile = format_promptpay_mobile(phone)
    mai = tlv("00", aid) + tlv("01", mobile)
    p += tlv("29", mai)

    p += tlv("53", "764")
    p += tlv("58", "TH")
    if amount is not None:
        p += tlv("54", f"{amount:.2f}")

    to_crc = (p + "6304").encode("ascii")
    crc = crc16_ccitt(to_crc)
    p += f"63{4:02d}{crc:04X}"
    return p

# ---------- Serial helpers ----------
def auto_pick_port(preferred: Optional[str] = None) -> str:
    if preferred:
        return preferred
    ports = list(list_ports.comports())
    for p in ports:
        desc = (p.description or "").lower()
        hwid = (p.hwid or "").lower()
        if "ch340" in desc or "1a86" in hwid:
            return p.device
    if ports:
        return ports[0].device
    raise RuntimeError("ไม่พบพอร์ต Serial ของ ESP32")

def wait_for_ready(ser: serial.Serial, timeout=6.0) -> bool:
    """พยายามหา 'Ready: Waiting for Promptpay payload' ในบัฟเฟอร์"""
    end = time.time() + timeout
    buf = b""
    # เก็บที่มีค้างอยู่ก่อน
    if ser.in_waiting:
        buf += ser.read(ser.in_waiting)
        if b"Ready: Waiting for Promptpay payload" in buf:
            return True
    while time.time() < end:
        chunk = ser.read(ser.in_waiting or 1)
        if chunk:
            buf += chunk
            if b"Ready: Waiting for Promptpay payload" in buf:
                return True
        else:
            time.sleep(0.05)
    return False

def open_serial_with_retry(dev: str, baud=115200, timeout=0.2, retries=8, delay=0.8) -> serial.Serial:
    last_err = None
    for _ in range(retries):
        try:
            return serial.Serial(dev, baud, timeout=timeout)
        except Exception as e:
            last_err = e
            time.sleep(delay)
    raise last_err

def send_payload(port: Optional[str], phone: str, amount: Optional[float] = None):
    dev = auto_pick_port(port)
    payload = build_promptpay_payload(phone, amount, static=True)
    line = "PAYLOAD:" + payload + "\r\n"

    print("Using port:", dev)
    print("EMV payload:", payload)

    ser = open_serial_with_retry(dev)
    try:
        ser.dtr = False
        ser.rts = False

        # ล้างบัฟเฟอร์และเผื่อเวลารีเซ็ต
        ser.reset_input_buffer()
        time.sleep(2.8)

        ready = wait_for_ready(ser, timeout=6.0)
        if not ready:
            # fallback: ทดสอบ PING
            ser.write(b"PING\r\n")
            ser.flush()
            time.sleep(0.2)
            if b"PONG" in ser.read(256):
                ready = True
        print("ESP32 ready?", ready)

        ser.write(line.encode("ascii"))
        ser.flush()

        # อ่าน log กลับ เพื่อยืนยัน
        t0 = time.time()
        buf = b""
        while time.time() - t0 < 2.0:
            buf += ser.read(ser.in_waiting or 1)
            time.sleep(0.05)
        if buf:
            print(buf.decode("utf-8", errors="ignore"))
    finally:
        ser.close()

if __name__ == "__main__":
    send_payload(
        port="/dev/serial/by-id/usb-1a86_USB_Serial-if00-port0",  # เปลี่ยนเป็น /dev/ttyUSB0 ได้
        phone="0953176495",
        amount=None
    )
