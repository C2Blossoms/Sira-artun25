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
from fastapi.middleware.cors import CORSMiddleware
import pandas as pd
from fastapi import FastAPI, Request
from fastapi.templating import Jinja2Templates
from fastapi.staticfiles import StaticFiles
from datetime import datetime, timedelta
import uuid


tokens = {}



templates = Jinja2Templates(directory="templates")
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

app.add_middleware(
    CORSMiddleware,
    allow_origins=["*"],  # ใส่ "*" ชั่วคราว (ในโปรดักชันแนะนำระบุโดเมน)
    allow_credentials=True,
    allow_methods=["*"],
    allow_headers=["*"],
)

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

app.mount("/result", StaticFiles(directory="result"), name="result")

@app.get("/")
def root():
    return {"message": "API server is running ..."}

@app.get("/message")
def get_message():
    return JSONResponse(content={"message": "Hello from FastAPI!"})

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

@app.post("/pay/{card_id}/{amount}/{vendor_id}", response_model=BalanceResponse)
def pay(card_id: str, amount: float, vendor_id: int):
    conn = get_conn()
    try:
        with conn.cursor() as cur:
            # 1️ตรวจสอบบัตร
            cur.execute("SELECT balance FROM cards WHERE card_id = %s", (card_id,))
            result = cur.fetchone()
            if not result:
                raise HTTPException(status_code=404, detail="Card not found")

            balance = float(result[0])
            if balance < amount:
                raise HTTPException(status_code=400, detail="Not enough balance")

            # 2 ตรวจสอบแม่ค้า
            cur.execute("SELECT vendor_id FROM vendors WHERE vendor_id = %s", (vendor_id,))
            if not cur.fetchone():
                raise HTTPException(status_code=404, detail="Vendor not found")

            #  หักเงินจากบัตรลูกค้า
            cur.execute(
                "UPDATE cards SET balance = balance - %s WHERE card_id = %s RETURNING balance",
                (amount, card_id)
            )
            new_balance = float(cur.fetchone()[0])

            #  เพิ่มยอดเงินให้แม่ค้า
            cur.execute(
                "UPDATE vendors SET balance = balance + %s WHERE vendor_id = %s RETURNING balance",
                (amount, vendor_id)
            )
            vendor_balance = float(cur.fetchone()[0])

            # บันทึกประวัติธุรกรรม
            cur.execute(
                "INSERT INTO transaction_logs (card_id, vendor_id, amount, purpose) VALUES (%s, %s, %s, %s)",
                (card_id, vendor_id, amount, 'payment')
            )

            conn.commit()

            return {
                "card_id": card_id,
                "balance": new_balance
            }


    except Exception as e:
        conn.rollback()
        raise HTTPException(status_code=500, detail=str(e))
    finally:
        release_conn(conn)


# ✅ History
@app.get("/history/{card_id}")
def get_history_daily(card_id: str):
    conn = get_conn()
    try:
        df = pd.read_sql(
            "SELECT amount, purpose, txn_time FROM transaction_logs WHERE card_id = %s",
            conn,
            params=(card_id,)
        )
    finally:
        conn.close()

    if df.empty:
        return []

    df['txn_time'] = pd.to_datetime(df['txn_time'])
    df['day'] = df['txn_time'].dt.strftime("%Y-%m-%d")

    daily_data = []
    for day, group in df.groupby("day"):
        topup = group.loc[group.purpose=="topup","amount"].sum()
        payment = group.loc[group.purpose=="payment","amount"].sum()
        daily_data.append({"day": day, "total_topup": topup, "total_payment": payment})

    return daily_data

@app.get("/api/history/full")
def api_history_full():
    conn = get_conn()
    try:
        query = """
        SELECT 
            txn_id,
            card_id,
            amount,
            purpose,
            txn_time
        FROM transaction_logs
        ORDER BY txn_time DESC;
        """
        df = pd.read_sql(query, conn)

        # ✅ แปลง NaN, inf ให้เป็น None เพื่อให้ JSON ส่งออกได้
        df = df.replace({np.nan: None})
        df = df.replace([np.inf, -np.inf], None)

        return df.to_dict(orient="records")

    except Exception as e:
        raise HTTPException(status_code=500, detail=str(e))
    finally:
        release_conn(conn)






# -------------------------------------------------------
# ✅ สร้าง QR ธรรมดา (สำห รับลิงก์ไปยังหน้า HTML ของเรา)
# -------------------------------------------------------
# @app.get("/create-qr/{card_id}/{amount}")
# def create_qr(card_id: str, amount: float):
#     pay_url = f"http://127.0.0.1:8000/pay-page/{card_id}/{amount}"
#     qr_img = qrcode.make(pay_url)
#     buf = BytesIO()
#     qr_img.save(buf, format="PNG")
#     qr_b64 = base64.b64encode(buf.getvalue()).decode("utf-8")
#     return {"pay_url": pay_url, "qr_base64": qr_b64}
# -------------------------------------------------------
@app.get("/create-qr-matrix/{card_id}/{amount}")
def create_qr_matrix(card_id: str, amount: float):
    token = str(uuid.uuid4())
    expire_at = datetime.now() + timedelta(minutes=5)
    tokens[token] = {"used": False, "expire_at": expire_at}

    conn = get_conn()
    with conn.cursor() as cur:
            cur.execute("""
                INSERT INTO transaction_logs (card_id, vendor_id, amount, purpose, txn_time, token)
                VALUES (%s, %s, %s, %s, NOW(), %s)
            """, (card_id, None, amount, "qr_create", token))
            conn.commit()

    pay_url = f"http://147.185.221.31:54264/pay-page/{card_id}/{amount}?token={token}"

    # ---------- สร้าง QR Code ----------
    qr = qrcode.QRCode(
        version=4,  #(v3 = 29x29 modules, v4=33x33, v5=37x37)
        error_correction=qrcode.constants.ERROR_CORRECT_L,
        box_size=1,
        border=1
    )
    qr.add_data(pay_url)
    qr.make(fit=True)

    # ดึง matrix (True=ดำ, False=ขาว)
    matrix = qr.get_matrix()

    # แปลงเป็น 0/1 string
    matrix_strings = ["".join("1" if cell else "0" for cell in row) for row in matrix]

    # ---------- ส่งกลับ JSON ----------
    return JSONResponse(content={
        "pay_url": pay_url,
        "matrix": matrix_strings
    })


# ------------------------------
# ✅ หน้า HTML เมื่อสแกน QR
# ------------------------------
@app.get("/pay-page/{card_id}/{amount}", response_class=HTMLResponse)
def pay_page(card_id: str, amount: float, token: str = None):
    if not token or token not in tokens:
        return HTMLResponse(content="<h2>ลิงก์ไม่ถูกต้อง</h2>", status_code=403)
    
    data = tokens[token]
    if data["used"]:
        return HTMLResponse(content="<h2>ลิงก์นี้ถูกใช้ไปแล้ว</h2>", status_code=403)
    if datetime.now() > data["expire_at"]:
        return HTMLResponse(content="<h2>ลิงก์นี้หมดอายุแล้ว</h2>", status_code=403)
    
    data["used"] = True

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


@app.get("/api/summary")
def api_summary():
    conn = get_conn()
    try:
        query = """
        SELECT
            SUM(CASE WHEN purpose='topup' THEN amount ELSE 0 END) AS total_topup,
            SUM(CASE WHEN purpose='payment' THEN amount ELSE 0 END) AS total_payment,
            SUM(CASE WHEN purpose='topup' THEN amount ELSE 0 END) -
            SUM(CASE WHEN purpose='payment' THEN amount ELSE 0 END) AS balance_left
        FROM transaction_logs;
        """
        df = pd.read_sql(query, conn)
        return df.iloc[0].to_dict()
    finally:
        release_conn(conn)

@app.get("/api/customers")
def api_customers():
    conn = get_conn()
    try:
        query = """
        SELECT c.card_id, s.full_name, c.balance
        FROM cards c
        JOIN students s ON c.student_id = s.student_id
        ORDER BY c.card_id
        LIMIT 10;
        """
        df = pd.read_sql(query, conn)
        return df.to_dict(orient="records")
    finally:
        release_conn(conn)

@app.get("/api/vendors")
def api_vendors():
    conn = get_conn()
    try:
        query = """
        SELECT v.vendor_id, v.shop_name, COALESCE(SUM(t.amount),0) AS total_spent
        FROM vendors v
        LEFT JOIN transaction_logs t ON v.vendor_id = t.vendor_id AND t.purpose='payment'
        GROUP BY v.vendor_id, v.shop_name
        ORDER BY total_spent DESC
        LIMIT 10;
        """
        df = pd.read_sql(query, conn)
        return df.to_dict(orient="records")
    finally:
        release_conn(conn)

# ----------------- Dashboard HTML -----------------

@app.get("/dashboard", response_class=HTMLResponse)
def get_dashboard(request: Request):
    return templates.TemplateResponse("dashboard.html", {"request": request})