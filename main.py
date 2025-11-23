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
from fastapi import Query
from fastapi import Request

tokens = {}
templates = Jinja2Templates(directory="templates")

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
    allow_origins=["*"],
    allow_credentials=True,
    allow_methods=["*"],
    allow_headers=["*"],
)

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

def get_conn():
    try:
        return db_pool.getconn()
    except Exception:
        raise HTTPException(status_code=500, detail="Database connection failed")
def release_conn(conn):
    if conn:
        db_pool.putconn(conn)
#----------------------------------------------API Routes -----------------------------------------
app.mount("/result", StaticFiles(directory="result"), name="result")

@app.get("/")
def root():
    return {"message": "API server is running ..."}

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
            cur.execute("SELECT vendor_id FROM vendors WHERE vendor_id = %s", (vendor_id,))

            if not cur.fetchone():
                raise HTTPException(status_code=404, detail="Vendor not found")
            cur.execute(
                "UPDATE cards SET balance = balance - %s WHERE card_id = %s RETURNING balance",
                (amount, card_id)
            )

            new_balance = float(cur.fetchone()[0])
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

@app.get("/create-qr-matrix/{card_id}/{amount}")
def create_qr_matrix(card_id: str, amount: float):
    token = str(uuid.uuid4())
    expire_at = datetime.now() + timedelta(minutes=5)
    tokens[token] = {"used": False, "expire_at": expire_at}

    qr_id = str(())
    conn = get_conn()
    with conn.cursor() as cur:
        cur.execute("""
            INSERT INTO qr_creates (card_id, amount, token, qr_time)
            VALUES (%s, %s, %s, NOW())
        """, (card_id, amount, token))
        conn.commit()

    topup_url = f"http://147.185.221.31:54264/topup-page/{card_id}/{amount}?token={token}"

    qr = qrcode.QRCode(
        version=4,
        error_correction=qrcode.constants.ERROR_CORRECT_L,
        box_size=1,
        border=1
    )
    qr.add_data(topup_url)
    qr.make(fit=True)

    matrix = qr.get_matrix()
    matrix_strings = ["".join("1" if cell else "0" for cell in row) for row in matrix]

    return JSONResponse(content={
        "topup_url": topup_url,
        "matrix": matrix_strings
    })

@app.get("/topup-page/{card_id}/{amount}", response_class=HTMLResponse)
def topup_page(
    request: Request,                     
    card_id: str,                          
    amount: float,                         
    token: str = Query(..., description="One-time token for access")
):
    if not token or token not in tokens:
        return templates.TemplateResponse("error.html", {
            "request": request,
            "message": "ลิงก์ไม่ถูกต้อง กรุณาตรวจสอบรายการอีกครั้ง"
        }, status_code=403)

    data = tokens[token]
    if data["used"]:
        return templates.TemplateResponse("error.html", {
            "request": request,
            "message": "ลิงก์นี้ถูกใช้ไปแล้ว กรุณาติดต่อเจ้าหน้าที่"
        }, status_code=403)

    if datetime.now() > data["expire_at"]:
        return templates.TemplateResponse("error.html", {
            "request": request,
            "message": "ลิงก์นี้หมดอายุแล้ว กรุณาทำรายการใหม่อีกครั้ง"
        }, status_code=403)

    return templates.TemplateResponse("topup.html", {
        "request": request,
        "card_id": card_id,
        "amount": amount,
        "token": token
    })

@app.post("/topup/{card_id}/{amount}", response_model=BalanceResponse)
def topup(request: Request, card_id: str, amount: float, token: str):
    if not token or token not in tokens:
        return templates.TemplateResponse("error.html", {
            "request": request,
            "message": "ลิงก์ไม่ถูกต้อง กรุณาตรวจสอบรายการอีกครั้ง"
        }, status_code=403)

    data = tokens[token]
    if data["used"]:
        return templates.TemplateResponse("error.html", {
            "request": request,
            "message": "ลิงก์นี้ถูกใช้ไปแล้ว กรุณาติดต่อเจ้าหน้าที่"
        }, status_code=403)

    if datetime.now() > data["expire_at"]:
        return templates.TemplateResponse("error.html", {
            "request": request,
            "message": "ลิงก์นี้หมดอายุแล้ว กรุณาทำรายการใหม่อีกครั้ง"
        }, status_code=403)

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
                "INSERT INTO transaction_logs (card_id, vendor_id, amount, purpose, token) VALUES (%s, %s, %s, %s, %s)",
                (card_id, None, amount, "topup", token)
            )
            conn.commit()

            data["used"] = True

            return templates.TemplateResponse("thx.html", {
                "request": request,
                "card_id": card_id,
                "balance": float(new_balance[0]),
                "message": "ขอบคุณที่ใช้บริการ"
            })
    except Exception as e:
        conn.rollback()
        raise HTTPException(status_code=500, detail=str(e))
    finally:
        release_conn(conn)

# --------------------------------- Dashboard HTML -------------------------------------

@app.get("/dashboard", response_class=HTMLResponse)
def get_dashboard(request: Request):
    return templates.TemplateResponse("dashboard.html", {"request": request})

#--------------------------------------- ใช้กับ dashborad ----------------------------------

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

        df = df.replace({np.nan: None})
        df = df.replace([np.inf, -np.inf], None)

        return df.to_dict(orient="records")

    except Exception as e:
        raise HTTPException(status_code=500, detail=str(e))
    finally:
        release_conn(conn)

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