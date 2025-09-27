from fastapi import FastAPI, HTTPException, Request
from pydantic import BaseModel
from fastapi.responses import JSONResponse
import os
import psycopg2
from psycopg2 import pool as pgpool
import stripe
import qrcode  # (ยังคงอยู่ได้ เผื่อใช้ภายหลัง แต่ไม่จำเป็นสำหรับ PromptPay)
import base64
from io import BytesIO
from PIL import Image
import numpy as np
import requests  # สำหรับโหลด PNG ของ QR จาก Stripe

# ------------------------------
# Config (ใช้ ENV จริงใน production)
# ------------------------------
STRIPE_SECRET_KEY = "sk_test_51S9lC5BHtEk7Kds9OlNl9wWo6Y0Hz3xpLSlVgaZezxZgdlpU2UeZgN49occ8Hde9pK5SZpNwUAPUXEmFz90HRMI200tSJWVTHy"
STRIPE_WEBHOOK_SECRET = "whsec_8ef26a5a06dc32b4468d2c00d9cf265e1a36401182350ccb7f480784ce54229b"

stripe.api_key = STRIPE_SECRET_KEY

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
# Models
# ------------------------------
class BalanceResponse(BaseModel):
    card_id: str
    balance: float   # หน่วยบาท

class HistoryItem(BaseModel):
    amount: float    # หน่วยบาท
    purpose: str
    time: str

class HistoryResponse(BaseModel):
    card_id: str
    history: list[HistoryItem]

class PaymentRequest(BaseModel):
    amount: int      # หน่วย "สตางค์" เช่น 5000 = 50.00 THB
    card_id: str

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
# ✅ Create PromptPay QR via Stripe (THB + promptpay)
# -------------------------------------------------------
@app.post("/create-promptpay-qr")
def create_promptpay_qr(req: PaymentRequest):
    """
    สร้าง PaymentIntent แบบ promptpay แล้วดึง QR พร้อมเพย์จาก next_action
    - amount: สตางค์ (int) เช่น 5000 = 50.00 THB
    - ส่งคืน: qr_png_base64, hosted_instructions_url, payment_intent_id
    """
    try:
        # สร้าง PaymentIntent แบบ PromptPay และยืนยันทันทีเพื่อให้ได้ next_action (QR)
        pi = stripe.PaymentIntent.create(
            amount=req.amount,                # หน่วยย่อยของ THB (สตางค์)
            currency="thb",
            payment_method_types=["promptpay"],
            confirm=True,                     # ให้ Stripe สร้าง QR ใน next_action
            metadata={"card_id": req.card_id} # ไว้อ่านใน webhook
        )

        # ตรวจว่ามี QR พร้อมเพย์ใน next_action
        na = pi.get("next_action") or {}
        pp = na.get("promptpay_display_qr_code") or {}
        image_url_png = pp.get("image_url_png")
        hosted_url = pp.get("hosted_instructions_url")

        if not image_url_png:
            # ในบางกรณีอาจต้อง refresh client (rare) — ส่ง hosted_url ให้ frontend เปิดได้
            raise HTTPException(status_code=400, detail="PromptPay QR is not available yet. Try hosted_instructions_url instead.")

        # โหลด PNG แล้วแปลงเป็น base64 (สะดวกส่งต่อให้จอ/ไคลเอนต์)
        png_bytes = requests.get(image_url_png, timeout=10).content
        qr_b64 = base64.b64encode(png_bytes).decode("utf-8")

        return JSONResponse(content={
            "payment_intent_id": pi["id"],
            "qr_png_base64": qr_b64,
            "hosted_instructions_url": hosted_url
        })

    except stripe.error.StripeError as e:
        # ข้อผิดพลาดจาก Stripe
        raise HTTPException(status_code=400, detail=f"Stripe error: {str(e)}")
    except Exception as e:
        raise HTTPException(status_code=500, detail=str(e))

# -------------------------------------------------------
# ✅ (ทางเลือก) PromptPay QR 64x64 matrix สำหรับจอ ESP32
# -------------------------------------------------------
@app.post("/create-promptpay-qr-matrix")
def create_promptpay_qr_matrix(req: PaymentRequest):
    """
    เหมือน /create-promptpay-qr แต่แปลง QR PNG เป็นบิตแมทริกซ์ 0/1 ขนาด 64x64
    """
    try:
        pi = stripe.PaymentIntent.create(
            amount=req.amount,                # สตางค์
            currency="thb",
            payment_method_types=["promptpay"],
            confirm=True,
            metadata={"card_id": req.card_id}
        )

        na = pi.get("next_action") or {}
        pp = na.get("promptpay_display_qr_code") or {}
        image_url_png = pp.get("image_url_png")
        hosted_url = pp.get("hosted_instructions_url")

        if not image_url_png:
            raise HTTPException(status_code=400, detail="PromptPay QR is not available yet.")

        png_bytes = requests.get(image_url_png, timeout=10).content
        img = Image.open(BytesIO(png_bytes)).convert("L")
        # Resize เป็น 64x64 (บิตภาพ)
        img = img.resize((64, 64), Image.NEAREST)
        matrix = np.array(img)
        matrix_01 = (matrix < 128).astype(int).tolist()
        matrix_strings = ["".join(str(cell) for cell in row) for row in matrix_01]

        return JSONResponse(content={
            "payment_intent_id": pi["id"],
            "matrix": matrix_strings,
            "hosted_instructions_url": hosted_url
        })

    except stripe.error.StripeError as e:
        raise HTTPException(status_code=400, detail=f"Stripe error: {str(e)}")
    except Exception as e:
        raise HTTPException(status_code=500, detail=str(e))

# -------------------------------------------------------
# ✅ Stripe Webhook: เติมเงินเมื่อจ่ายสำเร็จ (PromptPay)
# -------------------------------------------------------
@app.post("/webhook")
async def stripe_webhook(request: Request):
    payload_bytes = await request.body()
    sig = request.headers.get("stripe-signature") or request.headers.get("Stripe-Signature")
    payload = payload_bytes.decode("utf-8")

    try:
        event = stripe.Webhook.construct_event(payload, sig, STRIPE_WEBHOOK_SECRET)
    except Exception as e:
        raise HTTPException(status_code=400, detail=f"Webhook error: {str(e)}")

    # เมื่อโอนผ่าน PromptPay แล้วสำเร็จ Stripe จะส่ง event นี้
    if event["type"] == "payment_intent.succeeded":
        pi = event["data"]["object"]
        amount_major = float(pi["amount"]) / 100.0   # สตางค์ → บาท
        card_id = (pi.get("metadata") or {}).get("card_id")

        if not card_id:
            raise HTTPException(status_code=400, detail="No card_id in metadata")

        conn = get_conn()
        try:
            with conn.cursor() as cur:
                cur.execute(
                    "UPDATE cards SET balance = balance + %s WHERE card_id = %s RETURNING balance",
                    (amount_major, card_id)
                )
                new_balance = cur.fetchone()
                if not new_balance:
                    conn.rollback()
                    raise HTTPException(status_code=404, detail="Card not found")

                cur.execute(
                    "INSERT INTO transaction_logs (card_id, vendor_id, amount, purpose) VALUES (%s, %s, %s, %s)",
                    (card_id, None, amount_major, "topup")
                )
                conn.commit()
                print(f"✅ PromptPay topup {amount_major} THB to {card_id}. New balance: {new_balance[0]}")
        finally:
            release_conn(conn)

    elif event["type"] == "payment_intent.payment_failed":
        pi = event["data"]["object"]
        card_id = (pi.get("metadata") or {}).get("card_id")
        error_message = (pi.get("last_payment_error") or {}).get("message", "Unknown error")

        conn = get_conn()
        try:
            with conn.cursor() as cur:
                cur.execute(
                    """
                    INSERT INTO transaction_logs (card_id, vendor_id, amount, purpose, error_message)
                    VALUES (%s, %s, %s, %s, %s)
                    """,
                    (card_id, None, 0, "topup_failed", error_message)
                )
                conn.commit()
                print(f"❌ PromptPay topup failed for {card_id}: {error_message}")
        finally:
            release_conn(conn)

    return {"received": True}
