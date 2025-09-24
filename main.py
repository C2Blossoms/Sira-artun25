from fastapi import FastAPI, HTTPException, Request
from pydantic import BaseModel, field_validator
from fastapi.responses import JSONResponse
import psycopg2
from psycopg2 import pool
# import omise

# ------------------------------
# Config (ใช้ ENV จริงใน production)
# ------------------------------
OMISE_SECRET_KEY = "skey_test_xxx"  # เอามาจาก Omise Dashboard
# omise.api_secret = OMISE_SECRET_KEY

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
    amount: int      # สตางค์ เช่น 10000 = 100 บาท
    card_id: str

    @field_validator("amount")
    @classmethod
    def validate_amount(cls, v: int):
        if v <= 0:
            raise ValueError("amount must be positive (satang).")
        return v

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
    return {"message": "API server is running 🚀 (Omise PromptPay)"}

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
    if amount <= 0:
        raise HTTPException(status_code=400, detail="amount must be positive.")
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
    if amount <= 0:
        raise HTTPException(status_code=400, detail="amount must be positive.")
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

# ✅ Create PromptPay QR (Omise)
# @app.post("/create-promptpay-qr")
# def create_promptpay_qr(req: PaymentRequest):
#     try:
#         charge = omise.Charge.create(
#             amount=req.amount,   # หน่วยสตางค์
#             currency="thb",
#             source={"type": "promptpay"},
#             metadata={"card_id": req.card_id}
#         )
#         return {
#             "charge_id": charge.id,
#             "amount": charge.amount,
#             "qr_url": charge.source.scannable_code.image.download_uri
#         }
#     except Exception as e:
#         raise HTTPException(status_code=500, detail=str(e))

# ✅ Webhook จาก Omise
@app.post("/webhook")
async def omise_webhook(request: Request):
    payload = await request.json()
    if payload.get("object") == "charge" and payload.get("status") == "successful":
        card_id = payload["metadata"].get("card_id")
        amount = payload["amount"] / 100.0  # แปลงกลับเป็นบาท
        conn = get_conn()
        try:
            with conn.cursor() as cur:
                cur.execute(
                    "UPDATE cards SET balance = balance + %s WHERE card_id = %s RETURNING balance",
                    (amount, card_id)
                )
                new_balance = cur.fetchone()
                if new_balance:
                    cur.execute(
                        "INSERT INTO transaction_logs (card_id, vendor_id, amount, purpose) VALUES (%s, %s, %s, %s)",
                        (card_id, None, amount, "topup")
                    )
                    conn.commit()
                    print(f"✅ Topup {amount} THB for {card_id}. New balance = {new_balance[0]}")
        finally:
            release_conn(conn)
    return {"received": True}
