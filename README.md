# Sira-artun25
# FastAPI E-Wallet System (RFID / QR-Based)

ระบบ **E-Wallet (กระเป๋าเงินอิเล็กทรอนิกส์)** สำหรับนักเรียนและร้านค้า  
พัฒนาโดยใช้ **FastAPI + PostgreSQL + HTML (Jinja2 Template)**  
รองรับทั้งการ **ชำระเงิน (Pay)**, **เติมเงิน (Top-Up)** และ **แสดงแดชบอร์ดสรุปข้อมูลธุรกรรม**

---

## 1. ภาพรวมระบบ

ระบบนี้จำลอง **กระเป๋าเงินอิเล็กทรอนิกส์ของนักเรียน (Student Card)** ที่สามารถ  
- สแกนบัตร RFID เพื่อระบุ `card_id`  
- เติมเงินผ่าน **QR Code**  
- ใช้จ่ายเงินกับร้านค้า (`vendor`)  
- ดูประวัติธุรกรรมย้อนหลังและสรุปยอดทั้งหมดผ่านหน้า Dashboard  

ESP32 หรืออุปกรณ์ฝั่งผู้ใช้สามารถเรียกใช้ API เหล่านี้ผ่าน HTTP เพื่อรับผลลัพธ์ในรูปแบบ JSON หรือ HTML Page ได้ทันที

---

## 2. สถาปัตยกรรมและการเชื่่มต่อ

```
ESP32  <---->  FastAPI Backend  <---->  PostgreSQL Database
                    |
                    +--> Dashboard (Jinja2 + HTML)
                    |
                    +--> QR Topup Page
```

- **FastAPI**: ทำหน้าที่เป็น Web API Server  
- **PostgreSQL**: จัดเก็บข้อมูลบัตร, ร้านค้า และประวัติธุรกรรม  
- **Jinja2 Templates**: ใช้แสดงผลหน้าเว็บ (Top-up Page, Dashboard)  
- **ESP32 / Client Device**: ใช้สแกนบัตรและแสดง QR Code  

---

## 3. การทำงานของแต่ละ API Endpoint

| Method | Endpoint | รายละเอียด |
|--------|-----------|-------------|
| `GET` | `/` | ตรวจสอบสถานะเซิร์ฟเวอร์ |
| `GET` | `/balance/{card_id}` | ดึงยอดเงินคงเหลือของบัตร |
| `POST` | `/pay/{card_id}/{amount}/{vendor_id}` | หักเงินจากบัตรเพื่อจ่ายให้ร้านค้า |
| `GET` | `/create-qr-matrix/{card_id}/{amount}` | สร้าง QR Code สำหรับเติมเงิน พร้อม matrix 0/1 สำหรับแสดงบนจอ OLED |
| `GET` | `/topup-page/{card_id}/{amount}?token=` | หน้าเว็บสำหรับกด “ยืนยันการเติมเงิน” |
| `POST` | `/topup/{card_id}/{amount}` | เพิ่มยอดเงินให้บัตรเมื่อกดยืนยันในหน้าเว็บ |
| `GET` | `/dashboard` | หน้าแดชบอร์ดหลัก (HTML) |
| `GET` | `/history/{card_id}` | ดึงข้อมูลยอดเติม/จ่ายรายวัน |
| `GET` | `/api/history/full` | ดึงธุรกรรมทั้งหมดในระบบ |
| `GET` | `/api/summary` | สรุปยอดรวมทั้งหมด (เติม / จ่าย / คงเหลือ) |
| `GET` | `/api/customers` | รายชื่อลูกค้าพร้อมยอดเงินคงเหลือ |
| `GET` | `/api/vendors` | รายชื่อร้านค้าพร้อมยอดเงินที่ได้รับรวม |

---

## 4. หลักการสร้าง QR และ Token

ฟังก์ชัน `create_qr_matrix()`  
ใช้ในการสร้าง QR สำหรับการเติมเงิน โดยมีหลักการดังนี้:

1. ระบบสร้าง **Token (UUID)** สำหรับป้องกันการใช้ลิงก์ซ้ำ  
2. บันทึกข้อมูล QR ลงในตาราง `qr_creates`  
3. สร้าง URL เช่น  
   ```
   http://147.185.221.31:54264/topup-page/{card_id}/{amount}?token={token}
   ```
4. แปลง URL เป็น QR Matrix 0/1 เพื่อส่งให้ **ESP32 แสดงบนจอ OLED**
5. Token มีอายุ 5 นาที และถูกตั้งค่าสถานะ `used=True` ทันทีหลังใช้สำเร็จ  

---

## 5. การเชื่่มต่อกับ Dashboard

ระบบมีหน้าเว็บ `dashboard.html` แสดงข้อมูลรวมจากฐานข้อมูล เช่น

- ยอดรวมการเติมเงิน (`total_topup`)  
- ยอดรวมการจ่าย (`total_payment`)  
- ยอดคงเหลือทั้งหมด (`balance_left`)  
- ประวัติธุรกรรม (`/api/history/full`)  
- รายชื่อลูกค้าและร้านค้ายอดนิยม  

โดยใช้ **Pandas** เชื่อมต่อกับ PostgreSQL และจัดรูปแบบผลลัพธ์ในรูป JSON เพื่อให้ front-end นำไปแสดงผลกราฟได้สะดวก

---

## 6. โครงสร้างฐานข้ามาที่เกี่ยวข้อง

| Table | Columns | รายละเอียด |
|-------|----------|-------------|
| `cards` | `card_id`, `student_id`, `balance` | บัตรนักเรียนแต่ละใบ |
| `students` | `student_id`, `full_name` | รายชื่อนักเรียน |
| `vendors` | `vendor_id`, `shop_name`, `balance` | ร้านค้าภายในโรงเรียน |
| `transaction_logs` | `txn_id`, `card_id`, `vendor_id`, `amount`, `purpose`, `txn_time`, `token` | บันทึกประวัติการทำธุรกรรม |
| `qr_creates` | `card_id`, `amount`, `token`, `qr_time` | บันทึกข้อมูล QR ที่สร้าง |

---

## 7. วิธีการรันระบบ

```bash
# 1. สร้าง Virtual Environment
python -m venv venv
source venv/Scripts/activate   # (Windows)
# หรือ
source venv/bin/activate       # (Linux / macOS)

# 2. ติดตั้ง dependencies
pip install fastapi uvicorn psycopg2-binary qrcode pillow pandas numpy

# 3. ตั้งค่า Database (PostgreSQL)
# แก้ไขค่าภายใน .env หรือแก้โดยตรงใน main.py เช่น
PGHOST=localhost
PGUSER=postgres
PGPASSWORD=12345678
PGDATABASE=school_cashless
PGPORT=5432

# 4. รันเซิร์ฟเวอร์
uvicorn main:app --host 0.0.0.0 --port 8000 --reload
```

เปิดในเบราว์เซอร์ที่  
 `http://127.0.0.1:8000/dashboard`  
หรือเรียกใช้ API ผ่าน ESP32 หรือ Postman ได้ทันที

---

## หลักการทำงานโดยสรุป

1. **ผู้ใช้ (นักเรียน)** สแกนบัตร RFID เพื่อรับ `card_id`  
2. **ESP32** ส่งคำขอ `/create-qr-matrix` เพื่อสร้าง QR เติมเงิน  
3. **ผู้ใช้** สแกน QR ผ่านโทรศัพท์ → เปิดหน้า `/topup-page`  
4. เมื่อกดยืนยัน เติมเงินจะบันทึกใน `transaction_logs` และอัปเดตยอดใน `cards`  
5. เมื่อชำระเงิน `/pay` ระบบจะหักจากนักเรียนและเพิ่มยอดให้ร้านค้า  
6. **แดชบอร์ด** จะรวมข้อมูลจากทุกธุรกรรมเพื่อนำเสนอผลภาพรวม  

---

