from fastapi import FastAPI, HTTPException, Request
from pydantic import BaseModel
from fastapi.responses import JSONResponse
import psycopg2
from psycopg2 import pool
import stripe
import qrcode
import base64
from io import BytesIO
from PIL import Image
import numpy as np

# ------------------------------
# Config (ใช้ ENV จริงใน production)
# ------------------------------
STRIPE_SECRET_KEY = "sk_test_51S9lC5BHtEk7Kds9OlNl9wWo6Y0Hz3xpLSlVgaZezxZgdlpU2UeZgN49occ8Hde9pK5SZpNwUAPUXEmFz90HRMI200tSJWVTHy"
STRIPE_WEBHOOK_SECRET = "whsec_8ef26a5a06dc32b4468d2c00d9cf265e1a36401182350ccb7f480784ce54229b"

# STRIPE_WEBHOOK_SECRET = "whsec_jiJi7URUxjIbj3KIc765AtbQTuALqCc3"

stripe.api_key = STRIPE_SECRET_KEY

# ------------------------------
# Database Connection Pool
# ------------------------------
db_pool = psycopg2.pool.SimpleConnectionPool(
    1, 10,
    dbname="postgres",
    user="postgres",
    password="12345678",
    host="localhost",
    port="5432"
)

app = FastAPI()

# ------------------------------
# Models
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

class PaymentRequest(BaseModel):
    amount: int      # เช่น 5000 = $50.00 (หน่วยเป็น cent)
    card_id: str     # card_id ของผู้ใช้ที่จะเติมเงิน

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
# Routes
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

# ✅ Top up (manual)
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

            # log transaction
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

# ✅ Pay
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

            # deduct
            cur.execute(
                "UPDATE cards SET balance = balance - %s WHERE card_id = %s RETURNING balance",
                (amount, card_id)
            )
            new_balance = cur.fetchone()

            # log transaction
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

# ✅ Create Stripe PaymentIntent
@app.post("/create-payment")
def create_payment(req: PaymentRequest):
    try:
        intent = stripe.PaymentIntent.create(
            amount=req.amount,
            currency="usd",
            payment_method_types=["card"],
            metadata={   # ✅ ส่ง card_id ไปกับ Stripe
                "card_id": req.card_id
            }
        )
        # สร้าง URL ไป Stripe Payment Page (ตัวอย่างแบบ PaymentIntent)
        # payment_url = f"https://checkout.stripe.com/pay/{intent.client_secret}"

        # # สร้าง QR code
        # qr = qrcode.make(payment_url)
        # buf = BytesIO()
        # qr.save(buf, format="PNG")
        # qr_b64 = base64.b64encode(buf.getvalue()).decode("utf-8")

        # return JSONResponse(content={
        #     "payment_url": payment_url,
        #     "qr_code_base64": qr_b64
        # })
        # return {"clientSecret": intent.client_secret}
    except Exception as e:
        raise HTTPException(status_code=500, detail=str(e))

@app.post("/create-payment-qr")
def create_payment_qr(req: PaymentRequest):
    try:
        # ✅ สร้าง Checkout Session แทน PaymentIntent ตรง ๆ
        session = stripe.checkout.Session.create(
            payment_method_types=["card"],
            line_items=[{
                "price_data": {
                    "currency": "usd",
                    "product_data": {
                        "name": f"Topup for Card {req.card_id}",
                    },
                    "unit_amount": req.amount,  # หน่วยเป็น cent
                },
                "quantity": 1,
            }],
            mode="payment",
            success_url="https://example.com/success",   # TODO: เปลี่ยนตามจริง
            cancel_url="https://example.com/cancel",     # TODO: เปลี่ยนตามจริง
            metadata={   # ✅ เก็บ card_id
                "card_id": req.card_id
            }
        )

        payment_url = session.url

        # ✅ สร้าง QR Code ของ URL
        qr = qrcode.make(payment_url)
        buf = BytesIO()
        qr.save(buf, format="PNG")
        qr_b64 = base64.b64encode(buf.getvalue()).decode("utf-8")

        return JSONResponse(content={
            "payment_url": payment_url,
            "qr_code_base64": qr_b64
        })

    except Exception as e:
        raise HTTPException(status_code=500, detail=str(e))

@app.post("/create-payment-qr-matrix")
def create_payment_qr_matrix(req: PaymentRequest):
    try:
        # 1) สร้าง Stripe checkout session
        session = stripe.checkout.Session.create(
            payment_method_types=["card"],
            line_items=[{
                "price_data": {
                    "currency": "usd",
                    "product_data": {"name": f"Topup for {req.card_id}"},
                    "unit_amount": req.amount,
                },
                "quantity": 1,
            }],
            mode="payment",
            success_url="https://example.com/success",
            cancel_url="https://example.com/cancel",
            metadata={"card_id": req.card_id}
        )

        # 2) สร้าง QR Code จาก payment_url
        qr = qrcode.QRCode(
            version=1,
            error_correction=qrcode.constants.ERROR_CORRECT_L,
            box_size=1,
            border=0,
        )
        qr.add_data(session.url)
        qr.make(fit=True)
        img = qr.make_image(fill_color="black", back_color="white").convert("L")

        # 3) Resize เป็น 64x64
        img = img.resize((64, 64), Image.NEAREST)

        # 4) แปลงเป็น 0/1 matrix
        matrix = np.array(img)
        matrix_01 = (matrix < 128).astype(int).tolist()

        # 5) ส่งออกเป็น string array เช่น ["101010...", "111000..."]
        matrix_strings = ["".join(str(cell) for cell in row) for row in matrix_01]

        return JSONResponse(content={
            "payment_url": session.url,
            "matrix": matrix_strings
        })

    except Exception as e:
        raise HTTPException(status_code=500, detail=str(e))
    
# ✅ Stripe Webhook
@app.post("/webhook")
async def stripe_webhook(request: Request):
    payload = await request.body()
    sig = request.headers.get("stripe-signature")

    try:
        event = stripe.Webhook.construct_event(
            payload, sig, STRIPE_WEBHOOK_SECRET
        )
    except Exception as e:
        raise HTTPException(status_code=400, detail=f"Webhook error: {str(e)}")

    # ✅ Payment Success
    if event["type"] == "payment_intent.succeeded":
        payment_intent = event["data"]["object"]
        amount = payment_intent["amount"] / 100
        card_id = payment_intent["metadata"].get("card_id")

        if not card_id:
            raise HTTPException(status_code=400, detail="No card_id in metadata")

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

                # log transaction
                cur.execute(
                    "INSERT INTO transaction_logs (card_id, vendor_id, amount, purpose) VALUES (%s, %s, %s, %s)",
                    (card_id, None, amount, "topup")
                )
                conn.commit()
                print(f"✅ Topup {amount} to {card_id} success. New balance: {new_balance[0]}")
        finally:
            release_conn(conn)

    # Payment Failed
    elif event["type"] == "payment_intent.payment_failed":
        payment_intent = event["data"]["object"]
        card_id = payment_intent["metadata"].get("card_id")
        error_message = payment_intent["last_payment_error"]["message"]

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
                print(f"❌ Topup failed for {card_id}: {error_message}")
        finally:
            release_conn(conn)


    return {"received": True}