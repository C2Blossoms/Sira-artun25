from fastapi import FastAPI, HTTPException, Request
from pydantic import BaseModel
from fastapi.responses import JSONResponse
import psycopg2
from psycopg2 import pool
import stripe
import os

# ------------------------------
# Config (ใช้ ENV จริงใน production)
# ------------------------------
STRIPE_SECRET_KEY =  "sk_test_51S92JYGlCMj69RdDNOSqU0udgPIAjEI8fWu6OUsCMoTENbUR4LLMLxB6VyKLdH2V9mY0Vu9aeG4Fc6UjI1Hx2Bvo00GhsPQUCK"
STRIPE_WEBHOOK_SECRET = "whsec_dc955ab2ff085eed14ad543279c327c14e60932dd361f57a311e3c887769223b"

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
    card_id: int
    balance: float


class HistoryItem(BaseModel):
    amount: float
    purpose: str
    time: str


class HistoryResponse(BaseModel):
    card_id: int
    history: list[HistoryItem]


class PaymentRequest(BaseModel):
    amount: int   # เช่น 5000 = $50.00


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
def get_balance(card_id: int):
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


# ✅ Top up
@app.post("/topup/{card_id}/{amount}", response_model=BalanceResponse)
def topup(card_id: int, amount: float):
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
def pay(card_id: int, amount: float, vendor_id: int):
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
def get_history(card_id: int):
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
        )
        return {"clientSecret": intent.client_secret}
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

    if event["type"] == "payment_intent.succeeded":
        payment_intent = event["data"]["object"]
        print("✅ Payment successful:", payment_intent["id"])

        # TODO: คุณสามารถอัปเดต Database ที่นี่ได้ เช่น เติมเงินเข้า card_id ที่เกี่ยวข้อง
        # ตัวอย่าง: topup card_id=1, amount=50
        conn = get_conn()
        try:
            with conn.cursor() as cur:
                cur.execute(
                    "UPDATE cards SET balance = balance + %s WHERE card_id = %s RETURNING balance",
                    (50, 1)
                )
                conn.commit()
        finally:
            release_conn(conn)

    return {"received": True}
