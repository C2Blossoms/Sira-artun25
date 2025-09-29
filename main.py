from fastapi import FastAPI, HTTPException
from fastapi.responses import JSONResponse, HTMLResponse
from pydantic import BaseModel
import os
import psycopg2
from psycopg2 import pool as pgpool
import qrcode
import base64
from io import BytesIO
import numpy as np
from PIL import Image

# ------------------------------
# Database Connection Pool
# ------------------------------

db_pool = pgpool.SimpleConnectionPool(
    1, 10,
    dbname=os.getenv("PGDATABASE", "postgres"),
    user=os.getenv("PGUSER", "postgres"),
    password=os.getenv("PGPASSWORD", "12345678"),
    host=os.getenv("PGHOST", "localhost"),
    port=os.getenv("PGPORT", "5432")
)

app = FastAPI()

# ------------------------------
# Models (ยังใช้สำหรับตรวจสอบ/กำหนด schema response)
# ------------------------------
class BalanceResponse(BaseModel):
    card_id: str
    balance: float

class HistoryItem(BaseModel):
    amount: float
    purpose: str
    time: str

class HistoryResponse(BaseModel):
    card_id: str
    history: list[HistoryItem]

# ------------------------------
# Utils
# ------------------------------
def get_conn():
    try:
        return db_pool.getconn()
    except Exception:
        raise HTTPException(status_code=500, detail="Database connection failed")

def release_conn(conn):
    if conn:
        db_pool.putconn(conn)

# ------------------------------
# Basic Routes
# ------------------------------
@app.get("/")
def root():
    return {"message": "API server is running 🚀"}

@app.get("/message")
def get_message():
    return JSONResponse(content={"message": "Hello from FastAPI!"})

# ✅ Check balance
@app.get("/balance/{card_id}", response_model=BalanceResponse)
def get_balance(card_id: str):
    conn = get_conn()
    try:
        with conn.cursor() as cur:
            cur.execute("SELECT balance FROM cards WHERE card_id = %s", (card_id,))
            result = cur.fetchone()
            if not result:
                raise HTTPException(status_code=404, detail="Card not found")
            return {"card_id": card_id, "balance": float(result[0])}
    finally:
        release_conn(conn)

# ✅ Top up (manual, หน่วยบาท)
@app.post("/topup/{card_id}/{amount}", response_model=BalanceResponse)
def topup(card_id: str, amount: float):
    conn = get_conn()
    try:
        with conn.cursor() as cur:
            cur.execute(
                "UPDATE cards SET balance = balance + %s WHERE card_id = %s RETURNING balance",
                (amount, card_id)
            )
            new_balance = cur.fetchone()
            if not new_balance:
                conn.rollback()
                raise HTTPException(status_code=404, detail="Card not found")

            cur.execute(
                "INSERT INTO transaction_logs (card_id, vendor_id, amount, purpose) VALUES (%s, %s, %s, %s)",
                (card_id, None, amount, "topup")
            )
            conn.commit()
            return {"card_id": card_id, "balance": float(new_balance[0])}
    except Exception as e:
        conn.rollback()
        raise HTTPException(status_code=500, detail=str(e))
    finally:
        release_conn(conn)

# ✅ Pay (หักเงิน, หน่วยบาท)
@app.post("/pay/{card_id}/{amount}/{vendor_id}", response_model=BalanceResponse)
def pay(card_id: str, amount: float, vendor_id: int):
    conn = get_conn()
    try:
        with conn.cursor() as cur:
            cur.execute("SELECT balance FROM cards WHERE card_id = %s", (card_id,))
            result = cur.fetchone()
            if not result:
                raise HTTPException(status_code=404, detail="Card not found")

            balance = float(result[0])
            if balance < amount:
                raise HTTPException(status_code=400, detail="Not enough balance")

            cur.execute(
                "UPDATE cards SET balance = balance - %s WHERE card_id = %s RETURNING balance",
                (amount, card_id)
            )
            new_balance = cur.fetchone()

            cur.execute(
                "INSERT INTO transaction_logs (card_id, vendor_id, amount, purpose) VALUES (%s, %s, %s, %s)",
                (card_id, vendor_id, amount, "payment")
            )
            conn.commit()
            return {"card_id": card_id, "balance": float(new_balance[0])}
    except Exception as e:
        conn.rollback()
        raise HTTPException(status_code=500, detail=str(e))
    finally:
        release_conn(conn)

# ✅ History
@app.get("/history/{card_id}", response_model=HistoryResponse)
def get_history(card_id: str):
    conn = get_conn()
    try:
        with conn.cursor() as cur:
            cur.execute(
                """
                SELECT amount, purpose, txn_time 
                FROM transaction_logs 
                WHERE card_id = %s 
                ORDER BY txn_time DESC
                """,
                (card_id,)
            )
            rows = cur.fetchall()
            history = [
                {"amount": float(r[0]), "purpose": r[1], "time": str(r[2])}
                for r in rows
            ]
            return {"card_id": card_id, "history": history}
    finally:
        release_conn(conn)

# -------------------------------------------------------
# ✅ สร้าง QR ธรรมดา (สำหรับลิงก์ไปยังหน้า HTML ของเรา)
# -------------------------------------------------------
# @app.get("/create-qr/{card_id}/{amount}")
# def create_qr(card_id: str, amount: float):
#     pay_url = f"http://127.0.0.1:8000/pay-page/{card_id}/{amount}"
#     qr_img = qrcode.make(pay_url)
#     buf = BytesIO()
#     qr_img.save(buf, format="PNG")
#     qr_b64 = base64.b64encode(buf.getvalue()).decode("utf-8")
#     return {"pay_url": pay_url, "qr_base64": qr_b64}
@app.get("/create-qr-matrix/{card_id}/{amount}")
def create_qr_matrix(card_id: str, amount: float):
    # สร้างลิงก์สำหรับชำระเงินตาม card_id และ amount
    pay_url = f"http://172.20.10.6:8000/pay-page/{card_id}/{amount}"

    # สร้าง QR Code เป็นภาพขาว-ดำ
    qr = qrcode.make(pay_url).convert("L")       # convert เป็นโมโนโครม
    qr = qr.resize((64, 64), Image.NEAREST)      # ย่อขนาดเป็น 64x64

    # แปลงภาพเป็นเมทริกซ์ 0/1 (1=ดำ, 0=ขาว)
    matrix = np.array(qr)
    matrix_01 = (matrix < 128).astype(int).tolist()
    matrix_strings = ["".join(str(cell) for cell in row) for row in matrix_01]

    # คืนค่าเป็น JSON
    return JSONResponse(content={
        "pay_url": pay_url,
        "matrix": matrix_strings  # เป็น list ยาว 64 บรรทัด
    })

# ------------------------------
# ✅ หน้า HTML เมื่อสแกน QR
# ------------------------------
@app.get("/pay-page/{card_id}/{amount}", response_class=HTMLResponse)
def pay_page(card_id: str, amount: float):
    html = f"""
    <html>
        <head>
            <title>เติมเงิน</title>
        </head>
        <body style="font-family: Arial; text-align: center; margin-top: 50px;">
            <h2>เติมเงิน {amount:.2f} บาท</h2>
            <p>สำหรับบัตร: <b>{card_id}</b></p>
            <button onclick="doTopup()" 
                style="padding: 10px 20px; font-size: 16px; cursor: pointer;">
                ยืนยันเติมเงิน
            </button>

            <p id="result" style="margin-top:20px; color:green;"></p>

            <script>
                async function doTopup() {{
                    let res = await fetch('/topup/{card_id}/{amount}', {{
                        method: 'POST'
                    }});
                    let data = await res.json();
                    document.getElementById('result').innerText = 
                        'เติมเงินสำเร็จ! ยอดคงเหลือใหม่: ' + data.balance + ' บาท';
                }}
            </script>
        </body>
    </html>
    """
    return html
