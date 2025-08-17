from fastapi import FastAPI
from pydantic import BaseModel
import psycopg2
from datetime import datetime

app = FastAPI()

# เชื่อม PostgreSQL
conn = psycopg2.connect(
    dbname="postgres",
    user="postgres",
    password="12345678",
    host="localhost",
    port="5432"
)

@app.get("/")
def root():
    return {"message": "API server is running 🚀"}

# ✅ API: เช็คยอดเงิน
@app.get("/balance/{card_id}")
def get_balance(card_id: int):
    cur = conn.cursor()
    cur.execute("SELECT balance FROM cards WHERE card_id = %s", (card_id,))
    result = cur.fetchone()
    cur.close()
    if not result:
        return {"error": "Card not found"}
    return {"card_id": card_id, "balance": float(result[0])}


# ✅ API: เติมเงิน
@app.post("/topup/{card_id}/{amount}")
def topup(card_id: int, amount: float):
    cur = conn.cursor()
    cur.execute("UPDATE cards SET balance = balance + %s WHERE card_id = %s RETURNING balance", (amount, card_id))
    new_balance = cur.fetchone()
    
    if new_balance:
        # 🔹 บันทึกประวัติลง transaction_logs
        cur.execute(
            "INSERT INTO transaction_logs (card_id, vendor_id, amount, purpose) VALUES (%s, %s, %s, %s)",
            (card_id, None, amount, "topup")
        )
        conn.commit()
        cur.close()
        return {"card_id": card_id, "new_balance": float(new_balance[0])}
    else:
        cur.close()
        return {"error": "Card not found"}


# ✅ API: ตัดเงิน
@app.post("/pay/{card_id}/{amount}/{vendor_id}")
def pay(card_id: int, amount: float, vendor_id: int):
    cur = conn.cursor()
    cur.execute("SELECT balance FROM cards WHERE card_id = %s", (card_id,))
    result = cur.fetchone()
    if not result:
        cur.close()
        return {"error": "Card not found"}
    
    balance = float(result[0])
    if balance < amount:
        cur.close()
        return {"error": "Not enough balance"}
    
    # หักเงิน
    cur.execute("UPDATE cards SET balance = balance - %s WHERE card_id = %s RETURNING balance", (amount, card_id))
    new_balance = cur.fetchone()
    
    # บันทึก transaction
    cur.execute(
        "INSERT INTO transaction_logs (card_id, vendor_id, amount, purpose) VALUES (%s, %s, %s, %s)",
        (card_id, vendor_id, amount, "payment")
    )
    conn.commit()
    cur.close()
    
    return {"card_id": card_id, "new_balance": float(new_balance[0])}


# ✅ API: ดูประวัติ
@app.get("/history/{card_id}")
def get_history(card_id: int):
    cur = conn.cursor()
    cur.execute("""
        SELECT amount, purpose, txn_time 
        FROM transaction_logs 
        WHERE card_id = %s 
        ORDER BY txn_time DESC
    """, (card_id,))
    rows = cur.fetchall()
    cur.close()
    history = [{"amount": float(r[0]), "purpose": r[1], "time": str(r[2])} for r in rows]
    return {"card_id": card_id, "history": history}