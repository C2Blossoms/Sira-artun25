from flask import Flask, request
app = Flask(__name__)

@app.get("/health")
def health(): return {"ok": True}

@app.post("/auth/card")
def auth_card():
    data = request.get_json(force=True)
    return {"card_uid": data.get("card_uid"), "balance_cents": 0}

@app.post("/topup")
def topup():
    d = request.get_json(force=True)
    return {"approved": True, "balance_cents": int(d["amount_cents"])}

@app.post("/payment")
def payment():
    d = request.get_json(force=True); amt = int(d["amount_cents"])
    if amt < 10000: return {"approved": True, "balance_cents": 10000-amt}
    return {"approved": False, "reason": "INSUFFICIENT_FUNDS", "balance_cents": 10000}, 402

if __name__ == "__main__":
    app.run(host="0.0.0.0", port=5000)
