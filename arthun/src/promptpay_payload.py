# PromptPay Serial Communication
# Create Payload

import re
import serial
from time import sleep

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
    if d.startswith("0"):        # 08x...
        return "0066" + d[1:]
    if d.startswith("66"):       # 66xxxxxxxxx
        return "00" + d
    # เดาเป็นเบอร์ไทยที่ลืมใส่ 0
    if len(d) == 9:
        return "0066" + d
    raise ValueError("รูปแบบเบอร์ไม่ถูกต้อง")

def build_promptpay_payload(phone: str, amount: float | None = None, static=True) -> str:
    p = ""
    p += tlv("00", "01")                    # Payload Format Indicator (fixed "01")
    p += tlv("01", "11" if static else "12")  # Static/Dynamic per EMV

    # Merchant Account Info (Tag 29) -> PromptPay Credit Transfer
    aid = "A000000677010111"               # AID for PromptPay (MPM)
    mobile = format_promptpay_mobile(phone)
    mai = tlv("00", aid) + tlv("01", mobile)
    p += tlv("29", mai)

    p += tlv("53", "764")                  # Currency THB
    p += tlv("58", "TH")                   # Country TH

    if amount is not None:
        p += tlv("54", f"{amount:.2f}")    # Optional

    # CRC16 (Tag 63): compute over everything + "6304"
    to_crc = (p + "6304").encode("ascii")
    crc = crc16_ccitt(to_crc)
    p += f"63{4:02d}{crc:04X}"
    return p

# ---------- Serial send ----------
def send_payload(port: str, phone: str, amount: float | None = None):
    payload = build_promptpay_payload(phone, amount, static=True)
    print("EMV payload:", payload)
    ser = serial.Serial(port, 115200, timeout=1)
    sleep(0.3)
    ser.write(("PAYLOAD:" + payload + "\n").encode("ascii"))
    ser.close()

if __name__ == "__main__":
    send_payload(port="COM6", phone="0953176495", amount=None)
